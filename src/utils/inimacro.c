/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
 *
 * Copyright 2006 Dmitry A.Steklenev <glass@ptv.ru>
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

#define  INCL_WIN
#define  INCL_ERRORS
#include <os2.h>
#include "utilfct.h"

/* Opens the specified profile file. */
HINI
open_ini( const char* filename ) {
  return PrfOpenProfile( WinQueryAnchorBlock( HWND_DESKTOP ), (PSZ)filename );
}

/* Opens a profile file by the name of the module in the program directory. */
HINI
open_module_ini( void )
{
  HMODULE module;
  char    module_name[CCHMAXPATH];
  char    exe_path[CCHMAXPATH];
  char    ini_filename[CCHMAXPATH*2];

  getModule ( &module, module_name, CCHMAXPATH );
  getExeName( exe_path, CCHMAXPATH );
  sdrivedir ( ini_filename, exe_path );
  sfname    ( ini_filename + strlen( ini_filename ), module_name );
  strcat    ( ini_filename, ".ini" );

  return open_ini( ini_filename );
}

/* Closes a opened profile file. */
BOOL
close_ini( HINI hini ) {
  return PrfCloseProfile( hini );
}


