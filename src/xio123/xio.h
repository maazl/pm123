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

/* Note on 64 Bit support:
 *
 * The XIO API always uses the 64 bit API if applicable. So you are always
 * able to write > 2GB per file as long as the file system supports it.
 * Only if you deal with file pointers or file size you might note the difference.
 * The 32 bit functions fail if they cannot return the appropriate data.
 * But they will not fail unless you really passed beyond 2 GB. E.g if you open
 * a file, write 3 GB data, and then seek to 1GB with the 32 bit xio_fseek
 * nothing will fail.
 */

#ifndef XIO_H
#define XIO_H

#include <config.h>

#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct xstring;

#define XIO_MAX_FILETYPE  128

typedef enum _XIO_PROTOCOL
{ XIO_PROTOCOL_NONE
, XIO_PROTOCOL_FILE
, XIO_PROTOCOL_HTTP
, XIO_PROTOCOL_FTP
, XIO_PROTOCOL_CDDB
, XIO_PROTOCOL_SOCKET
} XIO_PROTOCOL;

typedef enum _XIO_SEEK
{ XIO_SEEK_SET
, XIO_SEEK_CUR
, XIO_SEEK_END
} XIO_SEEK;

typedef enum _XIO_META
{ XIO_META_NAME
, XIO_META_GENRE
, XIO_META_TITLE
} XIO_META;

typedef enum _XIO_SEEK_SUPPORT
{ XIO_NOT_SEEK
, XIO_CAN_SEEK
, XIO_CAN_SEEK_FAST
} XIO_SEEK_SUPPORT;

enum
{ S_IAREAD = 0x01   /**< File is marked as read-only */
, S_IAHID  = 0x02   /**< File is marked as hidden */
, S_IASYS  = 0x04   /**< File is marked as a system file */
, S_IADIR  = 0x10   /**< File is a directory */
, S_IAARCH = 0x20   /**< File needs backup ('archive' bit) */
};

typedef struct _XFILE XFILE;

typedef struct _XSTAT
{ long     size;    /**< -1 if not available */
  time_t   atime;   /**< -1 if not available */
  time_t   mtime;   /**< -1 if not available */
  time_t   ctime;   /**< -1 if not available */
  unsigned attr;    /**< see S_IA... */
  char     type[XIO_MAX_FILETYPE];/* tab separated list of EA types or mime type. */
} XSTAT;

typedef struct _XSTATL
{ int64_t  size;    /**< -1 if not available */
  time_t   atime;   /**< -1 if not available */
  time_t   mtime;   /**< -1 if not available */
  time_t   ctime;   /**< -1 if not available */
  unsigned attr;    /**< see S_IA... */
  char     type[XIO_MAX_FILETYPE];/* tab separated list of EA types or mime type. */
} XSTATL;

/** @brief Open file or URL.
 * @return Returns a pointer to a file structure that can be used
 * to access the open file. A NULL pointer return value indicates an
 * error.
 * @param mode In addition to the standard mode flags \c xio_open has the following
 * extensions:
 * \li R - random access (no buffering)
 * \li X - asynchronous buffer thread
 * \li U - unsynchronized (not thread-safe)
 */
XFILE* DLLENTRY xio_fopen(const char* filename, const char* mode);

/** Reads up to \a count items of size \a length from the input file and
 * stores them in the given \a buffer. The position in the file increases
 * by the number of bytes read.
 * @return Returns the number of full items
 * successfully read, which can be less than count if an error occurs
 * or if the end-of-file is met before reaching count. */
size_t DLLENTRY xio_fread(void* buffer, size_t size, size_t count, XFILE* x);

/** Writes up to \a count items, each of \a size bytes in length, from \a buffer
 * to the output stream \a x.
 * @return Returns the number of full items successfully
 * written, which can be fewer than count if an error occurs. */
size_t DLLENTRY xio_fwrite(const void* buffer, size_t size, size_t count, XFILE* x);

/** Closes a file pointed to by \a x.
 * @return Returns 0 if it successfully closes the file, or -1 if any errors were detected. */
