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

#ifndef XIO_H
#define XIO_H

#include <config.h>

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XIO_MAX_HOSTNAME  512
#define XIO_MAX_USERNAME  128
#define XIO_MAX_PASSWORD  128

#define XIO_FILE          0
#define XIO_HTTP          1
#define XIO_FTP           2
#define XIO_CDDB          3

#define XIO_SEEK_SET      0
#define XIO_SEEK_CUR      1
#define XIO_SEEK_END      2

#define XIO_META_NAME     0
#define XIO_META_GENRE    1
#define XIO_META_TITLE    2

#define XIO_NOT_SEEK      0
#define XIO_CAN_SEEK      1
#define XIO_CAN_SEEK_FAST 2

typedef struct _XFILE XFILE;

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
XFILE* DLLENTRY
xio_fopen( const char* filename, const char* mode );

/** Reads up to \a count items of size \a length from the input file and
 * stores them in the given \a buffer. The position in the file increases
 * by the number of bytes read.
 * @return Returns the number of full items
 * successfully read, which can be less than count if an error occurs
 * or if the end-of-file is met before reaching count. */
size_t DLLENTRY
xio_fread( void* buffer, size_t size, size_t count, XFILE* x );

/** Writes up to \a count items, each of \a size bytes in length, from \a buffer
 * to the output stream \a x.
 * @return Returns the number of full items successfully
 * written, which can be fewer than count if an error occurs. */
size_t DLLENTRY
xio_fwrite( const void* buffer, size_t size, size_t count, XFILE* x );

/** Closes a file pointed to by \a x.
 * @return Returns 0 if it successfully closes the file, or -1 if any errors were detected.
 */
int DLLENTRY
xio_fclose( XFILE* x );

/** Finds the current position of the file.
 * @return Returns the current file position. On error, returns -1L
 * and sets xio_errno() to a nonzero value. */
long DLLENTRY
xio_ftell( XFILE* x );

/** Changes the current file position to a new location within the file.
 * @return Returns 0 if it successfully moves the pointer. A nonzero return
 * value indicates an error. On devices that cannot seek the return
 * value is nonzero. */
long DLLENTRY
xio_fseek( XFILE* x, long int offset, int origin );

/** Repositions the file pointer associated with stream to the beginning
 * of the file. A call to xio_rewind is the same as:
 * <code>(void)xio_fseek(x, 0L, XIO_SEEK_SET)</code>
 * except that \c xio_rewind also clears the error indicator for
 * the stream. */
void DLLENTRY
xio_rewind( XFILE* x );

/** Returns the size of the file. A return value of -1L indicates an
 * error or an unknown size. */
long DLLENTRY
xio_fsize( XFILE* x );

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
int DLLENTRY
xio_ftruncate( XFILE* x, long size );

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
char* DLLENTRY
xio_fgets( char* string, int n, XFILE* x );

/** Copies string to the output file at the current position.
 * It does not copy the null character (\\0) at the end of the string.
 * Neither does it automatically appends a new-line character.
 * @return Returns -1 if an error occurs; otherwise, it returns a non-negative
 * value. */
int DLLENTRY
xio_fputs( const char* string, XFILE* x );

/** Causes an abnormal termination of all current read and write
 * operations of the file. All current and subsequent calls can
 * raise an error. */
void DLLENTRY
xio_fabort( XFILE* x );

/** Indicates whether the end-of-file flag is set for the given stream.
 * The end-of-file flag is set by several functions to indicate the
 * end of the file. The end-of-file flag is cleared by calling xio_rewind,
 * xio_fseek, or \c xio_clearerr for this stream. */
int DLLENTRY
xio_feof( XFILE* x );

/** Tests for an error in reading from or writing to the given stream.
 * If an error occurs, the error indicator for the stream remains set
 * until you close stream, call \c xio_rewind, or call \c xio_clearerr. */
int DLLENTRY
xio_ferror( XFILE* x );

