/*--------------------------------------------------------------------*/
/* REXX script to convert a bunch of .html files into os/2 .ipf files */
/*  which can be converted later into .inf files using ipfc compiler  */
/*                                                                    */
/*               Copyright (c) 1997 by FRIENDS software               */
/*                         All Rights Reserved                        */
/*                                                                    */
/* FidoNet: 2:5030/84.5                                               */
/* e-mail:  Andrew Zabolotny <bit@freya.etu.ru>                       */
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
/* user-customisable section start */

/* A command to convert any image file into os/2 bmp format           */
/* This script requires Image Alchemy for os/2, at least demo version */
/* If someone knows of a free proggy which provides at least partial  */
/* or equivalent functionality, please mail me                        */
/* Global.ImageConvert = 'alchemy.exe -o -O -8 <input> <output> >nul';*/
/* Global.ImageConvert = 'gbmbpp.exe "<input>" "<output>",1.1 >nul';  */
/* Global.ImageConvert = 'gif2bmp.exe <input> <output>';*/
 Global.ImageConvert = 'nconvert -out bmp -wflag os2 -swap bgr -o "<output>" "<input>"'
/* Executable/description of an external WWW browser to launch when   */
/* user selects an URL link. Normally, you shouldn`t change it (even  */
/* if you have Netscape) since WebEx is found on almost every OS/2    */
/* system, and Navigator is not.                                      */
 Global.WWWbrowser = 'netscape.exe*Netscape Communicator';
/* default book font; use warpsans bold for a nicer-looking books     */
 Global.DefaultFont = ':font facename=default size=0x0.';
/*                    ':font facename=''WarpSans Bold'' size=9x6.';   */
/* fonts for headings (1 through 6)                                   */
 Global.HeaderFont.1 = ':font facename=''Helv'' size=24x12.';
 Global.HeaderFont.2 = ':font facename=''Helv'' size=22x10.';
 Global.HeaderFont.3 = ':font facename=''Helv'' size=18x9.'
 Global.HeaderFont.4 = ':font facename=''Helv'' size=14x8.'
 Global.HeaderFont.5 = ':font facename=''Helv'' size=12x8.'
 Global.HeaderFont.6 = ':font facename=''Helv'' size=10x6.'
/* font for url links (which launches WebExplorer)                    */
/* Global.URLinkFont  = ':font facename=''WarpSans Bold'' size=9x6.'  */
 Global.URLinkFont  = '';
/* proportional font (for <tt>...</tt>                                */
 Global.ProportFont = ':font facename=''System VIO'' size=11x6.';

/* end of user-customisable section                                   */
/*--------------------------------------------------------------------*/
'@echo off'
 call rxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs'
 call SysLoadFuncs

 parse arg _cmdLine

/***************** hard-coded variables **********************/
/* maximal line length for ipfc :-( */
 Global.maxLineLength = 256;
/* unix end-of-line constant */
 Global.EOL = d2c(10);
/* file extensions and name of handler procedures for these; */
/* all other file extensions will be ignored */
 Global.TypeHandler = '*.HTML doParseHTML *.SHTML doParseHTML *.HTM doParseHTML',
                      '*.HT3 doParseHTML *.HTM3 doParseHTML *.TXT doParseText',
                      '*.TEXT doParseText *.CMD doParseText *.BAT doParseText',
                      '*.GIF doParseImage *.JPG doParseImage *.PNG doParseImage';
/* Set up some global variables */
 Global.Picture.0 = 0;                     /* keep track of embedded Pictures */
 Global.LinkID = 0;                         /* total number of external links */
 Global.URLinks = 0;                               /* keep track of url links */
 Global.Title = '';                                             /* book Title */
 Global.HREF = '';   /* Speedup: keep all encountered HREFs and IMG_SRCs in a */
 Global.IMGSRC = '';    /* string so we can use Pos() and WordPos() functions */
 Global.NewLine = 1;

