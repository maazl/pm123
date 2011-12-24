/**/
dir = TRANSLATE(DIRECTORY())
dirurl =  'FILE:///'TRANSLATE(dir,'/','\')

CALL CallPipe 'info format 'dir'\data\dir1'
reply = result
CALL Parse result
CALL Assert 'TRANSLATE(LEFT(data.decoder, 8))', '= "FOLDR123"', reply
PARSE VAR data.filetime year'-'month'-'day' 'hour':'min':'sec
CALL Assert 'LENGTH(year)', '= 4', data.filetime
CALL Assert 'LENGTH(sec)', '>= 2', data.filetime
CALL Assert 'POS("playlist", data.flags)', '\= 0', reply
CALL Assert 'data.subitems', '= 1', reply

CALL CallPipe 'info format 'dir'\data\dir1/?AllFiles=true'
reply = result
CALL Parse result
CALL Assert 'POS("playlist", data.flags)', '\= 0', reply
CALL Assert 'data.subitems', '= 2', reply

CALL CallPipe 'info format 'dir'\data\dir1/?Recursive=true'
reply = result
CALL Parse result
CALL Assert 'POS("playlist", data.flags)', '\= 0', reply
CALL Assert 'data.subitems', '= 2', reply

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
  EXIT 'Expected ""'ARG(1) ARG(2)'", found "'result'" 'ARG(3) 'last command = 'lastcmd

