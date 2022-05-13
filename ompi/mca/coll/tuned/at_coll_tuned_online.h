//
// Created by Sascha on 5/5/22.
//

#ifndef OMPI_MCA_COLL_TUNED_AT_COLL_TUNED_ONLINE_H
#define OMPI_MCA_COLL_TUNED_AT_COLL_TUNED_ONLINE_H


typedef struct {
  int ompi_alg_id;
  int seg_size;
} AT_col_t;

typedef enum
{
  MPI_ALLREDUCE = 1
} AT_mpi_call_t;


typedef struct
{
  int call_id;
  int our_alg_id;
  int buf_size;
  int comm_size;
  double stamp_start;
  double stamp_end;
} stamp_t;


int AT_get_allreduce_selection_id(void);
int AT_get_allreduce_ompi_id(int our_alg_id);
int AT_get_allreduce_ompi_segsize(int our_alg_id);
void AT_enable_collective_sampling(int flag);
int AT_is_collective_sampling_enabled(void);
int AT_is_collective_sampling_possible(void);

void AT_coll_tune_init(void);
void AT_coll_tune_finalize(void);
double AT_get_time(void);

void AT_record_start_timestamp(const AT_mpi_call_t callid, const int our_alg_id, const int buf_size, const int comm_size);
void AT_record_end_timestamp(const AT_mpi_call_t callid);

#endif //OMPI_MCA_COLL_TUNED_AT_COLL_TUNED_ONLINE_H
