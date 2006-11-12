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

/* CDDB functions */

#define COMMAND_ERROR 0
#define COMMAND_OK    1
#define COMMAND_MORE  2 /* more to read or write with appropriate member functions */

#define PROGRAM    "PM123"
#define VERSION    "1.3"
#define PROTOLEVEL 4

/* for get_sites_req() */
#define NOSERVER 0
#define CDDB     1
#define HTTP     2


typedef struct
{
   char category[24];
   char title[80];
   unsigned long discid_cd, discid_cddb;
} CDDBQUERY_DATA;


class CDDB_socket: public tcpip_socket
{
   public:
     CDDB_socket();
     ~CDDB_socket();

     unsigned long discid(CD_drive *cdDrive);
     int banner_req();
     int handshake_req(char *username, char *hostname);

     int query_req(CD_drive *cdDrive, CDDBQUERY_DATA *output);
     bool get_query_req(CDDBQUERY_DATA *output); /* returns one match after the other */

     int read_req(char *category, unsigned long discid);
     void parse_read_reply(char *reply);
     char *get_disc_title(int which) { return disctitle[which]; };
     char *get_track_title(int track, int which) { return titles[track][which]; };

     int sites_req();
     int get_sites_req(char *server, int size);

     char *gets_content(char *buffer, int size); /* read line of text from content */

     bool isContent() { return content; };

     /* used for HTTP kludge */
     bool connect(char *http_URL, char *proxy_URL, char *path);
     char *gets(char *buffer, int size);
     int write(char *buffer, int size);
     bool close();
     bool cancel();

     char *get_raw_reply() { return raw_reply; }

   protected:

     /* this is inefficient for read memory access, but the database imposes
       no rule on the order of the tracks, so it would be getting ugly to
       write.  filled after a read_req() */
     char *disctitle[3];   /* artist, disc title and extended stuff about disc */
     char *titles[256][2]; /* [0][0] = title of first track
                        [0][1] = extended stuff of first track */
     /* frees above */
     void free_junk();

     /* tells if a read should succeed */
     bool content;

     /* fallback server response processor */
     int process_default(char *response);

     /* CDDB server protocol level */
     bool change_protolevel();
     int protolevel;

     /* used for HTTP kludge */
     HTTP_socket *httpsocket;
     char *CGI; /* path to CGI script */
     char *username, *hostname; /* for hello */
     char cgi_command[2048]; /* gets filled in write() if not EOL */

     // raw data from read_req().  used for cache
     char *raw_reply;
};
