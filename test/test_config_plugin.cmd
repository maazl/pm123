/**/
dir = DIRECTORY()
/* decoder plug-ins */
CALL CallPipe 'plugin list decoder'
last = result
/*SAY 'last:' last*/
CALL CallPipe 'plugin list decoder @empty'
CALL VAssert 'RESULT', last
CALL CallPipe 'plugin list decoder'
CALL VAssert 'RESULT'

CALL CallPipe 'plugin params decoder mpg123?tryothers=1'
CALL VAssert 'RESULT'
CALL CallPipe 'getmessages'

CALL CallPipe 'plugin load all mpg123'
CALL VAssert 'RESULT', 'decoder'
CALL CallPipe 'plugin params decoder mpg123?tryothers=1'
CALL CallPipe 'plugin params decoder mpg123?filetypes=MPEG-Audio'
CALL ParsePlugin result
CALL VAssert 'PARAMS.TRYOTHERS', 'yes'
CALL CallPipe 'plugin params decoder mpg123?enabled=off'
CALL ParsePlugin result
CALL VAssert 'PARAMS.FILETYPES', 'MPEG-Audio'
CALL CallPipe 'plugin params decoder mpg123'
CALL ParsePlugin result
CALL VAssert 'PARAMS.ENABLED', 'no'

CALL CallPipe 'plugin load decoder mpg123'
CALL VAssert 'RESULT'
CALL CallPipe 'getmessages'
CALL CallPipe 'plugin load visual oggplay'
CALL VAssert 'RESULT'
CALL CallPipe 'getmessages'
CALL CallPipe 'plugin load decoder oggplay'
CALL VAssert 'RESULT', 'decoder'

CALL CallPipe 'plugin list decoder @default'
list = result
CALL Assert 'LENGTH(list)', '> 10'
CALL Assert 'SUBSTR(list,1,10)', '= "mpg123.dll"'
p = POS('0a'x, list)
CALL Assert 'p' '> 0'
CALL Assert 'SUBSTR(list,p+1,11)', '= "oggplay.dll"'

CALL CallPipe 'plugin list decoder 'TRANSLATE(last, '09'x, '0a'x)

EXIT

ParsePlugin: PROCEDURE EXPOSE plugin params.
  DROP plugin
  DROP params.
  str = ARG(1)
  p = POS('?', str)
  IF p \= 0 THEN DO
    plugin = LEFT(str, p-1)
    str = SUBSTR(str, p+1)
    END
  DO WHILE str \= ''
    p = POS('&', str)
    IF p \= 0 THEN DO
      attr = LEFT(str, p-1)
      str = SUBSTR(str, p+1)
      END
    ELSE DO
      attr = str
      str = ''
      END
    p = POS('=', attr)
    IF p \= 0 THEN
      CALL VALUE 'PARAMS.'LEFT(attr, p-1), SUBSTR(attr, p+1)
    ELSE
      CALL VALUE 'PARAMS.'attr, ''
    END
  RETURN

CallPipe: PROCEDURE EXPOSE lastcmd
  lastcmd = ARG(1)
  CALL 'pipecmd' ARG(1)
  IF LENGTH(result) <= 2 THEN
    RETURN
  RETURN SUBSTR(result, 1, LENGTH(result)-2)

Assert:
  INTERPRET 'result = 'ARG(1)
  INTERPRET 'IF \(result 'ARG(2)') THEN EXIT ''Expected "''ARG(1) ARG(2)''", found "''result''", last command: "''lastcmd''"'''
  RETURN

VAssert:
  IF ARG(2,'o') THEN DO
    IF VAR(ARG(1)) THEN
      EXIT 'Last command failed: "'lastcmd'" - expected empty result but found 'VALUE(ARG(1))
    END
  ELSE DO
    IF \VAR(ARG(1)) THEN
      EXIT 'Expected "'ARG(2)'", but got no result, last command: "'lastcmd'"'
    IF VALUE(ARG(1)) \= ARG(2) THEN
      EXIT 'Expected "'ARG(2)'", found "'VALUE(ARG(1))'", last command: "'lastcmd'"'
    END
  RETURN

