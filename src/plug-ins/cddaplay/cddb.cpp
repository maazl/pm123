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

#define INCL_PM
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <types.h>
#include <sys\socket.h>
#include <netinet\in.h>
#include <netdb.h>
#include <nerrno.h>

#include "readcd.h"
#include "utilfct.h"
#include "tcpipsock.h"
#include "http.h"
#include "cddb.h"
#include "cgicis.h"

#include "cdda.h"

CDDB_socket::CDDB_socket()
{
   disctitle[0] = NULL;
   disctitle[1] = NULL;
   disctitle[2] = NULL;

   for(int i=0; i < 256; i++)
      for(int j=0; j < 2; j++)
         titles[i][j] = NULL;

   content = false;
   protolevel = 0;

   httpsocket = NULL;
   CGI = NULL;
   username = NULL;
   hostname = NULL;

   raw_reply = NULL;

   memset(cgi_command, 0, sizeof(cgi_command));
}

CDDB_socket::~CDDB_socket()
{
   free_junk();

   free(CGI); CGI = NULL;

   free(username); username = NULL;
   free(hostname); hostname = NULL;

   close();
}


/* calculate discid */
unsigned long CDDB_socket::discid(CD_drive *cdDrive)
{
   int i, something = 0;

   for (i = 0; i < cdDrive->getCount(); i++)
   {
      int shit, crap;
      UCHAR track = cdDrive->getCDInfo()->firstTrack+i;

      shit = cdDrive->getTrackInfo(track)->start.minute*60 + cdDrive->getTrackInfo(track)->start.second;
      crap = 0;
      while(shit > 0)
      {
         crap += shit % 10;
         shit /= 10;
      }
      something += crap;
   }

   return( something % 0xFF << 24  |
           cdDrive->getTime() << 8 |
           cdDrive->getCount()       );
}

int CDDB_socket::process_default(char *response)
{

    /* is this useful?! in any case, it _needs_ a different implementation */
#if 0
   switch(response[1])
   {
      case '0': return COMMAND_OK;
      case '1': content = true; return COMMAND_MORE;
      case '2': return COMMAND_MORE;
      case '3': displayError("CDDB Error: %s", &response[4]); return COMMAND_ERROR;
   }
#endif

   switch(response[0])
   {
      case '1': return COMMAND_OK; /* ? */
      case '2': return COMMAND_OK;
      case '3': return COMMAND_MORE;
      case '4': displayError("CDDB Error: %s", &response[4]); return COMMAND_ERROR;
      case '5': displayError("CDDB Error: %s", &response[4]); return COMMAND_ERROR;
      default: displayError("CDDB Error: Unknown reply"); return COMMAND_ERROR;
   }
}

int CDDB_socket::banner_req()
{
   char buffer[512];
   content = false;

   /* no shit */
   if(httpsocket)
      return COMMAND_OK;

   if(!gets(buffer, sizeof(buffer)))
      return false;

   switch(buffer[2])
   {
      case '0': return COMMAND_OK;
      case '1': return COMMAND_OK;
      case '2': displayError("CDDB Error: %s", &buffer[4]); return 0;
      case '3': displayError("CDDB Error: %s", &buffer[4]); return 0;
      case '4': displayError("CDDB Error: %s", &buffer[4]); return 0;
   }

   return process_default(buffer);
}

bool CDDB_socket::change_protolevel()
{
   bool finished = false;
   char buffer[128];

   protolevel = PROTOLEVEL;

   /* this should work for HTTP also */

   do
   {
      sprintf(buffer,"proto %d\n",protolevel);
      if(write(buffer,strlen(buffer)) < 1)
         return false;

      if(!gets(buffer, sizeof(buffer)))
         return false;

      /* illegal protocol level */
      if(buffer[0] == '5' && buffer[2] == 1)
         protolevel--;
      else
         finished = true; /* hurray */

   } while(!finished);

   return true;
}

int CDDB_socket::handshake_req(char *set_username, char *set_hostname)
{
   char buffer[512];
   content = false;

   /* this is needed for HTTP since it's used on a per-request base */
   if(httpsocket)
   {
      username = (char*)realloc(username, strlen(set_username)+1); strcpy(username, set_username);
      hostname = (char*)realloc(hostname, strlen(set_hostname)+1); strcpy(hostname, set_hostname);
      if(!change_protolevel())
         return COMMAND_ERROR;
      return COMMAND_OK;
   }

   if(!change_protolevel())
      return COMMAND_ERROR;

   sprintf(buffer, "cddb hello %s %s %s %s\n", set_username, set_hostname, PROGRAM, VERSION);
   if(write(buffer,strlen(buffer)) < 1)
      return COMMAND_ERROR;

   if(!gets(buffer, sizeof(buffer)))
      return COMMAND_ERROR;

   switch(buffer[0])
   {
      case '2':
         switch(buffer[2])
         {
            case '0': return COMMAND_OK;
         }
         break;
      case '4':
         switch(buffer[2])
         {
            case '1': displayError("CDDB Error: %s", &buffer[4]); return COMMAND_ERROR;
            case '2': displayError("CDDB Warning: %s", &buffer[4]); return COMMAND_OK;
         }
         break;
   }

   return process_default(buffer);
}