int DLLENTRY xio_fclose(XFILE* x);

/** Finds the current position of the file.
 * @return Returns the current file position. On error, returns -1L
 * and sets xio_errno() to a nonzero value.
 * @deprecated You should prefer the 64 bit API \c xio_ftelll. */
long DLLENTRY xio_ftell(XFILE* x);
/** Finds the current position of the file.
 * @return Returns the current file position. On error, returns -1L
 * and sets xio_errno() to a nonzero value. */
int64_t DLLENTRY xio_ftelll(XFILE* x);

/** Changes the current file position to a new location within the file.
 * @return Returns the new position if it successfully moves the pointer.
 * A return value of -1L indicates an error. On devices that cannot seek
 * the return value is -1L.
 * @deprecated You should prefer the 64 bit API \c xio_fseekl. */
long DLLENTRY xio_fseek(XFILE* x, long int offset, XIO_SEEK origin);
/** Changes the current file position to a new location within the file.
 * @return Returns the new position if it successfully moves the pointer.
 * A return value of -1L indicates an error. On devices that cannot seek
 * the return value is -1L. */
int64_t DLLENTRY xio_fseekl(XFILE* x, int64_t offset, XIO_SEEK origin);

/** Repositions the file pointer associated with stream to the beginning
 * of the file. A call to xio_rewind is the same as:
 * <code>(void)xio_fseek(x, 0L, XIO_SEEK_SET)</code>
 * except that \c xio_rewind also clears the error indicator for
 * the stream.
 * @return Returns 0 if it successfully resets the file pointer. */
int DLLENTRY xio_rewind(XFILE* x);

/** Returns the size of the file. A return value of -1L indicates an
 * error or an unknown size.
 * @deprecated You should prefer the 64 bit API \c xio_fsizel. */
long DLLENTRY xio_fsize(XFILE* x);
/** Returns the size of the file. A return value of -1L indicates an
 * error or an unknown size. */
int64_t DLLENTRY xio_fsizel(XFILE* x);

/** Stat the current file.
 * @param x XFILE desrciptor
 * @param st [out] The stat result is written to this structure.
 * @return 0 => OK, else see xio_ferrno.
 * @deprecated You should prefer the 64 bit API \c xio_fstatl. */
int DLLENTRY xio_fstat(XFILE* x, XSTAT* st);
/** Stat the current file.
 * @param x XFILE desrciptor
 * @param st [out] The stat result is written to this structure.
 * @return 0 => OK, else see xio_ferrno.
 */
int DLLENTRY xio_fstatl(XFILE* x, XSTATL* st);

/** @brief Lengthens or cuts off the file to the length specified by size.
 * @details You must open the file in a mode that permits writing. Adds null
 * characters when it lengthens the file. When cuts off the file, it
 * erases all data from the end of the shortened file to the end
 * of the original file.
 * @return Returns the value 0 if it successfully changes the file size.
 * A return value of -1 shows an error. Note that \c xio_ftell can end up
 * to be beyond the end of the file. That happens if you have read data
 * and truncated the file to a smaller EOF position later.
 * @deprecated You should prefer the 64 bit API \c xio_ftruncatel. */
int DLLENTRY xio_ftruncate(XFILE* x, long size);
/** @brief Lengthens or cuts off the file to the length specified by size.
 * @details You must open the file in a mode that permits writing. Adds null
 * characters when it lengthens the file. When cuts off the file, it
 * erases all data from the end of the shortened file to the end
 * of the original file.
 * @return Returns the value 0 if it successfully changes the file size.
 * A return value of -1 shows an error. Note that \c xio_ftell can end up
 * to be beyond the end of the file. That happens if you have read data
 * and truncated the file to a smaller EOF position later.
 */
int DLLENTRY xio_ftruncatel(XFILE* x, int64_t size);

/** Reads bytes from the current file position up to and including the
 * first new-line character (\\n), up to the end of the file, or until
 * the number of bytes read is equal to n-1, whichever comes first.
 * Stores the result in string and adds a null character (\0) to the
 * end of the string. The string includes the new-line character, if
 * read. If n is equal to 1, the string is empty.
 * @return Returns a pointer to the string buffer if successful.
 * A NULL return value indicates an error or an end-of-file condition.
 * In case of EOF string contains the remaining content in the last
 * unterminated line of the file. */
