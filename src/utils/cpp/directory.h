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

#ifndef DIRECTORY_H
#define DIRECTORY_H

#define INCL_BASE
#include <stdlib.h>
#include <os2.h>

/// Enumerate the content of a directory.
/// @remarks Note that the DirScan iteration starts before the start.
/// You need to call \c Next() before \c Current().
class DirScan
{protected:
  /// Attributes for directory scan.
  const ULONG Attributes;
  /// Handle for OS/2 API
  /// @remarks Valid and not \c NULLHANDLE only after first call to \c Next().
  HDIR        Handle;
  /// Before first call to \c Next(): path and file mask.
  /// After \c Next() and \c CurrentPath() buffer for last full qualified file name.
  mutable char Path[_MAX_PATH];
  /// Length of the base path in \c Path without file mask or file name.
  size_t      PathLen;
  /// Last return from \c Next().
  APIRET      LastError;
  /// Number of remaining elements in the current package.
  ULONG       LastCount;
  /// Pointer to current member in \c Buffer.
  const FILEFINDBUF3* CurrentEntry;
  /// Buffer for \c DosFind...
  char        Buffer[1024];
 private: // non-copyable
  DirScan(const DirScan&);
  void operator=(const DirScan&);
 public:
  /// Start directory scan.
  /// @param path Directory where to start the scan.
  /// Only content of that directory is returned.
  /// @param mask File mask. Use \c "*" if you want to get all files and directories.
  DirScan(const char* path, const char* mask, ULONG attributes);
  /// Clean up resources.
  ~DirScan();
  /// Move the iterator to the next directory entry.
  /// @return 0 := OK, you got another entry, calling \c Current() will return valid information.
  /// Everything else: something went wrong. See OS/2 API docs.
  /// @remarks Calling \c Next() again after it returned non-zero is undefined behavior.
  APIRET      Next();
  /// Get the properties of the current directory entry.
  /// @return Information about the directory entry or \c NULL
  /// if \c Next() has not been called or returned an error.
  const FILEFINDBUF3* Current() const { return CurrentEntry; }
  /// Get the full qualified path of the current directory entry.
  /// @return Full path of the directory entry or \c NULL
  /// if \c Next() has not been called or returned an error.
  /// The returned storage is valid until the next non-const method call.
  /// @remarks The returned storage is valid until the next package.
  /// See \see RemainingPackageSize.
  const char* CurrentPath() const;
  /// Last return value from \c Next().
  APIRET      LastRC() const { return LastError; }
  /// Last return value of \c Next() as plain text.
  /// @return Pointer to a storage with an error text for the result of the last call to \c Next()
  /// or \c NULL if the last call to \c Next() succeeded.
  const char* LastErrorText() const;
  /// Return the number of entries available for retrieval without I/O
  /// and invalidation of storage.
  /// @return 0 := \c Next() returned an error or has not yet been called.
  /// > 0 := There is at least one current element available and
  /// the further n-1 calls to \c Next() will return zero and do not invalidate buffers.
  unsigned    RemainingPackageSize() const { return LastCount; }
};

#endif // DIRECTORY_H
