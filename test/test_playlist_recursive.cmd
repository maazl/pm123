/**/
dir = TRANSLATE(DIRECTORY())
dirurl =  'FILE:///'TRANSLATE(dir,'/','\')

CALL CallPipe 'info playlist 'dir'\data\list1.lst'
reply = result
CALL Parse result
CALL Assert 'data.songs', '= 3', reply
CALL Assert 'data.lists', '= 2', reply
CALL Assert 'data.invalid', '= 1', reply
CALL Assert 'data.totalsize', '= 60222', reply
CALL Assert 'data.totallength', '>= 31.099', reply
CALL Assert 'data.totallength', '< 31.112', reply

CALL CallPipe 'info playlist 'dir'\data\list2.lst'
reply = result
CALL Parse result
CALL Assert 'data.songs', '= 3', reply
CALL Assert 'data.lists', '= 3', reply
CALL Assert 'data.invalid', '= 1', reply
CALL Assert 'data.totalsize', '= 60222', reply
CALL Assert 'data.totallength', '>= 31.099', reply
CALL Assert 'data.totallength', '< 31.112', reply

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
  EXIT 'Expected "'ARG(1) ARG(2)'", found "'result'" 'ARG(3)

