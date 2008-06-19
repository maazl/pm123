/*
 * Copyright 2005-2007 Dmitry A.Steklenev
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
#define  INCL_WIN
#include <os2.h>
#include <uconv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "charset.h"
#include "strutils.h"
#include "debuglog.h"

static int ch_rus_detect( const char *source );

const CH_ENTRY ch_list[] =
{
  { "Default",                 CH_DEFAULT,    CH_SBCS, 0             },
  { "Cyrillic (KOI-8R)",       CH_CYR_KOI8R,  CH_SBCS, 0             },
  { "Cyrillic (CP-866)",       CH_CYR_DOS,    CH_SBCS, 0             },
  { "Cyrillic (Windows-1251)", CH_CYR_WIN,    CH_SBCS, 0             },
  { "Western (ISO-8859-1)",    CH_ISO_8859_1, CH_SBCS, 0             },
  { "Unicode (UTF-8)",         CH_UTF_8,      CH_MBCS, 0             },
  { "Unicode (UCS-2 BE)",      CH_UCS_2BE,    CH_DBCS, 0             },
  { "Unicode (UCS-2 LE)",      CH_UCS_2LE,    CH_DBCS, 0             },
  { "Unicode (UCS-2 BOM)",     CH_UCS_2BOM,   CH_DBCS, 0             },
  { "Russian (Auto-Detect)",   CH_CYR_AUTO,   CH_SBCS, ch_rus_detect }
};

const int ch_list_size = sizeof( ch_list ) / sizeof( CH_ENTRY );

/*
 * ch_default: returns the current system character set.
 */

