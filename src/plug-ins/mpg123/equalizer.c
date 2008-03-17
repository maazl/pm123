#include <math.h>
#include "common.h"

void
do_common_eq( MPG_FILE* mpg, real* bandPtr, int channel )
{
  int i;

  if( mpg->use_common_eq ) {
    for( i = 0; i < 32; i++ ) {
      bandPtr[i] *= mpg->common_eq[channel][i];
    }
  }
}

void
do_layer3_eq( MPG_FILE* mpg, real* bandPtr, int channel )
{
  int i;

  if( mpg->use_layer3_eq ) {
    mpg->use_common_eq = 0;    /* prevents the synth one from running */
    for( i = 0; i < 576; i++ ) {
      bandPtr[i] *= mpg->layer3_eq[channel][i];
    }
  }
}

/* tries to approximate the ISO R10 normal frequencies with a linear eq */

#define NUM_BANDS  10
#define START_R10  1.65
#define JUMP_R10   0.3

static void
octave_to_linear_eq( int rate, real* octave, real* linear, int bands )
{
  int   range = rate / 2;
  float res = (float)range / bands;
  int   i, e;
  int   table[NUM_BANDS];
  int   point1, point2;
  float slope;
  int   avg = 0;

  for( i = 0, e = 0; i < NUM_BANDS; i++ )
  {
    while( e < bands && ( e + 1 ) * res < pow( 10, ( i * JUMP_R10 ) + START_R10 )) { e++; };
    table[i] = e;
    if( !table[i] ) {
      table[i] = 1;
    }
  }
  table[i - 1] = bands;

  /* does linear interpolation */
  point1 = table[0] / 2;
  for( e = 0; e < point1 + 1; e++ ) {
    linear[e] = octave[0];
  }
  for( i = 1; i < NUM_BANDS; i++ )
  {
    if( table[i] == table[i - 1] ) {
      point2 = table[i] - 1;
    } else {
      point2 = ( table[i] + table[i - 1] ) / 2;
    }
    if( point1 == point2 ) {
      linear[point1] += octave[i];
      avg++;
    } else {
      if( avg ) {
        linear[point1] /= avg + 1;
        avg = 0;
      }
      slope = ( octave[i] - octave[i - 1] ) / ( point2 - point1 );
      for( e = point1 + 1; e < point2 + 1; e++ ) {
        linear[e] = slope * ( e - point2 ) + octave[i];
      }
    }
    point1 = point2;
  }
  for( e = point1; e < bands; e++ ) {
    linear[e] = octave[i - 1];
  }
}

void
eq_init( MPG_FILE* mpg )
{
  octave_to_linear_eq( mpg->samplerate, mpg->octave_eq[0], mpg->layer3_eq[0], 576 );
  octave_to_linear_eq( mpg->samplerate, mpg->octave_eq[1], mpg->layer3_eq[1], 576 );
  octave_to_linear_eq( mpg->samplerate, mpg->octave_eq[0], mpg->common_eq[0], 32  );
  octave_to_linear_eq( mpg->samplerate, mpg->octave_eq[1], mpg->common_eq[1], 32  );

  mpg->use_common_eq = mpg->use_layer3_eq;
}
