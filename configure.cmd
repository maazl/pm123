/* PM123 REXX Configuration Script */

  if RxFuncQuery('SysLoadFuncs') then do
     call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs'
     call SysLoadFuncs
  end

  comps.1.info   = "IBM VisualAge C++ 3.6"
  comps.1.cc     = "icc.exe"
  comps.1.opts   = "icc_36"
  comps.1.sig    = "Version 3.6"

  comps.2.info   = "IBM VisualAge C++ 3.0"
  comps.2.cc     = "icc.exe"
  comps.2.opts   = "icc_30"
  comps.2.sig    = "Version 3"

  comps.0        = 2
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
        '@'comps.i.cc '2>&1 1| find "'comps.i.sig'" > nul'
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
