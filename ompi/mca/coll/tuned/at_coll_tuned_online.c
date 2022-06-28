//
// Created by Sascha on 5/5/22.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

#include "at_coll_tuned_online.h"
#include "ompi/include/mpi.h"
#include "ompi/communicator/communicator.h"

#if defined(AT_ENABLE_GETTIME_REALTIME)
#include <time.h>
static double wtime;
static struct timespec ts;
#endif

static AT_col_t at_allreduce_algs[] = {
    /* 1 - no segmentation */
    { 1, 0 },

    /* 2 - no segmentation */
    { 2, 0 },

    /* 3 - no segmentation */
    { 3, 0 },

    /* 4 - no segmentation */
    { 4, 0 },

    /* 5 - segmented ring */
    { 5, 1 << 10 },
    { 5, 1 << 20 },
    { 5, 1 << 23 },

    /* 6 - no segmentation */
    { 6, 0 }
};

// indexed in order by message size, number of nodes, processes per node, algorithm-id
static float**** at_allreduce_probs;

static const char AT_MPI_COLL_TRACER_FNAME_DEFAULT[] = "timings.dat";
static char AT_MPI_COLL_TRACER_FNAME[1024];

static const char AT_MPI_COLL_PROBS_FNAME_DEFAULT[] = "probs.dat";
static char AT_MPI_COLL_PROBS_FNAME[1024];

static int AT_NB_COLLS = sizeof(at_allreduce_algs)/sizeof(AT_col_t);

static int AT_coll_selector = 0;

static int MAX_NB_TIMES = 500;
static stamp_t *at_time_stamps;
static int at_coll_cnt = 0;

static int AT_nb_nodes = 0;
static int AT_ppn = 0;
static int AT_nb_procs = 0;

static int AT_bins_m = 0;
static int AT_bins_n = 0;
static int AT_bins_ppn = 0;

static void _AT_get_timed_suffix(char *buf) {
  time_t t = time(NULL);
  struct tm *lt = localtime(&t);
  pid_t pid = getpid();

  sprintf(buf, "%d_%04d_%02d_%02d_%02d_%02d",
          pid, 1900 + lt->tm_year, 1+lt->tm_mon, lt->tm_mday,
          lt->tm_hour, lt->tm_min);
}

void AT_enable_collective_sampling(int flag) {
  AT_coll_selector = flag;
}

int AT_is_collective_sampling_enabled() {
  return AT_coll_selector;
}

int AT_is_collective_sampling_possible() {
  return at_coll_cnt < MAX_NB_TIMES;
}

int AT_get_allreduce_selection_id(int buf_size, int comm_size) {
  float r = (float) rand() / RAND_MAX;
  int m_bin, n_bin, ppn_bin;

  int nnodes = comm_size / AT_ppn;

  if (buf_size <= 10) {
    m_bin = 0;
  } else if (buf_size <= 100) {
    m_bin = 1;
  } else if (buf_size <= 10000) {
    m_bin = 2;
  } else if (buf_size <= 1000000) {
    m_bin = 3;
  } else {
    m_bin = 4;
  }

  if (nnodes <= 1) {
    n_bin = 0;
  } else if (nnodes <= 18) {
    n_bin = 1;
  } else {
    n_bin = 2;
  }

  if (AT_ppn <= 1) {
    ppn_bin = 0;
  } else if (AT_ppn <= 16) {
    ppn_bin = 1;
  } else {
    ppn_bin = 2;
  }

  float* p = at_allreduce_probs[m_bin][n_bin][ppn_bin];

  for (int i = 0; i < AT_NB_COLLS; i++) {
    if (r < p[i]) {
      return i;
    }
  }
  return -1;
}

int AT_get_allreduce_ompi_id(int our_alg_id) {
  return at_allreduce_algs[our_alg_id].ompi_alg_id;
}

int AT_get_allreduce_ompi_segsize(int our_alg_id) {
  return at_allreduce_algs[our_alg_id].seg_size;
}


void AT_record_start_timestamp(const AT_mpi_call_t callid, const int our_alg_id, const int buf_size, const int comm_size) {
  if (AT_is_collective_sampling_possible()) {
    at_time_stamps[at_coll_cnt].call_id = callid;
    at_time_stamps[at_coll_cnt].our_alg_id = our_alg_id;
    at_time_stamps[at_coll_cnt].buf_size = buf_size;
    at_time_stamps[at_coll_cnt].comm_size = comm_size;
    at_time_stamps[at_coll_cnt].stamp_start = AT_get_time();
  }
}

void AT_record_end_timestamp(const AT_mpi_call_t callid) {
  if (AT_is_collective_sampling_possible()) {
    at_time_stamps[at_coll_cnt].stamp_end = AT_get_time();
    at_coll_cnt++;
  }
}

