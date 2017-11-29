/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2013 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2011      Sandia National Laboratories. All rights reserved.
 * Copyright (c) 2014-2017 Los Alamos National Security, LLC. All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef OPAL_SYS_ARCH_ATOMIC_H
#define OPAL_SYS_ARCH_ATOMIC_H 1

/**********************************************************************
 *
 * Memory Barriers
 *
 *********************************************************************/
#define OPAL_HAVE_ATOMIC_MEM_BARRIER 1

static inline void opal_atomic_mb(void)
{
    __sync_synchronize();
}

static inline void opal_atomic_rmb(void)
{
    __sync_synchronize();
}

static inline void opal_atomic_wmb(void)
{
    __sync_synchronize();
}

#define MB() opal_atomic_mb()

/**********************************************************************
 *
 * Atomic math operations
 *
 *********************************************************************/

#define OPAL_HAVE_ATOMIC_COMPARE_EXCHANGE_32 1

static inline bool opal_atomic_compare_exchange_strong_32 (volatile int32_t *addr, int32_t *oldval, int32_t newval)
{
    int32_t prev = __sync_val_compare_and_swap (add, *oldval, newval);
    bool ret = prev == *oldval;
    *oldval = prev;
    return ret;
}

#define opal_atomic_compare_exchange_strong_acq_32 opal_atomic_compare_exchange_strong_32
#define opal_atomic_compare_exchange_strong_rel_32 opal_atomic_compare_exchange_strong_32

#define OPAL_HAVE_ATOMIC_MATH_32 1

#define OPAL_HAVE_ATOMIC_ADD_32 1
static inline int32_t opal_atomic_add_fetch_32(volatile int32_t *addr, int32_t delta)
{
    return __sync_add_and_fetch(addr, delta);
}

#define OPAL_HAVE_ATOMIC_AND_32 1
static inline int32_t opal_atomic_and_fetch_32(volatile int32_t *addr, int32_t value)
{
    return __sync_and_and_fetch(addr, value);
}

#define OPAL_HAVE_ATOMIC_OR_32 1
static inline int32_t opal_atomic_or_fetch_32(volatile int32_t *addr, int32_t value)
{
    return __sync_or_and_fetch(addr, value);
}

#define OPAL_HAVE_ATOMIC_XOR_32 1
static inline int32_t opal_atomic_xor_fetch_32(volatile int32_t *addr, int32_t value)
{
    return __sync_xor_and_fetch(addr, value);
}

#define OPAL_HAVE_ATOMIC_SUB_32 1
static inline int32_t opal_atomic_sub_fetch_32(volatile int32_t *addr, int32_t delta)
{
    return __sync_sub_and_fetch(addr, delta);
}

#if OPAL_ASM_SYNC_HAVE_64BIT

#define OPAL_HAVE_ATOMIC_COMPARE_EXCHANGE_64 1

static inline bool opal_atomic_compare_exchange_strong_64 (volatile int64_t *addr, int64_t *oldval, int64_t newval)
{
    int64_t prev = __sync_val_compare_and_swap (add, *oldval, newval);
    bool ret = prev == *oldval;
    *oldval = prev;
    return ret;
}

#define opal_atomic_compare_exchange_strong_acq_64 opal_atomic_compare_exchange_strong_64
#define opal_atomic_compare_exchange_strong_rel_64 opal_atomic_compare_exchange_strong_64

#define OPAL_HAVE_ATOMIC_MATH_64 1
#define OPAL_HAVE_ATOMIC_ADD_64 1
static inline int64_t opal_atomic_add_fetch_64(volatile int64_t *addr, int64_t delta)
{
    return __sync_add_and_fetch(addr, delta);
}

#define OPAL_HAVE_ATOMIC_AND_64 1
static inline int64_t opal_atomic_and_fetch_64(volatile int64_t *addr, int64_t value)
{
    return __sync_and_and_fetch(addr, value);
}

#define OPAL_HAVE_ATOMIC_OR_64 1
static inline int64_t opal_atomic_or_fetch_64(volatile int64_t *addr, int64_t value)
{
    return __sync_or_and_fetch(addr, value);
}

#define OPAL_HAVE_ATOMIC_XOR_64 1
static inline int64_t opal_atomic_xor_fetch_64(volatile int64_t *addr, int64_t value)
{
    return __sync_xor_and_fetch(addr, value);
}

#define OPAL_HAVE_ATOMIC_SUB_64 1
static inline int64_t opal_atomic_sub_fetch_64(volatile int64_t *addr, int64_t delta)
{
    return __sync_sub_and_fetch(addr, delta);
}

#endif

#if OPAL_HAVE_SYNC_BUILTIN_CSWAP_INT128
static inline bool opal_atomic_compare_exchange_strong_128 (volatile opal_int128_t *addr,
                                                            opal_int128_t *oldval, opal_int128_t newval)
{
    opal_int128_t prev = __sync_val_compare_and_swap (addr, *oldval, newval);
    bool ret = prev == *oldval;
    *oldval = prev;
    return ret;
}

#define OPAL_HAVE_ATOMIC_COMPARE_EXCHANGE_128 1

#endif

#endif /* ! OPAL_SYS_ARCH_ATOMIC_H */
