/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2020 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2015-2018 Research Organization for Information Science
 *                         and Technology (RIST).  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"

#include "mpi.h"
#include "opal/util/bit_ops.h"
#include "ompi/constants.h"
#include "ompi/datatype/ompi_datatype.h"
#include "ompi/communicator/communicator.h"
#include "ompi/mca/coll/coll.h"
#include "ompi/mca/coll/base/coll_tags.h"
#include "ompi/mca/pml/pml.h"
#include "ompi/op/op.h"
#include "coll_tuned.h"
#include "ompi/mca/coll/base/coll_base_topo.h"
#include "ompi/mca/coll/base/coll_base_util.h"
#include "at_coll_tuner.h"

/* allreduce algorithm variables */
static int coll_tuned_allreduce_forced_algorithm = 0;
static int coll_tuned_allreduce_segment_size = 0;
static int coll_tuned_allreduce_tree_fanout;
static int coll_tuned_allreduce_chain_fanout;

/* valid values for coll_tuned_allreduce_forced_algorithm */
static const mca_base_var_enum_value_t allreduce_algorithms[] = {
    {0, "ignore"},
    {1, "basic_linear"},
    {2, "nonoverlapping"},
    {3, "recursive_doubling"},
    {4, "ring"},
    {5, "segmented_ring"},
    {6, "rabenseifner"},
    {0, NULL}
};

/* The following are used by dynamic and forced rules */

/* publish details of each algorithm and if its forced/fixed/locked in */
/* as you add methods/algorithms you must update this and the query/map routines */

/* this routine is called by the component only */
/* this makes sure that the mca parameters are set to their initial values and perms */
/* module does not call this they call the forced_getvalues routine instead */

int ompi_coll_tuned_allreduce_intra_check_forced_init (coll_tuned_force_algorithm_mca_param_indices_t *mca_param_indices)
{
    mca_base_var_enum_t *new_enum;
    int cnt;

    for( cnt = 0; NULL != allreduce_algorithms[cnt].string; cnt++ );
    ompi_coll_tuned_forced_max_algorithms[ALLREDUCE] = cnt;

    (void) mca_base_component_var_register(&mca_coll_tuned_component.super.collm_version,
                                           "allreduce_algorithm_count",
                                           "Number of allreduce algorithms available",
                                           MCA_BASE_VAR_TYPE_INT, NULL, 0,
                                           MCA_BASE_VAR_FLAG_DEFAULT_ONLY,
                                           OPAL_INFO_LVL_5,
                                           MCA_BASE_VAR_SCOPE_CONSTANT,
                                           &ompi_coll_tuned_forced_max_algorithms[ALLREDUCE]);

    /* MPI_T: This variable should eventually be bound to a communicator */
    coll_tuned_allreduce_forced_algorithm = 0;
    (void) mca_base_var_enum_create("coll_tuned_allreduce_algorithms", allreduce_algorithms, &new_enum);
    mca_param_indices->algorithm_param_index =
        mca_base_component_var_register(&mca_coll_tuned_component.super.collm_version,
                                        "allreduce_algorithm",
                                        "Which allreduce algorithm is used. Can be locked down to any of: 0 ignore, 1 basic linear, 2 nonoverlapping (tuned reduce + tuned bcast), 3 recursive doubling, 4 ring, 5 segmented ring. "
                                        "Only relevant if coll_tuned_use_dynamic_rules is true.",
                                        MCA_BASE_VAR_TYPE_INT, new_enum, 0, MCA_BASE_VAR_FLAG_SETTABLE,
                                        OPAL_INFO_LVL_5,
                                        MCA_BASE_VAR_SCOPE_ALL,
                                        &coll_tuned_allreduce_forced_algorithm);
    OBJ_RELEASE(new_enum);
    if (mca_param_indices->algorithm_param_index < 0) {
        return mca_param_indices->algorithm_param_index;
    }

    coll_tuned_allreduce_segment_size = 0;
    mca_param_indices->segsize_param_index =
        mca_base_component_var_register(&mca_coll_tuned_component.super.collm_version,
                                        "allreduce_algorithm_segmentsize",
                                        "Segment size in bytes used by default for allreduce algorithms. Only has meaning if algorithm is forced and supports segmenting. 0 bytes means no segmentation.",
                                        MCA_BASE_VAR_TYPE_INT, NULL, 0, MCA_BASE_VAR_FLAG_SETTABLE,
                                        OPAL_INFO_LVL_5,
                                        MCA_BASE_VAR_SCOPE_ALL,
                                        &coll_tuned_allreduce_segment_size);

    coll_tuned_allreduce_tree_fanout = ompi_coll_tuned_init_tree_fanout; /* get system wide default */
    mca_param_indices->tree_fanout_param_index =
        mca_base_component_var_register(&mca_coll_tuned_component.super.collm_version,
                                        "allreduce_algorithm_tree_fanout",
                                        "Fanout for n-tree used for allreduce algorithms. Only has meaning if algorithm is forced and supports n-tree topo based operation.",
                                        MCA_BASE_VAR_TYPE_INT, NULL, 0, MCA_BASE_VAR_FLAG_SETTABLE,
                                        OPAL_INFO_LVL_5,
                                        MCA_BASE_VAR_SCOPE_ALL,
                                        &coll_tuned_allreduce_tree_fanout);

    coll_tuned_allreduce_chain_fanout = ompi_coll_tuned_init_chain_fanout; /* get system wide default */
    mca_param_indices->chain_fanout_param_index =
      mca_base_component_var_register(&mca_coll_tuned_component.super.collm_version,
                                      "allreduce_algorithm_chain_fanout",
                                      "Fanout for chains used for allreduce algorithms. Only has meaning if algorithm is forced and supports chain topo based operation.",
                                      MCA_BASE_VAR_TYPE_INT, NULL, 0, MCA_BASE_VAR_FLAG_SETTABLE,
                                      OPAL_INFO_LVL_5,
                                      MCA_BASE_VAR_SCOPE_ALL,
                                      &coll_tuned_allreduce_chain_fanout);

    return (MPI_SUCCESS);
}

