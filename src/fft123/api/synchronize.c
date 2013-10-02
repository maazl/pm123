/*
 * Copyright (c) 2013 Marcel Mueller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#define INCL_BASE
#include "api.h"
#include <os2.h>
#include <interlocked.h>


static volatile HMTX Mutex;

void enter_sync()
{
    if (Mutex == NULL)
    {   // first call => initialize mutex
        HMTX newmtx;
        DosCreateMutexSem(NULL, &newmtx, 0, FALSE);
        if (InterlockedCxc((volatile unsigned*)&Mutex, 0, (unsigned)newmtx) != 0)
            // compare exchange failed => destroy mutex
            DosCloseMutexSem(newmtx);
    }
    DosRequestMutexSem(Mutex, SEM_INDEFINITE_WAIT);
}

void leave_sync()
{
    DosReleaseMutexSem(Mutex);
}
