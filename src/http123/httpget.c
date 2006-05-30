/*
 *   httpget.c
 *
 *   Oliver Fromme  <oliver.fromme@heim3.tu-clausthal.de>
 *   Wed Apr  9 20:57:47 MET DST 1997
 */

#define INCL_DOS
#include <os2.h>
#include <stdarg.h>

#include <io.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <netdb.h>
//#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
//#include <arpa/inet.h>

#if defined(__IBMC__) || defined(__IBMCPP__)
  #include <errno.h>
#else
  #include <sys/errno.h>
#endif

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

#include "nerrno_str.h"
#include "httpget.h"

char *prgName = "MPG123.DLL_HTTP";
char *prgVersion = "1.1";

char *thread_error[100] = {0}; /* ie.: max 100 threads */

char *_System http_strerror()
{
   PTIB ptib;
   PPIB ppib;
   DosGetInfoBlocks(&ptib, &ppib);

   return thread_error[ptib->tib_ptib2->tib2_ultid];
}

void _System write_error(char *fmt, ... )
{
   PTIB ptib;
   PPIB ppib;
   va_list args;

   DosGetInfoBlocks(&ptib, &ppib);

   if(!thread_error[ptib->tib_ptib2->tib2_ultid])
      thread_error[ptib->tib_ptib2->tib2_ultid] = calloc(1024,1);

   va_start(args, fmt);
   vsprintf(thread_error[ptib->tib_ptib2->tib2_ultid],fmt,args);
   va_end(args);
}

int _System writebuffer (char *buffer, int size, int handle)
{
   int result;

   while (size) {
      if ((result = send(handle, buffer, size, 0)) < 0 && sock_errno()-10000 != EINTR) {
         write_error("send: %s",sock_strerror(sock_errno()));
         return 0;
      }
      else if (result == 0) {

         write_error("send: socket closed unexpectedly");
         return 0;
      }
      buffer += result;
      size -= result;
   }
   return 1;
}

int _System readbuffer (char *buffer, int size, int handle)
{
   int total = 0;
   int read;

   do {
      read = recv(handle, buffer+total, size-total, 0);
      if(read == 0 || read == -1) break;
//      if(!read) break;
//      if(read == -1 && sock_errno()-10000 != EINTR) break;
      total += read;
   } while (total < size);

   return total;
}

int _System readline(char *line, int maxlen, int handle)
{
   char c;
   int total = 0;

   while(readbuffer(&c,1,handle) && total < maxlen-1)
   {
      line[total++] = c;
      if(c == '\n')
      {
         line[total] = 0;
         break;
      }
   }
   line[total] = 0;

   return total;
}

void encode64 (char *source,char *destination)
{
  static char *Base64Digits =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int n = 0;
  int ssiz=strlen(source);
  int i;

  for (i = 0 ; i < ssiz ; i += 3) {
    unsigned int buf;
    buf = ((unsigned char *)source)[i] << 16;
    if (i+1 < ssiz)
      buf |= ((unsigned char *)source)[i+1] << 8;
    if (i+2 < ssiz)
      buf |= ((unsigned char *)source)[i+2];

    destination[n++] = Base64Digits[(buf >> 18) % 64];
    destination[n++] = Base64Digits[(buf >> 12) % 64];
    if (i+1 < ssiz)
      destination[n++] = Base64Digits[(buf >> 6) % 64];
    else
      destination[n++] = '=';
    if (i+2 < ssiz)
      destination[n++] = Base64Digits[buf % 64];
    else
      destination[n++] = '=';
  }
  destination[n++] = 0;
}

char *url2hostport (char *url, char **hname, unsigned long *hip, unsigned int *port)
{
   char *cptr;
   struct hostent *myhostent;
   struct in_addr myaddr;
   int isip = 1;

   if (!(strncmp(url, "http://", 7)))
      url += 7;
   cptr = url;
   while (*cptr && *cptr != ':' && *cptr != '/') {
      if ((*cptr < '0' || *cptr > '9') && *cptr != '.')
         isip = 0;
      cptr++;
   }
   *hname = strdup(url); /* removed the strndup for better portability */
   if (!(*hname)) {
      *hname = NULL;
      return (NULL);
   }
   (*hname)[cptr - url] = 0;
   if (!isip) {
      if (!(myhostent = gethostbyname(*hname)))
         return (NULL);
      memcpy (&myaddr, myhostent->h_addr, sizeof(myaddr));
      *hip = myaddr.s_addr;
   }
   else
      if ((*hip = inet_addr(*hname)) == INADDR_NONE)
         return (NULL);
   if (!*cptr || *cptr == '/') {
      *port = 80;
      return (cptr);
   }
   *port = atoi(++cptr);
   while (*cptr && *cptr != '/')
      cptr++;
   return (cptr);
}

char *proxyurl = NULL;
unsigned long proxyip = 0;
unsigned int proxyport;

#define ACCEPT_HEAD "Accept: audio/mpeg, audio/x-mpegurl, */*\r\n"

char *httpauth = NULL;

