/**/
file = ARG(1)
dir = DIRECTORY()

CALL CallPipe 'play 'dir||file
CALL Assert 'RESULT', '= 0'

CALL SysSleep(1)

lasttime = CallPipe('time')
CALL Assert 'DATATYPE(lasttime,"N")', ''
CALL Assert 'lasttime', '> 0'

CALL SysSleep(1)
time = CallPipe('time')
CALL Assert 'DATATYPE(time,"N")', ''
CALL Assert 'time', '>= lasttime+0.61', lasttime
CALL Assert 'time', '<= lasttime+1.41', lasttime

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

CALL SysSleep(2)
time = CallPipe('time')
CALL Assert 'DATATYPE(time,"N")', ''
CALL Assert 'time', '>= lasttime+1.82', lasttime
lasttime = time

CALL CallPipe 'forward 1'
CALL Assert 'RESULT', '= 0'
CALL SysSleep(1)
time = CallPipe('time')
CALL Assert 'DATATYPE(time,"N")', ''
CALL Assert 'time', '> lasttime+1.5', lasttime
lasttime = time

CALL CallPipe 'rewind'
CALL Assert 'RESULT', '= 0'
CALL SysSleep(1)
time = CallPipe('time')
CALL Assert 'DATATYPE(time,"N")', ''
CALL Assert 'time', '< lasttime-1.5', lasttime

CALL CallPipe 'rewind 0'
CALL Assert 'RESULT', '= 0'
CALL SysSleep(1)
time = CallPipe('time')
CALL Assert 'DATATYPE(time,"N")', ''
CALL Assert 'time', '> lasttime+0.2', lasttime

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
