/**/
pipe = VALUE('PIPE',,'OS2ENVIRONMENT')
IF pipe \= '' THEN
  pipe = ' -pipe 'pipe

/* spawn PM123 */
SAY Starting PM123 ...
CALL SETLOCAL
CALL DIRECTORY 'work'
'start /N pm123 -start'pipe
CALL ENDLOCAL

CALL SysSleep(1)

CALL 'pipecmd' 'plugin list decoder @default'
/*SAY "R:" result*/