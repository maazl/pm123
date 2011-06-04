/**/
pipe = '\pipe\pm123'

IF STREAM(pipe, 'c', 'open') \= 'READY:' THEN
  CALL Error 21, 'Cannot open pipe 'pipe

IF ARG(1) = '' THEN DO
  SAY "PM123 remote console. Interactive mode. Enter an empty line to quit."

  DO FOREVER
    PULL command
    IF command = '' THEN
      LEAVE
    CALL Remote(command)
    END
  END
ELSE CALL Remote(ARG(1))

CALL STREAM pipe, 'c', 'close'
EXIT

Remote: PROCEDURE EXPOSE pipe
  CALL LINEOUT pipe, '*'ARG(1)
  DO FOREVER
    line = LINEIN(pipe)
    SAY line
    IF line = '' THEN
      LEAVE
    END
  RETURN

Error: PROCEDURE
  CALL LINEOUT STDERR, ARG(2)
  EXIT ARG(1)

