/**/
dir = TRANSLATE(DIRECTORY())
dirurl =  'FILE:///'TRANSLATE(dir,'/','\')

CALL CallPipe 'playlist 'dir'\data\list2.lst'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/LIST2.LST"'

/* relative URL */
CALL CallPipe 'pl navigate test3.ogg'
CALL Assert 'TRANSLATE(RESULT)', '= """TEST3.OGG"""'
/* absolute URL */
CALL CallPipe 'pl navigate 'dir'\data\list1.lst'
CALL Assert 'TRANSLATE(RESULT)', '= """LIST1.LST""[2]"'

/* indexed URL */
CALL CallPipe 'pl reset'
CALL CallPipe 'pl navigate ./list1.lst[2]'
CALL Assert 'TRANSLATE(RESULT)', '= """LIST1.LST""[2]"'
/* reverse */
CALL CallPipe 'pl navigate list1.lst[-1]'
CALL Assert 'TRANSLATE(RESULT)', '= """LIST1.LST"""'

/* index only */
CALL CallPipe 'pl navigate [2]'
CALL Assert 'TRANSLATE(RESULT)', '= """NOT_EXIST.OGG"""'
/* with time */
CALL CallPipe 'pl navigate [-1];0:2.8'
CALL Assert 'TRANSLATE(TRANSLATE(RESULT),";","0a0d"x)', '= """TEST3.OGG"" ;0:02.8"'

/* up and down */
CALL CallPipe 'pl navigate ..;list1.lst;list1.lst'
CALL Assert 'TRANSLATE(TRANSLATE(RESULT),";","0a0d"x)', '= """LIST1.LST""[2] ;""LIST1.LST"""'
CALL CallPipe 'pl navigate ..;list1.lst[-1];list2.lst'
CALL Assert 'TRANSLATE(TRANSLATE(RESULT),";","0a0d"x)', '= """LIST1.LST"" ;""LIST2.LST"""'

/* with song time */
CALL CallPipe 'pl navigate .;[1];.3'
CALL Assert 'TRANSLATE(TRANSLATE(RESULT),";","0a0d"x)', '= """LIST1.LST"" ;""TEST.OGG"" ;0:00.3"'
CALL CallPipe 'pl navigate .3'
CALL Assert 'TRANSLATE(TRANSLATE(RESULT),";","0a0d"x)', '= """LIST1.LST"" ;""TEST.OGG"" ;0:00.6"'
CALL CallPipe 'pl navigate ..;[3]'
CALL Assert 'TRANSLATE(TRANSLATE(RESULT),";","0a0d"x)', '= ""'


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