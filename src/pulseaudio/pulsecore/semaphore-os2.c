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
#include <pulsecore/macro.h>
#include <pulsecore/log.h>

#include "semaphore.h"

/* Hack: Store the Semaphore handle directly in the pointer.
 * OS/2 returns pointer type handles anyway.
struct pa_semaphore
{
    HEV sema;
};*/

INLINE void set_posts(HEV event, unsigned count)
{   /* Well, there are obviously better implementations,
     * but as long as there are only small numbers it should not be
     * too harmful. */
    while (count--)
        DosPostEventSem(event);
}

pa_semaphore* pa_semaphore_new(unsigned value) {
    HEV s;

    APIRET rc = DosCreateEventSem(NULL, &s, 0, value != 0);
    pa_assert(rc == 0);

    if (value > 1)
        set_posts(s, value-1);

    return (pa_semaphore*)s;
}

void pa_semaphore_free(pa_semaphore *s) {
    pa_assert(s);
    DosCloseEventSem((HEV)s);
}

void pa_semaphore_post(pa_semaphore *s) {
    pa_assert(s);
    DosPostEventSem((HEV)s);
}

void pa_semaphore_wait(pa_semaphore *s) {
    HEV event = (HEV)s;
    ULONG count = 0;
    pa_assert(s);

    do {
        DosWaitEventSem(event, SEM_INDEFINITE_WAIT);
        DosResetEventSem(event, &count);
    } while (count == 0);

    if (count > 1)
        set_posts(event, count-1);
}
