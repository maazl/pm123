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

pipe = '\pipe\pm123'
CALL VALUE 'PIPE',pipe,'OS2ENVIRONMENT'

IF RxFuncAdd('SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs') = 0 THEN
  CALL SysLoadFuncs

/* init */
CALL DoInit

/* execute test cases */
CALL SysFileTree 'test_*.cmd', files, 'FO'
DO i = 1 TO files.0
  file = FILESPEC('N', files.i)
  file = SUBSTR(file, 1, LENGTH(file)-4)
  CALL DoTest file
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
    SAY "failed: "result
    summary.failed = summary.failed + 1
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

Error: PROCEDURE
  CALL LINEOUT STDERR, ARG(2)
  EXIT ARG(1)


