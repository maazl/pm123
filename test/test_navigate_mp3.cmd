/**/
dir = DIRECTORY()

CALL CallPipe 'play 'dir'\data\test.mp3'
CALL Assert 'REPLY', '= 0'

CALL CallPipe 'jump 0:03.5'
CALL Assert 'REPLY', '= 0'

CALL CallPipe 'time'
CALL Assert 'REPLY', '= 3.5'

CALL CallPipe 'play'
CALL Assert 'REPLY', '= 0'

lasttime = CallPipe('time')
CALL Assert 'REPLY', '>= 3.5'
CALL Assert 'REPLY', '<= 3.6'

CALL SysSleep(1)
CALL CallPipe 'time'
CALL Assert 'REPLY', '> 'lasttime
CALL Assert 'REPLY', '<= 'lasttime+1.2

CALL CallPipe 'jump 2'
CALL Assert 'REPLY', '= 0'

CALL CallPipe 'time'
CALL Assert 'REPLY', '>= 2'
CALL Assert 'REPLY', '<= 2.1'

EXIT


CallPipe: PROCEDURE EXPOSE lastcmd reply
  lastcmd = ARG(1)
  CALL 'pipecmd' ARG(1)
  IF LENGTH(result) <= 2 THEN
    RETURN
  reply = SUBSTR(result, 1, LENGTH(result)-2)
  RETURN reply

Assert:
  INTERPRET 'result = 'ARG(1)
  INTERPRET 'IF \(result 'ARG(2)') THEN EXIT ''Expected "''ARG(1) ARG(2)''", found "''result''" "''ARG(3)''", last command: "''lastcmd''"'''
  RETURN

Fail:
  CALL LINEOUT STDERR, result
  EXIT ARG(1)
