/* Wrapper from libatomic_ops to pm123's interlocked API */

#ifndef ATOMIC_OPS_H
#define ATOMIC_OPS_H

#include <config.h>
#include <interlocked.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned AO_t;

INLINE AO_t AO_load_full(AO_t* p)
{ return *p; }

INLINE void AO_store_full(AO_t* p, AO_t v)
{ *p = v; }

INLINE AO_t AO_fetch_and_add_full(AO_t* p, AO_t v)
{ return InterlockedXad(p, v);
}

INLINE AO_t AO_fetch_and_add1_full(AO_t* p)
{ return InterlockedXad(p, 1); }

INLINE AO_t AO_fetch_and_sub1_full(AO_t* p)
{ return InterlockedXad(p, -1); }

INLINE bool AO_compare_and_swap_full(AO_t* p, AO_t o, AO_t n)
{ return InterlockedCxc(p, o, n) == o;
}

#ifdef __cplusplus
}
#endif

#endif
