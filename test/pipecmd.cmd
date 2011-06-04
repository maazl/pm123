/**/
pipe = VALUE('PIPE',,'OS2ENVIRONMENT')
debug = VALUE('DEBUG',,'OS2ENVIRONMENT')

IF debug = '1' THEN
  SAY '>' ARG(1)
CALL LINEOUT pipe, '*'ARG(1)
result = ''
DO FOREVER
  line = LINEIN(pipe)
  IF line = '' THEN
    LEAVE
  result = result||line||'0d0a'x
  END
IF debug = '1' THEN
  SAY '<' result
EXIT result
