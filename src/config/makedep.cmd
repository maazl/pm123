/*
 * Copyright 2007-2012 Marcel Mueller
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
   SAY " -v      Strict include check with ""..."""
   SAY " -V      Strict include check with <...> and ""..."""
   SAY " -c      Check for recursions"
   SAY " -x      Unix like file expansion, e.g. *.cpp"
   SAY " -s      search in sub directories for sourcefiles too"
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
opt.subdir     = ""
opt.strict     = 0
opt.strict2    = 0
opt.checkrec   = 0
opt.wildcard   = 0
opt.cwd        = DIRECTORY()
opt.orexx      = 0
opt.opencmd    = 'OPEN'
opt.debug      = 0

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
         opt.subdir = "S"
       WHEN SUBSTR(param, 2) = 'v' THEN
         opt.strict = 1
       WHEN SUBSTR(param, 2) = 'V' THEN
         opt.strict2 = 1
       WHEN SUBSTR(param, 2) = 'c' THEN
         opt.checkrec = 1
       WHEN SUBSTR(param, 2) = 'x' THEN
         opt.wildcard = 1
       WHEN SUBSTR(param, 2) = '_' THEN
         opt.debug = 1
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
      CALL SysFileTree file.i, "match", "OF"opt.subdir
      IF match.0 \= 0 THEN DO
         dir = DirFromPath(file.i)
         absdir = DIRECTORY()
         IF dir \= '' THEN DO
            CALL SysFileTree dir, "absdir", "OD"
            IF absdir.0 = 0 THEN
               CALL Error 60, 'Failed to get absolute path of 'dir
            absdir = absdir.1
            END
         dirlen = LENGTH(absdir) - LENGTH(dir) + 2
         IF RIGHT(absdir, 1) \= '\' THEN
            dirlen = dirlen + 1
         IF RIGHT(dir, 1) \= '\' THEN
            dirlen = dirlen - 1
         DO j = 1 TO match.0
            /*SAY 'W: 'dir"_"absdir"_"match.j*/
            CALL DoFile SUBSTR(match.j, dirlen)
            END
         ITERATE
         END
      END
   ELSE
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
         symname = SymName(MakeAbs(STRIP(SUBSTR(data, 1, p-1)), opt.cwd))
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
      symname = Symname(NormPath(MakeAbs(rule.i, opt.cwd)))
      dep = VALUE(symname)
      /*SAY "W: "rule.i symname dep*/
      IF ABBREV(dep, 'RULE') THEN
         CALL Assert 'Inexistent rule 'rule.i
      /* ignore empty dependencies */
      IF dep = '' THEN
         ITERATE
      /* append rule */
      IF length(rule.i) < 7 THEN
         CALL LINEOUT ARG(1), rule.i":"||"0909"x||SUBSTR(dep, 2)
      ELSE
         CALL LINEOUT ARG(1), rule.i":"||"09"x||SUBSTR(dep, 2)
      END

   RETURN

/* Check single file
 * ARG(1)  file
 */
