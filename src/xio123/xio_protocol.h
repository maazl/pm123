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

#ifndef XIO_PROTOCOL_H
#define XIO_PROTOCOL_H

#include "xio.h"

#define XO_WRITE          0x0001
#define XO_READ           0x0002
#define XO_CREATE         0x0004
#define XO_APPEND         0x0008
#define XO_TRUNCATE       0x0010

#define XS_CAN_READ       0x0001
#define XS_CAN_WRITE      0x0002
#define XS_CAN_READWRITE  0x0004
#define XS_CAN_CREATE     0x0008
#define XS_CAN_SEEK       0x0010
#define XS_CAN_SEEK_FAST  0x0020
#define XS_USE_SPOS       0x0040
#define XS_NOT_BUFFERIZE  0x0080

struct XPROTOCOL {

  int supports;
  int eof;
  int error;

  /* Note: All methods of the protocol are serialized by library
     except for close, tell, seek and size. */

  /* Opens the file specified by filename. Returns 0 if it
     successfully opens the file. A return value of -1 shows an error. */
  int (*open )( XFILE* x, const char* filename, int oflags );

  /* Reads count bytes from the file into buffer. Returns the number
     of bytes placed in result. The return value 0 indicates an attempt
     to read at end-of-file. A return value -1 indicates an error.     */
  int  (*read )( XFILE* x, void* result, unsigned int count );

  /* Writes count bytes from source into the file. Returns the number
     of bytes moved from the source to the file. The return value may
     be positive but less than count. A return value of -1 indicates an
     error */
  int  (*write)( XFILE* x, const void* source, unsigned int count );

  /* Closes the file. Returns 0 if it successfully closes the file. A
     return value of -1 shows an error. */
  int  (*close)( XFILE* x );

  /* Returns the current position of the file pointer. The position is
     the number of bytes from the beginning of the file. On devices
     incapable of seeking, the return value is -1L. */
  long (*tell )( XFILE* x );

  /* Moves any file pointer to a new location that is offset bytes from
     the origin. Returns the offset, in bytes, of the new position from
     the beginning of the file. A return value of -1L indicates an
     error. */
  long (*seek )( XFILE* x, long offset, int origin );

  /* Returns the size of the file. A return value of -1L indicates an
     error or an unknown size. */
  long (*size )( XFILE* x );

  /* Lengthens or cuts off the file to the length specified by size.
     You must open the file in a mode that permits writing. Adds null
     characters when it lengthens the file. When cuts off the file, it
     erases all data from the end of the shortened file to the end
     of the original file. Returns the value 0 if it successfully
     changes the file size. A return value of -1 shows an error. */
  int  (*chsize)( XFILE* x, long size );

  /* Cleanups the protocol. */
  void (*clean)( XFILE* x );

  HMTX mtx_access; /* Serializes access to the protocol's data. */
  HMTX mtx_file;   /* Serializes all i/o operations.            */
  int  abort;      /* Abnormal termination of all operations.   */

  /* Can be used by protocol implementation. */

  int           s_handle;   /* Connection or file handle.                    */
  unsigned long s_pos;      /* Current position of the stream pointer.       */
  unsigned long s_size;     /* Size of the accociated file.                  */
  int           s_metaint;  /* How often the metadata is sent in the stream. */
  int           s_metapos;  /* Used by Shoutcast and Icecast protocols.      */
  unsigned long s_observer; /* Handle of a window that are to be notified    */
                            /* of changes in the state of the library.       */
  char*         s_metabuff; /* The library puts metadata in this buffer      */
  int           s_metasize; /* before notifying the observer.                */
  char*         s_location; /* Saved resource location. */

  /* Used by HTTP protocol implementation. */

  char  s_genre[128];
  char  s_name [128];
  char  s_title[128];

  /* Used by FTP  protocol implementation. */

  char  s_reply[512];
  int   s_datahandle;

  #ifdef XIO_SERIALIZE_DISK_IO
  int   s_serialized;
  #endif

};

#endif /* XIO_FILE_H */