/* outputs trackInfo while it's at it */
int CDDB_socket::query_req(CD_drive *cdDrive, CDDBQUERY_DATA *output)
{
   char buffer[512];
   content = false;

   output->discid_cd = this->discid(cdDrive);

   /* making the request line */
   sprintf(buffer, "cddb query %08x %d ", output->discid_cd, cdDrive->getCount());
   for(int i = 0; i < cdDrive->getCount(); i++)
   {
      UCHAR track = cdDrive->getCDInfo()->firstTrack+i;

      /* +150 because CDDB database doesn't use real sector addressing */
      _itoa(CD_drive::getLBA(cdDrive->getTrackInfo(track)->start)+150,strchr(buffer,'\0'),10);
      strcat(buffer," ");
   }
   _itoa(cdDrive->getTime(), strchr(buffer,'\0'), 10);
   strcat(buffer,"\n");

   if(write(buffer,strlen(buffer)) < 1)
      return false;

   if(!gets(buffer, sizeof(buffer)))
      return false;

   switch(buffer[0])
   {
      case '2':
         switch(buffer[1])
         {
            case '0':
               switch(buffer[2])
               {
                   case '0':
                      sscanf(buffer+4, "%19s %x %79[^\r\n]", output->category,
                             &output->discid_cddb, output->title);
                      return COMMAND_OK;

                   case '2':
                     return -1; /* ? */
               }
               break;

            case '1': /* both fuzzy and multiple exact match */
               content = true;
               return COMMAND_MORE;
         }
         break;

      case '4':
         switch(buffer[2])
         {
            case '3': displayError("CDDB Error: %s", &buffer[4]); return 0;
            case '9': displayError("CDDB Error: %s", &buffer[4]); return 0;
         }
         break;
   }

   return process_default(buffer);
}


bool CDDB_socket::get_query_req(CDDBQUERY_DATA *output)
{
   char buffer[512];

   if(gets_content(buffer,sizeof(buffer)))
   {
      sscanf(buffer, "%19s %x %79[^\r\n]", output->category,
             &output->discid_cddb, output->title);
      return true;
   }
   return false;
}

int CDDB_socket::sites_req()
{
   char buffer[512];

   strcpy(buffer,"sites\n");
   if(write(buffer,strlen(buffer)) < 1)
      return COMMAND_ERROR;

   if(!gets(buffer, sizeof(buffer)))
      return COMMAND_ERROR;

   switch(buffer[2])
   {
      case '0': content = true; return COMMAND_MORE;
      case '1': return COMMAND_ERROR;
   }

   return process_default(buffer);
}

/* the size parameter isn't used ... hrmf... */
int CDDB_socket::get_sites_req(char *server, int size)
{
   char buffer[1024];
   char host[512], path[512], servertype[8];
   int port;

   if(gets_content(buffer,sizeof(buffer)))
   {
      if(protolevel > 2)
      {
         sscanf(buffer,"%511s %7s %d %511s %*[^\n]\n",host,servertype,&port,path);

         if(strstr(servertype,"cddb"))
         {
            sprintf(server,"%s:%d",host,port);
            return CDDB;
         }
         else if(strstr(servertype,"http"))
         {
            sprintf(server,"http://%s:%d%s",host,port,path);
            return HTTP;
         }
         else
            return NOSERVER;
      }
      else
      {
         int port;
         sscanf(buffer,"%511s %d %*[^\n]\n",host,&port);
         sprintf(server,"%s:%d",host,port);
         return CDDB;
      }
   }
   return NOSERVER;
}

char *get_line(char *in, char *out, int size)
{
   char *eol;

   if((eol = strchr(in,'\n')) == NULL)
       eol = strchr(in,'\0');

   if(eol != NULL && eol != in)
   {
      if(size > eol-in+2)
         size = eol-in+2;
      strncpy(out,in,size-1);
      out[size-1] = 0;

      return out;
   }
   else
      return NULL;
}


