/**/
dir = TRANSLATE(DIRECTORY())
dirurl =  'FILE:///'TRANSLATE(dir,'/','\')

CALL CallPipe 'reset'
CALL CallPipe 'pl clear'

CALL CallPipe 'pl nextitem'
CALL Assert 'result', '= ""'

CALL CallPipe 'pl add "'dirurl'/DATA/TEST.OGG"'
CALL Assert 'result', '= "1"'
/* now: TEST */
CALL CallPipe 'pl itemindex'
CALL Assert 'result', '= ""'
CALL CallPipe 'pl previtem'
CALL Assert 'TRANSLATE(result)', '= "'dirurl'/DATA/TEST.OGG"'
/* now: (TEST) */

CALL CallPipe 'pl add "'dirurl'/DATA/TEST2.OGG" "'dirurl'/DATA/TEST3.OGG"'
CALL Assert 'result', '= "2"'
/* now: TEST2 TEST3 (TEST) */
CALL CallPipe 'pl item'
CALL Assert 'TRANSLATE(result)', '= "'dirurl'/DATA/TEST.OGG"'
CALL CallPipe 'pl itemindex'
CALL Assert 'result', '= "3"'
CALL CallPipe 'pl previtem'
CALL Assert 'TRANSLATE(result)', '= "'dirurl'/DATA/TEST3.OGG"'
/* now: TEST2 (TEST3) TEST */

CALL CallPipe 'pl remove'
CALL Assert 'TRANSLATE(result)', '= "'dirurl'/DATA/TEST3.OGG"'
/* now: TEST2 (TEST) */
CALL CallPipe 'pl itemindex'
CALL Assert 'result', '= "2"'
CALL CallPipe 'pl remove'
CALL Assert 'TRANSLATE(result)', '= "'dirurl'/DATA/TEST.OGG"'
/* now: TEST2 */
CALL CallPipe 'pl itemindex'
CALL Assert 'result', '= ""'

CALL CallPipe 'dir "'dirurl'/DATA/DIR1"'
CALL Assert 'result', '= "1"'
/* now: TEST2 DIR1/TEST.OGG */
CALL CallPipe 'pl previtem'
CALL Assert 'TRANSLATE(result)', '= "'dirurl'/DATA/DIR1/TEST.OGG"'
/* now: TEST2 (DIR1/TEST.OGG) */
CALL CallPipe 'pl itemindex'
CALL Assert 'result', '= "2"'

CALL CallPipe 'rdir "'dirurl'/DATA/DIR1"'
CALL Assert 'result', '= "3"'
CALL CallPipe 'pl itemindex'
CALL Assert 'result', '= "5"'
CALL CallPipe 'pl item'
CALL Assert 'TRANSLATE(result)', '= "'dirurl'/DATA/DIR1/TEST.OGG"'

EXIT

CallPipe: PROCEDURE EXPOSE lastcmd
  lastcmd = ARG(1)
  CALL 'pipecmd' ARG(1)
  IF LENGTH(result) <= 2 THEN
    RETURN ''
  RETURN SUBSTR(result, 1, LENGTH(result)-2)

Assert:
  INTERPRET 'result = 'ARG(1)
  IF ARG(2,'e') THEN
    INTERPRET 'IF \(result 'ARG(2)') THEN CALL Fail ''Expected ''ARG(1) ARG(2)'', found "''result''"'''
  ELSE IF result \= TRANSLATE(ARG(1)) THEN
    EXIT 'Did not expect the expression 'ARG(1)' to be defined: "'result'"'
  RETURN

Fail:
  /*CALL LINEOUT STDERR, result*/
  EXIT ARG(1) '- last command: 'lastcmd
