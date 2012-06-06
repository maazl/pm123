/*
 * Copyright 2007-2010 Marcel Mueller
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *    3. The name of the author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/****************************************************************************
*
*  Simple dependency generator
*
*  This script will look for the immediate dependencies of a C source file.
*  It will not care about #if blocks! So it is not possible to generate
*  dependencies of conditional compile interfaces. Furthermore you should not
*  generate dependencies to the runtime includes this way. Simply drop the
*  runtime include path from the -I directories.
*
*  This dependancy generator will not generate the dependencies for the
*  object files. Instead it will create empty rules for the source files.
*  These rules will not cause any immediate action during build, but they
*  invalidate their target in case of a newer source file. This is useful
*  together with implicit rules.
*
****************************************************************************/

IF ARG(1) = '' THEN DO
   SAY "Usage: makedep [options] sourcefiles"
   SAY
   SAY "Options:"
   SAY " -fFILE  use FILE as makefile"
   SAY "         The special name STDOUT will only write the rules to STDOUT."
   SAY " -a      append rules instead of replacing them"
   SAY " -IDIR   look at DIR for includes"
   SAY " -r      recursive mode"
   SAY "         In recursive mode makedep automatically calls itself for all nested"
   SAY "         includes. This only stops at if no more includes are found or if"
   SAY "         the includes are not found."
   SAY " -R      extended recursive mode"
   SAY "         Same as -r but also deal with include files in -I path"
   SAY " -s      Strip .. from include file paths if possible"
   SAY " -i      Strict include check with <> and """""
   SAY " -c      Check for recursions"
   SAY " -v      verbose mode"
   SAY " -x      Unix like file expansion, e.g. *.cpp"
   SAY " -q      Do not print warnings"
   SAY " --      no more options"
   EXIT 48
   END

files          = 0
opt.makefile   = "Makefile"
opt.append     = 0
opt.quiet      = 0
includes       = 0
opt.recursive  = 0
opt.Irecursive = 0
opt.strippar   = 0
opt.strict     = 0
opt.checkrec   = 0
opt.verbose    = 0
opt.wildcard   = 0
opt.cwd        = DIRECTORY()
opt.orexx      = 0
opt.opencmd    = 'OPEN'

IF SUBSTR(opt.cwd, LENGTH(opt.cwd), 1) \= '\' THEN
  opt.cwd = opt.cwd'\'
PARSE VERSION rexx dummy
IF rexx = 'OBJREXX' THEN DO
   opt.orexx = 1
   opt.opencmd = 'OPEN READ'
   END

params = STRIP(ARG(1))
nomoreopt = 0
DO WHILE params \= ''
   IF LEFT(params,1) = '"' THEN DO
      PARSE VAR params '"'param'"'params
      END
    ELSE
      PARSE VAR params param params
   params = STRIP(params, 'L')
   /* param is the next parameter to process ... */
   IF (\nomoreopt) & ((LEFT(param, 1) = '/') | (LEFT(param, 1) = '-')) THEN DO
      SELECT
       WHEN param = '--' THEN
         nomoreopt = 1
       WHEN SUBSTR(param, 2, 1) = 'f' THEN
         opt.makefile = SUBSTR(param, 3)
       WHEN SUBSTR(param, 2) = 'a' THEN
         opt.append = 1
       WHEN SUBSTR(param, 2) = 'q' THEN
         opt.quiet = 1
       WHEN SUBSTR(param, 2, 1) = 'I' THEN DO
         pdata = SUBSTR(param, 3)
         DO WHILE pdata \= ''
            PARSE VAR pdata dir';'pdata
            includes = includes + 1
            IF VERIFY(RIGHT(dir, 1), "\/", "M") = 0 THEN
               dir = dir"\"
            opt.include.includes = dir
            END
         END
       WHEN SUBSTR(param, 2) = 'r' THEN
         opt.recursive = 1
       WHEN SUBSTR(param, 2) = 'R' THEN DO
         opt.recursive = 1
         opt.Irecursive = 1
         END
       WHEN SUBSTR(param, 2) = 's' THEN
         opt.strippar = 1
       WHEN SUBSTR(param, 2) = '1' THEN
         opt.strict = 1
       WHEN SUBSTR(param, 2) = 'c' THEN
         opt.checkrec = 1
       WHEN SUBSTR(param, 2) = 'v' THEN
         opt.verbose = 1
       WHEN SUBSTR(param, 2) = 'x' THEN
         opt.wildcard = 1
       OTHERWISE
         SAY "illegal option '"param"'"
         END
      END

    ELSE DO
      /* any other arg ... */
      files = files + 1
      file.files = param
      END
   END

file.0 = files
opt.include.0 = includes

IF file.0 = 0 THEN
   CALL Error 40, "nothing to do"

/* Here we go! Scan the files */
rule.0 = 0
DO i = 1 TO file.0
   IF opt.wildcard & (VERIFY(file.i, "*?", "M") \= 0) THEN DO
      CALL SysFileTree file.i, "match", "OF"
      IF match.0 \= 0 THEN DO
         dir =  FILESPEC('D',file.i)FILESPEC('P',file.i)
         DO j = 1 TO match.0
            CALL DoFile dir||FILESPEC('N',match.j)
            END
         ITERATE
         END
      END
   /* single file mode */
   CALL DoFile file.i
   END

IF opt.checkrec THEN
   DO i = 1 TO rule.0
      stack.0 = 0
      CALL CheckRec rule.i
      END

/* write result */
CALL WriteRules opt.makefile

EXIT 0

/* write rules to a makefile
 * ARG(1)  Destination file
 */
WriteRules: PROCEDURE EXPOSE opt. rule.
   /* open makefile */
   IF \ABBREV(STREAM(ARG(1), "C", opt.opencmd), "READY") THEN
      CALL Error 21, "Can't open makefile "ARG(1)
   /* read old file result and remove redundancies */
   IF \opt.append THEN DO
      line = 0
      DO FOREVER
         data = LINEIN(ARG(1))
         IF STREAM(ARG(1), "S") \= "READY" THEN
            LEAVE
         line = line + 1
         data.line = data
         /* ignore non-rules */
         p = POS(':', data)
         IF p = 0 THEN
            ITERATE
         /* check wether we have a rule */
         symname = SymName(STRIP(SUBSTR(data, 1, p-1)))
         IF SYMBOL(symname) \= 'VAR' THEN
            ITERATE
         /* parse dependencies */
         q = VERIFY(data, "2009"x,, p+1)
         IF q = 0 THEN DO
            IF p = LENGTH(data) THEN
               data = data||"09"x
            q = LENGTH(data)
            END
         dep = VALUE(symname, '')
         /* no empty rules */
         IF dep = '' THEN DO
            line = line -1
            ITERATE
            END
         /* replace rule */
         data.line = SUBSTR(data, 1, q-1)SUBSTR(dep, 2)
         END
      /* close the file */
      CALL STREAM ARG(1), "C", "CLOSE"
      /* write back file content */
      /* TODO: make backup copy */
      CALL SysFileDelete ARG(1)
      DO i = 1 TO line
         CALL LINEOUT ARG(1), data.i
         END
      END
   /* append (remaining) rules */
   DO i = 1 to rule.0
      symname = Symname(rule.i)
      dep = VALUE(symname)
      /* ignore empty dependencies */
      IF dep = '' THEN
         ITERATE
      /* append rule */
      IF length(rule.i) < 7 THEN
         CALL LINEOUT ARG(1), rule.i":"||"09"x||"09"x||SUBSTR(dep, 2)
      ELSE
         CALL LINEOUT ARG(1), rule.i":"||"09"x||SUBSTR(dep, 2)
      END

   RETURN

/* Check single file
 * ARG(1)  file
 */
DoFile: PROCEDURE EXPOSE opt. rule.
   /* Do nothing if the file is already known */
   symname = SymName(ARG(1))
   /*SAY 'DoFile: 'ARG(1) SYMBOL(symname)*/
   IF SYMBOL(symname) = 'VAR' THEN
      RETURN ''
   CALL VALUE symname, ''
   /* remember list of rules */
   i = rule.0 + 1
   rule.i = ARG(1)
   rule.0 = i
   /* check source file */
   IF STREAM(ARG(1), "C", "QUERY EXISTS") = '' THEN
      RETURN Warn(ARG(1)" does not exist")
   IF \ABBREV(STREAM(ARG(1), "C", opt.opencmd), "READY") THEN
      RETURN Warn("Cannot open "ARG(1))
   /* Parse file */
   line = 0
   DO FOREVER
      src = STRIP(LINEIN(ARG(1)))
      IF STREAM(ARG(1), "S") \= "READY" THEN
         LEAVE
      line = line + 1
      /* Search for \s*#\s*include */
      IF \ABBREV(src, '#') THEN
         ITERATE
      src = STRIP(SUBSTR(src, 2), 'T')
      IF \ABBREV(src, 'include') THEN
         ITERATE
      src = STRIP(SUBSTR(src, 8), 'T')
      /* catch filename */
      PARSE VAR src '"'file'"'
      IF file \= '' THEN DO
         /* local include */
         IF STREAM(file, "C", "QUERY EXISTS") = '' THEN DO
            IF opt.strict THEN
               CALL Warn ARG(1)" line "line": Include "file" does not exist"
            ELSE
               /* try non-local too */
               CALL NonlocalInclude file, symname, line" of "ARG(1)
            ITERATE
            END
         /* found => add dependency */
         CALL DoInclude symname, file
         /* recursive? */
         IF opt.recursive THEN
            CALL DoFile file
         ITERATE
         END
      PARSE VAR src '<'file'>'
      IF file = '' THEN DO
         CALL Warn ARG(1)" line "line": found invalid #include directive"
         ITERATE
         END
      /* nonlocal include */
      CALL NonlocalInclude file, symname, line" of "ARG(1)
      END
   /* close file */
   CALL STREAM ARG(1), "C", "CLOSE"
   RETURN ''

/* Search for non-local include
 * ARG(1) include file name
 * ARG(2) symbol name
 * ARG(3) context
 */
NonlocalInclude: PROCEDURE EXPOSE opt. rule.
   /* nonlocal include */
   IF opt.include.0 = 0 THEN DO
      IF opt.verbose THEN
         CALL Warn "File "ARG(1)" included at line "ARG(3)": not found because of missing -I option."
      RETURN
      END
   /* search include directories */
   DO i = 1 TO opt.include.0
      filepath = opt.include.i||ARG(1)
      /* include found? */
      IF STREAM(filepath, "C", "QUERY EXISTS") \= '' THEN DO
         /* remove ../ */
         IF opt.strippar THEN
            filepath = NormPath(filepath)
         /* found => add dependency */
         CALL DoInclude ARG(2), filepath
         /* recursive? */
         IF opt.Irecursive THEN
            CALL DoFile filepath
         LEAVE
         END
      filepath = ''
      END
   IF opt.verbose & (filepath = '') THEN
      CALL Warn "File "file" included at line "ARG(3)": not found."
   RETURN

/* Normalize path
 * ARG(1)  include file path
 * RETURN  reduced include path
 */
NormPath: PROCEDURE EXPOSE opt.
   filename = ARG(1)
   /* First we have to normalize slashes to make things easy. */
   DO FOREVER
      p = POS('/', filename)
      IF p = 0 THEN
         LEAVE
      filename = OVERLAY('\', filename, p)
      END
   /* absolute path? */
   IF ABBREV(filename, '\') | ABBREV(SUBSTR(filename, 2), ':\') THEN
      RETURN ARG(1)
   /* reduce /xxx/../ */
   path = opt.cwd||filename
   /*SAY 'S:' path*/
   DO FOREVER
      p = POS('\..\', path)
      IF p <= 1 THEN
         LEAVE
      q = LASTPOS('\', SUBSTR(path, 1, p-1))
      IF q = 0 THEN
         RETURN ARG(1) /* invalid path */
      path = SUBSTR(path, 1, q)||SUBSTR(path, p+4)
      /*SAY 'SR:' SUBSTR(path, 1, p-1) path*/
      END
   /* Chkeck how many leading .. to keep */
   p = COMPARE(TRANSLATE(opt.cwd), TRANSLATE(path))
   /*SAY 'SC:' SUBSTR(opt.cwd, 1, p) p LENGTH(opt.cwd) path*/
   IF p <= LENGTH(opt.cwd) THEN DO
      p = LASTPOS('\', opt.cwd, p-1) +1
      /* count \ in different part of opt.cwd */
      q = WORDS(TRANSLATE(SUBSTR(opt.cwd, p), ' \', '\ '))
      /*SAY 'S\:' TRANSLATE(SUBSTR(opt.cwd, p), ' \', '\ ') q*/
      filename = COPIES('..\', q)||SUBSTR(path, p)
      END
   ELSE
      filename = SUBSTR(path, p)
   /* done */
   /*SAY 'SX:' opt.cwd ARG(1) filename
   SAY*/
   RETURN filename

/* Handle include file
 * ARG(1)  parent file's symbol name
 * ARG(2)  include file
 */
DoInclude: PROCEDURE EXPOSE opt. rule.
   /*SAY "DoInclude: "ARG(1)", "ARG(2)*/
   /* rules lookup */
   dep = VALUE(ARG(1))
   /* Is dependency not redundant */
   IF POS(ARG(2), dep) = 0 THEN
      /* add dependency */
      CALL VALUE ARG(1), dep' 'ARG(2)
   RETURN

/* Check for recursive entries
 * ARG(1)  file name
 */
CheckRec: PROCEDURE EXPOSE opt. rule. stack.
   sym = SymName(ARG(1))
   IF SYMBOL(sym) \= 'VAR' THEN
      /* No infos for this file => skip */
      RETURN
   dep = VALUE(sym)
   /* push */
   depth = stack.0 +1
   /*SAY 'D:' depth ARG(1) dep*/
   stack.0 = depth
   stack.depth = ARG(1)
   /* search for loops */
   DO i = 1 TO WORDS(dep)
      sub = WORD(dep, i)
      DO j = 1 TO depth
         IF sub = stack.j THEN DO
            /* recursion */
            IF j = 1 THEN DO
               msg = 'Recursion: '
               DO k = 1 TO depth
                  msg = msg||stack.k||' -> '
                  END
               msg = msg||sub
               CALL Warn msg
               END
            ITERATE i
            END
         END
      CALL CheckRec sub
      END
   /* pop */
   stack.0 = depth -1
   RETURN

/* Create a unique symbol that represents the filename
 * Well, this will not work if the file name is too long.
 * ARG(1)  filename
 * RETURN  symbol */
SymName: PROCEDURE
   RETURN "RULE."C2X(ARG(1))

/****************************************************************************
   Error handling
*/
Error: PROCEDURE
   CALL LINEOUT STDERR, ARG(2)
   EXIT ARG(1)

Warn: PROCEDURE EXPOSE opt.
   IF ARG(1) \= '' & \opt.quiet THEN
      CALL LINEOUT STDERR, ARG(1)
   RETURN ARG(1)

