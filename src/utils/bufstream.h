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

#ifndef __BUFSTREAM_H
#define __BUFSTREAM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
   void* buffer;
   int   size;
   int   length;
   int   position;
   int   created;

} BUFSTREAM;

/* Creates a buffering stream using specified buffer. */
BUFSTREAM* create_bufstream( int size );
/* Creates a new buffering stream. */
BUFSTREAM* open_bufstream( void* buffer, int size );
/* Returns a buffer associated with buffering stream. */
int get_buffer_bufstream( BUFSTREAM* b, void** buffer );
/* Reads a data from buffering stream. */
int read_bufstream( BUFSTREAM* b, void* buffer, int size );
/* Writes a data from buffering stream. */
int write_bufstream( BUFSTREAM* b, void* buffer, int size );
/* Closes a buffering stream. */
int close_bufstream( BUFSTREAM* b );

#ifdef __cplusplus
}
#endif

#endif /* __BUFSTREAM_H */
