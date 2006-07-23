/* * Copyright 2006 Marcel Mueller
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

#ifndef COLORMAP_H_
#define COLORMAP_H_

#define  INCL_GPI
#include <os2.h>
#include <stddef.h>

/* cylindrical representation of the YDbDr color space (SECAM) */
typedef struct
{ float Y;      // luminance
  float DR;     // chrominance, amplitude
  float DP;     // chrominance, color, radiants
} YDCyl_color;

/* interpolation entry */
typedef struct
{ float         Pos;
  YDCyl_color   Color;
} color_entry;


/* color space transformation */
void RGB2YDCyl(YDCyl_color* result, const RGB2* src);
void YDCyl2RGB(RGB2* result, const YDCyl_color* src);
/* create RGB palette from interpolation data */
void interpolate(RGB2* dst, size_t dstlen, const color_entry* src, size_t srclen);


#endif
