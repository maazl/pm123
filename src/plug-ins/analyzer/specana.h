/*
 * Code that uses fft123.dll to make some interesting data to display
 *
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 *           2006      Marcel Mueller
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

#ifndef SPECANA_H_
#define SPECANA_H_

#include <format.h>


typedef enum
{ WINFN_HAMMING,
  WINFN_BLACKMAN
} WIN_FN;

typedef enum
{ SPECANA_OK,
  SPECANA_UNCHANGED,
  SPECANA_NEWFORMAT,
  SPECANA_ERROR
} SPECANA_RET; 

/* entry points
   The must be set before calling specana_do.
*/
extern ULONG DLLENTRYP(decoderPlayingSamples)( FORMAT_INFO *info, char *buf, int len );
extern BOOL  DLLENTRYP(decoderPlaying)( void );


/* analyse samples
   numsamples: number of samples to analyse, must be a power of 2
   winfn:   desired windowing function
   *bands:  destination, size must be numsamples/2+1
   info:    pointer to FORMAT_INFO area.
   *info:   in:  last FORMAT_INFO. This may be completely zero if there is no last FORMAT_INFO.
            out: current FORMAT_INFO.
   returns: SPECANA_OK: OK data read
            SPECANA_UNCHANGED: Data would be the same than on the last call.
            SPECANA_NEWFORMAT: *info has changed, no analysis done
            SPECANA_ERROR: unhandled error, do not try any longer
   Remarks: The function automatically initializes the internal structures
   when it is called initially or when numsamples changed since the last call.
   This will take longer than a usual call.
*/
SPECANA_RET specana_do(int numsamples, WIN_FN winfn, float* bands, FORMAT_INFO* info);
/* explicitly uninitialize, free internal resources */
void specana_uninit(void);


#endif

