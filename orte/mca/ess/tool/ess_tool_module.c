/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2011 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2015-2017 Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */

#include "orte_config.h"
#include "orte/constants.h"
#include "opal/hash_string.h"

#include <sys/types.h>
#include <stdio.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "opal/runtime/opal_progress_threads.h"

#include "orte/util/show_help.h"
#include "orte/mca/plm/base/base.h"
#include "orte/mca/plm/base/plm_private.h"
#include "orte/mca/plm/plm.h"
#include "orte/mca/errmgr/errmgr.h"
#include "orte/util/proc_info.h"
#include "orte/runtime/orte_cr.h"

#include "orte/mca/ess/ess.h"
#include "orte/mca/ess/base/base.h"
#include "orte/mca/ess/tool/ess_tool.h"

static int rte_init(void);
static void rte_abort(int status, bool report) __opal_attribute_noreturn__;
static int rte_finalize(void);


orte_ess_base_module_t orte_ess_tool_module = {
    rte_init,
    rte_finalize,
    rte_abort,
    NULL /* ft_event */
};

static bool progress_thread_running = false;

static int rte_init(void)
{
    int ret;
    char *error = NULL;
    uint8_t flags;

    /* run the prolog */
    if (ORTE_SUCCESS != (ret = orte_ess_base_std_prolog())) {
        error = "orte_ess_base_std_prolog";
        goto error;
    }


    /* if requested, get an async event base - we use the
     * opal_async one so we don't startup extra threads if
     * not needed */
    if (mca_ess_tool_component.async) {
        orte_event_base = opal_progress_thread_init(NULL);
        progress_thread_running = true;
    }

    /* setup the tool connection flags */
    flags = 0;
    if (mca_ess_tool_component.do_not_connect) {
        flags = 0x01;
    } else if (mca_ess_tool_component.system_server_first) {
        flags = 0x02;
    } else if (mca_ess_tool_component.system_server_only) {
        flags = 0x04;
    }

    /* do the standard tool init */
    if (ORTE_SUCCESS != (ret = orte_ess_base_tool_setup(flags))) {
        ORTE_ERROR_LOG(ret);
        error = "orte_ess_base_tool_setup";
        goto error;
    }

    return ORTE_SUCCESS;

 error:
    if (ORTE_ERR_SILENT != ret && !orte_report_silent_errors) {
        orte_show_help("help-orte-runtime.txt",
                       "orte_init:startup:internal-failure",
                       true, error, ORTE_ERROR_NAME(ret), ret);
    }

    return ret;
}

static int rte_finalize(void)
{
    /* use the std finalize routing */
    orte_ess_base_tool_finalize();

    /* release the event base */
    if (progress_thread_running) {
        opal_progress_thread_finalize(NULL);
        progress_thread_running = false;
    }
    return ORTE_SUCCESS;
}

/*
 * If we are a tool-without-name, then we look just like the HNP.
 * In that scenario, it could be beneficial to get a core file, so
 * we call abort.
 */
static void rte_abort(int status, bool report)
{
    /* do NOT do a normal finalize as this will very likely
     * hang the process. We are aborting due to an abnormal condition
     * that precludes normal cleanup
     *
     * We do need to do the following bits to make sure we leave a
     * clean environment. Taken from orte_finalize():
     * - Assume errmgr cleans up child processes before we exit.
     */

    /* CRS cleanup since it may have a named pipe and thread active */
    orte_cr_finalize();

    /* - Clean out the global structures
     * (not really necessary, but good practice)
     */
    orte_proc_info_finalize();

    /* Now just exit */
    exit(status);
}
