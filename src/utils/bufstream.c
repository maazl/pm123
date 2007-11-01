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

#include <stdlib.h>
#include <string.h>
#include "bufstream.h"

/* Creates a buffering stream using specified buffer. */
BUFSTREAM*
open_bufstream( void* buffer, size_t size )
{
  BUFSTREAM* b = (BUFSTREAM*)malloc( sizeof( BUFSTREAM ));

  b->buffer   = buffer;
  b->size     = size;
  b->length   = size;
  b->position = 0;
  b->created  = 0;

  return b;
}

/* Creates a new buffering stream. */
BUFSTREAM*
create_bufstream( size_t size )
{
  BUFSTREAM* b = (BUFSTREAM*)malloc( sizeof( BUFSTREAM ));

  b->buffer   = malloc( size );
  b->size     = size;
  b->position = 0;
  b->length   = 0;
  b->created  = 1;

  return b;
}

/* Returns a buffer associated with buffering stream. */
size_t
get_buffer_bufstream( BUFSTREAM *b, void** buffer )
{
  *buffer = b->buffer;
  return b->length;
}

/* Reads a data from buffering stream. */
size_t
read_bufstream( BUFSTREAM* b, void* buffer, size_t size )
{
  size_t toread = size;
  if( b->length - b->position < toread ) {
    toread = b->length - b->position;
  }

  memcpy( buffer, (char*)b->buffer + b->position, toread );
  b->position += toread;

  return toread;
}

/* Writes a data from buffering stream. */
size_t
write_bufstream( BUFSTREAM* b, const void* buffer, size_t size )
{
  size_t towrite = size;

  if( towrite > b->size - b->position )
  {
    void* temp = (void*)realloc( b->buffer, 2 * b->size );

    if( temp == NULL ) {
      return 0;
    }

    b->buffer = temp;
    b->size *= 2;
  }

  memcpy( (char*)b->buffer + b->position, buffer, towrite );

  b->position += towrite;
  b->length   += towrite;

  return towrite;
}

/* Closes a buffering stream. */
int close_bufstream( BUFSTREAM* b )
{
  if( b->created ) {
    free( b->buffer );
  }

  free( b );
  return 0;
}
