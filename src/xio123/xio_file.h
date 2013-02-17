/*
 * Copyright 2008-2013 M.Mueller
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

#ifndef XIO_FILE_H
#define XIO_FILE_H

#include "xio.h"
#include "xio_protocol.h"

class XIOfile : public XPROTOCOL
{protected:
  HFILE s_handle;
 private:
  #ifdef XIO_SERIALIZE_DISK_IO
  bool  s_serialized;
  #endif

 private:
  virtual APIRET doOpen(const char* filename, ULONG flags, ULONG omode) = 0;
  virtual APIRET doSetFilePtr(int64_t pos, ULONG method, int64_t* newpos = NULL) = 0;
  virtual APIRET doQueryFileInfo(XSTATL* st) = 0;
  virtual APIRET doSetFileSize(int64_t size) = 0;
 public:
  /* Initializes the file protocol. */
  XIOfile();
  virtual ~XIOfile();
  virtual int open(const char* filename, XOFLAGS oflags);
  virtual int read(void* result, unsigned int count);
  virtual int write(const void* source, unsigned int count);
  virtual int close();
  virtual int64_t tell();
  virtual int64_t seek(int64_t offset, XIO_SEEK origin);
  virtual int64_t getsize();
  virtual int getstat(XSTATL* st);
  virtual int chsize(int64_t size);
  virtual XSFLAGS supports() const;
  virtual XIO_PROTOCOL protocol() const;
};

/// Implementation for the OS/2 file API using the old 32 bit calls.
class XIOfile32 : public XIOfile
{private:
  virtual APIRET doOpen(const char* filename, ULONG flags, ULONG omode);
  virtual APIRET doSetFilePtr(int64_t pos, ULONG method, int64_t* newpos);
  virtual APIRET doQueryFileInfo(XSTATL* st);
  virtual APIRET doSetFileSize(int64_t size);
};

/// Implementation for the OS/2 file API using the new 64 bit calls. Requires recent kernel.
class XIOfile64 : public XIOfile
{private:
  static const LONGLONG LL0;
 private:
  static APIRET (APIENTRY *pOpen)(PSZ filename, PHFILE phf, PULONG pulaction, LONGLONG cbfile,
                                  ULONG ulAttribute, ULONG fsOpenFlags, ULONG fsOpenMode, PEAOP2 peaop2);
  static APIRET (APIENTRY *pSetFilePtr)(HFILE hf, LONGLONG ib, ULONG method, PLONGLONG ibActual);
  static APIRET (APIENTRY *pSetFileSize)(HFILE hf, LONGLONG size);
 public:
  /// Check whether the 64 bit file API is supported.
  /// You must not use this class unless this function returned \c true.
  static bool supported();
 private:
  virtual APIRET doOpen(const char* filename, ULONG flags, ULONG omode);
  virtual APIRET doSetFilePtr(int64_t pos, ULONG method, int64_t* newpos);
  virtual APIRET doQueryFileInfo(XSTATL* st);
  virtual APIRET doSetFileSize(int64_t size);
};

#endif /* XIO_FILE_H */

