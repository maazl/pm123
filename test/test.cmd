/*
 * Copyright 2011-2011 M.Mueller
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

SIGNAL OFF ANY

pipe = '\pipe\pm123'
CALL VALUE 'PIPE',pipe,'OS2ENVIRONMENT'

IF RxFuncAdd('SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs') = 0 THEN
  CALL SysLoadFuncs

/* parse command line */
args = ARG(1)
DO i = 1
  PARSE VAR args args.i args
  IF args.i = '' THEN LEAVE
  END
args.0 = i - 1

/* init */
CALL DoInit

/* execute test cases */
CALL SysFileTree 'test_*.cmd', files, 'FO'
CALL QSort files
summary.current = 0
recover:
DO i = summary.current + 1 TO files.0
  summary.current = i
  file = FILESPEC('N', files.i)
  IF args.0 > 0 THEN DO
    include = 0
    DO j = 1 TO args.0
      IF POS(args.j, file) \= 0 THEN DO
        include = 1
        LEAVE
        END
      END
    IF \include THEN ITERATE
    END
  file = SUBSTR(file, 1, LENGTH(file)-4)
  /*SIGNAL ON syntax NAME TestCrashed*/
  CALL DoTest file
  SIGNAL OFF syntax
  END

/* print summary */
CALL DoFinish

EXIT summary.failed


DoInit: PROCEDURE EXPOSE summary.
  summary.passed = 0
  summary.failed = 0

  CALL 'init'
  IF SYMBOL('RESULT') = 'VAR' & result \= '' THEN
    CALL Error 29, "Failed to initialize: "result

  RETURN

DoTest: PROCEDURE EXPOSE summary.
  testcase = TRANSLATE(SUBSTR(FILESPEC('N', ARG(1)), 6), ' ', '_')
  CALL CHAROUT , 'Testing' testcase '... '
  /* setup */
  CALL 'setup'
  IF SYMBOL('RESULT') = 'VAR' & result \= '' THEN
    CALL Error 29, "Failed to setup test case: "result

  INTERPRET("CALL '"ARG(1)"'")
  IF SYMBOL('RESULT') = 'VAR' & result \= '' THEN DO
    SAY "FAILED: "result
    summary.failed = summary.failed + 1
    CALL 'pipecmd' 'getmessages'
    CALL CHAROUT , result
    END
  ELSE DO
    SAY "passed"
    summary.passed = summary.passed + 1
    END

  /* teardown */
  CALL 'teardown'
  RETURN

DoFinish: PROCEDURE EXPOSE summary.
  SAY
  sum = summary.passed + summary.failed
  SAY "Failed: "summary.failed", passed: "summary.passed", total: "sum
  RETURN

TestCrashed:
  /* For some myterious reason this function is called twice on the same error */
  IF summary.signal \= summary.current THEN DO
    summary.signal = summary.current
    SAY "crashed" CONDITION('D')
    SIGNAL OFF syntax
    summary.failed = summary.failed + 1
    /*CALL 'teardown'*/
    END
  ELSE
  SIGNAL recover

/* sort stem variable
   call qsort stem[, first][, last]
   stem
     stem variable name to sort
   first, last (optional)
     sort range tree.index.first to tree.index.last
   This function calls itself recursively.
*/
QSort:
  stem = TRANSLATE(ARG(1))
  IF ARG(2,'e') THEN DO
    IF ARG(3,'e') THEN
      CALL DoQSort ARG(2), ARG(3)
    ELSE
      CALL DoQSort ARG(2), VALUE(stem'.'0)
    END
  ELSE IF ARG(3,'e') THEN
    CALL DoQSort 1, ARG(3)
  ELSE
    CALL DoQSort 1, VALUE(stem'.'0)
  RETURN

DoQSort: PROCEDURE EXPOSE stem (stem)
  lo = ARG(1)
  hi = ARG(2)
  mi = (hi + lo) %2
  m = VALUE(stem'.'mi)
  /*SAY lo hi mi m*/
  DO WHILE hi >= lo
    DO WHILE VALUE(stem'.'lo) < m
      lo = lo +1
      END
    DO WHILE VALUE(stem'.'hi) > m
      hi = hi -1
      END
    IF hi >= lo THEN DO
      tmp = VALUE(stem'.'lo)
      CALL VALUE stem'.'lo, VALUE(stem'.'hi)
      CALL VALUE stem'.'hi, tmp
      lo = lo +1
      hi = hi -1
      END
    END
  IF hi > ARG(1) THEN
    CALL DoQSort ARG(1), hi
  IF ARG(2) > lo THEN
    CALL DoQSort lo, ARG(2)
  RETURN

Error: PROCEDURE
  CALL LINEOUT STDERR, ARG(2)
  EXIT ARG(1)
