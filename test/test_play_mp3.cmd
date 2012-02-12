/**/
dir = DIRECTORY()

CALL CallPipe 'play 'dir'\data\test.mp3'
CALL Assert 'RESULT', '= 0'

CALL SysSleep(1)

lasttime = CallPipe('time')
CALL Assert 'DATATYPE(lasttime,"N")', ''
CALL Assert 'lasttime', '> 0'

CALL SysSleep(1)
time = CallPipe('time')
CALL Assert 'DATATYPE(time,"N")', ''
CALL Assert 'time', '> lasttime+1'

CALL CallPipe 'pause'
CALL Assert 'RESULT', '= 0'

lasttime = CallPipe('time')
CALL Assert 'DATATYPE(time,"N")', ''
CALL Assert 'time', '\= 0'

time = CallPipe('time')
CALL Assert 'DATATYPE(time,"N")', ''
CALL Assert 'time', '= lasttime'
lasttime = time

CALL CallPipe 'pause 0'
CALL Assert 'RESULT', '= 0'

time = CallPipe('time')
CALL Assert 'DATATYPE(time,"N")', ''
CALL Assert 'time', '> lasttime'

EXIT


CallPipe: PROCEDURE EXPOSE lastcmd
  lastcmd = ARG(1)
  CALL 'pipecmd' ARG(1)
  IF LENGTH(result) <= 2 THEN
    RETURN
  RETURN SUBSTR(result, 1, LENGTH(result)-2)

Assert:
  INTERPRET 'result = 'ARG(1)
  INTERPRET 'IF \(result 'ARG(2)') THEN EXIT ''Expected "''ARG(1) ARG(2)''", found "''result''", last command: "''lastcmd''"'''
  RETURN

Fail:
  CALL LINEOUT STDERR, result
  EXIT ARG(1)