static void init_probabilities(void) {
  FILE *fh;
  char* line = NULL;
  size_t len = 0;
  ssize_t read;

  int alg = -1;

  int n_algs;

  fh = fopen(AT_MPI_COLL_PROBS_FNAME, "r");
  if (fh != NULL) {
    while ((read = getline(&line, &len, fh)) != -1) {
      if (line == NULL || line[0] == '\n' || line[0] == '#') continue;
      else if (strncmp("MPI_", line, 4) == 0) {
        if (strncmp("MPI_Allreduce ", line, 14) == 0) {
          alg = 0;
          sscanf(line, "MPI_Allreduce %d %d %d %d\n", &AT_bins_m, &AT_bins_n, &AT_bins_ppn, &n_algs);
          if (at_allreduce_probs == NULL) {
            at_allreduce_probs = (float****) malloc(AT_bins_m * sizeof(float***));
            for (int i = 0; i < AT_bins_m; i++) {
              at_allreduce_probs[i] = (float***) malloc(AT_bins_n * sizeof(float**));
              for (int j = 0; j < AT_bins_n; j++) {
                at_allreduce_probs[i][j] = (float**) malloc(AT_bins_ppn * sizeof(float*));
                for (int k = 0; k < AT_bins_ppn; k++) {
                  at_allreduce_probs[i][j][k] = (float*) calloc(n_algs, sizeof(float));
                }
              }
            }
          }
        } else if (strncmp("MPI_Bcast ", line, 10) == 0) {
          alg = 1;
        } else {
          alg = -1;
        }
      } else {
        int m, N, ppn, a;
        float prob;
        sscanf(line, "%d %d %d %d %f\n", &m, &N, &ppn, &a, &prob);
        if (alg == 0) {
          at_allreduce_probs[m][N][ppn][a] = prob;
        }
      }
    }
    fclose(fh);
  }
}

static void init_parameters(void) {
  char *trace_fname = NULL;
  char *probs_fname = NULL;
  char *nb_stamps_str = NULL;
  char *env_str = NULL;
  srand(0);

  // default params
  strncpy(AT_MPI_COLL_TRACER_FNAME, AT_MPI_COLL_TRACER_FNAME_DEFAULT, 1023);
  strncpy(AT_MPI_COLL_PROBS_FNAME, AT_MPI_COLL_PROBS_FNAME_DEFAULT, 1023);

  trace_fname = getenv("AT_MPI_COLL_TRACER_FNAME");
  if( trace_fname != NULL ) {
    int l = strnlen(trace_fname, 1024);
    strncpy(AT_MPI_COLL_TRACER_FNAME, trace_fname, l);
  } else {
    char sbuf[40];
    char fname[512];
    _AT_get_timed_suffix(sbuf);
    sprintf(AT_MPI_COLL_TRACER_FNAME, "%s_%s.dat", "timings", sbuf);
  }
  probs_fname = getenv("AT_MPI_COLL_PROBS_FNAME");
  if ( probs_fname != NULL ) {
    int l = strnlen(probs_fname, 1024);
    strncpy(AT_MPI_COLL_PROBS_FNAME, probs_fname, l);
  }
  nb_stamps_str = getenv("AT_MPI_COLL_TRACER_NB_STAMPS");
  if( nb_stamps_str != NULL ) {
    MAX_NB_TIMES = atoi(nb_stamps_str);
  }

  env_str = getenv("AT_MPI_COLL_TRACER_ENABLED");
  if( env_str != NULL ) {
    int enabled = atoi(env_str);
    if( enabled == 1 ) {
      AT_coll_selector = 1;
      init_probabilities();
    }
  }
}

void AT_coll_tune_init(void) {
  int *lranks = NULL;
  init_parameters();
  if( AT_is_collective_sampling_enabled() ) {
    at_time_stamps = (stamp_t *) calloc(MAX_NB_TIMES, sizeof(stamp_t));
    at_coll_cnt = 0;

    /* how many ranks are potentially participating and on my node? */
    ompi_comm_split_type_get_part (MPI_COMM_WORLD->c_local_group, MPI_COMM_TYPE_SHARED, &lranks, &AT_ppn);

    MPI_Comm_size(MPI_COMM_WORLD, &AT_nb_procs);
    AT_nb_nodes = AT_nb_procs / AT_ppn;
    if( AT_nb_procs % AT_ppn != 0 ) {
      fprintf(stderr, "WARNING: nprocs (%d) is not a multiple of ppn (%d)\n", AT_nb_procs, AT_ppn);
    }

  }
}

