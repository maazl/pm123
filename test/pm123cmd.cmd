/**/
pipe = '\pipe\pm123'

IF STREAM(pipe, 'c', 'open') \= 'READY:' THEN
  CALL Error 21, 'Cannot open pipe 'pipe
CALL LINEOUT pipe, '*'ARG(1)
DO FOREVER
  line = LINEIN(pipe)
  SAY line
  IF line = '' THEN
    LEAVE
  END
CALL STREAM pipe, 'c', 'close'
EXIT

Error: PROCEDURE
  CALL LINEOUT STDERR, ARG(2)
  EXIT ARG(1)

