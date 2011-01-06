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

/* Configuration dependent log functions.
 * To post a log message write
 *   DEBUGLOG(("Format string with printf parameters like %s\n", "parameter"));
 * Note the double braces!
 * If the application is compiled in debug mode (defining DEBUG) the message
 * is written to stderr which can be easily captured by " 2>logfile".
 * Otherwise the line will not be compiled at all. Even the arguments are not
 * evaluated for their side effects. So be sure not to use expressions with
 * side effects.
 */

#ifndef DEBUGLOG_H
#define DEBUGLOG_H

#include <errorstr.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Logging */
#ifdef DEBUG_LOG
  void debuglog( const char* fmt, ... );

  #define DEBUGLOG(x) debuglog x
#else
  // turn the log lines into a no-op, not even evaluating it's arguments
  #define DEBUGLOG(x)
#endif

// level 2 log
#if defined(DEBUG_LOG) && DEBUG_LOG >= 2
  #define DEBUGLOG2(x) debuglog x
#else
  #define DEBUGLOG2(x)
#endif


/* Assertions

- Normal assertion

    ASSERT(expression)

  Expression is only evaluated in debug builds.
  If expression is false the programm is aborted with an error message.
 
- Side-effect free assertion
 
    RASSERT(expression)
   
  The expression is only checked in debug builds, but the expresion is always evaluated.

- Return value assertion
 
    XASSERT(expression, condition)
   
  The condition is only checked in debug builds, but the expresion is always evaluated.
  If "expression condition" evaluates false the programm is aborted with an error message.

- C runtime assertion
 
    CASSERT(expression)
    CXASSERT(expression, condition)
   
  Same as ASSERT/XASSERT, but the value of errno is shown too.

- OS/2 core assertion 

    OASSERT(apiret)
   
  Similar to ASSERT, but the value of apiret is checked against NO_ERROR.
  The apiret expression is evaluated only in debug builds and it may be evaluated more than once.
 
- OS/2 return value assertion 

    ORASSERT(apiret)
   
  Similar to XASSERT, but the value of apiret is checked against NO_ERROR and if not equal
  the application aborts with an error message showing the value of apiret too.
  The apiret expression is always evaluated exactly once.
 
- PM assertion
 
    PMASSERT(expression)
    PMXASSERT(expression, condition)
 
  Same as ASSERT/XASSERT, but the value of WinGetErrorInfo is shown too.

- PM return value check assertion
 
    PMRASSERT(expression)
 
  Similar to PMXASSERT but with the default condition to check whether expression is true.
  In fact also the same as PMASSERT but expression is always evaluated.

- PM assertion
 
    PMEASSERT(expression)
 
  Similar to PMASSERT, but do not check the return value of expression at all. Check only if WinGetLastError returns non-zero. 

*/
 
#if defined(DEBUG) || defined(DEBUG_LOG)
  void dassert(const char* file, int line, const char* msg);
  #define ASSERT(expr) ((expr) ? (void)0 : dassert(__FILE__, __LINE__, #expr))
  #define RASSERT(expr) (!!(expr) ? (void)0 : dassert(__FILE__, __LINE__, #expr))
  #define XASSERT(expr, cond) (((expr) cond) ? (void)0 : dassert(__FILE__, __LINE__, #expr" "#cond))

  void cassert(const char* file, int line, const char* msg);
  #define CASSERT(expr) ((expr) ? (void)0 : cassert(__FILE__, __LINE__, #expr))
  #define CXASSERT(expr, cond) (((expr) cond) ? (void)0 : cassert(__FILE__, __LINE__, #expr" "#cond))

  void oassert(unsigned long apiret, const char* file, int line, const char* msg);
  #define OASSERT(apiret) ((apiret) ? (void)0 : oassert(apiret, __FILE__, __LINE__, #apiret))
  #define ORASSERT(apiret) oassert(apiret, __FILE__, __LINE__, #apiret)

  void pmassert(const char* file, int line, const char* msg);
  #define PMASSERT(expr) ((expr) ? (void)0 : pmassert(__FILE__, __LINE__, #expr))
  #define PMXASSERT(expr, cond) (((expr) cond) ? (void)0 : pmassert(__FILE__, __LINE__, #expr" "#cond))
  #define PMRASSERT(expr) (!!(expr) ? (void)0 : pmassert(__FILE__, __LINE__, #expr))
  #define PMEASSERT(expr) ((expr), pmassert(__FILE__, __LINE__, #expr))
#else
  #define ASSERT(erpr)
  #define RASSERT(expr) (expr)
  #define XASSERT(expr, cond) (expr)

  #define CASSERT(expr)
  #define CXASSERT(expr, cond) (expr)

  #define OASSERT(expr)
  #define ORASSERT(expr) (expr)

  #define PMASSERT(expr)
  #define PMXASSERT(expr, cond) (expr)
  #define PMRASSERT(expr) (expr)
  #define PMEASSERT(expr) (expr)
#endif

#ifdef __cplusplus
}
#endif

#endif
