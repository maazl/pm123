/***
  This file is part of PulseAudio.

  Copyright 2006 Pierre Ossman <ossman@cendio.se> for Cendio AB

  PulseAudio is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 2.1 of the License,
  or (at your option) any later version.

  PulseAudio is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with PulseAudio; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.
***/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#define INCL_ERRORS
#define INCL_DOS
#include <os2.h>

#include <pulse/xmalloc.h>
#include <pulsecore/log.h>
#include <pulsecore/once.h>
#include <pulsecore/dynarray.h>

#include "thread.h"

#include <debuglog.h>

/* We pass the thread ID directly as a pointer.
struct pa_thread {
    TID id;
    void* userdata;
    char* name;
};*/

#define TLS_USERDATA 0
#define TLS_NAME     1

/* Index of registered TLS objects.
 * These objects are of type pa_free_cb_t. */
static pa_dynarray* tls_index = NULL;
static unsigned tls_next_free = TLS_NAME+1;
/* Mutex for the above */
static pa_mutex* tls_mutex = NULL;

/* Well, OS/2 likes small thread numbers and using the TID as index to
 * static storage is implicitly thread-safe.
 * The array contains a dynarray with the TLS pointers for each thread. */
static pa_dynarray* tls_data[128] = { NULL };

/* This function is used to identify used TLS entries without a free_cb.
 */
static void no_op_free(void* data){}

static void *tls_get(unsigned ix, TID tid) {
    void* r = NULL;
    pa_dynarray* arr = tls_data[tid];

    if (arr)
        r = pa_dynarray_get(arr, ix);
    DEBUGLOG(("thread-os2:tls_get(%u, %lu) -> %p -> %p\n", ix, tid, arr, r));

    return r;
}

static void *tls_set(unsigned ix, TID tid, void *userdata) {
    void *r = NULL;
    pa_dynarray* arr = tls_data[tid];

    if (!arr) {
        arr = tls_data[tid] = pa_dynarray_new();
        pa_assert(arr);
    } else
        r = pa_dynarray_get(arr, ix);

    DEBUGLOG(("thread-os2:tls_set(%u, %lu, %p) -> %p -> %p\n", ix, tid, arr, r));
    pa_dynarray_put(arr, ix, userdata);

    return r;
}


static void APIENTRY internal_thread_func(ULONG param) {
    pa_thread_func_t tfn = (pa_thread_func_t)param;
    PTIB tib;
    void* userdata;

    DosGetInfoBlocks(&tib, NULL);
    userdata = tls_get(TLS_USERDATA, tib->tib_ptib2->tib2_ultid);

    DEBUGLOG(("thread-os2:internal_thread_func(%p) - %lu, %p\n", tfn, tib->tib_ptib2->tib2_ultid, userdata));
    (*tfn)(userdata);
}

static void APIENTRY monitor_thread_func(ULONG param) {
    TID tid;
    DEBUGLOG(("thread-os2:monitor_thread_func()\n"));

    do {
        pa_dynarray* arr;
        tid = 0;
        DosWaitThread(&tid, DCWW_WAIT);
        DEBUGLOG(("thread-os2:monitor_thread_func: thread %lu has ended\n", tid));

        arr = tls_data[tid];
        if (arr) {
            unsigned ix = 0;
            unsigned ixe = pa_dynarray_size(arr);
            while (ix < ixe) {
                pa_free_cb_t cb = pa_dynarray_get(tls_index, ix);
                DEBUGLOG(("thread-os2:monitor_thread_func: %i -> cb=%p\n", ix, cb));
                if (cb && cb != &no_op_free) {
                    void* data = pa_dynarray_get(arr, ix);
                    DEBUGLOG(("thread-os2:monitor_thread_func: %i -> data=%p\n", ix, data));
                    if (data)
                        (*cb)(data);
                }
                ++ix;
            }
            pa_dynarray_free(arr, NULL, NULL);
            tls_data[tid] = NULL;
        }
    } while (tid != 1);
    /* If thread 1 dies OS/2 will terminate the application */
}

static void init() {
    DEBUGLOG(("thread-os2:init()\n"));
    /* Assuming that at least the second thread is created after calling this function
     * the PA_ONCE pattern need not to be thread safe in case the check fails.
     */
    if (tls_mutex == NULL) {
        APIRET rc;
        TID id;
        tls_mutex = pa_mutex_new(FALSE, FALSE);
        pa_assert(tls_mutex);

        tls_index = pa_dynarray_new();
        pa_assert(tls_index);
        pa_dynarray_put(tls_index, TLS_USERDATA, &no_op_free);
        pa_dynarray_put(tls_index, TLS_NAME, &pa_xfree);

        rc = DosCreateThread(&id, monitor_thread_func, 0, 0, 65536*1024);
        pa_assert(rc == 0);
    }
}