int _System http_open (const char *url, HTTP_INFO *http_info)
{
   char *purl, *host, *request, *sptr;
   int linelength;
   unsigned long myip;
   unsigned int myport;
   int sock;
   int relocate, numrelocs = 0;
   struct sockaddr_in server;

   memset(http_info,0,sizeof(*http_info));  /* fail safe */

   if (!proxyip) {
      if (!proxyurl)
         if (!(proxyurl = getenv("MP3_HTTP_PROXY")))
            if (!(proxyurl = getenv("http_proxy")))
               proxyurl = getenv("HTTP_PROXY");
      if (proxyurl && proxyurl[0] && strcmp(proxyurl, "none")) {
         if (!(url2hostport(proxyurl, &host, &proxyip, &proxyport))) {
            write_error("Unknown proxy host \"%s\".",
               host ? host : "");
            return 0;
         }
         if (host)
            free (host);
      }
      else
         proxyip = INADDR_NONE;
   }
   
   if ((linelength = strlen(url)+200) < 1024)
      linelength = 1024;
   if (!(request = malloc(linelength)) || !(purl = malloc(1024))) {
      write_error("malloc() failed, out of memory.");
      return 0;
   }
   strncpy (purl, url, 1023);
   purl[1023] = '\0';

   do {
      host = NULL;
      strcpy (request, "GET ");
      if (proxyip != INADDR_NONE) {
         if (strncmp(url, "http://", 7))
            strcat (request, "http://");
         strcat (request, purl);
         myport = proxyport;
         myip = proxyip;
      }
      else {
         if (!(sptr = url2hostport(purl, &host, &myip, &myport))) {

            write_error("Unknown host \"%s\".",
               host ? host : "");
            return 0;
         }
         if(sptr[0] == 0)
            strcat (request, "/");
         else
            strcat (request, sptr);
      }
      sprintf (request + strlen(request),
         " HTTP/1.0\r\nUser-Agent: %s/%s\r\n",
         prgName, prgVersion);
      strcat (request, ACCEPT_HEAD);
      if(host)
      {
         sprintf(request + strlen(request), "Host: %s\r\n", host);
         free(host);
      }
      strcat(request,"icy-metadata:1\r\n");
      strcat(request, "\r\n");

      server.sin_family = AF_INET;
      server.sin_port = htons(myport);
      server.sin_addr.s_addr = myip;
      if ((sock = socket(PF_INET, SOCK_STREAM, 6)) < 0) {
         write_error("socket: %s",sock_strerror(sock_errno()));
         return 0;
      }
      if (connect(sock, (struct sockaddr *)&server, sizeof(server))) {
         write_error("connect: %s",sock_strerror(sock_errno()));
         return 0;
      }

      if (httpauth) {
         char buf[1023];
         strcat (request,"Authorization: Basic "); // need proxy auth too!!
         encode64(httpauth,buf);
         strcat (request,buf);
         strcat (request,"\r\n");
      }

      if(!writebuffer (request, strlen(request), sock)) return 0;
      relocate = FALSE;
      purl[0] = '\0';
      if(!readline(request, linelength-1, sock))
      {
        write_error("Error reading from socket or unexpected EOF.");
        return 0;
      }
      if ((sptr = strchr(request, ' ')) != NULL) {
         switch (sptr[1]) {
            case '3':
               relocate = TRUE;
            case '2':
               break;
            default:
               write_error("HTTP request failed: %s",
                  sptr+1);
               return 0;
         }
      }
      do {
         if(!readline(request, linelength-1, sock))
         {
           write_error("Error reading from socket or unexpected EOF.");
           return 0;
         }
         if(strchr(request,'\r') != NULL)
           *strchr(request,'\r') = 0;

         if(strchr(request,'\n') != NULL)
           *strchr(request,'\n') = 0;

         if(!strncmp(request, "Location:", 9))
            strncpy (purl, request+10, 1023);
         else if(!strnicmp(request, "Content-Length:", 15))
            http_info->length = atoi(request+16);
         else if(!strnicmp(request, "icy-metaint:", 12))
            http_info->icy_metaint = atoi(request+12);
         else if(!strnicmp(request, "x-audiocast-name:", 17))
            strncpy(http_info->icy_name, request+17, sizeof(http_info->icy_name)-1);
         else if(!strnicmp(request, "icy-name:", 9))
            strncpy(http_info->icy_name, request+9, sizeof(http_info->icy_name)-1);
         else if(!strnicmp(request, "x-audiocast-genre:", 18))
            strncpy(http_info->icy_genre, request+18, sizeof(http_info->icy_genre)-1);
         else if(!strnicmp(request, "icy-genre:", 10))
            strncpy(http_info->icy_genre, request+10, sizeof(http_info->icy_genre)-1);

      } while (request[0] != 0 && request[0] != '\r' && request[0] != '\n');
   } while (relocate && purl[0] && numrelocs++ < 5);

   if (relocate) {
      write_error("Too many HTTP relocations.");
      return 0;
   }
   free (purl);
   free (request);
   return sock;
}

int _System http_close(int socket)
{
   return soclose(socket);

}

