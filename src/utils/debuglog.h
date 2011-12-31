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


/** Queries the current thread ID. */
long getTID();


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
*/
#if defined(DEBUG) || defined(DEBUG_LOG)
  /** Raise assertion
   * @param file Code file, usually \c __FILE__
   * @param line Line number in code file, usually \c __LINE__
   * @param msg Error message
   * @remarks This function never returns. */
  void dassert(const char* file, int line, const char* msg);
  /** @brief Normal assertion
   * @example ASSERT(some_pointer != NULL);
   * @details If \a expr is \c false the program aborts with an error message.
   * \a expr is only evaluated in debug builds. */
  #define ASSERT(expr) ((expr) ? (void)0 : dassert(__FILE__, __LINE__, #expr))
  /** @brief Side-effect free assertion (return value check)
   * @example RASSERT(some_function_that_sould_return_true());
   * @details \a expr is only checked in debug builds, but \a expr is always evaluated. */
  #define RASSERT(expr) (!!(expr) ? (void)0 : dassert(__FILE__, __LINE__, #expr))
  /** @brief Return value assertion
   * @example XASSERT(some_function, == NO_ERROR);
   * @details \a cond is only checked in debug builds, but \a expr is always evaluated.
   * If "\a expr \a cond" evaluates false the program aborts with an error message. */
  #define XASSERT(expr, cond) (((expr) cond) ? (void)0 : dassert(__FILE__, __LINE__, #expr" "#cond))
  /** @brief Pointer validation (roughly)
   * @example PASSERT(some_pointer);
   * @details Asserts if the passed pointer is definitely invalid, i.e. points to bad memory.
   * NULL pointers are valid.
   * @remarks The check is not bullet proof. It only checks for pointers that are definitely invalid. */
  #define PASSERT(expr) ASSERT(!(expr) || ((unsigned)(expr) >= 0x10000 && (unsigned)(expr) < 0x40000000))

  /** Raise C runtime assertion and show value of \c errno.
   * @param file Code file, usually \c __FILE__
   * @param line Line number in code file, usually \c __LINE__
   * @param msg Error message
   * @remarks This function never returns. */
  void cassert(const char* file, int line, const char* msg);
  /** @brief C runtime assertion
   * @details Same as \c ASSERT, but the value of \c errno is shown in the error message too. */
  #define CASSERT(expr) ((expr) ? (void)0 : cassert(__FILE__, __LINE__, #expr))
  /** @brief C runtime assertion
   * @details Same as \c XASSERT, but the value of \c errno is shown in the error message too. */
  #define CXASSERT(expr, cond) (((expr) cond) ? (void)0 : cassert(__FILE__, __LINE__, #expr" "#cond))

  /** Raise OS/2 API assertion and show value of \a apiret.
   * @param apiret OS/2 API return code. If \a apiret ist non-zero \c oassert will fail.
   * @param file Code file, usually \c __FILE__
   * @param line Line number in code file, usually \c __LINE__
   * @param msg Error message */
  void oassert(unsigned long apiret, const char* file, int line, const char* msg);
  /** @brief OS/2 core assertion
   * @example OASSERT(rc);
   * @details Similar to \c ASSERT, but the value of \a apiret is checked against \c NO_ERROR.
   * The \a apiret expression is evaluated only in debug builds and it may be evaluated more than once. */
  #define OASSERT(apiret) ((apiret) ? (void)0 : oassert(apiret, __FILE__, __LINE__, #apiret))
  /** @brief OS/2 return value assertion
   * @example OASSERT(DosClose(hf));
   * @details Similar to \c XASSERT, but the value of \a apiret is checked against \c NO_ERROR
   * and if not equal the application aborts with an error message showing the value of \a apiret too.
   * The \a apiret expression is always evaluated exactly once. */
  #define ORASSERT(apiret) oassert(apiret, __FILE__, __LINE__, #apiret)

  /** Raise OS/2 PM assertion and show value of \c WinGetErrorInfo.
   * @param file Code file, usually \c __FILE__
   * @param line Line number in code file, usually \c __LINE__
   * @param msg Error message */
  void pmassert(const char* file, int line, const char* msg);
  /** @brief PM assertion
   * @details Same as \c ASSERT, but the value of \c WinGetErrorInfo is shown too. */
  #define PMASSERT(expr) ((expr) ? (void)0 : pmassert(__FILE__, __LINE__, #expr))
  /** @brief PM assertion
   * @details Same as \c XASSERT, but the value of \c WinGetErrorInfo is shown too. */
  #define PMXASSERT(expr, cond) (((expr) cond) ? (void)0 : pmassert(__FILE__, __LINE__, #expr" "#cond))
  /** @brief PM return value check assertion
   * @example PMRASSERT(WinPostMsg(hwnd, WM_Close, 0, 0));
   * @details Similar to \c PMXASSERT but with the default condition to check whether \a expr is true.
   * In fact also the same as PMASSERT but expression is always evaluated. */
  #define PMRASSERT(expr) (!!(expr) ? (void)0 : pmassert(__FILE__, __LINE__, #expr))
  /** PM assertion
   * Similar to \c PMASSERT, but do not check the return value of \a expr at all.
   * Check only if \c WinGetLastError returns non-zero. */
  #define PMEASSERT(expr) ((expr), pmassert(__FILE__, __LINE__, #expr))
#else
  #define ASSERT(erpr)
  #define RASSERT(expr) (expr)
  #define XASSERT(expr, cond) (expr)
  #define PASSERT(expr)

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