void CDDB_socket::parse_read_reply(char *reply)
{
   free_junk();
   char *next_line = reply;
   char buffer[1024];

   raw_reply = strdup(reply);

   while(get_line(next_line, buffer, 1024) != NULL && next_line[0] != 0)
   {
      next_line += strlen(buffer);

      char keyword[16] = {0}, data[512] = {0};

      uncomment(buffer);
      sscanf(buffer,"%15[^=]=%511[^\r\n]\r\n",keyword,data);
      if(!stricmp(keyword,"dtitle"))
      {
         char artist[512] = {0}, title[512] = {0};
         sscanf(data, "%511[^/]/%511[^\r\n]\r\n",artist,title);
         if(!title[0]) strcpy(title,artist);

         if(disctitle[0])
         {
            disctitle[0] = (char*)realloc(disctitle[0], strlen(disctitle[0])+strlen(artist)+2);
            strcat(disctitle[0], " ");
            strcat(disctitle[0], quote_strip(artist));
         }
         else
         {
            disctitle[0] = (char*)malloc(strlen(artist)+1);
            strcpy(disctitle[0], quote_strip(artist));
         }

         if(disctitle[1])
         {
            disctitle[1] = (char*)realloc(disctitle[1], strlen(disctitle[1])+strlen(title)+2);
            strcat(disctitle[1], " ");
            strcat(disctitle[1], quote_strip(artist));
         }
         else
         {
            disctitle[1] = (char*)malloc(strlen(title)+1);
            strcpy(disctitle[1], quote_strip(title));
         }
      }
      else if(!stricmp(keyword,"extd"))
      {
         if(disctitle[2])
         {
            disctitle[2] = (char*)realloc(disctitle[2], strlen(disctitle[2])+strlen(data)+2);
            strcat(disctitle[2], " ");
            strcat(disctitle[2], data);
         }
         else
         {
            disctitle[2] = (char*)malloc(strlen(data)+1);
            strcpy(disctitle[2], data);
         }
      }
      else if(strstr(strlwr(keyword),"ttitle") == keyword)
      {
         int track = -1;
         sscanf(keyword,"ttitle%d",&track);
         if(track > -1)
         {
            if(titles[track][0])
            {
               titles[track][0] = (char*)realloc(titles[track][0], strlen(titles[track][0])+strlen(data)+2);
               strcat(titles[track][0], " ");
               strcat(titles[track][0], data);
            }
            else
            {
               titles[track][0] = (char*)malloc(strlen(data)+1);
               strcpy(titles[track][0], data);
            }
         }
      }
      else if(strstr(strlwr(keyword),"extt") == keyword)
      {
         int track = -1;
         sscanf(keyword,"extt%d",&track);
         if(track > -1)
         {
            if(titles[track][1])
            {
               titles[track][1] = (char*)realloc(titles[track][1], strlen(titles[track][1])+strlen(data)+2);
               strcat(titles[track][1], " ");
               strcat(titles[track][1], data);
            }
            else
            {
               titles[track][1] = (char*)malloc(strlen(data)+1);
               strcpy(titles[track][1], data);
            }
         }
      }
   }
}


int CDDB_socket::read_req(char *category, unsigned long discid)
{
   char buffer[512];
   content = false;

   sprintf(buffer,"cddb read %s %08x\n", category, discid);
   if(write(buffer,strlen(buffer)) < 1)
      return COMMAND_ERROR;

   if(!gets(buffer, sizeof(buffer)))
      return COMMAND_ERROR;

   switch(buffer[2])
   {
      case '0':
      {
         content = true;

         char *bigbuffer = (char*) malloc(2048);
         int bigbufpos = 0;
         int bigbufsize = 2048;

         while(gets_content(bigbuffer+bigbufpos, bigbufsize-bigbufpos))
         {
            bigbufpos = strlen(bigbuffer);
            if(bigbufpos+1 >= bigbufsize) // +1, cuz we need space for '\0'
            {
                bigbuffer = (char *) realloc(bigbuffer,2*bigbufsize);
                bigbufsize *= 2;
            }
         }

         parse_read_reply(bigbuffer);
         free(bigbuffer); bigbuffer = NULL;

         return COMMAND_OK; /* found something */
      }

      case '1':
         free_junk();
         return COMMAND_OK; /* nothing found */

      case '2': displayError("CDDB Error: %s", &buffer[4]); return COMMAND_ERROR;
      case '3': displayError("CDDB Error: %s", &buffer[4]); return COMMAND_ERROR;
      case '9': displayError("CDDB Error: %s", &buffer[4]); return COMMAND_ERROR;
   }

   return process_default(buffer);
}


