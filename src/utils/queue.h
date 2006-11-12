/*
 * Copyright 2006 Dmitry A.Steklenev
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

#ifndef QUEUE_H
#define QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _QELEMENT
{
  unsigned long request;
  void* data;

  struct _QELEMENT* prev;
  struct _QELEMENT* next;

} QELEMENT, *PQELEMENT;

typedef struct _QUEUE
{
  struct _QELEMENT* first;
  struct _QELEMENT* last;

  unsigned long data_access;
  unsigned long data_ready;

} QUEUE, *PQUEUE;

/* Creates a queue. */
PQUEUE qu_create( void );
/* Requests ownership of the queue. */
void qu_request( PQUEUE queue );
/* Relinquishes ownership of the queue was requested by qu_request. */
void qu_release( PQUEUE queue );
/* Is a queue empty. */
int qu_empty( PQUEUE queue );
/* Purges a queue of all its elements. */
void qu_purge( PQUEUE queue );
/* Closes a queue. */
void qu_close( PQUEUE queue );
/* Reads an element from a queue. */
int qu_read( PQUEUE queue, unsigned long* request, void** data );
/* Examines a queue element without removing it from the queue. */
int qu_peek( PQUEUE queue, unsigned long* request, void** data );
/* Pushes an element to a front of a queue. */
int qu_push( PQUEUE queue, unsigned long request, void* data );
/* Adds an element to a queue. */
int qu_write( PQUEUE queue, unsigned long request, void* data );

#ifdef __cplusplus
}
#endif
#endif
