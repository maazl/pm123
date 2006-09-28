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

#ifndef PM123_TAG_H
#define PM123_TAG_H

#ifdef __cplusplus
extern "C" {
#endif

/* MPõ ID3 V1.x structure, for intermal use only */
typedef struct {
   char tag[3] ;
   char title[30] ;
   char artist[30] ;
   char album[30] ;
   char year[4] ;
   union      // comment and track
   {  struct  // ID3 V1.0
      {  char comment[30] ;
      } V1_0;
      struct  // ID3 V1.1
      {  char comment[28] ;
         char spacer ;
         unsigned char track ;
      } V1_1;
   }    u_comtrk ;          
   unsigned char genre ;
} tag ;

typedef struct {
   char title[128] ;
   char artist[128] ;
   char album[128] ;
   char year[128] ;
   char comment[128] ;
   char genre[128] ;
   int  track ;
   int  gennum;
   int  codepage;
} tune ;

void emptytag(tune* info);
int wipetag(int fd);
int gettag(int fd, tune *info);
/* Function removed because buggy and unused
void jointag(tune *to, tune *from);*/
int settag(int fd, const tune *info);

#ifdef __cplusplus
}
#endif
#endif /* PM123_TAG_H */
