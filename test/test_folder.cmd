/**/
dir = TRANSLATE(DIRECTORY())
dirurl =  'FILE:///'TRANSLATE(dir,'/','\')

CALL CallPipe 'playlist' dir'\data\dir1\?Allfiles=1'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/DIR1/?ALLFILES=1"'

CALL CallPipe 'pl next'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/DIR1/TEST.INVALID"'
CALL CallPipe 'pl index'
CALL Assert 'TRANSLATE(RESULT)', '= "1"'
CALL CallPipe 'pl next 1'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/DIR1/TEST.OGG"'
CALL CallPipe 'pl index'
CALL Assert 'TRANSLATE(RESULT)', '= "2"'

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
