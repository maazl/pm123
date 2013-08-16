/**/
dir = TRANSLATE(DIRECTORY())
dirurl = 'FILE:///'TRANSLATE(dir,'/','\')

CALL CallPipe 'playlist 'dir'\data\list2.lst'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/LIST2.LST"'

/* top level */
CALL CallPipe 'pl navigate *test3.ogg'
CALL Assert 'TRANSLATE(RESULT)', '= """TEST3.OGG"""'
/* level 1 */
CALL CallPipe 'pl navigate *test.ogg'
CALL Assert 'TRANSLATE(RESULT)', '= """LIST1.LST""[2];""TEST.OGG"""'
/* with index */
CALL CallPipe 'pl navigate /;*test.ogg[1]'
CALL Assert 'TRANSLATE(RESULT)', '= """LIST1.LST"";""TEST.OGG"""'
CALL CallPipe 'pl navigate /;*list1.lst[-3]'
CALL Assert 'TRANSLATE(RESULT)', '= """LIST1.LST"""'
/* pattern */
CALL CallPipe 'pl navigate ;?list[-2]'
CALL Assert 'TRANSLATE(RESULT)', '= """LIST1.LST"";""LIST2.LST"""'
CALL CallPipe 'pl navigate **?song[2]'
CALL Assert 'TRANSLATE(RESULT)', '= """LIST1.LST""[2];""TEST.OGG"""'
CALL CallPipe 'pl navigate *?song[-1]'
CALL Assert 'TRANSLATE(RESULT)', '= ""'
CALL CallPipe 'getmessages'

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
