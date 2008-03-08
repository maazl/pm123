/*
 * decoder MPEG Layer III
 * handle Xing header
 * mod 12/7/98 add vbr scale
 * Copyright 1998 Xing Technology Corp.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include "dxhead.h"

// 4   Xing
// 4   flags
// 4   frames
// 4   bytes
// 100 toc

static int
ExtractI4( unsigned char *buf )
{
  int x;

  // big endian extract
  x  = buf[0];
  x <<= 8;
  x |= buf[1];
  x <<= 8;
  x |= buf[2];
  x <<= 8;
  x |= buf[3];

  return x;
}

int
GetXingHeader( XHEADDATA* X,  unsigned char* buf )
{
  int i;
  int h_id, h_mode, h_sr_index;
  static int sr_table[4] = { 44100, 48000, 32000, 99999 };

  X->h_id      =  0;
  X->samprate  =  0;
  X->flags     =  0;
  X->frames    =  0;
  X->bytes     =  0;
  X->vbr_scale = -1;

  // get selected MPEG header data
  h_id       = ( buf[1] >> 3 ) & 1;
  h_sr_index = ( buf[2] >> 2 ) & 3;
  h_mode     = ( buf[3] >> 6 ) & 3;
  buf += 4;

  // determine offset of header
  if( h_id ) {    // mpeg1
    if( h_mode != 3 ) {
      buf += 32;
    } else {
      buf += 17;
    }
  } else {        // mpeg2
    if( h_mode != 3 ) {
      buf += 17;
    } else {
      buf += 9;
    }
  }

  if( strcmp( buf, "Xing" ) != 0 &&
      strcmp( buf, "Info" ) != 0 )
  {
    return 0;
  }

  buf += 4;

  X->h_id = h_id;
  X->samprate = sr_table[h_sr_index];
  if( h_id == 0 ) {
    X->samprate >>= 1;
  }

  X->flags = ExtractI4(buf);
  buf += 4;

  if( X->flags & FRAMES_FLAG ) {
    X->frames = ExtractI4( buf );
    buf += 4;
    if( X->frames < 1 ) {
      X->flags = 0;
      return 0;
    }
  }
  if( X->flags & BYTES_FLAG  ) {
    X->bytes  = ExtractI4( buf );
    buf += 4;
  }

  if( X->flags & TOC_FLAG  ) {
    for( i = 0; i < 100; i++ ) {
      if( i > 0 && buf[i] < buf[i-1] ) {
        X->flags &= ~TOC_FLAG;
        break;
      }
    }
    if( buf[99] == 0 ) {
      X->flags &= ~TOC_FLAG;
    }
  }
  if( X->flags & TOC_FLAG  ) {
    if( X->toc != NULL ) {
      for( i = 0; i < 100; i++ ) {
        X->toc[i] = buf[i];
      }
    }
    buf += 100;
  }

  if( X->flags & VBR_SCALE_FLAG ) {
    X->vbr_scale = ExtractI4( buf );
    buf += 4;
  }

  if( !X->bytes ) {
    X->flags &= ~BYTES_FLAG;
  }

/*
  if( X->toc != NULL ) {
    for( i = 0; i < 100; i++ ) {
      if(( i % 10 ) == 0 ) {
        printf( "\n" );
      }
      printf( " %3d", (int)(X->toc[i]));
    }
  }
*/
  return 1;
}

int
SeekPoint( unsigned char TOC[100], int file_bytes, float percent )
{
  // Interpolate in TOC to get file seek point in bytes
  int a, seekpoint;
  float fa, fb, fx;

  if( percent < 0.0 ) {
      percent = 0.0;
  }
  if( percent > 100.0 ) {
      percent = 100.0;
  }

  a = (int)percent;

  // Some compilers can round floating point numbers.
  // This not conforms to the standard but this is faster.
  if( a > percent ) {
    --a;
  }
  if( a > 99 ) {
      a = 99;
  }
  fa = TOC[a];
  if( a < 99 ) {
    fb = TOC[ a + 1 ];
  } else {
    fb = 256;
  }

  fx = fa + ( fb - fa ) * ( percent - a );
  seekpoint = (int)(( 1.0 / 256.0 ) * fx * file_bytes );
  return seekpoint;
}

