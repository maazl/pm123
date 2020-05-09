/* PM123 REXX Configuration Script */

  if RxFuncQuery('SysLoadFuncs') then do
     call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs'
     call SysLoadFuncs
  end

  comps.1.info   = "GNU C++ 4.x"
  comps.1.cc     = "g++.exe"
  comps.1.args   = "--version"
  comps.1.opts   = "gcc_4"
  comps.1.sig    = "(GCC) 4."

  comps.2.info   = "GNU C++ 3.x"
  comps.2.cc     = "g++.exe"
  comps.2.args   = "--version"
  comps.2.opts   = "gcc_3"
  comps.2.sig    = "(GCC) 3."

  comps.0        = 5
  comps.selected = 0

  parse arg options

  if options == "--help" then do
     say "Usage: configure [options]"
     say "Stand-alone options:"

     say substr( " --help", 1, 10 ) "print this help."

     say "Configurations options:"

     do i = 1 to comps.0
        say substr( " --"comps.i.opts, 1, 10 ) "configure to "comps.i.info
     end
     exit 1
  end

  do i = 1 to comps.0
     if options == "--"comps.i.opts then do
        comps.selected = i
        call configure
     end
  end

  say "The compiler is not specified (try 'configure --help' for more options)."
  say "Attempts to find any..."

  do i = 1 to comps.0
     if SysSearchPath( "PATH", comps.i.cc ) \= "" then do

        if comps.i.sig \= "" then
          '@'comps.i.cc comps.i.args '2>&1 1| find "'comps.i.sig'" > nul'
        else
           rc = 0

        if rc == 0 then do
           comps.selected = i
           say "Found "comps.i.info
           call configure
        end
     end
  end

  say "Nothing it is found. Configuration aborted"
  exit 1

configure: procedure expose comps. files.

  sel = comps.selected
  say "Configuring..."

  if comps.sel.opts \= "" then do
     '@copy src\config\makerules_'comps.sel.opts' src\config\makerules'
  end

  say "Done."
  exit 0
