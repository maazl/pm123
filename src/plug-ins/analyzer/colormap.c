/*
 * Copyright 2006 Marcel Mueller
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
 *
 * The very first PM123 visual plugin - the spectrum analyzer
 */

#include "analyzer.h"
#include "colormap.h"
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif


//#define DEBUG


/********** read pallette data **********************************************/

/* do cylindrical colorspace interpolation */
static void interpolate_color(YDCyl_color* result, double position,
  const color_entry* table, size_t table_size)
{ int i;
  // search for closest table entries
  const color_entry* lower_bound = NULL;
  const color_entry* upper_bound = NULL;
  const color_entry* current = table + table_size;
  do
  { --current;
    if (current->Pos > position)
    { // check upper bound
      if (upper_bound == NULL || current->Pos < upper_bound->Pos)
        upper_bound = current;
    } else if (current->Pos < position)
    { // check lower bound
      if (lower_bound == NULL || current->Pos > lower_bound->Pos)
        lower_bound = current;
    } else
    { // exact match
      *result = current->Color;
      return;
    }
  } while (current != table);
  #ifdef DEBUG
  fprintf(stderr, "IP: %f -> %p, %p\n", position, lower_bound, upper_bound);
  #endif
  // check boundaries
  if (lower_bound == NULL)
  { // position lower than lowest table entry
    *result = upper_bound->Color; // use lowest table entry
  } else if (upper_bound == NULL)
  { // position higher than highest table entry
    *result = lower_bound->Color; // use highest table entry
  } else
  { // interpolate
    float tmp, low_P, up_P;
    position = (position - lower_bound->Pos) / (upper_bound->Pos - lower_bound->Pos);
    result->Y  = lower_bound->Color.Y  + position * (upper_bound->Color.Y  - lower_bound->Color.Y );
    result->DR = lower_bound->Color.DR + position * (upper_bound->Color.DR - lower_bound->Color.DR);
    low_P = lower_bound->Color.DR < 1E-3 ? upper_bound->Color.DP : lower_bound->Color.DP;
    up_P = upper_bound->Color.DR < 1E-3 ? lower_bound->Color.DP : upper_bound->Color.DP;
    tmp = up_P - low_P;
    if (fabs(tmp) <= M_PI) // use the shorter way in the circular domain of DP
      result->DP = low_P + position * tmp;
     else if (tmp > 0)
      result->DP = low_P + position * (tmp - 2*M_PI);
     else
      result->DP = low_P + position * (tmp + 2*M_PI);
    #ifdef DEBUG
    fprintf(stderr, "IP: {%f->%f,%f,%f} {%f->%f,%f,%f}\n",
      lower_bound->Pos, lower_bound->Color.Y, lower_bound->Color.DR, lower_bound->Color.DP,    
      upper_bound->Pos, upper_bound->Color.Y, upper_bound->Color.DR, upper_bound->Color.DP);
    #endif    
  } 
}

/* color space transformation */
void RGB2YDCyl(YDCyl_color* result, const RGB2* src)
{ double Db, Dr;
  result->Y = 0.299 * src->bRed + 0.587 * src->bGreen + 0.114 * src->bBlue;
  Db =      - 0.450 * src->bRed - 0.883 * src->bGreen + 1.333 * src->bBlue;
  Dr =      - 1.333 * src->bRed + 1.116 * src->bGreen + 0.217 * src->bBlue;
  result->DR = sqrt(Db*Db + Dr*Dr);
  result->DP = atan2(Dr, Db);
}

static void StoreRGBvalue(BYTE* dst, double val)
{ if (val < 0)
    *dst = 0;
   else if (val > 255)
    *dst = 255;
   else
    *dst = val +.5; // rounding 
}

/* color space transformation */
void YDCyl2RGB(RGB2* result, const YDCyl_color* src)
{ double Db, Dr, tmp;
  Db = src->DR * cos(src->DP);
  Dr = src->DR * sin(src->DP);
  StoreRGBvalue(&result->bRed,   src->Y + 0.000092303716148 * Db - 0.525912630661865 * Dr);
  StoreRGBvalue(&result->bGreen, src->Y - 0.129132898890509 * Db + 0.267899328207599 * Dr);
  StoreRGBvalue(&result->bBlue,  src->Y + 0.664679059978955 * Db - 0.000079202543533 * Dr);
}

/* transfer interpolation data into palette */
void interpolate(RGB2* dst, size_t dstlen, const color_entry* src, size_t srclen)
{ int i;
  YDCyl_color col;
  for (i = 0; i < dstlen; ++i)
  { interpolate_color(&col, (double)i / (dstlen-1), src, srclen);
    YDCyl2RGB(dst, &col);
    #ifdef DEBUG
    fprintf(stderr, "PAL[%d] = RGB{%d,%d,%d} YDCyl{%f,%f,%f}\n", i, dst->bRed, dst->bGreen, dst->bBlue, col.Y, col.DR, col.DP);
    #endif
    ++dst;
  }
}

