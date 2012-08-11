/**/
dir = DIRECTORY()

CALL CallPipe 'load 'dir'\data\test.cue'
CALL Assert 'REPLY', '= 0'

CALL CallPipe 'navigate /;[3];7.495'
CALL Assert 'REPLY', '= 0'

CALL CallPipe 'time'
CALL Assert 'REPLY', '= 1.095'

CALL CallPipe 'location'
CALL Assert 'REPLY', '= """test2.wav""[2];0:07.495"'

CALL CallPipe 'jump 0.114'
CALL Assert 'REPLY', '= 0'

CALL CallPipe 'time'
CALL Assert 'REPLY', '= 0.114'

CALL CallPipe 'location'
CALL Assert 'REPLY', '= """test2.wav""[2];0:06.514"'

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
