/**/
dir = TRANSLATE(DIRECTORY())
dirurl =  'FILE:///'TRANSLATE(dir,'/','\')

CALL CallPipe 'playlist' dir'\data\listslice.lst'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/LISTSLICE.LST"'

/* Test start +5 */
CALL CallPipe 'pl nextitem'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/TEST.OGG"'
/* Check the slice */
CALL CallPipe 'pl info item'
reply = result
CALL Parse result
CALL Assert 'data.start', '= "0:05"', reply
CALL Assert 'data.stop', '= "DATA.STOP"', reply
/* check wether the object info is unchanged by the slice. */
CALL CallPipe 'pl info format'
reply = result
CALL Parse result
CALL Assert 'SafeFormat(data.songlength,3)', '= 17.777', reply
/* check if the rpl info takes care of the slice */
CALL CallPipe 'pl info playlist'
reply = result
CALL Parse result
CALL Assert 'data.songs', '= "1"'
CALL Assert 'data.lists', '= "0"'
CALL Assert 'data.invalid', '= "0"'
CALL Assert 'SafeFormat(data.totallength,3)', '= 12.777', reply
CALL Assert 'SafeFormat(data.totalsize/1000,1)', '= 22.0', reply

/* test stop -7 */
CALL CallPipe 'pl nextitem'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/TEST.OGG"'
/* Check the slice */
CALL CallPipe 'pl info item'
reply = result
CALL Parse result
CALL Assert 'data.start', '= "DATA.START"', reply
CALL Assert 'data.stop', '> "0:10.776"', reply
CALL Assert 'data.stop', '<= "0:10.777"', reply
/* check wether the object info is unchanged by the slice. */
CALL CallPipe 'pl info format'
reply = result
CALL Parse result
CALL Assert 'SafeFormat(data.songlength,3)', '= 17.777', reply
/* check if the rpl info takes care of the slice */
CALL CallPipe 'pl info playlist'
reply = result
CALL Parse result
CALL Assert 'data.songs', '= "1"'
CALL Assert 'data.lists', '= "0"'
CALL Assert 'data.invalid', '= "0"'
CALL Assert 'SafeFormat(data.totallength,3)', '= 10.777', reply
CALL Assert 'SafeFormat(data.totalsize/1000,1)', '= 18.6', reply

/* test stop -7 */
CALL CallPipe 'pl nextitem'
CALL Assert 'TRANSLATE(RESULT)', '= "'dirurl'/DATA/TEST.OGG"'

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