pa_thread* pa_thread_new(const char* name, pa_thread_func_t thread_func, void *userdata) {
    TID id;
    DEBUGLOG(("pa_thread_new(%s, %p, %p)\n", name, thread_func, userdata));
    pa_assert(thread_func);

    init();

    APIRET rc = DosCreateThread(&id, internal_thread_func, (ULONG)thread_func, CREATE_SUSPENDED, 4096*1024);
    if (rc != 0)
    {   pa_log_error("ERROR: failed to create thread: %u", rc);
        return NULL;
    }

    /* Store userdata in TLS too. */
    tls_set(TLS_USERDATA, id, userdata);
    if (name)
        tls_set(TLS_NAME, id, pa_xstrdup(name));
    /* now we can start the thread */
    DEBUGLOG(("pa_thread_new: resume new thread %lu\n", id));
    DosResumeThread(id);
    return (pa_thread*)id;
}

int pa_thread_is_running(pa_thread *t) {
    APIRET r;
    pa_assert(t);

    r = DosWaitThread((TID*)&t, DCWW_NOWAIT);
    DEBUGLOG(("pa_thread_is_running(%lu) -> %lu\n", t, r));
    return r == ERROR_THREAD_NOT_TERMINATED;
}

void pa_thread_free(pa_thread *t) {
    pa_assert(t);

    DEBUGLOG(("pa_thread_free(%lu)\n", t));
    pa_thread_join(t);
}

int pa_thread_join(pa_thread *t) {
    APIRET rc;
    pa_assert(t);

    DEBUGLOG(("pa_thread_join(%lu)\n", t));
    rc = DosWaitThread((TID*)&t, DCWW_WAIT);
    DEBUGLOG(("pa_thread_join(%lu): completed\n", t));

    return rc ? -1 : 0;
}

pa_thread* pa_thread_self(void) {
    PTIB tib;
    DosGetInfoBlocks(&tib, NULL);
    return (pa_thread*)tib->tib_ptib2->tib2_ultid;
}

void pa_thread_yield(void) {
    DEBUGLOG(("pa_thread_yield()\n"));
    DosSleep(0);
}

void* pa_thread_get_data(pa_thread *t) {
    pa_assert(t);
    return tls_get(TLS_USERDATA, (TID)t);
}

void pa_thread_set_data(pa_thread *t, void *userdata) {
    pa_assert(t);
    tls_set(TLS_USERDATA, (TID)t, userdata);
}

void pa_thread_set_name(pa_thread *t, const char *name) {
    pa_assert(t);
    pa_xfree(tls_set(TLS_NAME, (TID)t, pa_xstrdup(name)));
}

const char *pa_thread_get_name(pa_thread *t) {
    pa_assert(t);
    return tls_get(TLS_NAME, (TID)t);
}


pa_tls* pa_tls_new(pa_free_cb_t free_cb) {
    unsigned ix;
    DEBUGLOG(("pa_tls_new(%p)\n", free_cb));

    init();
    pa_mutex_lock(tls_mutex);

    while (pa_dynarray_get(tls_index, tls_next_free) != NULL)
        ++tls_next_free;

    /* avoid NULL values */
    if (free_cb == NULL)
        free_cb = &no_op_free;

    ix = tls_next_free++;
    pa_dynarray_put(tls_index, ix, free_cb);

    pa_mutex_unlock(tls_mutex);

    DEBUGLOG(("pa_tls_new -> %u\n", ix));
    return (pa_tls*)ix;
}

void pa_tls_free(pa_tls *t) {
    unsigned ix = (unsigned)t;
    pa_free_cb_t cb;
    pa_assert(t);

    /* free remaining resources */
    cb = pa_dynarray_get(tls_index, ix);
    DEBUGLOG(("pa_tls_free(%u) - cb = %p\n", ix, cb));
    assert(cb);
    if (cb != &no_op_free) {
        TID tid = 1;
        while (tid < sizeof tls_data / sizeof *tls_data) {
            pa_dynarray* arr = tls_data[tid];
            DEBUGLOG(("pa_tls_free: arr = %p\n", arr));
            if (arr) {
                void* data = pa_dynarray_get(arr, ix);
                DEBUGLOG(("pa_tls_free: data = %p\n", data));
                if (data) {
                    (*cb)(data);
                    pa_dynarray_put(arr, ix, NULL);
                }
            }
            ++tid;
        }
    }

    pa_mutex_lock(tls_mutex);
    pa_dynarray_put(tls_index, ix, NULL);
    if (ix < tls_next_free)
        tls_next_free = ix;
    pa_mutex_unlock(tls_mutex);
}

void *pa_tls_get(pa_tls *t) {
    PTIB tib;
    pa_assert(t);

    DosGetInfoBlocks(&tib, NULL);
    return tls_get(tib->tib_ptib2->tib2_ultid, (unsigned)t);
}

void *pa_tls_set(pa_tls *t, void *userdata) {
    PTIB tib;
    pa_assert(t);

    DosGetInfoBlocks(&tib, NULL);
    return tls_set(tib->tib_ptib2->tib2_ultid, (unsigned)t, userdata);
}
