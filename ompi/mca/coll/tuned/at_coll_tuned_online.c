//
// Created by Sascha on 5/5/22.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <time.h>

#include "at_coll_tuned_online.h"

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

static const char AT_MPI_COLL_TRACER_FNAME_DEFAULT[] = "timings.dat";
static char AT_MPI_COLL_TRACER_FNAME[1024];

static int AT_NB_COLLS = sizeof(at_allreduce_algs)/sizeof(AT_col_t);

static int AT_coll_selector = 0;

static int MAX_NB_TIMES = 500;
static stamp_t *at_time_stamps;
static int at_coll_cnt = 0;

void AT_enable_collective_sampling(int flag) {
  AT_coll_selector = flag;
}

int AT_is_collective_sampling_enabled() {
  return AT_coll_selector;
}

int AT_get_allreduce_selection_id() {
  int algid = rand() % AT_NB_COLLS;
  return algid;
}

int AT_get_allreduce_ompi_id(int our_alg_id) {
  return at_allreduce_algs[our_alg_id].ompi_alg_id;
}

int AT_get_allreduce_ompi_segsize(int our_alg_id) {
  return at_allreduce_algs[our_alg_id].seg_size;
}


void AT_record_start_timestamp(const AT_mpi_call_t callid, const int our_alg_id, const int buf_size, const int comm_size) {
  if (at_coll_cnt < MAX_NB_TIMES) {
    at_time_stamps[at_coll_cnt].call_id = callid;
    at_time_stamps[at_coll_cnt].our_alg_id = our_alg_id;
    at_time_stamps[at_coll_cnt].buf_size = buf_size;
    at_time_stamps[at_coll_cnt].comm_size = comm_size;
    at_time_stamps[at_coll_cnt].stamp_start = AT_get_time();
  }
}

void AT_record_end_timestamp(const AT_mpi_call_t callid) {
  if (at_coll_cnt < MAX_NB_TIMES) {
    at_time_stamps[at_coll_cnt].stamp_end = AT_get_time();
    at_coll_cnt++;
  }
}

static void init_parameters(void) {
  char *trace_fname = NULL;
  char *nb_stamps_str = NULL;
  char *env_str = NULL;
  srand(0);

  // default params
  strncpy(AT_MPI_COLL_TRACER_FNAME, AT_MPI_COLL_TRACER_FNAME_DEFAULT, 1023);

  trace_fname = getenv("AT_MPI_COLL_TRACER_FNAME");
  if( trace_fname != NULL ) {
    int l = strnlen(trace_fname, 1024);
    strncpy(AT_MPI_COLL_TRACER_FNAME, trace_fname, l);
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
    }
  }
}

void AT_coll_tune_init(void) {
  init_parameters();
  if( AT_is_collective_sampling_enabled() ) {
    at_time_stamps = (stamp_t *) calloc(MAX_NB_TIMES, sizeof(stamp_t));
    at_coll_cnt = 0;
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
        fprintf(fh, "#@ num_timestamps=%d\n", at_coll_cnt);
        fprintf(fh, "p\t  idx\tcallid\talgid\tbuf_size\tcomm_size\tstart\t        end\n");
        for (i = 0; i < at_coll_cnt; i++) {
          for (j = 0; j < nsize; j++) {
            fprintf(fh, "%d\t%5d\t%5d\t%6d\t%8d\t\%9d\t%.6f\t%.6f\n", j, i,
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