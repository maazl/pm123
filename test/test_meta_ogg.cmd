/**/
dir = DIRECTORY()

CALL 'pipecmd' 'info meta 'dir'\data\test.ogg'
CALL Parse result
CALL Check title, 'TestTitle1'
CALL Check artist, 'TestArtist1'
CALL Check album, 'TestAlbum1'
CALL Check year, '2011-02-03'
CALL Check comment, 'TestComment1'
CALL Check genre, 'TestGenre1'
CALL Check track, '1'
CALL Check copyright, 'TestCopyright1'

EXIT

Parse: PROCEDURE EXPOSE data.
  DROP data.
  s = 1
  DO WHILE s \= 0
    /* split next line */
    p = POS('0a'x, ARG(1), s)
    IF p = 0 THEN DO
      line = SUBSTR(ARG(1), s)
      s = 0
      END
    ELSE DO
      IF p > 1 & SUBSTR(ARG(1), p-1, 1) = '0d'x THEN
        line = SUBSTR(ARG(1), s, p-s-1)
      ELSE
        line = SUBSTR(ARG(1), s, p-s)
      s = p+1
      END
    /* split key */
    p = POS('=', line)
    IF p = 0 THEN
      data.empty = line
    ELSE DO
      key = SUBSTR(line, 1, p-1)
      IF SYMBOL(key) = 'BAD' THEN
        EXIT 'Bad reply key: 'key
      data.key = SUBSTR(line, p+1)
      /*SAY key '->' SUBSTR(line, p+1)*/
      END
    END
  RETURN

Check: PROCEDURE EXPOSE data.
  field = ARG(1)
  IF data.field \= ARG(2) THEN
    EXIT 'Expected 'field ARG(2)': 'data.field
  RETURN