DoFile: PROCEDURE EXPOSE opt. rule.
   file = TRANSLATE(ARG(1), '\', '/')
   /* check source file */
   filepath = STREAM(file, "C", "QUERY EXISTS")
   IF filepath = '' THEN
      CALL Assert file" does not exist"
   /* Do nothing if the file is already known */
   symname = SymName(filepath)
   IF opt.debug THEN
      SAY 'DoFile: 'file filepath SYMBOL(symname)
   IF POS('\..\', file) > 0 THEN
      CALL Assert 'Invalid file name 'file
   /*IF ABBREV(file, '\') | ABBREV(SUBSTR(file, 2), ':\') THEN
      CALL Assert 'DoFile resulted in absolute path 'file*/
   IF SYMBOL(symname) = 'VAR' THEN
      RETURN ''
   CALL VALUE symname, ''
   /* remember list of rules */
   i = rule.0 + 1
   rule.i = NormPath(filepath, opt.cwd)
   rule.0 = i
   /* change to file's directory */
   lastdir = DIRECTORY()
   dir = DirFromPath(filepath)
   /*SAY "D:" dir file*/
   IF dir = '' THEN
      dir = DIRECTORY()
   ELSE DO
      dir = DIRECTORY(dir)
      IF dir = '' THEN
         CALL Error 60, 'Cannot enter directory of 'file', currently at 'DIRECTORY()
      END
   /* Parse file */
   IF \ABBREV(STREAM(filepath, "C", opt.opencmd), "READY") THEN DO
      CALL DIRECTORY(lastdir)
      RETURN Warn("Cannot open "filepath)
      END
   line = 0
   DO FOREVER
      src = STRIP(LINEIN(filepath))
      IF STREAM(filepath, "S") \= "READY" THEN
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
      PARSE VAR src '"'inc'"'
      IF inc \= '' THEN DO
         /* local include */
         path = STREAM(inc, "C", "QUERY EXISTS")
         /*SAY "L:" inc path file*/
         IF path \= '' THEN DO
            /* found local => add dependency */
            path = NormPath(path, opt.cwd)
            CALL DoInclude symname, path
            /* recursive? */
            IF opt.recursive THEN
               CALL DoFile inc
            END
         /* try non-local too */
         ELSE IF \NonlocalInclude(file, symname) THEN DO
            IF opt.strict THEN
               CALL Warn "File "inc" included at line "line" of "file": does not exist."
            END
         ITERATE
         END
      PARSE VAR src '<'inc'>'
      IF inc = '' THEN DO
         CALL Warn file" line "line": found invalid #include directive"
         ITERATE
         END
      /* nonlocal include */
      IF \NonlocalInclude(inc, symname) THEN DO
         IF opt.strict2 THEN
            IF opt.include.0 = 0 THEN
               CALL Warn "File "inc" included at line "line" of "file": not found because of missing -I option."
            ELSE
               CALL Warn "File "inc" included at line "line" of "file": not found."
         END
      END
   /* close file */
   CALL STREAM filepath, "C", "CLOSE"
   CALL DIRECTORY(lastdir)
   RETURN ''

/* Search for non-local include
 * ARG(1) include file name
 * ARG(2) parent file's symbol name
 * RETURN true if succeded
 */
NonlocalInclude: PROCEDURE EXPOSE opt. rule.
   /* nonlocal include */
   IF opt.include.0 = 0 THEN
      RETURN 0
   /* search include directories */
   DO i = 1 TO opt.include.0
      filepath = opt.include.i||ARG(1)
      /* include found? */
      path = STREAM(filepath, "C", "QUERY EXISTS")
      IF path \= '' THEN DO
         /* found => add dependency */
         filepath = NormPath(filepath)
         CALL DoInclude ARG(2), filepath
         /* recursive? */
         IF opt.Irecursive THEN
            CALL DoFile filepath
         LEAVE
         END
      filepath = ''
      END
   IF filepath = '' THEN
      RETURN 0
   RETURN 1

/* Handle include file
 * ARG(1)  parent file's symbol name
 * ARG(2)  include file
 */
DoInclude: PROCEDURE EXPOSE rule. opt.
   inc = ARG(2)
   IF opt.debug THEN
      SAY "DoInclude: "ARG(1)", "inc
   IF POS('\..\', inc) > 0 THEN
      CALL Assert 'Invalid not normalized include dependency 'inc
   IF LEFT(inc, 1) = '\' | ABBREV(SUBSTR(inc, 2), ':') THEN
      CALL Assert 'Invalid absolute include dependency 'inc
   /* rules lookup */
   dep = VALUE(ARG(1))
   IF ABBREV(dep, 'RULE') THEN
      CALL Assert 'DoInclude called with invalid symbol' ARG(1)
   /* Is dependency not redundant */
   IF POS(inc, dep) = 0 THEN
      /* add dependency */
      CALL VALUE ARG(1), dep' 'inc
   RETURN

/* Normalize path
 * ARG(1)  include file path
 * ARG(2)  path context (optional)
 * RETURN  reduced include path
 */
NormPath: PROCEDURE EXPOSE opt.
   /* First we have to normalize slashes to make things easy. */
   path = TRANSLATE(ARG(1), '\', '/')
   /* reduce /xxx/../ */
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
   IF ARG(2, 'o') THEN
      RETURN path
   /*IF ABBREV(path, '\') | ABBREV(SUBSTR(path, 2), ':\') THEN
      RETURN path*/
   /* Chkeck how many leading .. to keep */
   p = COMPARE(TRANSLATE(ARG(2)), TRANSLATE(path))
   /*SAY 'SC:' SUBSTR(ARG(2), 1, p) p LENGTH(ARG(2)) path*/
   IF p <= LENGTH(ARG(2)) THEN DO
      p = LASTPOS('\', ARG(2), p-1) +1
      /* count \ in different part of opt.cwd */
      q = WORDS(TRANSLATE(SUBSTR(ARG(2), p), ' \', '\ '))
      /*SAY 'S\:' TRANSLATE(SUBSTR(opt.cwd, p), ' \', '\ ') q*/
      path = COPIES('..\', q)||SUBSTR(path, p)
      END
   ELSE
      path = SUBSTR(path, p)
   /* done */
   /*SAY 'SX:' opt.cwd ARG(1) path
   SAY*/
   RETURN path

/* Make path absolute
 * ARG(1)  file name
 * ARG(2)  path context
 * RETURN  absolute path
 */
MakeAbs: PROCEDURE EXPOSE opt.
   IF ABBREV(ARG(1), '\') | ABBREV(SUBSTR(ARG(1), 2), ':\') THEN
      RETURN ARG(1)
   IF RIGHT(ARG(2), 1) = '\' THEN
      RETURN NormPath(ARG(2)ARG(1))
   RETURN NormPath(ARG(2)'\'ARG(1))

/* Check for recursive entries
 * ARG(1)  file name
 */
CheckRec: PROCEDURE EXPOSE opt. rule. stack.
   sym = SymName(MakeAbs(ARG(1), opt.cwd))
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
 * RETURN  symbol
 */
SymName: PROCEDURE
   IF \ABBREV(ARG(1), '\') & \ABBREV(SUBSTR(ARG(1), 2), ':\') THEN
      CALL Assert 'SymName called with relative path 'ARG(1)
   inc = TRANSLATE(TRANSLATE(ARG(1)), '__ucsdtmpha', "\/_:;.,-+#'")
   p = 1
   DO FOREVER
      p = VERIFY(inc, 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_',, p)
      IF p = 0 THEN
         LEAVE
      inc = SUBSTR(inc, 1, p-1)'x'C2X(SUBSTR(inc, p, 1))SUBSTR(inc, p+1)
      p = p + 3
      END
   RETURN "RULE."inc

/* Get directory of absolute or relative file name
 * ARG(1) file name
 * RETURN directoory of file name without trailing \ except for the root
 */
DirFromPath: PROCEDURE
   dir = FILESPEC('D', ARG(1))FILESPEC('P', ARG(1))
   IF ((LENGTH(dir) > 3) | (POS(':',dir) = 0)) & (POS(RIGHT(dir, 1), '\/') \= 0) THEN
      dir = LEFT(dir, LENGTH(dir) - 1)
   RETURN dir


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

Assert: PROCEDURE
   CALL LINEOUT STDERR, ARG(1)
   CALL Die!