/** Resets the error indicator and end-of-file indicator for the
 * specified stream. Once set, the indicators for a specified stream
 * remain set until your program calls \c xio_clearerr or \c xio_rewind.
 * \c xio_fseek also clears the end-of-file indicator.
 * \c xio_clererr(NULL) only clears \c xio_errno. */
void DLLENTRY
xio_clearerr( XFILE* x );

/** Returns the last error code set by a library call in the current
 * thread. Subsequent calls do not reset this error code. */
int DLLENTRY
xio_errno( void );

/** Maps the error number in \a errnum to an error message string. */
const char* DLLENTRY
xio_strerror( int errnum );

/** Sets a callback function that is notified when meta information
 * changes in the stream. */
void DLLENTRY
xio_set_metacallback( XFILE* x, void DLLENTRYP(callback)(int type, const char* metabuff, long pos, void* arg), void* arg );

/** Returns a specified meta information if it is
 * provided by associated stream. */
char* DLLENTRY
xio_get_metainfo( XFILE* x, int type, char* result, int size );

/** DEPRECTATED - use xio_set_metacallback instead.
 * Sets a handle of a window that are to be notified of changes
 * in the state of the library. set_observer is mutually exlusive
 * with xio_set_metacallback.
 */
void DLLENTRY
xio_set_observer( XFILE* x, unsigned long window,
                            char* buffer, int buffer_size );

/** Returns \a XIO_NOT_SEEK (0) on streams incapable of seeking,
 * \a XIO_CAN_SEEK (1) on streams capable of seeking and returns
 * \a XIO_CAN_SEEK_FAST (2) on streams capable of fast seeking. */
int DLLENTRY
xio_can_seek( XFILE* x );

/** Returns the read-ahead buffer size. */
int DLLENTRY
xio_buffer_size( void );
/** Returns fills the buffer before reading state. */
int DLLENTRY
xio_buffer_wait( void );
/** Returns value of prefilling of the buffer. */
int DLLENTRY
xio_buffer_fill( void );

/** Sets the read-ahead buffer size. */
void DLLENTRY
xio_set_buffer_size( int size );
/** Sets fills the buffer before reading state. */
void DLLENTRY
xio_set_buffer_wait( int wait );
/** Sets value of prefilling of the buffer. */
void DLLENTRY
xio_set_buffer_fill( int percent );

/** Returns the name of the proxy server. */
void DLLENTRY
xio_http_proxy_host( char* hostname, int size );
/** Returns the port number of the proxy server. */
int DLLENTRY
xio_http_proxy_port( void );
/** Returns the user name of the proxy server. */
void DLLENTRY
xio_http_proxy_user( char* username, int size );
/** Returns the user password of the proxy server. */
void DLLENTRY
xio_http_proxy_pass( char* password, int size );

/** Returns an internet address of the proxy server.
 * Returns 0 if the proxy server is not defined or -1 if
 * an error occurs */
unsigned long DLLENTRY
xio_http_proxy_addr( void );

/** Sets the name of the proxy server. */
void DLLENTRY
xio_set_http_proxy_host( char* hostname );
/** Sets the port number of the proxy server. */
void DLLENTRY
xio_set_http_proxy_port( int port );
/** Sets the user name of the proxy server. */
void DLLENTRY
xio_set_http_proxy_user( char* username );
/** Sets the user password of the proxy server. */
void DLLENTRY
xio_set_http_proxy_pass( char* password );

/** Returns the TCP/IP connection timeout. */
int DLLENTRY
xio_connect_timeout( void );
/** Sets the TCP/IP connection timeout. */
void DLLENTRY
xio_set_connect_timeout( int seconds );

/** Returns the TCP/IP send/receive timeout. */
int DLLENTRY
xio_socket_timeout( void );
/** Sets the TCP/IP socket send/receive timeout. */
void DLLENTRY
xio_set_socket_timeout( int seconds );

#ifdef __cplusplus
}
#endif
#endif /* XIO_H */
