/**/
pipe = VALUE('PIPE',,'OS2ENVIRONMENT')

rc = STREAM(pipe, 'c', 'open')
IF \ABBREV(rc, 'READY') THEN
  EXIT 'Cannot open pipe 'pipe': 'rc
EXIT

