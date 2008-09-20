/*
 * Copyright 2007 Dmitry A.Steklenev
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *    3. The name of the author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DEBUGMTX_H
#define DEBUGMTX_H

#if defined( DEBUG ) && DEBUG > 1

  #include "debuglog.h"

  INLINE APIRET APIENTRY
  DbgDosRequestMutexSem( HMTX hmtx, ULONG ulTimeout, const char* file, int line )
  {
    APIRET rc;
    DEBUGLOG(( "%s: request mutex %08X at line %d\n", file, hmtx, line ));
    rc = DosRequestMutexSem( hmtx, ulTimeout );
    DEBUGLOG(( "%s: request mutex %08X at line %d is completed, rc=%d\n", file, hmtx, line, rc ));
    return rc;
  }

  INLINE APIRET APIENTRY
  DbgDosReleaseMutexSem( HMTX hmtx, const char* file, int line )
  {
    APIRET rc = DosReleaseMutexSem( hmtx );
    DEBUGLOG(( "%s: release mutex %08X at line %d is completed, rc=%d\n", file, hmtx, line, rc ));
    return rc;
  }

  #define DosRequestMutexSem( hmtx, ulTimeout ) \
          DbgDosRequestMutexSem( hmtx, ulTimeout, __FILE__, __LINE__ )

  #define DosReleaseMutexSem( hmtx ) \
          DbgDosReleaseMutexSem( hmtx, __FILE__, __LINE__ )
#endif
#endif
