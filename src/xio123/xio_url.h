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

#ifndef XIO_URL_H
#define XIO_URL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _XURL {

  char*    scheme;
  char*    username;
  char*    password;
  char*    host;
  unsigned port;
  char*    path;
  char*    params;
  char*    query;
  char*    fragment;

} XURL;

/* Allocates a URL structure. */
XURL* url_allocate( const char* url );
/* Frees a URL structure. */
void  url_free( XURL* url );

#define XURL_STR_FULLAUTH 0x0000 // Complete URL string.
#define XURL_STR_FULL     0x0001 // Complete URL string without username and password.
#define XURL_STR_REQUEST  0x0002 // Request part of the URL string.
#define XURL_STR_ENCODE   0x8000 // Encode of the URL string.

/* Returns the specified parts of the URL string. */
char* url_string( XURL* url, int parts );

#ifdef __cplusplus
}
#endif
#endif /* XIO_URL_H */
