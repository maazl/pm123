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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define  INCL_DOS
#define  INCL_WIN
#include <os2.h>
#include <uconv.h>
#include <unidef.h>
#include "utilfct.h"

#include "debuglog.h"

#include "charset.h"

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
  { "Thai (DOS, OS/2)",                             874        }
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

  for( i = 0; i < sizeof( ch_list ) / sizeof( CH_ENTRY ); i++ ) {
    if( ch_list[i].codepage == codepage ) {
      return ch_list + i;
    }
  }

  return NULL;
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
      { UniChar cpname[32];

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
ch_convert( ULONG cp_source, const char* source, ULONG cp_target, char* target, size_t size )
{
  if( cp_source == CH_CP_NONE ) {
    cp_source = ch_default_cp();
  }
  if( cp_target == CH_CP_NONE ) {
    cp_target = ch_default_cp();
  }
  
  if ( cp_source == cp_target ) {
    // no conversion required
    strlcpy( target, source, size );
  } else
  if ( !WinCpTranslateString( NULLHANDLE, cp_source, (PSZ)source, cp_target, size, target ) ) {
    // conversion error
    return NULL;
  }

  return target;
}

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

