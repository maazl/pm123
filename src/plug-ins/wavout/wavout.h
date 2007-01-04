/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
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

#ifndef _WAVOUT_H
#define _WAVOUT_H

#define DLG_CONFIGURE 1000
#define ID_NULL       1001
#define ST_FILENAME   1010
#define EF_FILENAME   1020
#define PB_BROWSE     1030
#define ST_ABOUT      1040

#define DLG_BROWSE    2000

#pragma pack(1)
typedef struct _RIFF_HEADER
{
  char           id_riff[4]; /* RIFF */
  unsigned long  len;
  char           id_wave[4]; /* WAVE */

} RIFF_HEADER;

typedef struct _CHNK_HEADER
{
  char           id[4];
  unsigned long  len;

} CHNK_HEADER;

typedef struct _FORMAT
{
  unsigned short FormatTag;
  unsigned short Channels;
  unsigned long  SamplesPerSec;
  unsigned long  AvgBytesPerSec;
  unsigned short BlockAlign;
  unsigned short BitsPerSample;

} FORMAT;

typedef struct WAVE_HEADER
{
  RIFF_HEADER  riff;
  CHNK_HEADER  format_header;
  FORMAT       format;
  CHNK_HEADER  data_header;
} WAVE_HEADER;

#pragma pack()

typedef struct _WAVOUT
{
  HEV   pause;
  int   playingpos;
  char  filename[CCHMAXPATH];   // filled by setup.
  char  fullpath[CCHMAXPATH];   // completed by open with outpath.
  char* buffer;
  FILE* file;

  WAVE_HEADER   header;
  OUTPUT_PARAMS original_info;  // to open the device.

} WAVOUT;

#endif
