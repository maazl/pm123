/*
 * Mpeg Layer-1,2,3 audio decoder
 * ------------------------------
 * copyright (c) 1995,1996,1997 by Michael Hipp, All rights reserved.
 * See also 'README'
 *
 * slighlty optimized for machines without autoincrement/decrement.
 * The performance is highly compiler dependend. Maybe
 * the decode.c version for 'normal' processor may be faster
 * even for Intel processors.
 */

#include <stdlib.h>
#include <math.h>

#include "common.h"

int
synth_1to1_mono2stereo( MPG_FILE* mpg, real* bandPtr, unsigned char* samples )
{
  int i, ret = synth_1to1( mpg, bandPtr, 0, samples );

  for( i = 0; i < 32; i++ ) {
    ((short*)samples )[1] = ((short*)samples )[0];
    samples += 4;
  }
  return ret;
}

int
synth_1to1( MPG_FILE* mpg, real* bandPtr, int channel, unsigned char* out )
{
  #define STEP 2

  if( mpg->use_common_eq ) {
    do_common_eq( mpg, bandPtr, channel );
  }

  if( mpg->use_mmx ) {
    return synth_1to1_MMX( &mpg->synth_data, bandPtr, channel, out );
  } else {
    real*  b0;
    real  (*buf)[0x110];
    short* samples = (short*)out;
    int    clip = 0;
    int    b1;
    int    bo = mpg->synth_data.bo;

    if( !channel ) {
      bo--;
      bo &= 0xf;
      buf = mpg->synth_data.buffs[0];
    } else {
      samples++;
      buf = mpg->synth_data.buffs[1];
    }

    if( bo & 0x1 ) {
      b0 = buf[0];
      b1 = bo;
      dct64( buf[1] + (( bo + 1 ) & 0xf ), buf[0] + bo, bandPtr );
    } else {
      b0 = buf[1];
      b1 = bo + 1;
      dct64( buf[0] + bo, buf[1] + bo + 1, bandPtr );
    }

    {
      register int j;
      real* window = decwin + 16 - b1;

      for( j = 16; j; j--, b0 += 0x10, window += 0x20, samples += STEP )
      {
        real sum;
        sum  = window[0x0] * b0[0x0];
        sum -= window[0x1] * b0[0x1];
        sum += window[0x2] * b0[0x2];
        sum -= window[0x3] * b0[0x3];
        sum += window[0x4] * b0[0x4];
        sum -= window[0x5] * b0[0x5];
        sum += window[0x6] * b0[0x6];
        sum -= window[0x7] * b0[0x7];
        sum += window[0x8] * b0[0x8];
        sum -= window[0x9] * b0[0x9];
        sum += window[0xA] * b0[0xA];
        sum -= window[0xB] * b0[0xB];
        sum += window[0xC] * b0[0xC];
        sum -= window[0xD] * b0[0xD];
        sum += window[0xE] * b0[0xE];
        sum -= window[0xF] * b0[0xF];

        WRITE_SAMPLE( samples, sum, clip );
      }

      {
        real sum;
        sum  = window[0x0] * b0[0x0];
        sum += window[0x2] * b0[0x2];
        sum += window[0x4] * b0[0x4];
        sum += window[0x6] * b0[0x6];
        sum += window[0x8] * b0[0x8];
        sum += window[0xA] * b0[0xA];
        sum += window[0xC] * b0[0xC];
        sum += window[0xE] * b0[0xE];

        WRITE_SAMPLE( samples, sum, clip );
        b0 -= 0x10, window -= 0x20, samples += STEP;
      }
      window += b1 << 1;

      for( j = 15; j; j--, b0 -= 0x10, window -= 0x20, samples += STEP )
      {
        real sum;
        sum = -window[-0x1] * b0[0x0];
        sum -= window[-0x2] * b0[0x1];
        sum -= window[-0x3] * b0[0x2];
        sum -= window[-0x4] * b0[0x3];
        sum -= window[-0x5] * b0[0x4];
        sum -= window[-0x6] * b0[0x5];
        sum -= window[-0x7] * b0[0x6];
        sum -= window[-0x8] * b0[0x7];
        sum -= window[-0x9] * b0[0x8];
        sum -= window[-0xA] * b0[0x9];
        sum -= window[-0xB] * b0[0xA];
        sum -= window[-0xC] * b0[0xB];
        sum -= window[-0xD] * b0[0xC];
        sum -= window[-0xE] * b0[0xD];
        sum -= window[-0xF] * b0[0xE];
        sum -= window[-0x0] * b0[0xF];

        WRITE_SAMPLE( samples, sum, clip );
      }
    }
    mpg->synth_data.bo = bo;
    return clip;
  }
}
