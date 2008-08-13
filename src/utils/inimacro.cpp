/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
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

#include "inimacro.h"
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
  char    exe_path[CCHMAXPATH];
  char    mod_filename[CCHMAXPATH];
  char    ini_filename[CCHMAXPATH];

  getModule ( &module, mod_filename, CCHMAXPATH );
  getExeName( exe_path, CCHMAXPATH );
  sfname    ( mod_filename, mod_filename, sizeof( mod_filename ));
  sdrivedir ( ini_filename, exe_path, sizeof( ini_filename ));
  strlcat   ( ini_filename, mod_filename, sizeof( ini_filename ));
  strlcat   ( ini_filename, ".ini", sizeof( ini_filename ));

  return open_ini( ini_filename );
}

/* Closes a opened profile file. */
BOOL
close_ini( HINI hini ) {
  return PrfCloseProfile( hini );
}

void load_ini_bool_core( HINI hini, const char* section, const char* key, bool* dst )
{ ULONG datasize = 0;
  PrfQueryProfileSize( hini, section, key, &datasize );
  switch (datasize)
  {case sizeof(int):
    datasize = 1;
   case 1:
    *dst = 0;
    PrfQueryProfileData( hini, section, key, dst, &datasize );
  }
}

void load_ini_data_core( HINI hini, const char* section, const char* key, void* dst, size_t size )
{ ULONG datasize = 0;
  PrfQueryProfileSize( hini, section, key, &datasize );
  if (datasize == size)
    PrfQueryProfileData( hini, section, key, dst, &datasize );
}