int ompi_coll_tuned_allreduce_intra_do_this(const void *sbuf, void *rbuf, int count,
                                            struct ompi_datatype_t *dtype,
                                            struct ompi_op_t *op,
                                            struct ompi_communicator_t *comm,
                                            mca_coll_base_module_t *module,
                                            int algorithm, int faninout, int segsize)
{
  int res = MPI_ERR_ARG;

  if( AT_is_collective_sampling_enabled() && AT_is_collective_sampling_possible() ) {
    size_t type_size;
    int comm_size;
    algorithm = AT_get_allreduce_ompi_id(our_alg_id);
    ompi_datatype_type_size(dtype, &type_size);
    comm_size = ompi_comm_size(comm);
    int our_alg_id = AT_get_allreduce_selection_id(count * type_size, comm_size);
    segsize   = AT_get_allreduce_ompi_segsize(our_alg_id);
    AT_record_start_timestamp(MPI_ALLREDUCE, our_alg_id, count * type_size, comm_size);
  }
  //printf("selected algorithm %d (seg size %d)\n", algorithm, segsize);
  
  OPAL_OUTPUT((ompi_coll_tuned_stream,"coll:tuned:allreduce_intra_do_this algorithm %d topo fan in/out %d segsize %d",
      algorithm, faninout, segsize));

  switch (algorithm) {
  case (0):
    res = ompi_coll_tuned_allreduce_intra_dec_fixed(sbuf, rbuf, count, dtype, op, comm, module);
    break;
  case (1):
    res = ompi_coll_base_allreduce_intra_basic_linear(sbuf, rbuf, count, dtype, op, comm, module);
    break;
  case (2):
    res = ompi_coll_base_allreduce_intra_nonoverlapping(sbuf, rbuf, count, dtype, op, comm, module);
    break;
  case (3):
    res = ompi_coll_base_allreduce_intra_recursivedoubling(sbuf, rbuf, count, dtype, op, comm, module);
    break;
  case (4):
    res = ompi_coll_base_allreduce_intra_ring(sbuf, rbuf, count, dtype, op, comm, module);
    break;
  case (5):
    res = ompi_coll_base_allreduce_intra_ring_segmented(sbuf, rbuf, count, dtype, op, comm, module, segsize);
    break;
  case (6):
    res = ompi_coll_base_allreduce_intra_redscat_allgather(sbuf, rbuf, count, dtype, op, comm, module);
    break;
  } /* switch */
  OPAL_OUTPUT((ompi_coll_tuned_stream,"coll:tuned:allreduce_intra_do_this attempt to select algorithm %d when only 0-%d is valid?",
      algorithm, ompi_coll_tuned_forced_max_algorithms[ALLREDUCE]));

  if( AT_is_collective_sampling_enabled() && AT_is_collective_sampling_possible() ) {
    AT_record_end_timestamp(MPI_ALLREDUCE);
  }

  return (res);

}