/* Default state for all switches */
 Global.optCO = 1;  /* COlored output */
 Global.optCE = 1;  /* enable CEntering */
 Global.optCH = 0;  /* disable CHecking */
 Global.optP = 1;   /* embed Pictures */
 Global.optS = 1;   /* Sort links */
 Global.optD = 0;   /* Debug log */
 call AnalyseOptions;
 call DefineQuotes;

 call ShowHeader;
 if length(_fName) = 0
  then call ShowHelp;

 Global.oName = _oName;
 if length(Global.oName) = 0
  then do
        i = lastPos('.', _fName);
        if i > 0
         then Global.oName = left(_fName, i)||'ipf';
         else Global.oName = _fName||'.ipf';
       end;
 call SetColor lCyan;
 if Global.OptCH
  then say 'Checking the integrity of links for '_fName;
  else do
        say 'Output goes into 'Global.oName;
        call SysFileDelete(Global.oName);
       end;

 DateTime = Date(n)', 'Time(c);
 call logError ''
 call logError '--- ['DateTime'] conversion started: index file '_fName;

 call putline '.*'copies('-', 76)'*';
 call putline '.*'center('Converted by HTML2IPF from '_fName' at 'DateTime, 76)'*';
 call putline '.*'copies('-', 76)'*';
 call putline ':userdoc.';
 call putline ':docprof toc=12345.';
 call putline ':ctrldef.';
 call putline ":ctrl controls='Esc Search Print Contents Back Forward' coverpage.";
 call putline ':ectrldef.';

 call time 'R'
 call ParseFile _fName, 1;
 do until ResolveLinks(1) = 0;
 end;
 call ConvertPictures;
 call OutputURLs;

 call putline ':euserdoc.';

 call SetColor lCyan;
 elapsed = time('E');
 say 'finished; elapsed time = 'elapsed%3600':'elapsed%60':'trunc(elapsed//60,1);
 DateTime = Date(n)', 'Time(c);
 call logError '--- ['DateTime'] conversion finished';

 DO i = 1 TO Global.LinkID
   IF \Global.LinkID.i.Resolved THEN
     SAY 'Unresolved internal link:' Global.LinkID.i
   END;

exit;

AnalyseOptions:
 _fName = ''; _oName = '';
 do i = 1 to words(_cmdLine)
  nw = word(_cmdLine, i);
  if left(nw, 1) = '-'
   then do
         nw = translate(substr(nw, 2));
         OptState = pos(right(nw, 1), '-+');
         if OptState > 0
          then nw = left(nw, length(nw) - 1);
          else OptState = 2;
         OptState = OptState - 1;
         select
          when abbrev('COLORS', nw, 2)
           then Global.OptCO = OptState;
          when abbrev('CENTER', nw, 2)
           then Global.OptCE = OptState;
          when abbrev('CHECK', nw, 2)
           then Global.OptCH = OptState;
          when abbrev('SORT', nw, 1)
           then Global.OptS = OptState;
          when abbrev('PICTURES', nw, 1)
           then Global.OptP = OptState;
          when abbrev('DEBUG', nw, 1)
           then Global.OptD = OptState;
          otherwise
           do
            call ShowHeader;
            call SetColor lRed;
            say 'Invalid option in command line: 'word(_cmdLine, i);
            call ShowHelp;
           end;
         end;
        end
   else if length(_fName) = 0
         then _fName = nw
         else
        if length(_oName) = 0
         then _oName = nw
         else do
               call ShowHeader;
               call SetColor lRed;
               say 'Extra filename in command line: 'word(_cmdLine, i);
               call ShowHelp;
              end;
 end;
return;

ShowHeader:
 call SetColor white
 say '��� HTML2IPF ��� Version 0.1.1 ��� Copyright (c) 1997 by FRIENDS software ���'
return;

ShowHelp:
 call SetColor Yellow
 say 'Usage: HTML2IPF [IndexFilename.HTML] {OutputFilename.IPF} {conversion options}'
 call SetColor lGreen;
 say '[IndexFilename.HTML]'
 call SetColor Green;
 say '�Ĵis the "root" .HTML file to start with'
 call SetColor lGreen;
 say '{OutputFilename.IPF}'
 call SetColor Green;
 say '�Ĵis the output filename (usually with the .IPF extension)'
 call SetColor lGreen;
 say '{conversion options}'
 call SetColor Green;
 say '��´are one or more of the following:'
 say '  ��´-CO{LORS}{+|-}'
 say '   ���use (+) or don`t use (-) ansi [c]olors in output'
 say '   �´-CE{NTER}{+|-}'
 say '   ���enable (+) or disable (-) processing <CENTER> tags'
 say '   �´-CH{ECK}{+|-}'
 say '   ���enable (+) or disable (-) checking files only'
 say '   �´-S{ORT}{+|-}'
 say '   ���sort (+) or don`t sort (-) links alphabetically'
 say '   �´-P{ICTURES}{+|-}'
 say '   ���include (+) or don`t include (-) [p]ictures in .IPF file'
 say '   �´-D{EBUG}{+|-}'
 say '    ��enable (+) or disable (-) [d]ebug logging into HTML2IPF.LOG'
 call SetColor lCyan;
 say 'default HTML2IPF options:'
 call SetColor Cyan;
 say '�Ĵ-COLORS+ -CENTER+ -CHECK- -SORT+ -PICTURES+ -DEBUG-'
exit(1);

ConvertPictures:
 procedure expose Global.;
 if (\Global.optP) | (Global.OptCH) then return;

 do i = 1 to Global.Picture.0
  /* get time stamp of destination file */
  tstmp = translate('123456789abc', stream(Global.Picture.i.dst, 'c', 'Query Datetime'), '56 34 12 78 9a bc');
  /*say Global.Picture.i.src'_'Global.Picture.i.dst'_'stream(Global.Picture.i.dst, 'c', 'Query Exists')'___'tstmp*/
  if (tstmp = '') | (tstmp < translate('123456789abc', stream(Global.Picture.i.src, 'c', 'Query Datetime'), '56 34 12 78 9a bc')) then
     call RunCmd Global.ImageConvert, Global.Picture.i.src, Global.Picture.i.dst;
 end;
return;

RunCmd:
 procedure expose Global.;
 cmd = arg(1);
 in = arg(2);
 out = arg(3);
 call SetColor lGreen
 ip = pos('<input>', cmd);
 if ip <> 0 then cmd = left(cmd, ip - 1)||translate(in,"/","\" )||substr(cmd, ip + 7);
 op = pos('<output>', cmd);
 if op <> 0 then cmd = left(cmd, op - 1)||translate(out,"/","\")||substr(cmd, op + 8);
 /*say "XXX"cmd'_'in'_'out'_'*/

 cmd;
return;

OutputURLs:
/* make a chapter with links to internet locations */
 if Global.URLinks = 0
  then return;
/* call putline ':h1.External links';
 call putline Global.DefaultFont;
 call putline ':p.This chapter contains all external links referenced in this book -';
 call putline 'either link is an Unified Resource Locator (URL) or simply to a';
 call putline 'local file which is not a part of this book.';*/
/* Sort URLs alphabetically */
 if Global.OptS
  then do i = 1 to Global.URLinks;
        ii = Global.URLinks.i;
        do j = i + 1 to Global.URLinks;
         ji = Global.URLinks.j;
         if Global.LinkID.ji < Global.LinkID.ii
          then do
                tmp = Global.URLinks.i;
                Global.URLinks.i = Global.URLinks.j;
                Global.URLinks.j = tmp;
                ii = ji;
               end;
        end;
       end;
 if Global.OptCH
  then do
        call SetColor LGreen;
        do i = 1 to Global.URLinks
         j = Global.URLinks.i;
         say 'Unresolved link: 'Global.LinkID.j.RealName;
         call logError '--- Unresolved link: 'Global.LinkID.j.RealName;
        end;
        return;
       end;
 Global.CurrentDir = '';
 do i = 1 to Global.URLinks
  j = Global.URLinks.i;
  call putline ':h2 hide id='GetLinkID(Global.LinkID.j)'.'IPFstring(Global.LinkID.j.RealName);
  call putline Global.DefaultFont;
  call putline ':p.:lines align=center.';
  call putline IPFstring('The link you selected points to an external resource. Click the',
                             'URL below to launch 'substr(Global.WWWbrowser, pos('*', Global.WWWbrowser) + 1));
  call putline Global.URLinkFont;
  call putline ':p.:link reftype=launch object='''left(Global.WWWbrowser, pos('*', Global.WWWbrowser) - 1),
                             ''' data='''Global.LinkID.j.RealName'''.';
  call putline IPFstring(Global.LinkID.j.RealName);
  call putline ':elink.:elines.';
 end;
return;

/* Parse a HTML file; called recursively if needed */
ParseFile:
 procedure expose Global.;
 parse arg fName, DeepLevel;
 call SetColor Cyan;
 say
 call charout ,'Parsing 'fName' ...';

 Global.CurrentDir = '';
 id = GetLinkID(fName);
 if id > 0 then Global.LinkID.id.Resolved = 1;

 tmp = translate(stream(fName, 'c', 'Query Exists'), '/', '\');
 if length(tmp) = 0
  then do
        call SetColor lRed;
        say ' not found';
        call logError '--- file 'fName' not found';
        return;
       end;
 fName = Shorten(tmp);
 Global.CurrentDir = fileSpec('P', translate(fName, '\', '/'));
 Global.CurrentFile = fName;
 call logError '--- Parsing file "'fName'" ...';

 Global.Article.Title = '';                                  /* Article Title */
 Global.Article.line.0 = 0;                      /* count of lines in Article */
 Global.Article.Hidden = 0;  /* Is current article hidden from book contents? */
 Global.Article.ResId  = 0;                    /* Article resource identifier */
 Global.Article.Level = 0;        /* Overide automatic detection of deeplevel */
 Global.OpenTag.0 = 0;  /* keep track of open tags to close at end of chapter */
 Global.RefEndTag = '';       /* end tag to put at next end-of-reference <\a> */
 Global.IsTable = 0;               /* We`re inside a <TABLE>...</TABLE> pair? */
 Global.IsCentered = 0;          /* We`re inside a <CENTER>...</CENTER> pair? */
 Global.TextStyle = '';                 /* Collection of current text styles: */
                       /* I(talic), U(nderline), B(old), E(mphasis), S(trong) */
 Global.IsOutputEnabled = 1; /* A global switch to enable/disable text output */
 Global.SkipSpaces = 1;                   /* set to 1 in lists to skip spaces */
 Global.HasText = 0;                  /* 1 if the current section has content */
 Global.Paragraph = 'Inc';/* 'Inc' if a paragraph distance is included in last tag */
                          /* 'Req' if a paragraph distance is required by the next text block */
 Global.PageLinks = 0;  /* This stem keeps track of links in the current page */
 Global.SubLinks = 0;           /* This stem keeps track of the SUBLINKS tags */
 Global.NoSubLinks = 0;       /* This stem keeps track of the NOSUBLINKS tags */
 call PutToken Global.EOL;                     /* initialize output subsystem */
 Global.EOF = 0;
 Global.CurFont = Global.DefaultFont;

 fExt = max(lastPos('/', fName), lastPos('\', fName));
 if lastPos('.', fName) > fExt
  then fExt = translate(substr(fName, lastPos('.', fName) + 1))
  else fExt = '';
 fExt = wordpos('*.'fExt, Global.TypeHandler);
 if fExt > 0
  then fExt = word(Global.TypeHandler, fExt + 1)
  else do
        call SetColor lRed;
        say ' unknown file type';
        call logError '--- File 'fName': unknown type - ignored';
        return;
       end;

 Global.FileSize = chars(fName);                                 /* file size */
 select
  when fExt = 'doParseHTML'  then call doParseHTML;
  when fExt = 'doParseImage' then call doParseImage;
  when fExt = 'doParseText'  then call doParseText;
  otherwise call logError 'Unknown file type handler: 'fExt;
 end;
 call ProgressBar;
 call stream Global.CurrentFile, 'c', 'close';            /* close input file */

 if length(Global.Article.Title) = 0
  then Global.Article.Title = IPFstring(filespec('N', translate(fName, '\', '/')));
 if (length(Global.Title) = 0)
  then do
        Global.Title = ':title.'Global.Article.Title;
        call putline Global.Title; IndexFile = 'Y';
       end;
 call putline '.* Source filename: 'fName;
 if id > 0
  then do
        /* Override deeplevel? */
        if Global.Article.Level > 0
         then DeepLevel = Global.Article.Level;
        if (Global.Article.Hidden) & (IndexFile \= 'Y')
         then do
               i = max(1, DeepLevel - 1);
               j = ' hide';
               Global.SubLinks = 1; Global.Sublinks.1 = '*';
              end;
         else do
               i = DeepLevel;
               j = '';
              end;

        if Global.Article.ResId > 0 then
           call putline ':h'i' res='Global.Article.ResId' id='id||j'.'Global.Article.Title;
        else
           call putline ':h'i' id='id||j'.'Global.Article.Title;
       end;
 call putline Global.DefaultFont;
 call putline ':p.';
 do i = 1 to Global.Article.line.0
  call putline Global.Article.line.i;
 end;
 drop Global.Article.;

 call SetColor Blue;
 call charout ,' done';
 call CRLF;

 call ResolveLinks DeepLevel+1;
return;

ResolveLinks:
 procedure expose Global.;
 DeepLevel = ARG(1);
 LinkCount = 0;
 Links.0 = 0;

 /* follow explicit sublinks if any */
 if Global.SubLinks > 0 then
  do i = 1 to Global.SubLinks
   id = GetLinkID(Global.SubLinks.i);
   if id = 0 then iterate;
   /* skip resolved */
   if Global.LinkID.id.Resolved then iterate;
   /* always exclude nosublinks */
   do j = 1 to Global.NoSubLinks
    if Global.NoSubLinks.j = Global.SubLinks.i
     then do; j = -1; leave; end;
   end;
   if j = -1 then Iterate;
   /* add link to the list of files to process */
   Links.0 = Links.0 + 1; j = Links.0;
   Links.j = Global.LinkID.id.RealName;
   Global.LinkID.id.Resolved = 1;
  end;
 /* otherwise follow page links */
 else
  do i = 1 to Global.PageLinks
   id = GetLinkID(Global.PageLinks.i);
   if id = 0 then iterate;
   /* skip resolved */
   if Global.LinkID.id.Resolved then iterate;
   /* always exclude nosublinks */
   do j = 1 to Global.NoSubLinks
    if Global.NoSubLinks.j = Global.SubLinks.i
     then do; j = -1; leave; end;
   end;
   if j = -1 then Iterate;
   /* add link to the list of files to process */
   Links.0 = Links.0 + 1; j = Links.0;
   Links.j = Global.LinkID.id.RealName;
   Global.LinkID.id.Resolved = 1;
  end;

 drop Global.PageLinks.;
 drop Global.SubLinks.;
 drop Global.NoSubLinks.;

 if Global.OptS
  then call SortLinks 1, Links.0;
 if DeepLevel > 6 then DeepLevel = 6;
 do i = 1 to Links.0
  call ParseFile translate(Links.i, '/', '\'), DeepLevel;
  LinkCount = LinkCount + 1;
 end;
return LinkCount;

SortLinks:
 procedure expose Links.;
 arg iLeft, iRight;

 Left = iLeft; Right = iRight;
 Middle = (Left + Right) % 2;
 MidVar = Links.Middle;
 do until Left > Right
  do while Links.Left < MidVar;
   Left = Left + 1;
  end;
  do while Links.Right > MidVar;
   Right = Right - 1;
  end;

  if Left <= Right
   then do
         tmp = Links.Left;
         Links.Left = Links.Right;
         Links.Right = tmp;
         Left = Left + 1;
         Right = Right - 1;
        end;
 end;
 if iLeft < Right
  then call SortLinks iLeft, Right;
 if Left < iRight
  then call SortLinks Left, iRight;
return;

doParseHTML:
 Global.FileContents = '';
 call ParseContents 'EMPTY';
return;

doParseText:
 Global.SubLinks = 1;
 Global.SubLinks.1 = '*';           /* A plain text file cannot have sublinks */
 call PutToken ':lines align=left.';
 call SetFont Global.ProportFont; /* draw text using proportional font */
 do while chars(fName) > 0;
  call ProgressBar;
  Global.FileContents = charin(fName,,4096);
/* remove all \0x0d Characters from output stream */
  do until i = 0
   i = pos(d2c(13), Global.FileContents);
   if i > 0 then Global.FileContents = delstr(Global.FileContents, i, 1);
  end;
  call PutText Global.FileContents;
 end;
 call PutToken ':elines.';
return;

doParseImage:
 _imgBitmap = GetPictureID(fName);
 if (\Global.optP) | (length(_imgBitmap) <= 1)
  then do
        if Global.optP
         then do
               call SetColor Yellow;
               parse value SysCurPos() with row col;
               if col > 0 then call CRLF;
               say 'Warning: Picture "'Global._imgname'" missing';
               call logError 'Picture "'Global._imgname'" missing';
              end;
        call PutText ':lines align=center.';
        call PutText fName;
        call PutText ':elines.';
       end
  else do
        Global.Picture.0 = Global.Picture.0 + 1;
        i = Global.Picture.0;
        Global.Picture.i.dst = left(_imgBitmap, pos('*', _imgBitmap) - 1);
        Global.Picture.i.src = substr(_imgBitmap, pos('*', _imgBitmap) + 1);
        Global.Picture.i.alt = fName;
        call PutToken ':artwork name='''Global.Picture.i.dst''' align=center.';
       end;
return;

ParseContents:
 procedure expose Global.;
 arg TextHandler;
 do until (length(Global.FileContents) = 0) & (Global.EOF)
  Token = GetToken();
  if left(Token, 1) = d2c(0)
   then do
         Token = translate(strip(substr(Token, 2)),,'0A'x);
      /* assume everything starting with <! is not important */
         if left(Token, 1) = '!'
          then iterate;
         Tag = strip(translate(Token, xrange('A','Z')'_!', xrange('a','z')'-/'));
         TagBreakPos = pos(' ', Tag);
         if TagBreakPos > 0
          then Tag = left(Tag, TagBreakPos - 1);
         TagBreakPos = 0;
         select
          when Tag = 'HTML'	then TagBreakPos = doTagHTML();
          when Tag = '!HTML'	then TagBreakPos = doTag!HTML();
          when Tag = 'HEAD'	then TagBreakPos = doTagHEAD();
          when Tag = '!HEAD'	then TagBreakPos = doTag!HEAD();
          when Tag = 'BODY'	then TagBreakPos = doTagBODY();
          when Tag = '!BODY'	then TagBreakPos = doTag!BODY();
          when Tag = 'META'	then TagBreakPos = doTagMETA();
          when Tag = 'TITLE'	then TagBreakPos = doTagTITLE();
          when Tag = '!TITLE'	then TagBreakPos = doTag!TITLE();
          when Tag = 'META'	then TagBreakPos = doTagMETA();
          when Tag = 'A'	then TagBreakPos = doTagA();
          when Tag = '!A'	then TagBreakPos = doTag!A();
          when Tag = 'IMG'	then TagBreakPos = doTagIMG();
          when Tag = 'I'	then TagBreakPos = doTagI();
          when Tag = '!I'	then TagBreakPos = doTag!I();
          when Tag = 'B'	then TagBreakPos = doTagB();
          when Tag = '!B'	then TagBreakPos = doTag!B();
          when Tag = 'U'	then TagBreakPos = doTagU();
          when Tag = '!U'	then TagBreakPos = doTag!U();
          when Tag = 'EM'	then TagBreakPos = doTagEM();
          when Tag = '!EM'	then TagBreakPos = doTag!EM();
          when Tag = 'TT'	then TagBreakPos = doTagTT();
          when Tag = '!TT'	then TagBreakPos = doTag!TT();
          when Tag = 'KBD'	then TagBreakPos = doTagKBD();
          when Tag = '!KBD'	then TagBreakPos = doTag!KBD();
          when Tag = 'P'	then TagBreakPos = doTagP();
          when Tag = '!P'	then TagBreakPos = doTag!P();
          when Tag = 'H1'	then TagBreakPos = doTagH1();
          when Tag = '!H1'	then TagBreakPos = doTag!H1();
          when Tag = 'H2'	then TagBreakPos = doTagH2();
          when Tag = '!H2'	then TagBreakPos = doTag!H2();
          when Tag = 'H3'	then TagBreakPos = doTagH3();
          when Tag = '!H3'	then TagBreakPos = doTag!H3();
          when Tag = 'H4'	then TagBreakPos = doTagH4();
          when Tag = '!H4'	then TagBreakPos = doTag!H4();
          when Tag = 'H5'	then TagBreakPos = doTagH5();
          when Tag = '!H5'	then TagBreakPos = doTag!H5();
          when Tag = 'H6'	then TagBreakPos = doTagH6();
          when Tag = '!H6'	then TagBreakPos = doTag!H6();
          when Tag = 'OL'	then TagBreakPos = doTagOL();
          when Tag = '!OL'	then TagBreakPos = doTag!OL();
          when Tag = 'UL'	then TagBreakPos = doTagUL();
          when Tag = '!UL'	then TagBreakPos = doTag!UL();
          when Tag = 'LI'	then TagBreakPos = doTagLI();
          when Tag = 'DL'	then TagBreakPos = doTagDL();
          when Tag = '!DL'	then TagBreakPos = doTag!DL();
          when Tag = 'DT'	then TagBreakPos = doTagDT();
          when Tag = 'DD'	then TagBreakPos = doTagDD();
          when Tag = 'BR'	then TagBreakPos = doTagBR();
          when Tag = 'CITE'	then TagBreakPos = doTagCITE();
          when Tag = '!CITE'	then TagBreakPos = doTag!CITE();
          when Tag = 'CENTER'	then TagBreakPos = doTagCENTER();
          when Tag = '!CENTER'	then TagBreakPos = doTag!CENTER();
          when Tag = 'PRE'	then TagBreakPos = doTagPRE();
          when Tag = '!PRE'	then TagBreakPos = doTag!PRE();
          when Tag = 'META'	then TagBreakPos = doTagMETA();
          when Tag = 'MENU'	then TagBreakPos = doTagMENU();
          when Tag = '!MENU'	then TagBreakPos = doTag!MENU();
          when Tag = 'CODE'	then TagBreakPos = doTagCODE();
          when Tag = '!CODE'	then TagBreakPos = doTag!CODE();
          when Tag = 'VAR'	then TagBreakPos = doTagVAR();
          when Tag = '!VAR'	then TagBreakPos = doTag!VAR();
          when Tag = 'SAMP'	then TagBreakPos = doTagSAMP();
          when Tag = '!SAMP'	then TagBreakPos = doTag!SAMP();
          when Tag = 'STRONG'	then TagBreakPos = doTagSTRONG();
          when Tag = '!STRONG'	then TagBreakPos = doTag!STRONG();
          when Tag = 'ADDRESS'	then TagBreakPos = doTagADDRESS();
          when Tag = '!ADDRESS'	then TagBreakPos = doTag!ADDRESS();
          when Tag = 'HR'	then TagBreakPos = doTagHR();
          when Tag = 'TABLE'	then TagBreakPos = doTagTABLE();
          when Tag = '!TABLE'	then TagBreakPos = doTag!TABLE();
          when Tag = 'TR'	then TagBreakPos = doTagTR();
          when Tag = '!TR'	then TagBreakPos = doTag!TR();
          when Tag = 'TH'	then TagBreakPos = doTagTH();
          when Tag = '!TH'	then TagBreakPos = doTag!TH();
          when Tag = 'TD'	then TagBreakPos = doTagTD();
          when Tag = '!TD'	then TagBreakPos = doTag!TD();
          when Tag = 'BLOCKQUOTE'then TagBreakPos = doTagBLOCKQUOTE();
          when Tag = '!BLOCKQUOTE'then TagBreakPos = doTag!BLOCKQUOTE();
          otherwise call logError 'Unexpected tag <'Token'>';
         end;
         if TagBreakPos then leave;
        end;
   else select
         when TextHandler = 'EMPTY'	then call doTextEMPTY;
         when TextHandler = 'HEAD'	then call doTextHEAD;
         when TextHandler = 'BODY'	then call doTextBODY;
        end;
 end;
return;

ParseTag:
 procedure expose Global.;
 parse arg Prefix Tag
 Prefix = translate(Prefix);
 do while length(Tag) > 0
  parse value translate(Tag, ' ', Global.EOL) with subTag '=' Tag;
  Tag = strip(Tag, 'leading');
  if left(Tag, 1) = '"'
   then parse var Tag '"' subTagValue '"' Tag
   else parse var Tag subTagValue Tag;
  subTag = translate(strip(subTag));
  subTagValue = strip(subTagValue);
  select
   when Prefix = 'A'
    then select
          when subTag = 'HREF'		then call doTagA_HREF;
          when subTag = 'NAME'		then call doTagA_NAME;
          otherwise call logError 'Unexpected subTag 'subTag'="'subTagValue'"';
         end;
   when Prefix = 'IMG'
    then select
          when subTag = 'SRC'		then call doTagIMG_SRC;
          when subTag = 'ALT'		then call doTagIMG_ALT;
          when subTag = 'ALIGN'		then call doTagIMG_ALIGN;
          when subTag = 'WIDTH'		then call doTagIMG_WIDTH;
          when subTag = 'HEIGHT'	then call doTagIMG_HEIGHT;
          otherwise call logError 'Unexpected subTag 'subTag'="'subTagValue'"';
         end;
   when Prefix = 'HTML'
    then select
          when subTag = 'HIDDEN'     then call doTagHTML_HIDDEN;
          when subTag = 'SUBLINKS'   then call doTagHTML_SUBLINKS;
          when subTag = 'NOSUBLINKS' then call doTagHTML_NOSUBLINKS;
          when subTag = 'RESID'      then call doTagHTML_RESID;
          when subTag = 'LEVEL'      then call doTagHTML_LEVEL;
          otherwise call logError 'Unexpected subTag 'subTag'="'subTagValue'"';
         end;
  end;
 end;
return;

doTagHTML:
 call ParseTag Token;
 call ParseContents 'EMPTY';
return 0;

doTag!HTML:
return 1;

doTagHTML_HIDDEN:
 Global.Article.Hidden = 1;
return 0;

doTagHTML_RESID:
 Global.Article.ResID = SubTagValue;
return 0;

doTagHTML_LEVEL:
 Global.Article.Level = SubTagValue;
return 0;

doTagHTML_SUBLINKS: PROCEDURE EXPOSE Global. SubTagValue
 SubTagValue = translate(SubTagValue)
 i = Global.SubLinks + 1;
 DO FOREVER
  p = POS(';',SubTagValue);
  IF p == 0 THEN
   LEAVE;
  Global.SubLinks.i = LEFT(SubTagValue, p-1);
  i = i + 1;
  SubTagValue = SUBSTR(SubTagValue, p+1);
  END;
 Global.SubLinks.i = SubTagValue;
 Global.SubLinks = i;
return 0;

doTagHTML_NOSUBLINKS: PROCEDURE EXPOSE Global. SubTagValue
 SubTagValue = translate(SubTagValue)
 i = Global.NoSubLinks + 1;
 DO FOREVER
  p = POS(';',SubTagValue);
  IF p == 0 THEN
   LEAVE;
  Global.NoSubLinks.i = LEFT(SubTagValue, p-1);
  i = i + 1;
  SubTagValue = SUBSTR(SubTagValue, p+1);
  END;
 Global.NoSubLinks.i = SubTagValue;
 Global.NoSubLinks = i;
return 0;

doTagHEAD:
 Global.grabTitle = 0;
 call ParseContents 'HEAD';
return 0;

doTag!HEAD:
 Global.grabTitle = 0;
return 1;

doTagBODY:
 Global.grabTitle = 0;
 call ParseContents 'BODY';
return 0;

doTag!BODY:
return 1;

doTagTITLE:
 Global.grabTitle = 1;
 Global.Article.Title = '';
return 0;

doTag!TITLE:
 Global.grabTitle = 0;
return 0;

doTagCITE:
doTagI:
doTagVAR:
 call doSetEmphasis 'I';
return 0;

doTag!CITE:
doTag!I:
doTag!VAR:
 call doResetEmphasis 'I';
return 0;

doTagEM:
 call doSetEmphasis 'E';
return 0;

doTag!EM:
 call doResetEmphasis 'E';
return 0;

doTagB:
 call doSetEmphasis 'B';
return 0;

doTag!B:
 call doResetEmphasis 'B';
return 0;

doTagSTRONG:
 call doSetEmphasis 'S';
return 0;

doTag!STRONG:
 call doResetEmphasis 'S';
return 0;

doTagU:
 call doSetEmphasis 'U';
return 0;

doTag!U:
 call doResetEmphasis 'U';
return 0;

doTagCODE:
doTagSAMP:
doTagTT:
 call SetFont Global.ProportFont;
return 0;

doTag!CODE:
doTag!SAMP:
doTag!TT:
 call SetFont Global.DefaultFont;
return 0;

doTagKBD:
 call SetFont Global.ProportFont;
 call doSetEmphasis 'B';
return 0;

doTag!KBD:
 call doResetEmphasis 'B';
 call SetFont Global.DefaultFont;
return 0;
doTagBLOCKQUOTE:
 call PutToken ':lm margin=6.';
 Global.Paragraph = 'Req';
return 0;

doTag!BLOCKQUOTE:
 call PutToken ':lm margin=1.';
 Global.Paragraph = 'Req';
 Global.HasText = 0;
return 0;

doTagP:
 call doOpenP 1;
return 0;

doTag!P:
 Global.Paragraph = 'Req';
 Global.HasText = 0;
return 0;

doTagBR:
 if Global.IsTable
  then return 0; /* IPFC does not allow .br`s in tables */
 call NewLine;
 call PutToken '.br';
 call PutToken Global.EOL;
 Global.Paragraph = 'Inc';
return 0;

doTagPRE:
 call NewLine;
 call PutToken ':cgraphic.';
 Global.SkipSpaces = 0;
 Global.Paragraph = 'Inc';
return 0;

doTag!PRE:
 Global.SkipSpaces = 1;
 call PutToken ':ecgraphic.';
 call PutToken Global.EOL;
 Global.Paragraph = 'Req';
 Global.HasText = 0;
return 0;

doTagH_begin:
 arg i;
/* if \Global.IsTable
  then do; call PutToken '.br'; call NewLine; end;*/
 call SetFont Global.HeaderFont.i;
/* if \Global.IsTable
  then do; call NewLine; call PutToken '.br'; end;*/
 call NewLine;
 call doOpenP 1;
 call doSetEmphasis 'B';
return;

doTagH1:
 call doTagH_begin 1;
return 0;

doTagH2:
 call doTagH_begin 2;
return 0;

doTagH3:
 call doTagH_begin 3;
return 0;

doTagH4:
 call doTagH_begin 4;
return 0;

doTagH5:
 call doTagH_begin 5;
return 0;

doTagH6:
 call doTagH_begin 6;
return 0;

doTag!H1:
doTag!H2:
doTag!H3:
doTag!H4:
doTag!H5:
doTag!H6:
 call doResetEmphasis 'B';
 call SetFont Global.DefaultFont;
/* if \Global.IsTable
  then do;
        call NewLine;
        call PutToken '.br';
       end;*/
 call PutToken Global.EOL;
 Global.Paragraph = 'Req';
 Global.HasText = 0;
return 0;

doTagHR:
 call NewLine;
 call PutToken ':cgraphic.'copies('�', 80)':ecgraphic.';
 call PutToken Global.EOL;
 Global.Paragraph = 'Req';
 Global.HasText = 0;
return 0;

doTagOL:
 if Global.IsTable
  then return 0;
 call doOpenOL;
return 0;

doTag!OL:
 if Global.IsTable
  then return 0;
 call doCloseTag ':eol.';
return 0;

doTagMENU:
doTagUL:
 if Global.IsTable
  then return 0;
 call doOpenUL;
return 0;

doTag!MENU:
doTag!UL:
 if Global.IsTable
  then return 0;
 call doCloseTag ':eul.';
return 0;

doTagLI:
 if Global.IsTable
  then return 0;
 if (doCheckTag(':eul.') = 0) & (doCheckTag(':eol.') = 0)
  then call doOpenUL;
 call doOpenP 0;
 call PutToken ':li.';
return 0;

doTagDL:
 if Global.IsTable
  then return 0;
 call doOpenDL;
return 0;

doTag!DL:
 if Global.IsTable
  then return 0;
 if \Global.DLDescDefined
  then call doTagDD;
 call doCloseTag ':edl.';
return 0;

doTagDT:
 if Global.IsTable
  then return 0;
 if doCheckTag(':edl.') = 0
  then call doOpenDL;
 call doOpenP 0;
 call PutToken ':dt.';
 Global.DLTermDefined = 1;
 Global.DLDescDefined = 0;
return 0;

doTagDD:
 if Global.IsTable
  then return 0;
 if doCheckTag(':edl.') = 0
  then call doOpenDL;
 call NewLine;
 if \Global.DLTermDefined
  then call doTagDT;
 call doOpenP 0;
 call PutToken ':dd.';
 Global.DLTermDefined = 0;
 Global.DLDescDefined = 1;
return 0;

doTagA:
 call CloseRef;
 call ParseTag Token;
return 0;

doTag!A:
 call CloseRef;
return 0;

doTagA_HREF:
 /* create link */
 i = GetLinkID(subTagValue);
 /*SAY "Link:" subtagvalue "->" i*/
 if i > 0
  then do
        call PutToken ':link reftype=hd refid='i'.';
        Global.CurLink = i;
        Global.RefEndTag = ':elink.'||Global.RefEndTag;
        /* remeber links in this page */
        i = Global.PageLinks +1;
        Global.PageLinks = i;
        Global.PageLinks.i = translate(subTagValue);
       end;
return 0;

doTagA_NAME:
 /* ignore */
return 0;

doTagIMG:
 call doOpenP 0;
 Global.HasText = 1;
 Global._altName = 'missing Picture';
 Global._imgName = '';
 if Global.IsCentered /* Choose default picture alignment */
  then Global._imgAlign = 'center';
  else Global._imgAlign = 'left';
 call ParseTag Token;
 _imgBitmap = GetPictureID(Global._imgName);
 if (\Global.optP) | (length(_imgBitmap) <= 1),
    | Global.IsTable       /* Since IPF does not allow pictures in tables :-( */
  then do
        if Global.optP & \Global.IsTable
         then do
               call SetColor Yellow;
               parse value SysCurPos() with row col;
               if col > 0 then call CRLF;
               say 'Warning: Picture "'Global._imgName'" missing';
               call logError 'Picture "'Global._imgName'" missing';
              end;
        call PutText ' 'Global._altName' ';
       end
  else do
        if pos(':elink.', Global.RefEndTag) > 0
         then do /* image is a link */
               call PutToken ':elink.';
              end;
        Global.Picture.0 = Global.Picture.0 + 1;
        i = Global.Picture.0;
        Global.Picture.i.dst = left(_imgBitmap, pos('*', _imgBitmap) - 1);
        Global.Picture.i.src = substr(_imgBitmap, pos('*', _imgBitmap) + 1);
        Global.Picture.i.alt = Global._altName;
        call PutToken ':artwork name='''Global.Picture.i.dst''' align='Global._imgAlign' runin.';
        Global.Paragraph = '';
        if pos(':elink.', Global.RefEndTag) > 0
         then do /* image is a link */
               call PutToken ':artlink.:link reftype=hd refid='Global.CurLink'.:eartlink.';
               call PutToken ':link reftype=hd refid='Global.CurLink'.';
              end;
       end;
return 0;

doTagIMG_ALIGN:
 if pos('<'translate(subTagValue)'>', '<LEFT><RIGHT><CENTER>') > 0
  then Global._imgAlign = subTagValue;
return 0;

doTagIMG_SRC:
 Global._imgName = subTagValue;
return 0;

doTagIMG_ALT:
 Global._altName = subTagValue;
return 0;

doTagIMG_WIDTH:
doTagIMG_HEIGHT:
/* nop */
return 0;

doTagADDRESS:
/* nop */
return 0;

doTag!ADDRESS:
/* nop */
return 0;

doTagMETA:
/* nop */
return 0;

doTagCENTER:
 if \Global.OptCE
  then return 0;
 Global.IsCentered = 1;
 call PutToken ':lines align=center.';
return 0;

doTag!CENTER:
 if \Global.OptCE
  then return 0;
 if Global.IsCentered
  then do
        Global.IsCentered = 0;
        call NewLine;
        call PutToken ':elines.';
        call NewLine;
        call PutToken '.br';
       end;
return 0;

doTagTABLE:
 call doOpenP 0;
 Global.Table.WasCentered = Global.IsCentered;
 if Global.IsCentered
  then call doTag!CENTER;
 call NewLine;
 call PutToken '.* table';
 Global.Table.Begin = Global.Article.Line.0;
 call NewLine;
 Global.Table.Width = 0;
 Global.Table.MaxWidth = 0;
 Global.IsTable = 1;
 Global.IsOutputEnabled = 0;
return 0;

doTag!TABLE:
 call NewLine;
 if (Global.IsTable)
  then do
        i = Global.Table.Begin;
        if Global.Table.MaxWidth > 0
         then ColWidth = (79 - Global.Table.MaxWidth) % Global.Table.MaxWidth
         else tableCols = 78;
        tableCols = '';
        do j = 1 to Global.Table.MaxWidth
         tableCols = tableCols' 'ColWidth;
        end;
        if \Global.OptCH
         then Global.Article.Line.i = ':table cols='''substr(tableCols, 2)'''.';
        call PutToken ':etable.';
        call PutToken Global.EOL;
       end;
 Global.Table.Begin = 0;
 Global.IsTable = 0;
 Global.IsOutputEnabled = 1;
 if Global.Table.WasCentered
  then call doTagCENTER;
 Global.Paragraph = 'Inc';
return 0;

doTagTR:
 call NewLine;
 call PutToken ':row.';
 call PutToken Global.EOL;
 Global.IsOutputEnabled = 0;
return 0;

doTag!TR:
 call CloseRef;
 if Global.Table.Width > Global.Table.MaxWidth
  then Global.Table.MaxWidth = Global.Table.Width;
 Global.Table.Width = 0;
return 0;

doTagTH:
 Global.IsOutputEnabled = 1;
 Global.Table.Width = Global.Table.Width + 1;
 call NewLine;
 call PutToken ':c.'; call doTagU;
return 0;

doTag!TH:
 call CloseRef;
 call doTag!U;
return 0;

doTagTD:
 Global.IsOutputEnabled = 1;
 Global.Table.Width = Global.Table.Width + 1;
 call NewLine;
 call PutToken ':c.';
return 0;

doTag!TD:
 call CloseRef;
return 0;

doTextEMPTY:
 Token = translate(Token, ' ', xrange(d2c(0),d2c(31)));
 if length(strip(Token)) > 0
  then call logError 'Unexpected text 'Token;
return;

doTextHEAD:
 if Global.grabTitle = 1
  then Global.Article.Title = Global.Article.Title||translate(Token, '  ', d2c(9)d2c(10))
  else call dotextempty;
return;

doTextBODY:
 call PutText Token;
return;

doOpenOL:
 call NewLine;
 call doOpenTag ':ol compact.',':eol.';
return;

doOpenUL:
 call NewLine;
 call doOpenTag ':ul compact.',':eul.';
return;

doOpenDL:
 call NewLine;
 call doOpenTag ':dl compact break=all.', ':edl.';
 Global.DLTermDefined = 0;
 Global.DLDescDefined = 0;
return;

CloseRef:
 call PutToken Global.RefEndTag;
 Global.RefEndTag = '';
return;

/* Text emphasis */
doSetEmphasis:
 procedure expose Global.;
 p = pos(ARG(1), Global.TextStyle);
 if p = 0
  then do
   oldipfstyle = GetIPFStyle(Global.TextStyle);
   Global.TextStyle = Global.TextStyle||ARG(1);
   /*SAY "doSetEmphasis("ARG(1)") "Global.TextStyle*/
   call ChangeIPFStyle oldipfstyle, GetIPFStyle(Global.TextStyle);
  end;
return;

doResetEmphasis:
 procedure expose Global.;
 p = pos(ARG(1), Global.TextStyle);
 if p > 0
  then do
   oldipfstyle = GetIPFStyle(Global.TextStyle);
   Global.TextStyle = substr(Global.TextStyle, 1, p-1)||substr(Global.TextStyle, p+1);
   /*SAY "doResetEmphasis("ARG(1)") "Global.TextStyle*/
   call ChangeIPFStyle oldipfstyle, GetIPFStyle(Global.TextStyle);
  end;
return;

ChangeIPFStyle:
 procedure expose Global.;
 if ARG(1) <> ARG(2)
  then do
   /*SAY "ChangeIPFStyle("ARG(1)","ARG(2)")"*/
   if ARG(1) <> 0 then
    call PutToken ':ehp'ARG(1)'.';
   if ARG(2) <> 0 then
    call PutToken ':hp'ARG(2)'.';
  end;
return;

GetIPFStyle:
 procedure;
 if pos('S', ARG(1)) <> 0 then return 8;
 if pos('E', ARG(1)) <> 0 then return 4;
 s = 0;
 if pos('I', ARG(1)) <> 0 then s = s + 1;
 if pos('B', ARG(1)) <> 0 then s = s + 2;
 if pos('U', ARG(1)) <> 0 then s = s + 5;
 if s = 8 then return 9;
return s;

/* recursive Tags management */
doOpenTag:
 parse arg ot, ct;
 call PutToken ot;
 i = Global.OpenTag.0 + 1;
 Global.OpenTag.0 = i;
 Global.OpenTag.i = ct;
 Global.OpenTag.i.open = ot;
 Global.Paragraph = 'Inc';
return;

doCloseTag:
 parse arg bottom;
 call NewLine;
 if length(bottom) = 0
  then i = 1
  else do i = Global.OpenTag.0 to 0 by -1
        if bottom = Global.OpenTag.i
         then leave;
       end;
 if i > 0
  then do
        call NewLine;
        do j = Global.OpenTag.0 to i by -1
         call PutToken Global.OpenTag.j;
         call PutToken Global.EOL;
        end;
        Global.OpenTag.0 = i - 1;
        Global.Paragraph = 'Req';
        return 1;
       end;
 return 0;

doCheckTag:
 procedure expose Global.;
 do i = Global.OpenTag.0 to 1 by -1
  if pos(arg(1), Global.OpenTag.i) > 0
   then return 1;
 end;
return 0;

doCheckParentTag:
 procedure expose Global.;
 i = Global.OpenTag.0
 do j = 1 while arg(j, 'e')
  if pos(arg(j), Global.OpenTag.i) > 0
    then return 1;
 end;
return 0;

/* Ensure that paragraph is open.
 * If ARG is 1 a paragraph is enforced even if not required. */
doOpenP:
 if (Global.Paragraph = 'Req') | (ARG(1) & (Global.Paragraph \= 'Inc'))
  then do
   call NewLine;
   if (doCheckParentTag(':eul.', ':eol.', ':edl.')) then do
    /* Exception for lists with paragraphs. IPFC otherwise renders two blank lines. */
    call PutToken '.br';
    call NewLine;
    end;
   else
    call PutToken ':p.';
   Global.HasText = 0;
   Global.Paragraph = 'Inc';
  end;
return;

/* Set the current font in output stream */
SetFont:
 parse arg Font;
 if Global.IsTable
  then return;
 if Global.CurFont = Font
  then return;
 Global.CurFont = Font;
 call PutToken Font;
return;

/* Get id number depending of link value (<A HREF=...>) */
/* Returns 0 if link belongs to same page (alas, IPF doesn`t permit this...) */
GetLinkID:
 procedure expose Global.;
 link = ARG(1);

 InitialLink = link;
 if pos('#', link) > 0
  then link = left(link, pos('#', link) - 1);
 if length(link) = 0
  then return 0;
 link = FindFile(link);
 ulink = translate(link);
 i = wordpos(ulink, Global.HREF);
 if i > 0 then return i;
 Global.LinkID = Global.LinkID + 1;
 i = Global.LinkID;
 Global.LinkID.i = ulink;
 Global.LinkID.i.RealName = link;
 Global.LinkID.i.InitialName = InitialLink;
 Global.HREF = Global.HREF||ulink||' ';
 if length(stream(link, 'c', 'query exists')) = 0
  then do
        Global.LinkID.i.Resolved = 1;
        Global.URLinks = Global.URLinks + 1;
        j = Global.URLinks; Global.URLinks.j = i;
        parse var link prot ':' location;
        if (length(location) = 0) | (pos('/', prot) > 0)
         then Global.LinkID.i.RealName = filespec('N', translate(link, '\', '/'))
       end;
  else Global.LinkID.i.Resolved = 0;
return i;

/* transform image extension into .bmp */
GetPictureID:
 procedure expose Global.;
 PictName = ARG(1);

 PictName = FindFile(PictName);
 if length(stream(PictName, 'c', 'query exists')) > 0
  then do
        tmp = PictName;
        i = lastPos('.', tmp);
        if i > 0
         then PictName = left(tmp, i)||'bmp';
         else PictName = tmp||'.bmp';
       end
  else do
        tmp = '';
        PictName = '';
       end;
return PictName||'*'||tmp;

/* Actively search for file on all possible paths */
FindFile:
 procedure expose Global.;
 fName = ARG(1);

 /* Skips full qualified URLs. */
 if pos( "://", fName ) > 0 then
    return fName;

 ifName = fName;
 parse var fName prot ':' location;
 if (length(location) > 0) & (pos('/', prot) = 0)
  then fName = location;
 tmp = '';
 do while length(fName) > 0
  do while pos(left(fName, 1), '/\') > 0
   fName = substr(fName, 2);
  end;
  if length(fName) = 0
   then leave;
  tmp = stream(fName, 'c', 'query exists');
  if length(tmp) > 0 then return Shorten(tmp);
  tmp = stream(Global.CurrentDir||fName, 'c', 'query exists');
  if length(tmp) > 0 then return Shorten(tmp);
  tmp = verify(fName, '/\', 'M')
  if tmp > 0
   then fName = substr(fName, tmp)
   else fName = '';
 end;
return ifName;

/* return next Token (a Tag or a text string) from input stream */
GetToken:
 procedure expose Global.;
 if (length(Global.FileContents) < 512) & (\Global.EOF)
  then
GetData:
       do
     /* read next chunk of file */
        Global.FileContents = Global.FileContents||charin(Global.CurrentFile,,1024);
        call ProgressBar;
     /* remove all \0x0d Characters from input stream */
        do until i = 0
         i = pos('0D'x, Global.FileContents);
         if i > 0 then Global.FileContents = delstr(Global.FileContents, i, 1);
        end;
        Global.EOF = (chars(Global.CurrentFile) = 0);
       end;

 i = pos('<', Global.FileContents);
 if (i = 0)
  then if (\Global.EOF)
        then signal GetData;
        else do
              i = length(Global.FileContents) + 1;
              if i = 1 then return '';
             end;
 if (i = 1)
  then do
        j = pos('>', Global.FileContents);
        if (j = 0)
         then if \Global.EOF
               then signal GetData;
               else j = length(Global.FileContents) + 1;
        Token = '00'x||substr(Global.FileContents, 2, j - 2);
        Global.FileContents = substr(Global.FileContents, j + 1);
       end
  else do
        Token = NoQuotes(left(Global.FileContents, i - 1));
        Global.FileContents = substr(Global.FileContents, i);
       end;
return Token;

/* put an IPF Token into output stream */
PutToken:
 procedure expose Global.;
 Output = ARG(1);

 if Global.OptCH then return;

 if Output = Global.EOL
  then do
    Global.Article.line.0 = Global.Article.line.0 + 1;
    i = Global.Article.line.0;
    Global.Article.line.i = '';
    Global.NewLine = 1;
  end
 else do
    i = Global.Article.line.0;
    if length(Global.Article.line.i) + length(Output) > Global.maxLineLength
      then do;
        call PutToken Global.EOL;
        i = Global.Article.line.0;
      end;
    Global.Article.line.i = Global.Article.line.i||Output;
    Global.NewLine = 0;
 end;
return;

/* Put an text string into Output stream */
/* If EOLs are present, string is subdivided */
PutText:
 procedure expose Global.;
 parse arg Output;

 if Global.OptCH then return;

 if (Global.IsTable) & (\Global.IsOutputEnabled)
  then return; /* Skip everything out of :c. ... :c. or :row. tags */

 if Global.SkipSpaces then do
    Output = replace( Output, d2c(9), " " );
    /* condense blanks */
    curpos = 1;
    do forever
      curpos = pos('  ', Output, curpos);
      if curpos = 0 then leave;
      endpos = verify(Output, ' ', 'N', curpos + 2);
      if endpos = 0
        then Output = left(Output, curpos)
        else Output = left(Output, curpos)||substr(Output, endpos)
    end;
 end

 do while length(Output) > 0

  EOLpos = pos(Global.EOL, Output);
  if EOLpos > 0
   then do
         if EOLpos > 1
          then _text_ = left(Output, EOLpos - 1);
         Output = substr(Output, EOLpos + 1);
         if EOLpos > 1
          then call PutText _text_;
         /*call PutToken '.* XXX';*/
         if Global.SkipSpaces
          then call NewLine;
          else call PutToken Global.EOL;
        end;
   else do
        if Global.SkipSpaces & (verify(Output, ' ', 'N') = 0)
         then return;
        if Global.Newline then
         Output = strip(Output, 'l');
        /*call PutToken '<@'left(Output,4,'_')','Global.Paragraph'>';*/
        call doOpenP 0;
        /* replace tab Characters with needed number of spaces */
         curpos = -1;
         do forever
          tabpos = pos(d2c(9), Output);
          if tabpos = 0 then leave;
          if curpos = -1 /* effective position not yet computed? */
           then do
                 i = Global.Article.line.0;
                 tmpS = Global.Article.line.i;
                 curpos = 0;
                 do while length(tmpS) > 0
                  if pos(left(tmpS, 1), '&:.') > 0
                   then do
                         EOLpos = pos('.', tmpS, 2);
                         if EOLpos = 0 then leave;
                         if left(tmpS, 1) = '&'
                          then tmpS = substr(tmpS, EOLpos);
                          else do; tmpS = substr(tmpS, EOLpos + 1); iterate; end;
                        end;
                  curpos = curpos + 1; tmpS = substr(tmpS, 2);
                 end;
                end;
          Output = left(Output, tabpos - 1)||copies(' ',
           ,8 - (curpos + tabpos - 1)//8)||substr(Output, tabpos + 1);
         end;
        /* SubDivide Output string if it is too long */
         i = Global.Article.line.0;
         do while length(Global.Article.line.i) + length(Output) > Global.maxLineLength
          EOLpos = Global.maxLineLength - length(Global.Article.line.i);
          j = EOLpos;
          do while (EOLpos > 0)
           if (c2d(substr(Output, EOLpos, 1)) <= 32) then Leave;
           EOLpos = EOLpos - 1;
          end;
          if (EOLpos = 0) & (length(Global.Article.line.i) = 0)
           then do /* Line cannot be split on word-bound :-( */
                 EOLpos = j;
                 _text_ = left(Output, EOLpos - 1);
                 Output = substr(Output, EOLpos);
                end
           else do
                 if EOLpos > 1
                  then _text_ = left(Output, EOLpos - 1)
                  else _text_ = '';
                 Output = substr(Output, EOLpos + 1);
                end;
          Global.Article.line.i = Global.Article.line.i||_text_;
          call PutToken Global.EOL;
          i = Global.Article.line.0;
         end;
         Global.Article.line.i = Global.Article.line.i||Output;
         Output = '';
         Global.Paragraph = '';
         Global.HasText = 1;
        end;
 end;
return;

PutLine:
 parse arg str;
 if Global.OptCH then return;
 call lineout Global.oName, str;
return;

NewLine:
 procedure expose Global.;
 nli = Global.Article.line.0;
 if length(Global.Article.line.nli) > 0
  then do;
   /*call PutToken '.* YYY "'Global.Article.line.nli'"'*/
   call PutToken Global.EOL;
   return 1;
  end;
return 0;

/* replace special charaters with IFP symbols
 * ARG(1) string to convert
 * ARG(2) characters to replace
 */
IPFstring:
 procedure;
 str = arg(1);
 if arg(2, 'e')
  then match = arg(2);
  else match = '&.:';
 curpos = 1;
 do forever;
  curpos = verify(str, match, 'M', curpos);
  if curpos = 0 then return str;
  select
   when substr(str, curpos, 1) = ':';
   then repl = '&colon.';
   when substr(str, curpos, 1) = '.';
   then repl = '&per.';
   when substr(str, curpos, 1) = '&';
   then repl = '&amp.';
  end;
  str = left(str, curpos-1)||repl||substr(str, curpos+1);
  curpos = curpos + length(repl);
 end;

Shorten:
 procedure;
 fName = translate(stream(ARG(1), 'c', 'query exists'), '/', '\');
 tmp = translate(Directory(), '/', '\');
 if abbrev(fName, tmp)
  then return substr(fName, length(tmp) + 2);
 if substr(fName, 2, 1) = ':'
  then return substr(fName, 3);
return fName;

logError:
 procedure expose Global.;
 if Global.optD
  then do
        parse arg line;
        call lineout 'HTML2IPF.log', line;
       end;
return;

CRLF:
 parse value SysTextScreenSize() with maxrow maxcol;
 parse value SysCurPos() with row col;
 call charout ,copies(' ', maxcol-col);
return;

ProgressBar:
 parse value SysCurPos() with row col;
 if col > 79 - 18 then say '';
 Rest = ((Global.FileSize - chars(Global.CurrentFile)) * 16) % Global.FileSize;
 call setcolor lcyan; call charOut ,'[';
 call setcolor white; call charOut ,copies('�', Rest)copies('�', 16-Rest);
 call setcolor lcyan; call charOut ,']'copies('08'x, 18);
return;

SetColor:
 arg col;
 col = ColorNo(col);
 if \Global.optCO then return;

 if col = -1 then return -1;
 if col > 7
  then col = '1;3'col-8;
  else col = '0;3'col;
 call Charout ,d2c(27)'['col'm';
return 0;

ColorNo:
 arg colname;
 if substr(colname, 1, 1) = 'L'
  then do
        colname = right(colname, length(colname) - 1);
        light = 8;
       end
  else light = 0;
 select
  when abbrev('BLACK', colname, 3)
   then return light + 0;
  when abbrev('BLUE', colname, 3)
   then return light + 4;
  when abbrev('GREEN', colname, 3)
   then return light + 2;
  when abbrev('CYAN', colname, 3)
   then return light + 6;
  when abbrev('RED', colname, 3)
   then return light + 1;
  when abbrev('MAGENTA', colname, 3)
   then return light + 5;
  when abbrev('BROWN', colname, 3)
   then return light + 3;
  when abbrev('GRAY', colname, 3)
   then return light + 7;
  when abbrev('DGRAY', colname, 3)
   then return 8;
  when abbrev('YELLOW', colname, 3)
   then return 11;
  when abbrev('WHITE', colname, 3)
   then return 15;
 end;
 return -1;

/* these constants have been ripped from    */
/* HTM2txt v 1.0, mar.11,1997 by otto r�der */
DefineQuotes:
/* --------------------------------------------- */
/* constants contributed by tremro@digicom.qc.ca */
/* modified by Marcel Mueller, defaults commented out */
/* --------------------------------------------- */
 Global.Quotes = ,
  "copy   (C)",
  "space  0x20",
  "quot   0x22",
  "nbsp   &rbl.",
  "#160   &rbl.",
  "iexcl  &inve.",
  "pound  &Lsterling.",
  "curren &bx1202.",
  "brvbar &splitvbar.",
  "sect   0xA7",
  "uml    0xA8",
  "ordf   &aus.",
  "laqno  &odqf.",
  "not    &lnot.",
  "shy    -",
  "reg    (R)",
  "hibar  0xAF",
  "deg    &deg.",
  "plusmn &pm.",
  "acute  �",
  "micro  &mu.",
  "para   ?",
  "middot &dot.",
  "cedil  0xB8",
  "ordm   &ous.",
  "raquo  &cdqf.",
  "iquest &invq.",
  "agrave &ag.",
  "Agrave &Ag.",
  "aacute &aa.",
  "Aacute &Aa.",
  "acirc  &ac.",
  "Acirc  &Ac.",
  "atilde &at.",
  "Atilde &At.",
  "auml   &ae.",
  "Auml   &Ae.",
  "aring  &ao.",
  "Aring  &Ao.",
  "ccedil &cc.",
  "Ccedil &Cc.",
  "egrave &eg.",
  "Egrave &Eg.",
  "eacute &ea.",
  "Eacute &Ea.",
  "ecirc  &ec.",
  "Ecirc  &Ec.",
  "euml   &ee.",
  "Euml   &Ee.",
  "igrave &ig.",
  "Igrave &Ig.",
  "iacute &ia.",
  "Iacute &Ia.",
  "icirc  &ic.",
  "Icirc  &Ic.",
  "iuml   &ie.",
  "Iuml   &Ie.",
  "eth    0xD0",
  "ETH    0xF0",
  "ntilde &nt.",
  "Ntilde &Nt.",
  "ograve &og.",
  "Ograve &Og.",
  "oacute &oa.",
  "Oacute &Oa.",
  "ocirc  &oc.",
  "Ocirc  &Oc.",
  "otilde &ot.",
  "Otilde &Ot.",
  "ouml   &oe.",
  "Ouml   &Oe.",
  "times  0xD7",
  "oslash 0xD8",
  "Oslash 0xF8",
  "ugrave &ug.",
  "Ugrave &Ug.",
  "uacute &ua.",
  "Uacute &Ua.",
  "ucirc  &uc.",
  "Ucirc  &Uc.",
  "uuml   &ue.",
  "Uuml   &Ue.",
  "yacute &ya.",
  "Yacute &Ya.",
  "yuml   &ye.",
  "thorn  0xDE",
  "THORN  0xFE",
  "szlig  &Beta.",
  "uarr   &uarrow.",
  "darr   &darrow.",
  "larr   &larrow.",
  "rarr   &rarrow.",
  "infin  &infinity.",
/*"lt     <",
  "gt     >",*/
/* no-ops
  "aelig  &aelig.",
  "AElig  &AElig.",
  "frac14 &frac14.",
  "frac12 &frac12.",
  "frac34 &frac34.",
  "sup1   &sup1.",
  "sup2   &sup2.",
  "sup3   &sup3.",
  "yen    &yen.",
  "cent   &cent.",
  "amp    &amp.",
  "divide &divide.",*/
return;

/* substitute quoted Characters */
NoQuotes:
 procedure expose Global.;
 text = ARG(1);
 cPos = 1;
 do forever;
  sPos = pos('&', text, cPos);
  if sPos == 0 then leave;
  qPos = pos(';', text, sPos + 1);
  if qPos == 0 then leave;
  Token = substr(text, sPos + 1, qPos - sPos - 1);

  wordN = wordpos(Token, Global.Quotes);
  if wordN = 0
   then do
         if (left(Token, 1)='#') & (datatype(substr(Token, 2), 'num'))
          then do
                Token = substr(Token,2);
                Token = d2c(Token);
               end
          else do
                Token = '&'||Token||'.';
               end
        end
   else do
         Token = word(Global.Quotes, wordN + 1);
         if left(Token, 2)="0x" then Token=x2c(substr(Token, 3));
        end
  cvttext = IPFstring(substr(text, cPos, sPos - cPos));
  text = left(text, cPos - 1)||cvttext||Token||substr(text, qPos+1);
  cPos = cPos + length(cvttext) + length(Token);
 end;

return left(text, cPos - 1)||IPFstring(substr(text, cPos));

replace:

  parse arg source, string, substitute
  string = translate(string)

  i = pos( string, translate(source))

  do while i \= 0
     source = substr( source, 1, i-1 ) || substitute ||,
              substr( source, i+length(string))

     i = pos( string, translate(source), i + length(substitute))
  end

return source;
