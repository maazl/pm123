/**/
dir = DIRECTORY()

CALL 'pipecmd' 'info format 'dir'\data\test.mp3'
reply = result
CALL Parse result
CALL Assert 'data.filesize', '= 72514'
PARSE VAR data.filetime year'-'month'-'day' 'hour':'min':'sec
CALL Assert 'LENGTH(year)', '= 4', data.filetime
CALL Assert 'LENGTH(sec)', '>= 2', data.filetime
CALL Assert 'data.samplerate', '= 44100'
CALL Assert 'data.channels', '= 1'
CALL Assert 'POS("song", data.flags)', '\= 0', data.flags
CALL Assert 'TRANSLATE(LEFT(data.decoder, 6))' '= "MPG123"'
IF FORMAT(data.songlength,,3) \= 17.777 & FORMAT(data.songlength*44100,,0) \= 783955 THEN
  EXIT 'The song length should be equivalent to 783955 samples or approx. 17.777s: 'data.songlength
CALL Assert 'data.bitrate', '>= 32000'
CALL Assert 'data.bitrate', '< 32235'

CALL 'pipecmd' 'info playlist 'dir'\data\test.mp3'
reply = result
CALL Parse result
CALL Assert 'data.songs', '= 1'
CALL Assert 'data.lists', '= 0'
CALL Assert 'data.invalid', '= 0'
CALL Assert 'data.totalsize', '= 72514'
IF FORMAT(data.totallength,,3) \= 17.777 & FORMAT(data.totallength*44100,,0) \= 783955 THEN
  EXIT 'The total length should be equivalent to 783955 samples or approx. 17.777s: 'data.songlength

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
  INTERPRET 'IF \(result 'ARG(2)') THEN CALL Fail ''Expected "''ARG(1) ARG(2)''", found "''result''" ARG(3)'''
  RETURN

Fail:
  CALL LINEOUT STDERR, result
  EXIT ARG(1)
