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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "charset.h"
#include "strutils.h"
#include "debuglog.h"
#include <uconv.h>
#include <unidef.h>
#include "utilfct.h"
#include <os2.h>

#include "debuglog.h"

const CH_ENTRY ch_list[] =
{
  { "System default",                               CH_CP_NONE },
  { "United States",                                437        },
  { "Latin-1 Western Europe (DOS, OS/2)",           850        },
  { "Latin-1 Western Europe (ISO-8859-1, Windows)", 1004       },
  { "Latin-1 Western Europe (Apple)",               1275       },
  { "Latin-2 Eastern Europe (DOS, OS/2)",           852        },
  { "Latin-2 Eastern Europe (ISO-8859-2)",          912        },
  { "Latin-2 Eastern Europe (Windows)",             1250       },
  { "Latin-9 Southeastern Europe (DOS, OS/2)",      859        },
  { "Latin-9 Southeastern Europe (ISO-8859-15)",    923        },
  { "Turkish (DOS, OS/2)",                          857        },
  { "Turkish (Windows)",                            1254       },
  { "Greek (DOS, OS/2)",                            869        },
  { "Greek (ISO-8859-7, Windows)",                  1253       },
  { "Baltic (DOS, OS/2)",                           921        },
  { "Baltic (Windows)",                             1257       },
  { "Estonia (DOS, OS/2)",                          922        },
  { "Cyrillic (KOI-8R)",                            878        },
  { "Cyrillic (DOS, OS/2)",                         855        },
  { "Cyrillic - Russian (DOS, OS/2)",               866        },
  { "Cyrillic - Ukraine (KOI-8U)",                  1125       },
  { "Cyrillic - Belarus (DOS, OS/2)",               1131       },
  { "Cyrillic (ISO-8859-5)",                        915        },
  { "Cyrillic (Windows)",                           1251       },
  { "Arabic (DOS, OS/2)",                           864        },
  { "Arabic (Windows)",                             1256       },
  { "Hebrew (DOS, OS/2)",                           862        },
  { "Hebrew (Windows)",                             1255       },
  { "Portuguese (DOS, OS/2)",                       860        },
  { "Canadian French (DOS, OS/2)",                  863        },
  { "Nordic (DOS, OS/2)",                           865        },
  { "Icelandic (DOS, OS/2)",                        861        },
  { "Thai (DOS, OS/2)",                             874        },
  { "UTF-8 (Unicode)",                              1208       },
  { "UCS-2 (UTF-16, Unicode)",                      1200       }
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
 *          specified codepage.
 *
 *    codepage codepage to find
 *
 *    return   pointer to matching CH_ENTRY or NULL if not found
 */
const CH_ENTRY*
ch_find( ULONG codepage )
{
  int i;

  for( i = 0; i < ch_list_size; i++ ) {
    if( ch_list[i].codepage == codepage ) {
      return ch_list + i;
    }
  }

  return NULL;
}

BOOL ch_info( ULONG codepage, uconv_attribute_t* attr )
{
  UniChar           uch_name[16];
  UconvObject       uco;
  int               rc;

  if( codepage == CH_CP_NONE ) {
    codepage = ch_default();
  }
  if(( rc = UniMapCpToUcsCp( codepage, uch_name, 16 )) != ULS_SUCCESS ) {
    DEBUGLOG(( "charset: UniMapCpToUcsCp failed, rc=%08X\n", rc ));
    return FALSE;
  }
  if(( rc = UniCreateUconvObject( uch_name, &uco )) != ULS_SUCCESS ) {
    DEBUGLOG(( "charset: UniCreateUconvObject failed, rc=%08X\n", rc ));
    return FALSE;
  }
  rc = UniQueryUconvObject( uco, attr, sizeof *attr, NULL, NULL, NULL );
  UniFreeUconvObject( uco );
  if(rc != ULS_SUCCESS) {
    DEBUGLOG(( "charset: UniQueryUconvObject failed, rc=%08X\n", rc ));
    return FALSE;
  }
  return TRUE;
}

/*
 * ch_get_weight: get character weight from unicode character type
 *
 *    uc      unicode character
 *
 *    return  weight
 */
INLINE int
ch_get_weight(UniChar uc)
{
  // weights
  const int weights[] =
  { 1,  /* Upper case alphabetic character */
    2,  /* Lower case alphabetic character */
    2,  /* Digits 0-9                      */
    0,  /* White space and line ends       */
    0,  /* Punctuation marks               */
    -5, /* Control and format characters   */
    0,  /* Space and tab                   */
    0,  /* Hex digits                      */
    1,  /* Letters and linguistic marks    */
    0,  /* Alphanumeric                    */
    0,  /* All except controls and space   */
    0,  /* Everything except controls      */
    0,  /* Integral number                 */
    -1  /* Symbol                          */
  //0,  /* reserved                        */
  //0   /* In standard ASCII set           */
  };

  int itype = UniQueryCharType(uc)->itype;
  const int* wp = weights;
  int result = 0;
  
  while ( wp != weights + sizeof weights / sizeof *weights ) {
    result += (itype & 1) * *wp++;
    itype >>= 1;
  }
  
  DEBUGLOG2(("ch_get_weight: %04x -> %04x = %i\n", uc, UniQueryCharType(uc)->itype, result ));
  return result; 
}

/*
 * ch_detect: determine a characters string character set.
 *            This will check the system's prepared codepages and the one
 *            specified as parameter.
 *
 *    cp_source probable source character set or CH_CP_NONE if unknown
 *    source    source string
 *
 *    return    identified codepage or CH_CP_NONE if unknown.
 */

ULONG
ch_detect( ULONG cp_source, const char* source )
{
  ULONG cpage[6];
  ULONG n, n_distinct;
  int   i;
  // for the winner ...
  int   max_cp    = CH_CP_NONE;
  int   max_value = 0;
  int   max_count = 0;

  // get prepared codepages
  if ( cp_source != CH_CP_NONE ) {
    cpage[0] = cp_source;
    DosQueryCp( sizeof cpage - sizeof *cpage, cpage +1, &n );
    n += sizeof *cpage;
  } else {
    DosQueryCp( sizeof cpage, cpage, &n );
  }
  n /= sizeof *cpage;
  
  // try different codepages
  n_distinct = 0;
  for ( i = 0; i < n; ++i )
  { 
    UniChar ustring[1000];
    int     j;

    // skip duplicates
    for ( j = 0; j < i; ++j ) {
      if ( cpage[i] == cpage[j] ) {
        goto continue_outer;
      }
    }
    ++n_distinct;

    // convert to unicode
    { UconvObject ucv;
    
      // load codepage
      { UniChar cpname[16];

        if ( UniMapCpToUcsCp( cpage[i], cpname, sizeof cpname / sizeof *cpname ) != ULS_SUCCESS ||
             UniCreateUconvObject( cpname, &ucv ) != ULS_SUCCESS ) { 
          continue; // can't help
        }
      }

      // set options
      { uconv_attribute_t uattr;

        UniQueryUconvObject( ucv, &uattr, sizeof uattr, NULL, NULL, NULL );
        uattr.options = UCONV_OPTION_SUBSTITUTE_BOTH;
        uattr.displaymask = DSPMASK_DISPLAY;
        UniSetUconvObject( ucv, &uattr );
      }
      
      // convert string to unicode   
      j = UniStrToUcs( ucv, ustring, (char*)source, sizeof ustring );
      // free resources anyway
      UniFreeUconvObject( ucv );
        
      if ( j != ULS_SUCCESS && j != ULS_BUFFERFULL ) {
        continue; // can't help
      }       
    }

    // calculate statistics
    { UniChar* ucp   = ustring;
      int      value = 0;
     
      for ( j = sizeof ustring / sizeof *ustring; j && *ucp; --j, ++ucp) {
        value += ch_get_weight( *ucp );
      }
      DEBUGLOG(("ch_detect: result for CP %u = %i\n", cpage[i], value));
    
      // update winner
      if ( value > max_value ) {
        max_cp    = cpage[i];
        max_value = value;
        max_count = 1;
      } else
      if ( value == max_value ) {
        ++max_count;
      }
    }

    continue_outer:;
  }

  // return result
  if ( max_count != 1 && max_count == n_distinct ) {
    // indetrmined
    return CH_CP_NONE;
  }
  return max_cp;
}

/*
 * ch_convert: convert a character string from one character set to another.
 *
 *    cp_source source codepage or CH_CP_NONE (0) to use the application's default
 *    source    source string
 *    cp_target target codepage or CH_CP_NONE (0) to use the application's default
 *    target    result buffer
 *    size      size of result buffer
 *    flags     see CH_FLAGS_...
 *
 *    return    != NULL: converted string
 *              == NULL: error
 */

char*
ch_convert( ULONG cp_source, const char* source, ULONG cp_target, char* target, size_t size, unsigned flags )
{
  UniChar           uch_name[16];
  UconvObject       uco;
  uconv_attribute_t uco_attr;
  UniChar*          buffer = NULL;
  const UniChar*    ucs_source;
  size_t            src_chars;
  char*             target_bak = target;
  int               rc;

  // default parameters
  if( cp_source == CH_CP_NONE ) {
    cp_source = ch_default();
  }
  if( cp_target == CH_CP_NONE ) {
    cp_target = ch_default();
  }
  // source uconv
  if(( rc = UniMapCpToUcsCp( cp_source, uch_name, 16 )) != ULS_SUCCESS ) {
    DEBUGLOG(( "charset: UniMapCpToUcsCp failed, rc=%08X\n", rc ));
    return NULL;
  }
  if(( rc = UniCreateUconvObject( uch_name, &uco )) != ULS_SUCCESS ) {
    DEBUGLOG(( "charset: UniCreateUconvObject failed, rc=%08X\n", rc ));
    return NULL;
  }
  if(( rc = UniQueryUconvObject( uco, &uco_attr, sizeof uco_attr, NULL, NULL, NULL )) != ULS_SUCCESS) {
    DEBUGLOG(( "charset: UniQueryUconvObject failed, rc=%08X\n", rc ));
    UniFreeUconvObject( uco );
    return NULL;
  }
  
  if (( flags & CH_FLAGS_WRITE_BOM ) && size >= 4 )
  { // Write byte order marker (but only if enough space for terminating zero)
    switch ( cp_target ) {
      case 1200:
        *(UniChar*)target = 0xFEFF;
        target += 2;
        size   -= 2;
        break;
      case 1208:
        target[0] = 0xEF; target[1] = 0xBB; target[2] = 0xBF;
        target += 3;
        size   -= 3;
    }
  } else if ( size == 0 ) { // avoid exception at size - 1 terms below 
    UniFreeUconvObject( uco );
    return target_bak;
  }

  // Convert source to UCS-2
  if ( cp_source == 1200 && *(UniChar*)source != 0xFFFE ) // Is already UCS-2
  { // convert target string directly from source
    UniFreeUconvObject( uco );

    // skip byte order mark
    if( *(const UniChar*)source == 0xFEFF )
      source += 2;
    
    if ( cp_source == cp_target ) {
     copyDBCS:
      // no conversion required
      if ( size >= 2 ) {
        UniStrncpy( (UniChar*)target, (const UniChar*)source, size / 2 );
        ((UniChar*)target)[ size / 2 - 1 ] = 0; // always null-terminate
      }
      return target_bak;
    }

    ucs_source = (const UniChar*)source;
    src_chars  = UniStrlen((UniChar*)source );

  } else {
    // not UCS-2 or little endian
    void*       sbs_buffer;
    size_t      sbs_bytesleft;
    UniChar*    ucs_buffer;
    size_t      ucs_charsleft;
    size_t      nonidentical;
  
    if (( uco_attr.esid & 0xF00 ) == 0x200 ) { // is DBCS
      // check for byte order mark
      if ( cp_source == 1200 ) {
        switch (*(UniChar*)source) {
          case 0xFFFE:
            // Modify uconv attributes to swap bytes
            uco_attr.endian.source = ENDIAN_LITTLE;
            if(( rc = UniSetUconvObject( uco, &uco_attr )) != ULS_SUCCESS) {
              DEBUGLOG(( "charset: UniSetUconvObject failed, rc=%08X\n", rc ));
              UniFreeUconvObject( uco );
              return NULL;
            }
          case 0xFEFF: // Native byte order of this OS => ignore
            source += 2;
        }
      }
      
      if ( cp_source == cp_target ) {
        goto copyDBCS; // same as above
      }

      src_chars     = UniStrlen((UniChar*)source );
      sbs_bytesleft = src_chars * 2;

    } else {
      // skip BOM?
      // BOM is meaningless in UTF-8, however, this prevents not everyone from using it.
      if( cp_source == 1208 && source[0] == 0xEF && source[1] == 0xBB && source[2] == 0xBF )
      { source += 3;
      }

      if ( cp_source == cp_target ) {
        // no conversion required
        strlcpy( target, source, size );
        UniFreeUconvObject( uco );
        return target_bak;
      }

      src_chars     = strlen( source );
      sbs_bytesleft = src_chars;
    }

    if ( cp_target == 1200 ) {
      // target is UCS-2 => operate in-place
      ucs_buffer = (UniChar*)target;
      ucs_charsleft = size / 2 - 1;
    } else {
      // allocate temporary buffer
      if(( buffer = (UniChar*)calloc( sizeof( UniChar ), src_chars + 1 )) == NULL ) {
        UniFreeUconvObject( uco );
        return NULL;
      }
      ucs_buffer = buffer;
      ucs_charsleft = src_chars;
    }
    // Convert the source string to unicode.
    sbs_buffer    = (void*)source;
    nonidentical  = 0;

    rc = UniUconvToUcs( uco, &sbs_buffer, &sbs_bytesleft,
                             &ucs_buffer, &ucs_charsleft, &nonidentical );
    UniFreeUconvObject( uco );

    if( rc != ULS_SUCCESS && rc != ULS_BUFFERFULL ) {
      free( buffer );
      return NULL;
    }
    
    if ( buffer == NULL )
    { // in-place => so we are done
      ((UniChar*)target)[ size / 2 - ucs_charsleft - 1 ] = 0;
      return target_bak;
    }
    
    // convert target from the temporary location
    ucs_source = buffer;
  }
  // now ucs_buffer points to a big-endian UCS-2 string to convert into the target format

  // target uconv
  if(( rc = UniMapCpToUcsCp( cp_target, uch_name, 16 )) != ULS_SUCCESS ) {
    DEBUGLOG(( "charset: UniMapCpToUcsCp failed (target), rc=%08X\n", rc ));
    free( buffer );
    return NULL;
  }
  if(( rc = UniCreateUconvObject( uch_name, &uco )) != ULS_SUCCESS ) {
    DEBUGLOG(( "charset: UniCreateUconvObject failed (target), rc=%08X\n", rc ));
    free( buffer );
    return NULL;
  }
  if(( rc = UniQueryUconvObject( uco, &uco_attr, sizeof uco_attr, NULL, NULL, NULL )) != ULS_SUCCESS) {
    DEBUGLOG(( "charset: UniQueryUconvObject failed (target), rc=%08X\n", rc ));
    UniFreeUconvObject( uco );
    free( buffer );
    return NULL;
  }
  // Convert unicode to the target string.
  { void*       sbs_buffer    = (void*)target;
    size_t      sbs_bytesleft = size - 1;
    UniChar*    ucs_buffer    = (UniChar*)ucs_source; // OS/2 and const...
    size_t      ucs_charsleft = src_chars;
    size_t      nonidentical  = 0;

    rc = UniUconvFromUcs( uco, &ucs_buffer, &ucs_charsleft,
                               &sbs_buffer, &sbs_bytesleft, &nonidentical );
    UniFreeUconvObject( uco );
    free( buffer );

    if( rc != ULS_SUCCESS && rc != ULS_BUFFERFULL ) {
      return NULL;
    }

    target[ size - sbs_bytesleft - 1 ] = 0;
  }
  return target_bak;
}

#if 0

#include <stdio.h>
#include <stdlib.h>

int
main( int argc, char* argv[] )
{ static char buffer1[65536] = {0};
  static char buffer2[65536];
  size_t n;
  freopen("","rb",stdin);
  freopen("","wb",stdout);
  n = fread(buffer1, sizeof buffer1, 1, stdin);
  fprintf(stderr, "%i\n", ch_convert(atoi(argv[1]), buffer1, atoi(argv[2]), buffer2, sizeof buffer2, !!argv[3]));
  fwrite(buffer2, sizeof buffer2, 1, stdout);
  return 0;
}

#endif

#if 0

#include <ctype.h>

int
main( int argc, char* argv[] )
{
  int   i;
  int   ch_source = CH_CP_NONE;
  int   ch_target = CH_CP_NONE;
  char* source;
  char* target;
  BOOL  autodetect = FALSE;

  if ( argc >= 2 && toupper(argv[1][0]) == 'A' )
  { autodetect = TRUE;
    ++argv;
    --argc;
  }
  if( argc < 3 || argc > 4 )
  {
    fprintf( stderr, "usage: charset [A[UTO]] <ch_source> [<ch_target>] <source>\n\n" );
    fprintf( stderr, "supported charsets:\n\n" );
    for( i = 0; i < sizeof( ch_list ) / sizeof( CH_ENTRY ); i++ ) {
      fprintf( stderr, "\t%4u - %s\n", ch_list[i].codepage, ch_list[i].name );
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
  
  if ( autodetect )
  {
    ch_source = ch_detect( ch_source, source );
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

