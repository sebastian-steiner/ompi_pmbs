//
// Created by Sascha on 6/27/22.
//

#include <stdio.h>
#include "at_coll_tuner.h"

void AT_coll_tune_init(void) {
  printf("dummy\n");
}

void AT_coll_tune_finalize(void) {
  printf("dummy\n");
}

int AT_get_allreduce_selection_id(void) {
  printf("dummy\n");
  return 0;
}

int AT_get_allreduce_ompi_id(int our_alg_id) {
  printf("dummy\n");
  return 0;
}

int AT_get_allreduce_ompi_segsize(int our_alg_id) {
  printf("dummy\n");
  return 64;
}

void AT_enable_collective_sampling(int flag) {
  printf("dummy\n");
}

int AT_is_collective_sampling_enabled(void) {
  printf("dummy\n");
  return 1;
}

int AT_is_collective_sampling_possible(void) {
  printf("dummy\n");
  return 1;
}

double AT_get_time(void) {
  printf("dummy\n");
  return 0.0;
}

void AT_record_start_timestamp(const AT_mpi_call_t callid, const int our_alg_id, const int buf_size, const int comm_size) {
  printf("dummy\n");
}

void AT_record_end_timestamp(const AT_mpi_call_t callid) {
  printf("dummy\n");
}