char* DLLENTRY xio_fgets(char* string, int n, XFILE* x);

/** Copies string to the output file at the current position.
 * It does not copy the null character (\\0) at the end of the string.
 * Neither does it automatically appends a new-line character.
 * @return Returns -1 if an error occurs; otherwise, it returns a non-negative
 * value. */
int DLLENTRY xio_fputs(const char* string, XFILE* x);

/** Causes an abnormal termination of all current read and write
 * operations of the file. All current and subsequent calls can
 * raise an error. */
void DLLENTRY xio_fabort(XFILE* x);

/** Indicates whether the end-of-file flag is set for the given stream.
 * The end-of-file flag is set by several functions to indicate the
 * end of the file. The end-of-file flag is cleared by calling xio_rewind,
 * xio_fseek, or \c xio_clearerr for this stream. */
int DLLENTRY xio_feof(XFILE* x);

/** Tests for an error in reading from or writing to the given stream.
 * If an error occurs, the error indicator for the stream remains set
 * until you close stream, call \c xio_rewind, or call \c xio_clearerr. */
int DLLENTRY xio_ferror(XFILE* x);

/** Resets the error indicator and end-of-file indicator for the
 * specified stream. Once set, the indicators for a specified stream
 * remain set until your program calls \c xio_clearerr or \c xio_rewind.
 * \c xio_fseek also clears the end-of-file indicator.
 * \c xio_clererr(NULL) only clears \c xio_errno. */
void DLLENTRY xio_clearerr(XFILE* x);

/** Returns the last error code set by a library call in the current
 * thread. Subsequent calls do not reset this error code. */
int DLLENTRY xio_errno(void);

/** Maps the error number in \a errnum to an error message string. */
const char* DLLENTRY xio_strerror(int errnum);

/** Sets a callback function that is notified when meta information
 * changes in the stream. A non-zero return value indicates an error.
 * @deprecated You should prefer the 64 bit API \c xio_set_metacallbackl. */
int DLLENTRY xio_set_metacallback(XFILE* x, void DLLENTRYP(callback)(XIO_META type, const char* metabuff, long pos, void* arg), void* arg);
/** Sets a callback function that is notified when meta information
 * changes in the stream. A non-zero return value indicates an error. */
int DLLENTRY xio_set_metacallbackl(XFILE* x, void DLLENTRYP(callback)(XIO_META type, const char* metabuff, int64_t pos, void* arg), void* arg);

/** Returns a specified meta information if it is
 * provided by associated stream. */
void DLLENTRY xio_get_metainfo(XFILE* x, XIO_META type, struct xstring* result);

/** @deprecated use xio_set_metacallback instead.
 * @details Sets a handle of a window that are to be notified of changes
 * in the state of the library. set_observer is mutually exclusive
 * with xio_set_metacallback. A non-zero return value indicates an error. */
int DLLENTRY xio_set_observer(XFILE* x, unsigned long window, char* buffer, int buffer_size);

/** Returns \c XIO_NOT_SEEK (0) on streams incapable of seeking,
 * \c XIO_CAN_SEEK (1) on streams capable of seeking and returns
 * \c XIO_CAN_SEEK_FAST (2) on streams capable of fast seeking. */
XIO_SEEK_SUPPORT DLLENTRY xio_can_seek(XFILE* x);

/** Returns the protocol handler used by this handle.
 * @return exactly one of the XIO_FILE, XIO_ */
XIO_PROTOCOL DLLENTRY xio_protocol(XFILE* x);

/** Returns the protocol of this URL.
 * The function does no I/O and does not check anything but the protocol header of an URL.
 * @return The identified protocol or \c XIO_PROTOCOL_NONE (0) if unsupported. */
XIO_PROTOCOL DLLENTRY xio_urlprotocol(const char* url);


