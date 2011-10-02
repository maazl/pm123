/**/
dir = DIRECTORY()

'@copy 'dir'\data\test.mp3 work >nul'
CALL 'pipecmd' 'info invalidate 'dir'\work\test.mp3'
step = 1
CALL 'pipecmd' 'write meta rst'
CALL 'pipecmd' 'write meta set title=testtitle'
CALL 'pipecmd' 'write meta set album=testalbum'
CALL 'pipecmd' 'write meta set comment=testcomment'||'1b1b1b'x||'n'||'1b'x||'r'
CALL 'pipecmd' 'write meta set track=17'
CALL 'pipecmd' 'write meta to 'dir'\work\test.mp3'
CALL Equal LEFT(result, LENGTH(result)-2), 0
step = 2
CALL 'pipecmd' 'info meta 'dir'\work\test.mp3'
CALL Parse result
CALL Check title, 'testtitle'
CALL Check artist, 'TestArtist1'
CALL Check album, 'testalbum'
CALL Check year, '2011-02-03'
CALL Check comment, 'testcomment'||'1b1b1b'x||'n'||'1b'x||'r'
CALL Check genre, 'TestGenre1'
CALL Check track, '17'
CALL Check copyright, 'TestCopyright1'
step = 3
CALL 'pipecmd' 'info refresh 'dir'\work\test.mp3'
CALL 'pipecmd' 'info meta 'dir'\work\test.mp3'
CALL Parse result
CALL Check title, 'testtitle'
CALL Check artist, 'TestArtist1'
CALL Check album, 'testalbum'
CALL Check year, '2011-02-03'
CALL Check comment, 'testcomment'||'1b1b1b'x||'n'||'1b'x||'r'
CALL Check genre, 'TestGenre1'
CALL Check track, '17'
CALL Check copyright, 'TestCopyright1'
step = 4
CALL 'pipecmd' 'write meta rst purge'
CALL 'pipecmd' 'write meta set artist=testartist'
CALL 'pipecmd' 'write meta set year=testyear'
CALL 'pipecmd' 'write meta set genre=testgenre'
CALL 'pipecmd' 'write meta set copyright='
CALL 'pipecmd' 'write meta to 'dir'\work\test.mp3'
CALL Equal LEFT(result, LENGTH(result)-2), 0
step = 5
CALL 'pipecmd' 'info meta 'dir'\work\test.mp3'
CALL Parse result
CALL Check title, '@purge@'
CALL Check artist, 'testartist'
CALL Check album, '@purge@'
CALL Check year, 'testyear'
CALL Check comment, '@purge@'
CALL Check genre, 'testgenre'
CALL Check track, '@purge@'
CALL Check copyright, ''
step = 6
CALL 'pipecmd' 'info refresh 'dir'\work\test.mp3'
CALL 'pipecmd' 'info meta 'dir'\work\test.mp3'
CALL Parse result
CALL Check title, '@purge@'
CALL Check artist, 'testartist'
CALL Check album, '@purge@'
CALL Check year, 'testyear'
CALL Check comment, '@purge@'
CALL Check genre, 'testgenre'
CALL Check track, '@purge@'
CALL Check copyright, ''

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
      data.line = '@purge@'
    ELSE DO
      key = SUBSTR(line, 1, p-1)
      IF SYMBOL(key) = 'BAD' THEN
        EXIT 'Bad reply key: 'key
      data.key = SUBSTR(line, p+1)
      /*SAY key '->' SUBSTR(line, p+1)*/
      END
    END
  RETURN

Check: PROCEDURE EXPOSE data. step
  field = ARG(1)
  IF ARG(2,'o') THEN DO
    IF SYMBOL('DATA.'field) = 'VAR' THEN
      EXIT 'Step 'step': Did not expect key' field': 'data.field' 'C2X(data.field)
    END
  ELSE IF data.field \= ARG(2) THEN
    EXIT 'Step 'step': Expected 'field ARG(2)': 'data.field' 'C2X(data.field)
  RETURN

Equal: PROCEDURE EXPOSE step
  IF ARG(1) \= ARG(2) THEN
    EXIT 'Step 'step': expected 'ARG(2)' found 'ARG(1)' 'C2X(ARG(1))
  RETURN
