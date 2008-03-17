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

#define  INCL_DOS
#define  INCL_ERRORS
#include <os2.h>
#include <stdlib.h>
#include "queue.h"

/* Creates a queue. */
PQUEUE
qu_create( void )
{
  PQUEUE queue = calloc( sizeof( QUEUE ), 1 );

  if( queue && DosCreateMutexSem( NULL, &queue->data_access, 0, FALSE ) == NO_ERROR
            && DosCreateEventSem( NULL, &queue->data_ready,  0, FALSE ) == NO_ERROR )
  {
    return queue;
  }

  if( queue->data_access ) {
    DosCloseMutexSem( queue->data_access );
  }
  if( queue->data_ready  ) {
    DosCloseMutexSem( queue->data_ready  );
  }

  free( queue );
  return NULL;
}

/* Requests ownership of the queue. */
void
qu_request( PQUEUE queue ) {
  DosRequestMutexSem( queue->data_access, SEM_INDEFINITE_WAIT );
}

/* Relinquishes ownership of the queue was requested by qu_request. */
void
qu_release( PQUEUE queue ) {
  DosReleaseMutexSem( queue->data_access );
}

/* Waits a data ready semaphore. */
static void
qu_wait_data( PQUEUE queue ) {
  DosWaitEventSem( queue->data_ready, SEM_INDEFINITE_WAIT );
}

/* Posts a data ready semaphore. */
static void
qu_have_data( PQUEUE queue ) {
  DosPostEventSem( queue->data_ready );
}

/* Resets a data ready semaphore. */
static void
qu_no_data( PQUEUE queue )
{
  ULONG post_count;
  DosResetEventSem( queue->data_ready, &post_count );
}

/* Purges a queue of all its elements. */
void
qu_purge( PQUEUE queue )
{
  PQELEMENT node, next;

  qu_request( queue );

  for( node = queue->first; node; node = next ) {
    next = node->next;
    free( node );
  }

  queue->first = NULL;
  queue->last  = NULL;
  qu_no_data( queue );
  qu_release( queue );
}

/* Closes a queue. */
void
qu_close( PQUEUE queue )
{
  qu_purge( queue );
  DosCloseMutexSem( queue->data_access );
  DosCloseEventSem( queue->data_ready  );
  free( queue );
}

/* Is a queue empty. */
int
qu_empty( PQUEUE queue ) {
  return !queue->first;
}

/* Reads an element from a queue. */
int
qu_read( PQUEUE queue, unsigned long* request, void** data )
{
  for(;;) {
    qu_wait_data( queue );
    qu_request  ( queue );

    if( queue->first )
    {
      PQELEMENT node = queue->first;

      *request = node->request;
      *data    = node->data;

      queue->first = node->next;
      free( node );

      if( queue->first ) {
        queue->first->prev = NULL;
      } else {
        qu_no_data( queue );
        queue->last = NULL;
      }
      qu_release( queue );
      break;
    } else {
      qu_no_data( queue );
      qu_release( queue );
    }
  }

  return TRUE;
}

/* Examines a queue element without removing it from the queue. */
int
qu_peek( PQUEUE queue, unsigned long* request, void** data )
{
  qu_request( queue );

  if( queue->first ) {
    *request = queue->first->request;
    *data    = queue->first->data;

    qu_release( queue );
    return TRUE;
  } else {
    qu_release( queue );
    return FALSE;
  }
}

/* Adds an element to a queue. */
int
qu_write( PQUEUE queue, unsigned long request, void* data )
{
  PQELEMENT node = calloc( sizeof( QELEMENT ), 1 );

  if( node ) {
    qu_request( queue );

    node->request = request;
    node->data    = data;

    if( queue->last ) {
      node->prev = queue->last;
      queue->last->next = node;
      queue->last = node;
    } else {
      queue->first = node;
      queue->last  = node;
    }

    qu_have_data( queue );
    qu_release  ( queue );

    return TRUE;
  } else {
    return FALSE;
  }
}

/* Pushes an element to a front of a queue. */
int
qu_push( PQUEUE queue, unsigned long request, void* data )
{
  PQELEMENT node = calloc( sizeof( QELEMENT ), 1 );

  if( node ) {
    qu_request( queue );

    node->request = request;
    node->data    = data;

    if( queue->first ) {
      node->next = queue->first;
      queue->first->prev = node;
      queue->first = node;
    } else {
      queue->first = node;
      queue->last  = node;
    }

    qu_have_data( queue );
    qu_release  ( queue );

    return TRUE;
  } else {
    return FALSE;
  }
}
