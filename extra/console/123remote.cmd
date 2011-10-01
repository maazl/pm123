/**/
pipe = '\pipe\pm123'

CALL STREAM pipe, 'c', 'close'
rc = STREAM(pipe, 'c', 'open')
IF \ABBREV(rc, 'READY') THEN
  CALL Error 21, 'Cannot open pipe 'pipe': 'rc

IF ARG(1) = '' THEN DO
  SAY "PM123 remote console. Interactive mode. Enter an empty line to quit."

  DO FOREVER
    CALL CHAROUT , '> '
    PARSE PULL command
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
  rc = STREAM(pipe, 's')
  IF \ABBREV(rc, 'READY') THEN
    CALL Error 27, 'I/O error while writing pipe 'pipe': 'rc
  DO FOREVER
    line = LINEIN(pipe)
    IF line = '' THEN
      LEAVE
    SAY line
    END
  RETURN

Error: PROCEDURE
  CALL LINEOUT STDERR, ARG(2)
  EXIT ARG(1)