/** Returns the read-ahead buffer size. */
int DLLENTRY xio_buffer_size(void);
/** Returns fills the buffer before reading state. */
int DLLENTRY xio_buffer_wait(void);
/** Returns value of prefilling of the buffer. */
int DLLENTRY xio_buffer_fill(void);

/** Sets the read-ahead buffer size. */
void DLLENTRY xio_set_buffer_size(int size);
/** Sets fills the buffer before reading state. */
void DLLENTRY xio_set_buffer_wait(int wait);
/** Sets value of prefilling of the buffer. */
void DLLENTRY xio_set_buffer_fill(int percent);

/** Returns the name of the proxy server including the port. */
void DLLENTRY xio_http_proxy(char* proxy, int size);
/** Returns the user name and password of the proxy server. */
void DLLENTRY xio_http_proxy_auth(char* auth, int size);

/** Sets the name of the proxy server including the port. */
void DLLENTRY xio_set_http_proxy(const char* proxy);
/** Sets the user name and password of the proxy server. */
void DLLENTRY xio_set_http_proxy_auth(const char* auth);

/** Returns the TCP/IP connection timeout. */
int DLLENTRY xio_connect_timeout(void);
/** Sets the TCP/IP connection timeout. */
void DLLENTRY xio_set_connect_timeout(int seconds);

/** Returns the TCP/IP send/receive timeout. */
int DLLENTRY xio_socket_timeout(void);
/** Sets the TCP/IP socket send/receive timeout. */
void DLLENTRY xio_set_socket_timeout(int seconds);

#ifdef __cplusplus
}

/* Syntactic sugar for C++ fanboys. */
#include <cpp/xstring.h>

class XIO
{protected:
  XFILE*  Handle;
 public:
          XIO()                                         : Handle(NULL) {}
  explicit XIO(XFILE* x)                                : Handle(x) {}
          ~XIO()                                        { if (Handle) xio_fabort(Handle); }

  bool    Open(const char* filename, const char* mode)  { return (Handle = xio_fopen(filename, mode)) != NULL; }
  int     Close()                                       { return xio_fclose(Handle); }
  void    Abort()                                       { if (!Handle) return; xio_fabort(Handle); Handle = NULL; }

  XIO_PROTOCOL Protocol() const                         { return xio_protocol(Handle); }
  bool    IsEOF() const                                 { return !!xio_feof(Handle); }
  int     Error() const                                 { return xio_ferror(Handle); }

  size_t  Read(void* buffer, size_t size, size_t count) { return xio_fread(buffer, size, count, Handle); }
  bool    Read(void* buffer, size_t size)               { return xio_fread(buffer, size, 1, Handle) == 1; }
  size_t  Write(const void* buffer, size_t size, size_t count) { return xio_fwrite(buffer, size, count, Handle); }
  bool    Write(const void* buffer, size_t size)        { return xio_fwrite(buffer, size, 1, Handle) == 1; }
  char*   GetS(char* string, int n)                     { return xio_fgets(string, n, Handle); }
  int     PutS(const char* string)                      { return xio_fputs(string, Handle); }

  int64_t Tell() const                                  { return xio_ftelll(Handle); }
  XIO_SEEK_SUPPORT CanSeek() const                      { return xio_can_seek(Handle); }
  int64_t Seek(int64_t offset, XIO_SEEK origin = XIO_SEEK_SET) { return xio_fseekl(Handle, offset, origin); }
  int     Rewind()                                      { return xio_rewind(Handle); }

  int64_t Size() const                                  { return xio_fsizel(Handle); }
  int     Stat(XSTATL* st) const                        { return xio_fstatl(Handle, st); }
  int     Truncate(int64_t size)                        { return xio_ftruncatel(Handle, size); }

  int     SetMetaCallback(void DLLENTRYP(callback)(XIO_META type, const char* metabuff, int64_t pos, void* arg), void* arg) { return xio_set_metacallbackl(Handle, callback, arg); }
  struct xstring GetMetaInfo(XIO_META type)             { xstring ret; xio_get_metainfo(Handle, type, &ret); return ret; }
};
#endif
#endif /* XIO_H */
