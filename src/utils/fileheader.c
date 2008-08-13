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

#define  INCL_BASE
#include <os2.h>

#include "debuglog.h"

#define BUFFER_SIZE 60*1024 // OS/2 decreases performance on larger buffers sometimes.


unsigned long filehdr_replace( const char* file, unsigned long old_header_len,
                               const void* new_header, unsigned long new_header_len )
{ char* buffer = NULL;
  int   buf_count = 1;
  int   cur_buf;
  char* cur;
  APIRET rc;
  HFILE in;
  HFILE out;
  ULONG dummy;
  ULONG count;
  ULONG must_complete;
  FILESTATUS3 info;
  
  DEBUGLOG(("filehdr_replace(%s, %lu, %p, %lu)\n", file, old_header_len, new_header, new_header_len));
  
  // calculate buffer size
  if (new_header_len > old_header_len)
    buf_count += (new_header_len - old_header_len -1) / BUFFER_SIZE + 1; 
  
  // Open output stream
  rc = DosOpen((PSZ)file, &out, &dummy, 0, 0,
               OPEN_ACTION_FAIL_IF_NEW|OPEN_ACTION_OPEN_IF_EXISTS,
               OPEN_FLAGS_SEQUENTIAL|OPEN_FLAGS_NOINHERIT|OPEN_SHARE_DENYWRITE|OPEN_ACCESS_WRITEONLY, NULL);
  if (rc != NO_ERROR)
    goto end4;
  
  // Short cut?
  if (new_header_len == old_header_len)
  { // write new header
    rc = DosWrite(out, (PVOID)new_header, new_header_len, &dummy);
    if (rc == NO_ERROR && dummy < new_header_len)
      rc = ERROR_DISK_FULL;
    goto end3;
  }
  
  // Allocate buffer
  rc = DosAllocMem((PVOID*)&buffer, BUFFER_SIZE * buf_count, PAG_COMMIT|PAG_READ|PAG_WRITE);
  if (rc != NO_ERROR)
    goto end3;
    
  // Reserve space for the additional header if new_header_len is larger.
  // This enforces disk errors to occur befor the point of no return.
  if (new_header_len > old_header_len)
  { rc = DosQueryFileInfo(out, FIL_STANDARD, &info, sizeof info);
    if (rc != NO_ERROR)
      goto end2;
    rc = DosSetFileSize(out, info.cbFile + (new_header_len - old_header_len));
    if (rc != NO_ERROR)
      goto end2;
  }

  // Open input stream
  // (Two streams to the same file are significantly faster than anything else.)
  rc = DosOpen((PSZ)file, &in, &dummy, 0, 0,
               OPEN_ACTION_FAIL_IF_NEW|OPEN_ACTION_OPEN_IF_EXISTS,
               OPEN_FLAGS_SEQUENTIAL|OPEN_FLAGS_NOINHERIT|OPEN_SHARE_DENYNONE|OPEN_ACCESS_READONLY, NULL);
  if (rc != NO_ERROR)
    goto end2;

  // Seek to payload start.
  rc = DosSetFilePtr(in, old_header_len, FILE_BEGIN, &dummy);
  if (rc != NO_ERROR)
    goto end1;

  // fill buffer, because something might be overwritten by the new header.
  rc = DosRead(in, buffer, BUFFER_SIZE * buf_count, &count);
  if (rc != NO_ERROR)
    goto end1;

  // extend the file if new header is larger
  if (new_header_len > old_header_len)  
  
  // From here any errors are destructive, so be careful.
  must_complete = 0;
  DosEnterMustComplete(&must_complete);

  // write new header
  rc = DosWrite(out, (PVOID)new_header, new_header_len, &dummy);
  if (rc != NO_ERROR)
    goto end0;
  if (dummy < new_header_len)
  { rc = ERROR_DISK_FULL;
    goto end0;
  }
  // Point of no return!
  
  // Copy payload
  cur_buf = 0;
  cur = buffer;
  while (count >= BUFFER_SIZE)
  { // write one buffer
    rc = DosWrite(out, cur, BUFFER_SIZE, &dummy);
    DEBUGLOG(("filehdr_replace - after write %i %lu %lu\n", cur_buf, rc, dummy));
    if (rc != NO_ERROR)
      goto end0;
    if (dummy < BUFFER_SIZE)
    { rc = ERROR_DISK_FULL;
      goto end0;
    }
    count -= BUFFER_SIZE;

    // read one buffer
    rc = DosRead(in, cur, BUFFER_SIZE, &dummy);
    if (rc != NO_ERROR)
      goto end0;
    count += dummy;

    // next buffer
    ++cur_buf;
    cur_buf %= buf_count;
    cur = buffer + cur_buf * BUFFER_SIZE;
  }

  // Write last buffer
  rc = DosWrite(out, cur, count, &dummy);
  DEBUGLOG(("filehdr_replace - after last write %i %lu %lu\n", cur_buf, rc, dummy));
  if (rc != NO_ERROR)
    goto end0;
  if (dummy < count)
  { rc = ERROR_DISK_FULL;
    goto end0;
  }
  
  // Truncate file (if required)
  if (new_header_len < old_header_len)
  { rc = DosSetFilePtr(out, 0, FILE_CURRENT, &count);
    if (rc != NO_ERROR)
      goto end0;
    rc = DosSetFileSize(out, count);
    if (rc != NO_ERROR)
      goto end0;
  }
 
 end0:  
  DosExitMustComplete(&must_complete);
 end1:
  DosClose(in);
 end2:
  DosFreeMem(buffer);
 end3:
  DosClose(out);
 end4:
  DEBUGLOG(("filehdr_replace: %lu\n", rc));
  return rc; 
}

