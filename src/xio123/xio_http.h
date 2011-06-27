/*
 * Copyright 2008-2011 M.Mueller
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

#ifndef XIO_HTTP_H
#define XIO_HTTP_H

#include "xio.h"
#include "xio_protocol.h"
#include "xio_socket.h"

#ifndef HTTPBASEERR
#define HTTPBASEERR 21000
#endif

#define HTTP_OK               200 /* The request was successful.                 */
#define HTTP_CREATED          201 /* The request was successful and a new        */
                                  /* resource was created.                       */
#define HTTP_ACCEPTED         202 /* The request was accepted for processing,    */
                                  /* but the processing is not yet complete.     */
#define HTTP_NO_CONTENT       204 /* The server has processed the request but    */
                                  /* there is no new information to be returned. */
#define HTTP_PARTIAL          206 /* Partial content.                            */
#define HTTP_MULTIPLE         300 /* The requested resource is available at one  */
                                  /* or more locations.                          */
#define HTTP_MOVED_PERM       301 /* The requested resource has been assigned a  */
                                  /* new URL and any further references should   */
                                  /* use this new URL.                           */
#define HTTP_MOVED_TEMP       302 /* The requested resource resides at a         */
                                  /* different location, but will return to this */
                                  /* location in the future.                     */
#define HTTP_SEE_OTHER        303 /* See other.                                  */
#define HTTP_NOT_MODIFIED     304 /* The requested resource has not been         */
                                  /* modified since the date specified in the    */
                                  /* If-Modified_Since header.                   */
#define HTTP_BAD_REQUEST      400 /* The server could not properly interpret the */
                                  /* request.                                    */
#define HTTP_NEED_AUTH        401 /* The request requires user authorization.    */
#define HTTP_FORBIDDEN        403 /* The server has understood the request and   */
                                  /* has refused to satisfy it.                  */
#define HTTP_NOT_FOUND        404 /* The server cannot find the information      */
                                  /* specified in the request.                   */
#define HTTP_BAD_RANGE        416 /* Requested range not satisfiable.            */
#define HTTP_SERVER_ERROR     500 /* The server could not satisfy the request    */
                                  /* due to an internal error condition.         */
#define HTTP_NOT_IMPLEMENTED  501 /* The server does not support the requested   */
                                  /* feature.                                    */
#define HTTP_BAD_GATEWAY      502 /* The server received an invalid response     */
                                  /* from the server from which it was trying to */
                                  /* retrieve information.                       */
#define HTTP_UNAVAILABLE      503 /* The server cannot process this request      */
                                  /* at the current time.                        */
#define HTTP_TOO_MANY_REDIR   998 /* Too many redirections.                      */
#define HTTP_PROTOCOL_ERROR   999 /* Another socket or library error.            */

/* Maximum number of redirects to follow. */
#define HTTP_MAX_REDIRECT       5

class XIOhttp : public XIOreadonly
{private:
  XSFLAGS       support;
  XIOsocket     s_socket;   /* Connection class.                             */
  unsigned long s_pos;      /* Current position of the stream pointer.       */
  unsigned long s_size;     /* Size of the accociated file.                  */
  time_t        s_mtime;    /* modification time of the associated file.     */
  int           s_metaint;  /* How often the metadata is sent in the stream. */
  int           s_metapos;  /* Used by Shoutcast and Icecast protocols.      */
  char*         s_location; /* Saved resource location. */

  char  s_genre[128];
  char  s_name [128];
  char  s_title[128];

  // TODO: @@@@@ I think this is crap...
  Mutex mtx_access; /* Serializes access to the protocol's data. */

 private:
  /* Get and parse HTTP reply. */
  int http_read_reply( );
  /* Appends basic authorization string of the specified type
     to the request. */
  static char* http_basic_auth_to( char* result, const char* typname,
                                   const char* username,
                                   const char* password, int size );
  /* Opens the file specified by filename for reading. Returns 0 if it
     successfully opens the file. A return value of -1 shows an error. */
  int read_file( const char* filename, unsigned long range );
  int read_and_notify( void* result, unsigned int count );

 public:
  /* Initializes the http protocol. */
  XIOhttp();
  virtual ~XIOhttp();
  virtual int open( const char* filename, XOFLAGS oflags );
  virtual int read( void* result, unsigned int count );
  virtual int close();
  virtual long tell( long* offset64 = NULL );
  virtual long seek( long offset, int origin, long* offset64 = NULL );
  virtual long getsize( long* offset64 = NULL );
  virtual int getstat( XSTAT* st );
  virtual char* get_metainfo( int type, char* result, int size );
  virtual Iobserver* set_observer( Iobserver* observer );
  virtual XSFLAGS supports() const;

  /* Maps the error number in errnum to an error message string. */
  static const char* strerror( int errnum );

  // Base64 encoding.
  static char* base64_encode(const char* src);
};

#endif /* XIO_HTTP_H */

