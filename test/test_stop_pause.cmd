/**/
dir = DIRECTORY()

CALL CallPipe 'play 'dir'\data\test.mp3'
CALL Assert 'RESULT', '= 0'

CALL SysSleep(1)

CALL CallPipe 'pause'
CALL Assert 'RESULT', '= 0'

CALL CallPipe 'stop'
CALL Assert 'RESULT', '= 0'

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
