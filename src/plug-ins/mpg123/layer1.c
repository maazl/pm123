/*
 * Mpeg Layer-1 audio decoder
 * --------------------------
 * copyright (c) 1995 by Michael Hipp, All rights reserved. See also 'README'
 * near unoptimzed ...
 *
 * may have a few bugs after last optimization ...
 *
 */

#include "common.h"

static void
I_step_one( struct bit_stream* bs, unsigned int balloc[], unsigned int scale_index[2][SBLIMIT], struct frame* fr )
{
  unsigned int* ba = balloc;
  unsigned int* sca = (unsigned int*)scale_index;

  if( fr->channels == 2 ) {
    int i;
    int jsbound = fr->jsbound;
    for( i = 0; i < jsbound; i++ ) {
      *ba++ = getbits( bs, 4 );
      *ba++ = getbits( bs, 4 );
    }
    for( i = jsbound; i < SBLIMIT; i++ ) {
      *ba++ = getbits( bs, 4 );
    }

    ba = balloc;

    for( i = 0; i < jsbound; i++ ) {
      if(( *ba++ )) {
        *sca++ = getbits( bs, 6 );
      }
      if(( *ba++ )) {
        *sca++ = getbits( bs, 6 );
      }
    }
    for( i = jsbound; i < SBLIMIT; i++ ) {
      if(( *ba++ )) {
        *sca++ =  getbits( bs, 6 );
        *sca++ =  getbits( bs, 6 );
      }
    }
  } else {
    int i;
    for( i = 0; i < SBLIMIT; i++ ) {
      *ba++ = getbits( bs, 4 );
    }
    ba = balloc;
    for( i = 0; i < SBLIMIT; i++ ) {
      if(( *ba++ )) {
        *sca++ = getbits( bs, 6 );
      }
    }
  }
}

static void
I_step_two( struct bit_stream* bs, real fraction[2][SBLIMIT], unsigned int balloc[2* SBLIMIT],
            unsigned int scale_index[2][SBLIMIT], struct frame* fr )
{
  int  i, n;
  int  smpb[2 * SBLIMIT]; /* values: 0-65535 */
  int* sample;
  register unsigned int* ba;
  register unsigned int* sca = (unsigned int*)scale_index;

  if( fr->channels == 2 ) {
    int jsbound = fr->jsbound;
    register real* f0 = fraction[0];
    register real* f1 = fraction[1];
    ba = balloc;
    for( sample = smpb, i = 0; i < jsbound; i++ )  {
      if(( n = *ba++ )) {
        *sample++ = getbits( bs, n + 1 );
      }
      if(( n = *ba++ )) {
        *sample++ = getbits( bs, n + 1 );
      }
    }
    for( i = jsbound; i < SBLIMIT; i++ ) {
      if(( n = *ba++ )) {
        *sample++ = getbits( bs, n + 1 );
      }
    }

    ba = balloc;
    for( sample = smpb, i = 0; i < jsbound; i++ ) {
      if(( n = *ba++ )) {
        *f0++ = (real)((( -1 ) << n ) + ( *sample++ ) + 1 ) * muls[n + 1][*sca++];
      } else {
        *f0++ = 0.0;
      }
      if(( n = *ba++ )) {
        *f1++ = (real)((( -1 ) << n ) + ( *sample++ ) + 1 ) * muls[n + 1][*sca++];
      } else {
        *f1++ = 0.0;
      }
    }
    for( i = jsbound; i < SBLIMIT; i++ ) {
      if(( n = *ba++ )) {
        real samp = ((( -1 ) << n ) + ( *sample++ ) + 1 );
        *f0++ = samp * muls[n + 1][*sca++];
        *f1++ = samp * muls[n + 1][*sca++];
      } else {
        *f0++ = *f1++ = 0.0;
      }
    }
    for( i = SBLIMIT; i < 32; i++ ) {
      fraction[0][i] = fraction[1][i] = 0.0;
    }
  } else {
    register real* f0 = fraction[0];
    ba = balloc;
    for( sample = smpb, i = 0; i < SBLIMIT; i++ ) {
      if(( n = *ba++ )) {
        *sample++ = getbits( bs, n + 1 );
      }
    }
    ba = balloc;
    for( sample = smpb, i = 0; i < SBLIMIT; i++ ) {
      if(( n = *ba++ )) {
        *f0++ = (real)((( -1 ) << n ) + ( *sample++ ) + 1 ) * muls[n + 1][*sca++];
      } else {
        *f0++ = 0.0;
      }
    }
    for( i = SBLIMIT; i < 32; i++ ) {
      fraction[0][i] = 0.0;
    }
  }
}

int
do_layer1( MPG_FILE* mpg )
{
  int clip = 0;
  int i;
  unsigned int balloc[2*SBLIMIT];
  unsigned int scale_index[2][SBLIMIT];
  real fraction[2][SBLIMIT];
  int point = 0;

  I_step_one( &mpg->bs, balloc, scale_index, &mpg->fr );

  for( i = 0; i < SCALE_BLOCK; i++ )
  {
    I_step_two( &mpg->bs, fraction, balloc, scale_index, &mpg->fr );

    if( mpg->fr.channels == 1 ) {
      clip += synth_1to1_mono2stereo( mpg, (real*)fraction[0], mpg->pcm_samples + point );
    } else {
      clip += synth_1to1( mpg, (real*)fraction[0], 0, mpg->pcm_samples + point );
      clip += synth_1to1( mpg, (real*)fraction[1], 1, mpg->pcm_samples + point );
    }

    point += 128;
  }

  mpg->pcm_point = 0;
  mpg->pcm_count = point;
  return clip;
}
