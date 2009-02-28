/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com> *
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2006-2009 Marcel Mueller
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

#define  INCL_DOS
#define  INCL_ERRORS

#include <stdio.h>
#include "utilfct.h"
#include <os2.h>
#include <ctype.h>

#include "debuglog.h"

APIRET APIENTRY DosQueryModFromEIP( HMODULE *phMod, ULONG *pObjNum, ULONG BuffLen,
                                    PCHAR pBuff, ULONG *pOffset, ULONG Address );

/* Queries a module handle and name. */
void
getModule( HMODULE* hmodule, char* name, int name_size )
{
  ULONG ObjNum, Offset;

  DosQueryModFromEIP( hmodule, &ObjNum, name_size, name, &Offset, (ULONG)(&getModule));
  if( name_size ) {
    DosQueryModuleName( *hmodule, name_size, name );
  }
}

/* Queries a program name. */
void
getExeName( char* name, int name_size )
{
  if( name && name_size > 0 )
  {
    PPIB ppib;

    DosGetInfoBlocks( NULL, &ppib );
    DosQueryModuleName( ppib->pib_hmte, name_size, name );
  }
}

/* Removes leading and trailing spaces. */
char*
blank_strip( char* string )
{
  int   i;
  char* pos = string;

  while( *pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r') {
    pos++;
  }

  i = strlen(pos)+1;

  if( pos != string ) {
    memmove( string, pos, i );
  }

  i -= 2;

  while( string[i] == ' ' || string[i] == '\t' || string[i] == '\n' || string[i] == '\r' ) {
    string[i] = 0;
    i--;
  }

  return string;
}

/* Replaces series of control characters by one space. */
char*
control_strip( char* string )
{
  char* s = string;
  char* t = string;

  while( *s ) {
    if( iscntrl( *s )) {
      while( *s && iscntrl( *s )) {
        ++s;
      }
      if( *s ) {
        *t++ = ' ';
      }
    } else {
      *t++ = *s++;
    }
  }
  *t = 0;
  return string;
}

/* Removes leading and trailing spaces and quotes. */
char*
quote_strip( char* string )
{
  int   i, e;
  char* pos = string;

  while( *pos == ' ' || *pos == '\t' || *pos == '\n' ) {
    pos++;
  }

  i = strlen( pos ) - 1;

  for( i = strlen( pos ) - 1; i > 0; i-- ) {
    if( pos[i] != ' ' && pos[i] != '\t' && pos[i] != '\n' ) {
      break;
    }
  }

  if( *pos == '\"' && pos[i] == '\"' )
  {
    pos++;
    i -= 2;
  }

  for( e = 0; e < i + 1; e++ ) {
    string[e] = pos[e];
  }

  string[e] = 0;
  return string;
}

/* Removes comments starting with "#". */
char*
uncomment( char *string )
{
  int  source   = 0;
  BOOL inquotes = FALSE;

  while( string[source] ) {
     if( string[source] == '\"' ) {
       inquotes = !inquotes;
     } else if( string[source] == '#' && !inquotes ) {
       string[source] = 0;
       break;
     }
     source++;
  }
  return string;
}
