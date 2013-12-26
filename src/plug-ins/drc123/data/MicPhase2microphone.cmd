/* Convert microphone calibration file with dB gain and phase information
 * to a .microphone calibration file suitable for DRC123. */

IF RxFuncAdd("SysLoadFuncs","RexxUtil","SysLoadFuncs") = 0 THEN
  CALL SysLoadFuncs
CALL RxFuncAdd "MathLoadFuncs","RxMathFn","MathLoadFuncs"
/* You get an error here? You need RxMathFn.DLL from IBM. Search the internet, it will be around there. */
CALL MathLoadFuncs

PARSE ARG file

IF file = '' THEN
  CALL Fail "Filename required"

target = file
p = LASTPOS('.', target)
IF p \= 0 THEN DO
  q = LASTPOS('\', target)
  IF q > p THEN
    p = q
  q = LASTPOS(':', target)
  IF q > p THEN
    p = q
  target = SUBSTR(target, 1, p-1)
  END
target = target'.microphone'

IF \ABBREV(STREAM(file, "c", "open read"), 'READY') THEN
  CALL Fail "File "file" failed to open."
CALL SysFileDelete(target)
IF \ABBREV(STREAM(target, "c", "open write"), 'READY') THEN
  CALL Fail "File "target" failed to open."

lastphase = 0
DO WHILE ABBREV(STREAM(file, "s"), 'READY')
  line = LINEIN(file)
  IF LENGTH(line) <= 1 || ABBREV(line, '#') THEN DO
    CALL LINEOUT target, line
    ITERATE
    END
  PARSE VAR line frequency gain phase dummy
  gain = POW(10, gain/20.0)
  delay = (phase - lastphase) / 180.0 * PI() / frequency
  CALL LINEOUT target, frequency' 'gain' 'delay
  END

CALL STREAM file, "c", "close"
CALL STREAM target, "c", "close"

EXIT 0


Fail: PROCEDURE
  CALL LINEOUT STDERR, ARG(1)
  EXIT 1