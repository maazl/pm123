/**/
dir = TRANSLATE(DIRECTORY())
dirurl =  'FILE:///'TRANSLATE(dir,'/','\')

CALL CallPipe 'reset'
CALL Assert 'result', '= ""'
CALL CallPipe 'pl current'
CALL Assert 'RIGHT(result,10)', '= "/PM123.LST"'
CALL CallPipe 'playlist' dir'\data\list1.lst'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/LIST1.LST"'
CALL CallPipe 'cd' dir
CALL CallPipe 'cd'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/"'
CALL CallPipe 'playlist data/list2.lst'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/LIST2.LST"'

CALL CallPipe 'pl next'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/TEST2.OGG"'
CALL CallPipe 'pl index'
CALL Assert 'TRANSLATE(RESULT)', '= "1"'
CALL CallPipe 'pl next 2'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/TEST3.OGG"'
CALL CallPipe 'pl index'
CALL Assert 'TRANSLATE(RESULT)', '= "3"'
CALL CallPipe 'pl next 4'
CALL Assert 'TRANSLATE(RESULT)', '= ""'
CALL CallPipe 'pl index'
CALL Assert 'TRANSLATE(RESULT)', '= ""'
CALL CallPipe 'pl prev'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/LIST1.LST"'
CALL CallPipe 'pl prev 2'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/TEST3.OGG"'
CALL CallPipe 'pl reset'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/TEST3.OGG"'
CALL CallPipe 'pl index'
CALL Assert 'TRANSLATE(RESULT)', '= ""'

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
