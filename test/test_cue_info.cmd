/**/
dir = TRANSLATE(DIRECTORY())
dirurl =  'FILE:///'TRANSLATE(dir,'/','\')

CALL CallPipe 'info format' dir'\data\test.cue'
reply = result
CALL Parse result
CALL Assert 'data.filesize', '= "'STREAM(dir'\data\test.cue', 'c', 'query size')'"'
CALL Assert 'POS("PLAYLIST", TRANSLATE(data.flags))', '\= 0', reply
CALL Assert 'TRANSLATE(data.decoder)', '= "PLIST123.DLL"'
CALL Assert 'data.subitems', '= "4"'

CALL CallPipe 'info meta' dir'\data\test.cue'
reply = result
CALL Parse result
CALL Assert 'data.artist', '= "TestArtistC"'
CALL Assert 'data.title', '= "TestTitleC"'
CALL Assert 'data.track', '= "DATA.TRACK"'
CALL Assert 'data.replaygain', '= ",,-5.5,-3.0"'

CALL CallPipe 'info playlist' dir'\data\test.cue'
reply = result
CALL Parse result
CALL Assert 'data.songs', '= "4"'
CALL Assert 'data.lists', '= "1"'

EXIT

CallPipe: PROCEDURE EXPOSE lastcmd
  lastcmd = ARG(1)
  CALL 'pipecmd' ARG(1)
  IF LENGTH(result) <= 2 THEN
    RETURN ''
  RETURN SUBSTR(result, 1, LENGTH(result)-2)

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
  INTERPRET 'IF result 'ARG(2)' THEN RETURN'
  EXIT 'Expected ""'ARG(1) ARG(2)'", found "'result'" 'ARG(3) 'last command: 'lastcmd

SafeFormat: PROCEDURE
  IF DATATYPE(ARG(1),'N') THEN
    RETURN FORMAT(ARG(1),,ARG(2))
  RETURN ARG(1)

