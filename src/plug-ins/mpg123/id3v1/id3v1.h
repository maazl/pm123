/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
 *
 * Copyright 2006 Dmitry A.Steklenev <glass@ptv.ru>
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

#ifndef ID3V1_TAG_H
#define ID3V1_TAG_H

#include <xio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ID3V1_TITLE   1
#define ID3V1_ARTIST  2
#define ID3V1_ALBUM   3
#define ID3V1_YEAR    4
#define ID3V1_COMMENT 5
#define ID3V1_TRACK   6
#define ID3V1_GENRE   7

typedef struct _ID3V1_TAG
{
  char id     [ 3];
  char title  [30];
  char artist [30];
  char album  [30];
  char year   [ 4];
  char comment[28];
  unsigned char empty;
  unsigned char track;
  unsigned char genre;

} ID3V1_TAG;

/* Reads a ID3v1 tag from the input file and stores them in
   the given structure. Returns 0 if it successfully reads the
   tag or if the input file don't have a ID3v1 tag. A nonzero
   return value indicates an error. */
int id3v1_gettag( XFILE* x, ID3V1_TAG* tag );

/* Writes a ID3v1 tag from the given structure to the output
   file. Returns 0 if it successfully writes the tag. A nonzero
   return value indicates an error. */
int id3v1_settag( XFILE* x, ID3V1_TAG* tag );

/* Cleanups  of a ID3v1 tag structure. */
void id3v1_clrtag( ID3V1_TAG* tag );

/* Returns a specified field of the given tag. */
char* id3v1_getstring( ID3V1_TAG* tag, int type, char* result, int size );
/* Sets a specified field of the given tag. */
void  id3v1_setstring( ID3V1_TAG* tag, int type, char* source );

#ifdef __cplusplus
}
#endif
#endif /* ID3V1_TAG_H */