void CDDB_socket::free_junk()
{
   free(raw_reply); raw_reply = NULL;

   free(disctitle[0]); disctitle[0] = NULL;
   free(disctitle[1]); disctitle[1] = NULL;
   free(disctitle[2]); disctitle[2] = NULL;

   for(int i=0; i < 256; i++)
      for(int j=0; j < 2; j++)
      { free(titles[i][j]); titles[i][j] = NULL; }
}

/* returns NULL if this is the end */
char *CDDB_socket::gets_content(char *line, int size)
{
   if(content && gets(line, size))
   {
      content = (*line != '.');

      if(content)
         return line;
      else
         return NULL;
   }
   return NULL;
}



/* kludges for HTTP pass through */

bool CDDB_socket::connect(char *http_URL, char *proxy_URL, char *path)
{
   CGI = (char*) realloc(CGI, strlen(path)+1);
   strcpy(CGI,path);

   delete httpsocket;
   httpsocket = new HTTP_socket;
   httpsocket->connect(http_URL, proxy_URL);
   online = httpsocket->isOnline();
   socket_error = httpsocket->getSocketError();
   tcpip_error = httpsocket->getTcpipError();
   return online;
}

char *CDDB_socket::gets(char *buffer, int size)
{
   if(httpsocket)
   {
      char *rc = httpsocket->gets_content(buffer,size);
      online = httpsocket->isOnline();
      socket_error = httpsocket->getSocketError();
      tcpip_error = httpsocket->getTcpipError();
      return rc;
   }
   else
      return tcpip_socket::gets(buffer,size);
}

int CDDB_socket::write(char *buffer, int size)
{
   if(httpsocket)
   {
      char hello[512];
      char *last_char;

      /* the following needs some kind of overflow check, damn sprintf() */

      if(!cgi_command[0])
      {
         strcpy(cgi_command, CGI);
         strcat(cgi_command, "?cmd=");
      }

      last_char = strchr(buffer,'\0');
      if(last_char == NULL && (last_char-buffer) > size)
         return -1;

      if(*(last_char-1)=='\n') /* EOL */
      {
         *(last_char-1) = '\0'; // not too good hey
         urlncode(buffer,strchr(cgi_command,'\0'),sizeof(cgi_command) - strlen(cgi_command));
         strcat(cgi_command, "&hello=");
         sprintf(hello, "%s %s %s %s", username, hostname, PROGRAM, VERSION);
         urlncode(hello, strchr(cgi_command,'\0'), sizeof(cgi_command) - strlen(cgi_command));
         strcat(cgi_command, "&proto=");
         _itoa(protolevel,strchr(cgi_command,'\0'),10);

         if(httpsocket->get_req(persistent, cgi_command) == 200)
         {
            online = httpsocket->isOnline();
            socket_error = httpsocket->getSocketError();
            tcpip_error = httpsocket->getTcpipError();
            cgi_command[0] = 0;
            return size;
         }
         else
         {
            online = httpsocket->isOnline();
            socket_error = httpsocket->getSocketError();
            tcpip_error = httpsocket->getTcpipError();
            cgi_command[0] = 0;
            return -1;
         }
      }
      else
      {
         urlncode(buffer,strchr(cgi_command,'\0'),sizeof(cgi_command) - strlen(cgi_command));
         return size;
      }
   }
   else
      return tcpip_socket::write(buffer,size);
}

bool CDDB_socket::close()
{
   if(httpsocket)
   {
      bool rc = httpsocket->close();
      online = httpsocket->isOnline();
      socket_error = httpsocket->getSocketError();
      tcpip_error = httpsocket->getTcpipError();
      delete(httpsocket); httpsocket = NULL;
      return rc;
   }
   else
      return tcpip_socket::close();
}

bool CDDB_socket::cancel()
{
   if(httpsocket)
   {
      bool rc = httpsocket->cancel();
      online = httpsocket->isOnline();
      socket_error = httpsocket->getSocketError();
      tcpip_error = httpsocket->getTcpipError();
      return rc;
   }
   else
      return tcpip_socket::cancel();
}


#if 0

int main()
{
   CDDB_socket cddb;
   CDDBQUERY_DATA queryData;

//   cddb.tcpip_socket::connect("www.cddb.com",8880);
//   cddb.connect("http://sunsite.unc.edu",NULL,"/~cddb/cddb.cgi");
   cddb.tcpip_socket::connect("sunsite.unc.edu",8880);
//   cddb.connect("http://cddb.moonsoft.com",NULL,"/~cddb/cddb.cgi");
   cddb.banner_req();
   cddb.handshake_req("guardia","cam.org");
   cddb.query_req("M:", trackInfo, &queryData);
   cddb.read_req(queryData.category, queryData.discid_cddb);

   return 0;
}

#endif