void AT_coll_tune_finalize(void) {

  if( AT_is_collective_sampling_enabled() ) {
    int i, j;
    int rank, nsize;
    stamp_t *total_mpi_times = NULL;
    MPI_Datatype mytype[6] = {
        MPI_INT,  // callid
        MPI_INT,  // our alg id
        MPI_INT,  // buf_size
        MPI_INT,  // comm_size
        MPI_DOUBLE, // start stamp
        MPI_DOUBLE  // end stamp
    };
    MPI_Datatype timestamp_mpi_t;
    int blocklengths[6] = {1, 1, 1, 1, 1, 1};
    MPI_Aint disp[6];
    MPI_Aint base_address;

    PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
    PMPI_Comm_size(MPI_COMM_WORLD, &nsize);

    if (rank == 0) {
      total_mpi_times = (stamp_t *) calloc(nsize * at_coll_cnt, sizeof(stamp_t));
    }

    if (at_coll_cnt > 0) {
      MPI_Get_address(&at_time_stamps[0], &base_address);
      MPI_Get_address(&at_time_stamps[0].call_id, &disp[0]);
      MPI_Get_address(&at_time_stamps[0].our_alg_id, &disp[1]);
      MPI_Get_address(&at_time_stamps[0].buf_size, &disp[2]);
      MPI_Get_address(&at_time_stamps[0].comm_size, &disp[3]);
      MPI_Get_address(&at_time_stamps[0].stamp_start, &disp[4]);
      MPI_Get_address(&at_time_stamps[0].stamp_end, &disp[5]);

      disp[0] = MPI_Aint_diff(disp[0], base_address);
      disp[1] = MPI_Aint_diff(disp[1], base_address);
      disp[2] = MPI_Aint_diff(disp[2], base_address);
      disp[3] = MPI_Aint_diff(disp[3], base_address);
      disp[4] = MPI_Aint_diff(disp[4], base_address);
      disp[5] = MPI_Aint_diff(disp[5], base_address);

      MPI_Type_create_struct(6, blocklengths, disp, mytype, &timestamp_mpi_t);
      MPI_Type_commit(&timestamp_mpi_t);

      PMPI_Gather(at_time_stamps,
                  at_coll_cnt,
                  timestamp_mpi_t,
                  total_mpi_times,
                  at_coll_cnt,
                  timestamp_mpi_t,
                  0,
                  MPI_COMM_WORLD);
    }

    if (rank == 0) {
      FILE *fh;

      fh = fopen(AT_MPI_COLL_TRACER_FNAME, "w");
      if (fh != NULL) {
        time_t timer;
        char buffer[26];
        struct tm *tm_info;

        timer = time(NULL);
        tm_info = localtime(&timer);
        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);

        fprintf(fh, "#@ time=%s\n", buffer);
        fprintf(fh, "#@ nprocs=%d\n", nsize);
        fprintf(fh, "#@ nnodes=%d\n", AT_nb_nodes);
        fprintf(fh, "#@ ppn=%d\n", AT_ppn);
        fprintf(fh, "#@ num_timestamps=%d\n", at_coll_cnt);
        fprintf(fh, "p\t  idx\tcallid\talgid\tbuf_size\tcomm_size\tstart\t        end\n");
        for (i = 0; i < at_coll_cnt; i++) {
          for (j = 0; j < nsize; j++) {
            fprintf(fh, "%d\t%5d\t%5d\t%6d\t%8d\t%9d\t%.6f\t%.6f\n", j, i,
                    total_mpi_times[j * at_coll_cnt + i].call_id,
                    total_mpi_times[j * at_coll_cnt + i].our_alg_id,
                    total_mpi_times[j * at_coll_cnt + i].buf_size,
                    total_mpi_times[j * at_coll_cnt + i].comm_size,
                    total_mpi_times[j * at_coll_cnt + i].stamp_start,
                    total_mpi_times[j * at_coll_cnt + i].stamp_end);
          }
        }
        fclose(fh);
      } else {
        fprintf(stderr, "Cannot write output file\n");
      }
      free(total_mpi_times);
      
      if (AT_coll_selector == 1) {
        for (int i = 0; i < AT_bins_m; i++) {
          for (int j = 0; j < AT_bins_n; j++) {
            for (int k = 0; k < AT_bins_ppn; k++) {
              free(at_allreduce_probs[i][j][k]);
            }
            free(at_allreduce_probs[i][j]);
          }
          free(at_allreduce_probs[i]);
        }
        free(at_allreduce_probs);
      }
    }

    if (at_time_stamps != NULL) {
      free(at_time_stamps);
    }

  } // end if enabled
}


double AT_get_time(void) {
#if defined(AT_ENABLE_GETTIME_REALTIME)
  if( clock_gettime( CLOCK_REALTIME, &ts) == -1 ) {
      perror( "clock gettime" );
      exit( EXIT_FAILURE );
    }
    wtime = (double)(ts.tv_nsec) / 1.0e+9 + ts.tv_sec;
    return wtime;
#else
  return MPI_Wtime();
#endif
}