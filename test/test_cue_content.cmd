/**/
dir = TRANSLATE(DIRECTORY())
dirurl =  'FILE:///'TRANSLATE(dir,'/','\')

CALL CallPipe 'playlist' dir'\data\test.cue'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/TEST.CUE"'

CALL CallPipe 'pl next'
CALL Assert 'TRANSLATE(result)', '= "'dirurl'/DATA/TEST2.WAV"'
CALL CallPipe 'pl info playlist'
CALL Parse result
CALL Assert 'data.songs', '= "1"'
CALL Assert 'data.lists', '= "0"'
CALL Assert 'data.totallength', '= 6.4'

/* Once we loaded the playlist, the item should be readable. */
CALL CallPipe 'info format' dir'\data\test2.raw'
CALL Parse result
CALL Assert 'data.filesize', '= "'STREAM(dir'\data\test2.raw', 'c', 'query size')'"'
CALL Assert 'POS("SONG", TRANSLATE(data.flags))', '\= 0', reply
CALL Assert 'TRANSLATE(data.decoder)', '= "WAVPLAY.DLL"'
CALL Assert 'data.samplerate', '= "44100"'
CALL Assert 'data.channels', '= "2"'
CALL Assert 'data.songlength', '<= 17.777'
CALL Assert 'data.songlength', '> 17.776'

CALL CallPipe 'pl info meta'
CALL Parse result
CALL Assert 'data.title', '= "TestTitleC2"'
CALL Assert 'data.artist', '= "TestArtistC"'
CALL Assert 'data.replaygain', '= ",,-5.5,-3.0"'

CALL CallPipe 'pl next'
CALL Assert 'TRANSLATE(result)', '= "'dirurl'/DATA/TEST2.RAW"'
CALL CallPipe 'pl info playlist'
CALL Parse result
CALL Assert 'data.songs', '= "1"'
CALL Assert 'data.lists', '= "0"'
CALL Assert 'data.totallength', '<= 7.777'
CALL Assert 'data.totallength', '> 7.776'

CALL CallPipe 'pl info meta'
CALL Parse result
CALL Assert 'data.title', '= "DATA.TITLE"'
CALL Assert 'data.artist', '= "TestArtistC"'

CALL CallPipe 'pl next'
CALL Assert 'TRANSLATE(result)', '= "'dirurl'/DATA/TEST2.WAV"'
CALL CallPipe 'pl info playlist'
CALL Parse result
CALL Assert 'data.songs', '= "1"'
CALL Assert 'data.lists', '= "0"'
CALL Assert 'data.totallength', '<= 11.377'
CALL Assert 'data.totallength', '> 11.376'

CALL CallPipe 'pl info meta'
CALL Parse result
CALL Assert 'data.title', '= "TestTitleC4"'
CALL Assert 'data.artist', '= "TestArtistC4"'

CALL CallPipe 'pl info item'
CALL Parse result
CALL Assert 'data.pregap', '= 1.8'
CALL Assert 'data.postgap', '= 0.12'

CALL CallPipe 'pl next'
CALL Assert 'TRANSLATE(result)', '= "'dirurl'/DATA/TEST2.RAW"'
CALL CallPipe 'pl info playlist'
CALL Parse result
CALL Assert 'data.songs', '= "1"'
CALL Assert 'data.lists', '= "0"'
CALL Assert 'data.totallength', '= 10'

CALL CallPipe 'pl info meta'
CALL Parse result
CALL Assert 'data.title', '= "TestTitleC5"'
CALL Assert 'data.artist', '= "TestArtistC"'
CALL Assert 'data.replaygain', '= "-6.0,6.0,-5.5,-3.0"'

EXIT

CallPipe: PROCEDURE EXPOSE lastcmd
  lastcmd = ARG(1)
  CALL 'pipecmd' ARG(1)
  IF LENGTH(result) <= 2 THEN
    RETURN ''
  RETURN SUBSTR(result, 1, LENGTH(result)-2)

Parse: PROCEDURE EXPOSE data. reply
  DROP data.
  reply = ARG(1)
  s = 1
  DO WHILE s \= 0
    /* split next line */
    p = POS('0a'x, reply, s)
    IF p = 0 THEN DO
      line = SUBSTR(reply, s)
      s = 0
      END
    ELSE DO
      IF p > 1 & SUBSTR(reply, p-1, 1) = '0d'x THEN
        line = SUBSTR(reply, s, p-s-1)
      ELSE
        line = SUBSTR(reply, s, p-s)
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
  EXIT 'Expected ""'ARG(1) ARG(2)'", found "'result'" 'reply 'last command: 'lastcmd

SafeFormat: PROCEDURE
  IF DATATYPE(ARG(1),'N') THEN
    RETURN FORMAT(ARG(1),,ARG(2))
  RETURN ARG(1)

