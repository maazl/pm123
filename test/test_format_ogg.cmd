/**/
dir = DIRECTORY()

CALL 'pipecmd' 'info format 'dir'\data\test.ogg'
CALL Parse result
IF VERIFY(data.filesize, '0123456789') \= 0 THEN
  EXIT 'Valid file size expected: 'data.filesize
PARSE VAR data.filetime year'-'month'-'day' 'hour':'min':'sec
IF LENGTH(year) \= 4 | LENGTH(sec) < 2 THEN
  EXIT 'Valid file time stamp expected: 'data.filesize
IF data.samplerate \= 44100 THEN
  EXIT 'Samplerate should be 44100: 'data.samplerate
IF data.channels \= 1 THEN
  EXIT 'Number of channels should be 1: 'data.channels
IF POS('song', data.flags) = 0 THEN
  EXIT 'File flags should contain ''song'': 'data.flags
IF \ABBREV(TRANSLATE(data.decoder), 'OGGPLAY') THEN
  EXIT 'Decoder should be oggplay: 'data.decoder
IF data.songlength \= 0.045 & FORMAT(data.songlength*44100,,0) \= 1982 THEN
  EXIT 'The song length should be equivalent to 1982 samples or approx. 45ms: 'data.songlength
IF data.bitrate < 62834 | data.bitrate > 63000 THEN
  EXIT 'The bit rate should be 62834...63000: 'data.bitrate

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
