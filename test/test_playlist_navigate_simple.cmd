/**/
dir = TRANSLATE(DIRECTORY())
dirurl = 'FILE:///'TRANSLATE(dir,'/','\')

CALL CallPipe 'playlist' dir'\data\list2.lst'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/LIST2.LST"'

CALL CallPipe 'pl nextitem 2 song'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/TEST3.OGG"'
CALL CallPipe 'pl nextitem 1 song'
CALL Assert 'TRANSLATE(RESULT)', '= ""'

CALL CallPipe 'pl previtem 1 invalid,song'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/NOT_EXIST.OGG"'
CALL CallPipe 'pl previtem 1 playlist'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/LIST1.LST"'

CALL CallPipe 'pl enter'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/LIST1.LST"'
CALL CallPipe 'pl itemindex'
CALL Assert 'TRANSLATE(RESULT)', '= ""'

CALL CallPipe 'pl previtem 1 song'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/TEST.OGG"'
CALL CallPipe 'pl nextitem 1 song,playlist'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/LIST2.LST"'
CALL CallPipe 'pl enter'
CALL Assert 'TRANSLATE(RESULT)', '= ""'

CALL CallPipe 'pl leave'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/LIST1.LST"'

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