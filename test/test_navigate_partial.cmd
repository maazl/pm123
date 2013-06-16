/**/
dir = DIRECTORY()

CALL CallPipe 'load 'dir'\data\list1.lst'
CALL Assert 'REPLY', '= 0'
CALL CallPipe 'navigate list2.lst;test3.ogg;0:02'
CALL Assert 'REPLY', '= 0'
CALL CallPipe 'playlist 'dir'\data\list2.lst'
CALL Assert 'REPLY', '<> ""'

/* partial navigation within song */
CALL CallPipe 'playlist 'dir'\data\list2.lst'
CALL Assert 'REPLY', '<> ""'
CALL CallPipe 'pl navigate test3.ogg;0:04'
CALL Assert 'REPLY', '= """test3.ogg"";0:04"'
CALL CallPipe 'pl navto'
CALL Assert 'REPLY', '= 0'
CALL CallPipe 'location'
CALL Assert 'REPLY', '= """list2.lst"";""test3.ogg"";0:04"'

/* partial navigation within sublist */
CALL CallPipe 'pl navigate /;test2.ogg;0:01'
CALL Assert 'REPLY', '= """test2.ogg"";0:01"'
CALL CallPipe 'pl navto'
CALL Assert 'REPLY', '= 0'
CALL CallPipe 'location'
CALL Assert 'REPLY', '= """list2.lst"";""test2.ogg"";0:01"'

/* partial navigation within outer playlist */
/* Testcase bad, because ambigouos
CALL CallPipe 'pl navigate /;list1.lst;test.ogg'
CALL Assert 'REPLY', '= """list1.lst"";""test.ogg"""'
CALL CallPipe 'pl navto'
CALL Assert 'REPLY', '= 0'
CALL CallPipe 'location'
CALL Assert 'REPLY', '= """list2.lst"";""test3.ogg"";0:04"'*/

EXIT


CallPipe: PROCEDURE EXPOSE lastcmd reply
  lastcmd = ARG(1)
  CALL 'pipecmd' ARG(1)
  IF LENGTH(result) <= 2 THEN
    RETURN
  reply = SUBSTR(result, 1, LENGTH(result)-2)
  RETURN reply

Assert:
  INTERPRET 'result = 'ARG(1)
  INTERPRET 'IF \(result 'ARG(2)') THEN EXIT ''Expected "''ARG(1) ARG(2)''", found "''result''" "''ARG(3)''", last command: "''lastcmd''"'''
  RETURN

Fail:
  CALL LINEOUT STDERR, result
  EXIT ARG(1)
