/*
 * Copyright 2013-2013 Marcel Mueller
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

#include "directory.h"
#include "cpp/container/stringset.h"
#include <strutils.h>
#include <errorstr.h>
#include <memory.h>

DirScan::DirScan(const char* path, const char* mask, ULONG attributes)
: Attributes(attributes)
, Handle(NULLHANDLE)
, LastError(0)
, LastCount(0)
, CurrentEntry(NULL)
{ PathLen = strnlen(path, sizeof Path - 2);
  memcpy(Path, path, PathLen);
  if (PathLen && Path[PathLen-1] != '/' && Path[PathLen-1] != '\\' && Path[PathLen-1] != ':')
    Path[PathLen++] = '\\';
  strlcpy(Path + PathLen, mask, sizeof Path - PathLen);
}

DirScan::~DirScan()
{ if (Handle)
    DosFindClose(Handle);
}

APIRET DirScan::Next()
{ if (LastCount > 1)
  { --LastCount;
    CurrentEntry = (FILEFINDBUF3*)((char*)CurrentEntry + CurrentEntry->oNextEntryOffset);
    Path[PathLen] = 0;
    return 0;
  }
  LastCount = 100;
  if (!Handle)
  { Handle = HDIR_CREATE;
    LastError = DosFindFirst(Path, &Handle, Attributes,
      &Buffer, sizeof Buffer, &LastCount, FIL_STANDARD);
  } else
    LastError = DosFindNext(Handle, &Buffer, sizeof Buffer, &LastCount);
  if (!LastError)
  { // Success => prepare result
    CurrentEntry = (const FILEFINDBUF3*)Buffer;
  } else
  { if (Handle && Handle != HDIR_CREATE)
      // At the end => release resources as soon as possible.
      DosFindClose(Handle);
    Handle = NULLHANDLE;
    LastCount = 0;
    CurrentEntry = NULL;
  }
  Path[PathLen] = 0;
  return LastError;
}

const char* DirScan::CurrentPath() const
{ if (!CurrentEntry)
    return NULL;
  if (!Path[PathLen])
    // prepare full name cache
    strlcpy((char*)Path + PathLen, CurrentEntry->achName, sizeof Path - PathLen);
  return Path;
}

const char* DirScan::LastErrorText() const
{ if (!LastError)
    return NULL;
  return os2_strerror(LastError, (char*)Buffer, sizeof Buffer);
}

/*APIRET DirScan::GetAll(stringset& target, bool withpath)
{
  APIRET rc;
  while ((rc = Next()) == 0)
  { target.ensure(withpath ? CurrentPath() : CurrentEntry->achName);
  }
  return rc;
}*/