int
ch_default( void )
{
  static ULONG cpage[1] = { 0 };

  ULONG incb = sizeof( cpage );
  ULONG oucb = 0;

  if( !cpage[0] ) {
    DosQueryCp( incb, cpage, &oucb );
  }
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
 * ch_type: returns the type of the specified character set.
 */

static int
ch_type( int id )
{
  const CH_ENTRY* entry = ch_find( id );

  if( entry ) {
    return entry->type;
  } else {
    return CH_SBCS;
  }
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
 * ch_convert_sbcs: convert a characters string from one single-byte
 *                  character set to another.
 *
 *    ch_source source character set
 *    source    source string
 *    ch_target target character set
 *    target    result buffer
 *    size      size of result buffer
 *
 *    return    != NULL: converted string
 *              == NULL: error
 */

static char*
ch_convert_sbcs( int ch_source, const char* source, int ch_target, char* target, size_t size )
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
  return target;
}

/*
 * ch_convert_mbcs: convert a characters string from one multi-byte
 *                  character set to another.
 *
 *    ch_source source character set
 *    source    source string
 *    ch_target target character set
 *    target    result buffer
 *    size      size of result buffer
 *
 *    return    != NULL: converted string
 *              == NULL: error
 */

static char*
ch_convert_mbcs( int ch_source, int ch_source_type, const char* source,
                 int ch_target, int ch_target_type, char* target, size_t size )
{
  UniChar     uch_name[16];
  UconvObject uco;
  size_t      src_chars;
  size_t      src_bytes;
  UniChar*    buffer;
  int         i, j;
  int         rc;

  void*       sbs_buffer;
  size_t      sbs_bytesleft;
  UniChar*    ucs_buffer;
  size_t      ucs_charsleft;
  size_t      nonidentical;

  // Allocate a conversion buffer.
  if( ch_source_type & CH_DBCS ) {
    src_chars = UniStrlen((UniChar*)source );
    src_bytes = src_chars * 2;
  } else {
    src_chars = strlen( source );
    src_bytes = src_chars;
  }

  if(( buffer = (UniChar*)calloc( 1, sizeof( UniChar ) * ( src_chars + 1 ))) == NULL ) {
    return NULL;
  }

  // Convert the source string to unicode.
  if( ch_source == CH_UCS_2LE ) {
    UniStrncpy( buffer, (UniChar*)source, src_chars );
  } else if( ch_source == CH_UCS_2BE ) {
    for( i = 0, j = 0; i <= src_chars; i++, j += 2 ) {
      buffer[i] = source[j] << 8 | source[j+1];
    }
  } else if( ch_source == CH_UCS_2BOM ) {
    if((unsigned char)source[0] == 0xFF &&
       (unsigned char)source[1] == 0xFE )
    {
      UniStrncpy( buffer, (UniChar*)source + 1, src_chars );
    } else {
      for( i = 0, j = 2; i < src_chars; i++, j += 2 ) {
        buffer[i] = source[j] << 8 | source[j+1];
      }
    }
  } else {
    if(( rc = UniMapCpToUcsCp( ch_source, uch_name, 16 )) != ULS_SUCCESS ) {
      DEBUGLOG(( "charset: UniMapCpToUcsCp failed, rc=%08X\n", rc ));
      free( buffer );
      return NULL;
    }
    if(( rc = UniCreateUconvObject( uch_name, &uco )) != ULS_SUCCESS ) {
      DEBUGLOG(( "charset: UniCreateUconvObject failed, rc=%08X\n", rc ));
      free( buffer );
      return NULL;
    }

    sbs_buffer    = (void*)source;
    sbs_bytesleft = src_bytes;
    ucs_buffer    = buffer;
    ucs_charsleft = src_chars;
    nonidentical  = 0;

    rc = UniUconvToUcs( uco, &sbs_buffer, &sbs_bytesleft,
                             &ucs_buffer, &ucs_charsleft, &nonidentical );
    UniFreeUconvObject( uco );

    if( rc != ULS_SUCCESS ) {
      free( buffer );
      return NULL;
    }
  }

  // Convert unicode to the target string.
  if( ch_target == CH_UCS_2LE ) {
    if( size >= 2 ) {
      UniStrncpy((UniChar*)target, buffer, size / 2 );
      ((UniChar*)target)[ size / 2 - 1 ] = 0;
    }
  } else if( ch_target == CH_UCS_2BE  ) {
    if( size >= 2 ) {
      for( i = 0, j = 0; i < size / 2 && i <= src_chars; i++, j += 2 ) {
        target[j  ] = ( buffer[i] >> 8 ) & 0xFF;
        target[j+1] = ( buffer[i]      ) & 0xFF;
      }
     ((UniChar*)target)[ size / 2 - 1 ] = 0;
    }
  } else if( ch_target == CH_UCS_2BOM ) {
    if( size >= 2 ) {
      UniStrncpy((UniChar*)target + 1, buffer, size / 2 - 1 );
      target[0] = 0xFF;
      target[1] = 0xFE;
      ((UniChar*)target)[ size / 2 - 1 ] = 0;
    }
  } else {
    if( UniMapCpToUcsCp( ch_target, uch_name, 16 ) != ULS_SUCCESS ||
        UniCreateUconvObject( uch_name, &uco ) != ULS_SUCCESS )
    {
      free( buffer );
      return NULL;
    }

    sbs_buffer    = (void*)target;
    sbs_bytesleft = size - 1;
    ucs_buffer    = buffer;
    ucs_charsleft = src_chars;
    nonidentical  = 0;

    rc = UniUconvFromUcs( uco, &ucs_buffer, &ucs_charsleft,
                               &sbs_buffer, &sbs_bytesleft, &nonidentical );
    UniFreeUconvObject( uco );

    if( rc != ULS_SUCCESS && rc != ULS_BUFFERFULL ) {
      free( buffer );
      return NULL;
    }

    target[ size - sbs_bytesleft - 1 ] = 0;
  }

  free( buffer );
  return target;
}

/*
 * ch_convert: convert a characters string from one character
 *             set to another.
 *
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
  int ch_source_type;
  int ch_target_type;

  ch_source = ch_detect( ch_source, source );

  if( ch_source < 0 || ch_target < 0 ) {
    return NULL;
  }

  if( size > 0 ) {
    if( ch_source == CH_DEFAULT ) {
      ch_source = ch_default();
    }
    if( ch_target == CH_DEFAULT ) {
      ch_target = ch_default();
    }

    ch_source_type = ch_type( ch_source );
    ch_target_type = ch_type( ch_target );

    if( ch_source == ch_target ) {
      if( source != target ) {
        if( ch_source_type & CH_DBCS ) {
          if( size >= 2 ) {
            UniStrncpy((UniChar*)target, (UniChar*)source, size / 2 );
            ((UniChar*)target)[ size / 2 - 1 ] = 0;
          }
        } else {
          strlcpy( target, source, size );
        }
      }
    } else if( ch_source_type & CH_MBCS || ch_target_type & CH_MBCS ) {
      return ch_convert_mbcs( ch_source, ch_source_type, source,
                              ch_target, ch_target_type, target, size );
    } else if( ch_source_type & CH_DBCS || ch_target_type & CH_DBCS ) {
      return ch_convert_mbcs( ch_source, ch_source_type, source,
                              ch_target, ch_target_type, target, size );
    } else {
      return ch_convert_sbcs( ch_source, source, ch_target, target, size );
    }
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
      fprintf( stderr, "\t%4d - %s\n", ch_list[i].id, ch_list[i].name );
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

  if(( target = malloc( strlen( source ) * 5 + 1 )) == NULL ) {
    fprintf( stderr, "Not enough memory.\n" );
    return -1;
  }
  if( !ch_convert( ch_source, source, ch_target, target, strlen( source ) * 5 + 1 )) {
    fprintf( stderr, "Incorrect call of the utility.\n" );
    free( target );
    return -2;
  } else {
    fprintf( stdout, "%s\n", target );
  }

  free( target );
  return 0;
}

#endif
