/**/
pipe = VALUE('PIPE',,'OS2ENVIRONMENT')

IF STREAM(pipe, 'c', 'open') \= 'READY:' THEN
  EXIT 'Cannot open pipe 'pipe
EXIT

