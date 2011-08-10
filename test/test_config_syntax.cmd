/**/
dir = DIRECTORY()

/* boolean options */
CALL CallPipe 'autouse'
IF result <> 'on' & result <> 'off' THEN
  EXIT 'Boolean property returned unexpected result "'result'", last command: "'lastcmd'"'
last = result
CALL CallPipe 'autouse on'
CALL Assert result, last
CALL CallPipe 'autouse off'
CALL Assert result, 'on'
CALL CallPipe 'autouse toggle'
CALL Assert result, 'off'
CALL CallPipe 'autouse 0'
CALL Assert result, 'on'
CALL CallPipe 'autouse false'
CALL Assert result, 'off'
CALL CallPipe 'autouse 1'
CALL Assert result, 'off'
CALL CallPipe 'autouse true'
CALL Assert result, 'on'
CALL CallPipe 'autouse yes'
CALL Assert result, 'on'
CALL CallPipe 'autouse no'
CALL Assert result, 'on'
CALL CallPipe 'autouse xxx'
CALL Assert result
CALL CallPipe 'autouse 'last
CALL Assert result, 'off'

CALL CallPipe 'option playonload'
last = result
CALL CallPipe 'option playonload=on'
CALL Assert result, last
CALL CallPipe 'option playonload=off'
CALL Assert result, 'on'
CALL CallPipe 'option playonload=toggle'
CALL Assert result, 'off'
CALL CallPipe 'option playonload=0'
CALL Assert result, 'on'
CALL CallPipe 'option playonload=false'
CALL Assert result, 'off'
CALL CallPipe 'option playonload=1'
CALL Assert result, 'off'
CALL CallPipe 'option playonload=true'
CALL Assert result, 'on'
CALL CallPipe 'option playonload=yes'
CALL Assert result, 'on'
CALL CallPipe 'option playonload=no'
CALL Assert result, 'on'
CALL CallPipe 'option playonload=xxx'
CALL Assert result
CALL CallPipe 'option playonload='
CALL Assert result
CALL CallPipe 'option playonload='last
CALL Assert result, 'off'


EXIT

CallPipe: PROCEDURE EXPOSE lastcmd
  lastcmd = ARG(1)
  CALL 'pipecmd' ARG(1)
  IF LENGTH(result) <= 2 THEN
    RETURN '@Error@'
  RETURN SUBSTR(result, 1, LENGTH(result)-2)

Assert: PROCEDURE EXPOSE lastcmd
  IF ARG(2,'o') THEN DO
    IF ARG(1) \= '@Error@' THEN
      EXIT 'Last command failed: "'lastcmd'"'
    END
  ELSE IF ARG(1) \= ARG(2) THEN
    EXIT 'Expected "'ARG(2)'", found "'ARG(1)'", last command: "'lastcmd'"'
  RETURN

