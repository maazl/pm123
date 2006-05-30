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

/* #define USER_AGENT for your program */

#define USER_AGENT "PM123/1.2"

/* headers used internally to mimic the HTTP structure */

typedef enum { persistent, non_persistent } CONNECTION;
typedef enum { none, chunked } TRANSFER;

typedef struct
{
   CONNECTION connection;
   TRANSFER transfer;
} GENERAL_HEADER;

typedef struct
{
   char *host; /* needed if URI doesn't contain host and
               that happens when not sent through a proxy */
   char *user_agent;

} REQUEST_HEADER;

typedef struct
{
   char *proxy_auth;
   char *www_auth;
   char *location;
   char *retry_after;

} RESPONSE_HEADER;

typedef struct
{
   int length;
} ENTITY_HEADER;


class HTTP_socket : public tcpip_socket
{
   public:
     HTTP_socket();
     ~HTTP_socket();

      bool connect(char *http_URL, char *proxy_URL);

      /* automated GET request */
      int get_req(CONNECTION connection, char *path);

      char *gets_content(char *buffer, int size); /* read line of text from content */
      int read_content(char *buffer, int size); /* read binary from content */
//    int write_content(char *buffer, int size); /* write anything to content */

      bool isContent() { return (dataleft != 0); };

   protected:
      /* connect() keeps info on the http host and port, and if we are on a
         proxy connection */
      char *http_host;
      int http_port;
      bool proxy;

      /* keeps count of what should be left to read in the content.
         if -1, read as chunked or till disconnected if no chunks */
      int dataleft;

      unsigned int major, minor; /* server http version */
      unsigned int status_code;  /* status code xxx */

      /* keeps information about response for reading entity-body */
      GENERAL_HEADER  response_general;
      RESPONSE_HEADER response_response;
      ENTITY_HEADER   response_entity;

      /* fills up previous structures */
      int process_headers();

      /* resets the structures to default and does a new request */
      int new_request(char *method, char *URI,
                      GENERAL_HEADER *req_general, REQUEST_HEADER *req_req);

      /* builds URI for GET request */
      void build_URI(char *out, char *path);
};
