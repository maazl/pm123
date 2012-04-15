/**/
pipe = VALUE('PIPE',,'OS2ENVIRONMENT')

rc = STREAM(pipe, 'c', 'open')
IF \ABBREV(rc, 'READY') THEN DO
  /* Give a second try. Had some curious timing problems. */
  CALL SysSleep .1
  rc = STREAM(pipe, 'c', 'open')
  IF \ABBREV(rc, 'READY') THEN
    EXIT 'Cannot open pipe 'pipe': 'rc
  END
  CALL 'pipecmd' 'reset'
EXIT

