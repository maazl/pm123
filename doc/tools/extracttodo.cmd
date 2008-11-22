/* Extract open issues from todo */
skip = 0

DO WHILE STREAM('STDIN', 'STATE') = 'READY'
  line = LINEIN()
  lineu = TRANSLATE(line)
  IF skip THEN DO
    IF POS('</TR', lineu) > 0 THEN
      skip = 0
    END
  ELSE DO
    p = POS('<TR CLASS="', lineu)
    CALL LINEOUT STDERR, p " " POS('FIXED', lineu, p+11)
    IF p \= 0 & POS('FIXED', lineu, p+11) = p+11 THEN
      skip = 1
    ELSE
      CALL LINEOUT , line
    END
  END

