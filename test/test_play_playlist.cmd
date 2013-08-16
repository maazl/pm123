/**/
dir = TRANSLATE(DIRECTORY())
dirurl = 'FILE:///'TRANSLATE(dir,'/','\')

CALL CallPipe 'play 'dir'\data\list.lst'
CALL Assert 'RESULT', '= 0'

CALL CallPipe 'current root'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/LIST.LST"'

CALL CallPipe 'current'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/TEST.OGG"'

CALL SysSleep(1)

lasttime = CallPipe('time')
CALL Assert 'DATATYPE(lasttime,"N")', ''
CALL Assert 'lasttime', '> 0'

EXIT


CallPipe: PROCEDURE EXPOSE lastcmd
  lastcmd = ARG(1)
  CALL 'pipecmd' ARG(1)
  IF LENGTH(result) <= 2 THEN
    RETURN
  RETURN SUBSTR(result, 1, LENGTH(result)-2)

Assert:
  INTERPRET 'result = 'ARG(1)
  INTERPRET 'IF \(result 'ARG(2)') THEN EXIT ''Expected "''ARG(1) ARG(2)''", found "''result''" "''ARG(3)''", last command: "''lastcmd''"'''
  RETURN

Fail:
  CALL LINEOUT STDERR, result
  EXIT ARG(1)

