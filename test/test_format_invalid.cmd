/**/
dir = DIRECTORY()

CALL 'pipecmd' 'info format 'dir'\data\not_exist.ogg'
reply = result
CALL Parse result
CALL Assert 'data.filesize'
CALL Assert 'data.filetime'
CALL Assert 'data.samplerate'
CALL Assert 'data.channels'
CALL Assert 'POS("invalid", data.fileattr)', '\= 0', data.flags
CALL Assert 'data.songlength'
CALL Assert 'data.bitrate'

CALL 'pipecmd' 'info playlist 'dir'\data\not_exist.ogg'
reply = result
CALL Parse result
CALL Assert 'data.songs', '= 0'
CALL Assert 'data.lists', '= 0'
CALL Assert 'data.invalid', '= 1'
CALL Assert 'data.totalsize', '= 0'
CALL Assert 'data.totallength', '= 0'

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

Assert:
  INTERPRET 'result = 'ARG(1)
  IF ARG(2,'e') THEN
    INTERPRET 'IF \(result 'ARG(2)') THEN CALL Fail ''Expected ''ARG(1) ARG(2)'', found "''result''"'' ARG(3)'
  ELSE IF result \= TRANSLATE(ARG(1)) THEN
    EXIT 'Did not expect the expression 'ARG(1)' to be defined: "'result'"'
  RETURN

Fail:
  CALL LINEOUT STDERR, result
  EXIT ARG(1)
