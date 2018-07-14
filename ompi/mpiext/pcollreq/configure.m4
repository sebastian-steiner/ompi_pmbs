# -*- shell-script -*-
#
# Copyright (c) 2017      FUJITSU LIMITED.  All rights reserved.
# Copyright (c) 2018      Research Organization for Information Science
#                         and Technology (RIST). All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

# OMPI_MPIEXT_pcollreq_CONFIG([action-if-found], [action-if-not-found])
# -----------------------------------------------------------
AC_DEFUN([OMPI_MPIEXT_pcollreq_CONFIG],[
    AC_CONFIG_FILES([
        ompi/mpiext/pcollreq/Makefile
        ompi/mpiext/pcollreq/c/Makefile
        ompi/mpiext/pcollreq/c/profile/Makefile
    ])

    AS_IF([test "$ENABLE_pcollreq" = "1" || \
           test "$ENABLE_EXT_ALL" = "1"],
          [$1],
          [$2])
])
