/*
 * Copyright 2006 Marcel Mueller
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

/* Configuration dependant log functions.
 * To post a log message write
 *   DEBUGLOG(("Format string with printf parameters like %s\n", "parameter"));
 * Note the double braces!
 * If the application is compiled in debug mode (defining DEBUG) the message
 * is written to stderr which can be easily captured by " 2>logfile".
 * Otherwise the line will not be compiled at all. Even the arguments are not
 * evaluated for their side effects. So be sure not to use expressions with
 * side effects.
 */

#ifndef _DEBUGLOG_H
#define _DEBUGLOG_H

#ifdef DEBUG

  #include <stdio.h>
  #include <stdarg.h>
  #include <time.h>

  // log to stderr
  static void
  debuglog( const char* fmt, ... )
  {
    va_list va;
    PTIB ptib;

    va_start( va, fmt );
    DosGetInfoBlocks( &ptib, NULL );
    DosEnterCritSec();
    fprintf( stderr, "%x %d\t", clock(), ptib->tib_ptib2->tib2_ultid );
    vfprintf( stderr, fmt, va );
    DosExitCritSec();
    va_end( va );
  }

  #define DEBUGLOG(x) debuglog x
#else
  // turn the log lines into a no-op, not even evaluating it's arguments
  #define DEBUGLOG(x)
#endif

// level 2 log
#if defined(DEBUG) && DEBUG >= 2
  #define DEBUGLOG2(x) debuglog x
#else
  #define DEBUGLOG2(x)
#endif

#endif