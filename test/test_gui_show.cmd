/**/
CALL CallPipe 'isvisible'
CALL Assert 'RESULT', '= 1'
CALL CallPipe 'hide'
CALL Assert 'RESULT', '= 1'
CALL CallPipe 'show'
CALL Assert 'RESULT', '= 0'
CALL CallPipe 'isvisible'
CALL Assert 'RESULT', '= 1'

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
