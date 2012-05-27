/**/
dir = DIRECTORY()

/* detail view */
CALL CallPipe 'show playlist 'dir'\data\list.lst'
CALL Assert 'RESULT', '= 0'
CALL CallPipe 'hide playlist 'dir'\data\list.lst'
CALL Assert 'RESULT', '= 1'
CALL CallPipe 'isvisible playlist 'dir'\data\list.lst'
CALL Assert 'RESULT', '= 0'

/* tree view */
CALL CallPipe 'show tree 'dir'\data\list.lst'
CALL Assert 'RESULT', '= 0'
CALL CallPipe 'hide tree 'dir'\data\list.lst'
CALL Assert 'RESULT', '= 1'
CALL CallPipe 'isvisible tree 'dir'\data\list.lst'
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
