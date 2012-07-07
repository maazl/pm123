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

#define INCL_DOS
#include <os2.h>

#include <pulse/xmalloc.h>
#include <pulsecore/hashmap.h>
#include <pulsecore/log.h>

#include "mutex.h"

#include <debuglog.h>

/* Hack: HMTX is directly stored in the pointer.
 * OS/2 returns a pointer handle anyway.
struct pa_mutex {
    HMTX mutex;
};*/

pa_mutex* pa_mutex_new(pa_bool_t recursive, pa_bool_t inherit_priority) {
    HMTX m;

    if (DosCreateMutexSem(NULL, &m, 0, FALSE) != 0)
        return NULL;

    DEBUGLOG(("pa_mutex_new(%u, %u) -> %p\n", recursive, inherit_priority, m));

    return (pa_mutex*)m;
}

void pa_mutex_free(pa_mutex *m) {
    DEBUGLOG(("pa_mutex_free(%p)\n", m));
    assert(m);

    DosCloseMutexSem((HMTX)m);
}

void pa_mutex_lock(pa_mutex *m) {
    DEBUGLOG2(("pa_mutex_lock(%p)\n", m));
    assert(m);

    DosRequestMutexSem((HMTX)m, SEM_INDEFINITE_WAIT);
    DEBUGLOG2(("pa_mutex_lock(%p) granted\n", m));
}

void pa_mutex_unlock(pa_mutex *m) {
    DEBUGLOG2(("pa_mutex_unlock(%p)\n", m));
    assert(m);

    DosReleaseMutexSem((HMTX)m);
}


typedef struct cond_wait_entry {
    struct cond_wait_entry* link;
    unsigned state;
} cond_wait_entry;

struct pa_cond {
    /// Pointer to linked list of cond_wait_entry.
    cond_wait_entry* volatile wait_queue;
    HEV event_sem;
};

pa_cond *pa_cond_new(void) {
    pa_cond *c;
    APIRET rc;

    c = pa_xnew(pa_cond, 1);
    c->wait_queue = NULL;
    rc = DosCreateEventSem(NULL, &c->event_sem, 0, FALSE);
    pa_assert(rc == 0);

    DEBUGLOG(("pa_cond_new() - %p\n", c));
    return c;
}

void pa_cond_free(pa_cond *c) {
    APIRET rc;

    pa_assert(c);
    DEBUGLOG(("pa_cond_free(%p{%p,%p})\n", c, c->wait_queue, c->event_sem));
    pa_assert(c->wait_queue == NULL);
    rc = DosCloseEventSem(c->event_sem);
    pa_assert(rc == 0);

    pa_xfree(c);
}

void pa_cond_signal(pa_cond *c, int broadcast) {
    cond_wait_entry* we;
    cond_wait_entry* old;
    ULONG dummy;

    pa_assert(c);
    DEBUGLOG(("pa_cond_signal(%p{%p}, %i)\n", c, c->wait_queue, broadcast));

    if (broadcast) {
        we = (cond_wait_entry*)InterlockedXch((volatile unsigned*)&c->wait_queue, NULL);
        // wake all threads
        while (we) {
            old = we->link;
            we->state = 1;
            we = old;
        }

    } else {
        we = c->wait_queue;
        // remove first item.
        do {
            if (we == NULL)
                return;
            old = we;
            we = (cond_wait_entry*)InterlockedCxc((volatile unsigned*)&c->wait_queue, (unsigned)old, (unsigned)we->link);
        } while (we != old);
        // wake one thread
        we->state = 1;
    }
    DosPostEventSem(c->event_sem);
    DosResetEventSem(c->event_sem, &dummy);
}

int pa_cond_wait(pa_cond *c, pa_mutex *m) {
    cond_wait_entry wait;
    cond_wait_entry* old;
    APIRET rc;
    ULONG dummy;

    DEBUGLOG(("pa_cond_wait(%p{%p}, %p)\n", c, c->wait_queue, m));
    assert(c);
    assert(m);

    // Enqueue wait item
    wait.state = 0;
    do
    { wait.link = c->wait_queue;
      // CXC loop
      do {
          old = wait.link;
          wait.link = (cond_wait_entry*)InterlockedCxc((volatile unsigned*)&c->wait_queue, (unsigned)old, (unsigned)&wait);
      } while (wait.link != old);

      pa_mutex_unlock(m);

      rc = DosWaitEventSem(c->event_sem, SEM_INDEFINITE_WAIT);
      DEBUGLOG(("pa_cond_wait resume - %lu, %u\n", rc, wait.state));
      DosResetEventSem(c->event_sem, &dummy);

      pa_mutex_lock(m);
    } while (!wait.state); // Retry?

    return 0;
}



pa_mutex* pa_static_mutex_get(pa_static_mutex *s, pa_bool_t recursive, pa_bool_t inherit_priority) {
    pa_mutex *m;

    pa_assert(s);

    /* First, check if already initialized and short cut */
    if ((m = pa_atomic_ptr_load(&s->ptr)))
        return m;

    /* OK, not initialized, so let's allocate, and fill in */
    m = pa_mutex_new(recursive, inherit_priority);
    if ((pa_atomic_ptr_cmpxchg(&s->ptr, NULL, m)))
        return m;

    pa_mutex_free(m);

    /* Him, filling in failed, so someone else must have filled in
     * already */
    pa_assert_se(m = pa_atomic_ptr_load(&s->ptr));
    return m;
}
