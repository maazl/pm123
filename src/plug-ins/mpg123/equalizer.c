#include <math.h>

#include "mpg123.h"

real octave_eq[2][10];
static real equalizer[2][32];
static real mp3_eq[2][576];

void do_equalizer(real *bandPtr,int channel)
{
   int i;
   if(flags.equalizer) {
      for(i=0;i<32;i++)
         bandPtr[i] *= equalizer[channel][i];
   }
}

void do_mp3eq(real *bandPtr,int channel)
{
   int i;
   if(flags.mp3_eq) {
      flags.equalizer = 0;  /* prevents the synth one from running */
      for(i=0;i<576;i++)
         bandPtr[i] *= mp3_eq[channel][i];
   }
}

/* tries to approximate the ISO R10 normal frequencies with a linear eq */
#define NUM_BANDS  10
#define START_R10  1.65
#define JUMP_R10   0.3
void octave_to_linear_eq(int rate, real *octave, real *linear, int bands)
{
   int range = rate/2;
   float res = (float)range/bands;
   int i,e;
   int table[NUM_BANDS];
   int point1,point2;
   float slope;
   int avg = 0;

   for(i = 0, e = 0; i < NUM_BANDS; i++)
   {
      while(e < bands && (e+1)*res < pow(10,(i*JUMP_R10)+START_R10)) { e++; };
      table[i] = e;
      if(!table[i]) table[i] = 1;
   }
   table[i-1] = bands;

   /* does linear interpolation */
   point1 = table[0]/2;
   for(e=0;e<point1+1;e++)
      linear[e] = octave[0];
   for(i = 1; i < NUM_BANDS; i++)
   {
      if(table[i]==table[i-1])
         point2 = table[i]-1;
      else
         point2 = (table[i]+table[i-1])/2;
      if(point1==point2)
      {
         linear[point1] += octave[i];
         avg++;
      }
      else
      {
         if(avg)
         {
            linear[point1] /= avg+1;
            avg = 0;
         }
         slope = (octave[i]-octave[i-1])/(point2-point1);
         for(e=point1+1;e<point2+1;e++)
            linear[e] = slope*(e-point2) + octave[i];
      }
      point1 = point2;
   }
   for(e=point1;e<bands;e++)
      linear[e] = octave[i-1];
}

void init_eq(int rate)
{
//   float gain[10]={2.0,1.8,1.6,1.4,1.2,1.0,1.0,1.1,1.2,1.3};
//   float gain2[576] = {0};

#if 0
   octave_eq[0][0] = octave_eq[1][0] = 2.0;
   octave_eq[0][1] = octave_eq[1][1] = 1.8;
   octave_eq[0][2] = octave_eq[1][2] = 1.6;
   octave_eq[0][3] = octave_eq[1][3] = 1.4;
   octave_eq[0][4] = octave_eq[1][4] = 1.2;
   octave_eq[0][5] = octave_eq[1][5] = 1.0;
   octave_eq[0][6] = octave_eq[1][6] = 1.0;
   octave_eq[0][7] = octave_eq[1][7] = 1.1;
   octave_eq[0][8] = octave_eq[1][8] = 1.2;
   octave_eq[0][9] = octave_eq[1][9] = 1.3;
#endif

   octave_to_linear_eq(rate, octave_eq[0],mp3_eq[0],576);
   octave_to_linear_eq(rate, octave_eq[1],mp3_eq[1],576);
   octave_to_linear_eq(rate, octave_eq[0],equalizer[0],32);
   octave_to_linear_eq(rate, octave_eq[1],equalizer[1],32);

   flags.equalizer = flags.mp3_eq;
}
