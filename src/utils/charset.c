/*
 * Copyright 2005 Dmitry A.Steklenev
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

#include "charset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define  INCL_DOS
#define  INCL_WIN
#include <os2.h>

static int ch_rus_detect( const char *source );

const CH_ENTRY ch_list[] =
{
  { "Default",                 CH_DEFAULT,   0             },
  { "Cyrillic (KOI-8R)",       CH_CYR_KOI8R, 0             },
  { "Cyrillic (CP-866)",       CH_CYR_DOS,   0             },
  { "Cyrillic (Windows-1251)", CH_CYR_WIN,   0             },
  { "Russian (Auto-Detect)",   CH_CYR_AUTO,  ch_rus_detect }
};

const int ch_list_size = sizeof( ch_list ) / sizeof( CH_ENTRY );

/*
 * ch_default_cp: returns the current system code page.
 *
 */

static int
ch_default_cp( void )
{
  ULONG cpage[1] = { 0 };
  ULONG incb     = sizeof( cpage );
  ULONG oucb     = 0;

  DosQueryCp( incb, cpage, &oucb );
  return cpage[0];
}

/*
 * ch_find: returns a pointer to the character set entry for the
 *          specified identifier.
 */

static const CH_ENTRY*
ch_find( int id )
{
  int i;

  for( i = 0; i < ch_list_size; i++ ) {
    if( ch_list[i].id == id ) {
      return ch_list + i;
    }
  }

  return NULL;
}

/*
 * ch_rus_detect: determine a characters string character set for
 *                russian language.
 *
 *    source  source string
 *    return  character set identifier
 *
 *    note:   Automatic detection of a russian character set works
 *            correctly only with the strings consisting of lower
 *            case letters.
 */

static int
ch_rus_detect( const char *source )
{
  int k = 0;
  int w = 0;
  int d = 0;

  const int LOWERCASE = 3;
  const int UPPERCASE = 1;

  unsigned char c;

  while(( c = *source++ ) != 0 )
  {
    // non-russian characters
    if( c < 128 ) {
      continue;
    }

    // CP866
    if(( c > 159 && c < 176 ) || ( c > 223 && c < 242 )) {
       d += LOWERCASE;
    }
    if(( c > 127 && c < 160 )) {
       d += UPPERCASE;
    }

    // KOI8-R
    if(( c > 191 && c < 223 )) {
      k += LOWERCASE;
    }
    if(( c > 222 )) {
      k += UPPERCASE;
    }

    // WIN-1251

    if( c > 223 ) {
      w += LOWERCASE;
    }

    if( c > 191 && c < 224 ) {
      w += UPPERCASE;
    }
  }

  if( d > w && d > k ) {
    return CH_CYR_DOS;
  }

  if( k > w && k > d ) {
    return CH_CYR_KOI8R;
  }

  if( w > k && w > d ) {
    return CH_CYR_WIN;
  }

  return CH_DEFAULT;
}

/*
 * ch_detect: determine a characters string character set.
 *
 *    ch_source source character set
 *    source    source string
 *
 *    return    character set identifier
 */

int
ch_detect( int ch_source, const char* source )
{
  const CH_ENTRY* ch_entry = ch_find( ch_source );

  if( ch_entry && ch_entry->pfn ) {
    ch_source = ch_entry->pfn( source );
  }

  return ch_source;
}

/*
 * ch_convert: convert a characters string from one character
 *             set to another.
 *
 *    hab       program anchor block handle
 *    ch_source source character set
 *    source    source string
 *    ch_target target character set
 *    target    result buffer
 *    size      size of result buffer
 *
 *    return    != NULL: converted string
 *              == NULL: error
 */

char*
ch_convert( int ch_source, const char* source, int ch_target, char* target, size_t size )
{
  ch_source = ch_detect( ch_source, source );

  if( ch_source < 0 || ch_target < 0 ) {
    return NULL;
  }

  if( ch_source == CH_DEFAULT ) {
    ch_source = ch_default_cp();
  }
  if( ch_target == CH_DEFAULT ) {
    ch_target = ch_default_cp();
  }

  if( ch_source == ch_target ) {
    strncpy( target, source, size );
  }
  else
  {
    const char* p_src = source;
    char* p_trg = target;

    while( *p_src && --size )
    {
      if((unsigned char)*p_src > 0x20 ) {
        *p_trg = WinCpTranslateChar( NULLHANDLE, ch_source, *p_src, ch_target );
      } else {
        *p_trg = *p_src;
      }
      ++p_src;
      ++p_trg;
    }

    *p_trg = 0;
  }
  return target;
}

#if 0

int
main( int argc, char* argv[] )
{
  int   i;
  int   ch_source = CH_DEFAULT;
  int   ch_target = CH_DEFAULT;
  char* source;
  char* target;

  if( argc < 3 || argc > 4 )
  {
    fprintf( stderr, "usage: charset <ch_source> [<ch_target>] <source>\n\n" );
    fprintf( stderr, "supported charsets:\n\n" );
    for( i = 0; i < sizeof( ch_list ) / sizeof( CH_ENTRY ); i++ ) {
      fprintf( stderr, "\t%2u - %s\n", ch_list[i].id, ch_list[i].name );
    }
    return 1;
  }

  if( argc == 3 ) {
    ch_source = atoi( argv[1] );
    source = argv[2];
  } else {
    ch_source = atoi( argv[1] );
    ch_target = atoi( argv[2] );
    source = argv[3];
  }

  target = strdup( source );

  if( !ch_convert( ch_source, source, ch_target, target, strlen( source ) + 1 )) {
    fprintf( stderr, "Incorrect call of the utility.\n" );
  } else {
    fprintf( stdout, "%s\n", target );
  }

  free( target );
  return 0;
}

#endif
