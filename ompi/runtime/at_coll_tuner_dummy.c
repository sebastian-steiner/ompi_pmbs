//
// Created by Sascha on 6/27/22.
//

#include <stdio.h>
#include "at_coll_tuner.h"

void AT_coll_tune_init(void) {
  printf("WARNING: dummy init... use LD_PRELOAD\n");
}

void AT_coll_tune_finalize(void) {
  printf("WARNING: dummy finalize... use LD_PRELOAD\n");
}

int AT_get_allreduce_selection_id(int buf_size, int comm_size) {
  // empty
  return 0;
}

int AT_get_bcast_selection_id(int buf_size, int comm_size) {
  // empty
  return 0;
}

int AT_get_allgather_selection_id(int buf_size, int comm_size) {
  // empty
  return 0;
}

int AT_get_alltoall_selection_id(int buf_size, int comm_size) {
  // empty
  return 0;
}

AT_col_t AT_get_allreduce_our_alg(int our_alg_id) {
  // empty
  AT_col_t res;
  res.ompi_alg_id = 0;
  res.seg_size = 64;
  res.faninout = 0;
  res.max_requests = 128;
  return res;
}

AT_col_t AT_get_bcast_our_alg(int our_alg_id) {
  // empty
  AT_col_t res;
  res.ompi_alg_id = 0;
  res.seg_size = 64;
  res.faninout = 0;
  res.max_requests = 128;
  return res;
}

AT_col_t AT_get_allgather_our_alg(int our_alg_id) {
  // empty
  AT_col_t res;
  res.ompi_alg_id = 0;
  res.seg_size = 64;
  res.faninout = 0;
  res.max_requests = 128;
  return res;
}

AT_col_t AT_get_alltoall_our_alg(int our_alg_id) {
  // empty
  AT_col_t res;
  res.ompi_alg_id = 0;
  res.seg_size = 64;
  res.faninout = 0;
  res.max_requests = 128;
  return res;
}

void AT_enable_collective_sampling(int flag) {
  // empty
}

int AT_is_collective_sampling_enabled(void) {
  // empty
  return 0;
}

int AT_is_collective_sampling_possible(void) {
  // empty
  return 0;
}

double AT_get_time(void) {
  // empty
  return 0.0;
}

int AT_record_start_timestamp(const AT_mpi_call_t callid, const int our_alg_id, const int buf_size, const int comm_size) {
  // empty
}

void AT_record_end_timestamp(const AT_mpi_call_t callid, const int coll_cnt) {
  // empty
}
