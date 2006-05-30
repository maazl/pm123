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

/* TCP/IP functions */

#define INCL_PM
#include <os2.h>
#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>

#include <types.h>
#include <sys\socket.h>
#include <netinet\in.h>
#include <netdb.h>
#include <nerrno.h>

#include "nerrno_str.h"
#include "utilfct.h"
#include "tcpipsock.h"

#include "cdda.h"

tcpip_socket::~tcpip_socket()
{
   close();
}

tcpip_socket::tcpip_socket()
{
   sock = 0;
   online = false;
   socket_error = 0;
   tcpip_error = 0;
}

bool tcpip_socket::connect(int setsock)
{
   sock = setsock;
   online = true; /* let's say we are */
   return online;
}

bool tcpip_socket::connect(char *address, int port)
{
   struct sockaddr_in server;

   socket_error = 0;
   tcpip_error = 0;

   if(online) this->close();

   sock = socket(PF_INET, SOCK_STREAM, 0);

   memset(&server,0,sizeof(server));
   server.sin_family = AF_INET;
   server.sin_addr.s_addr = inet_addr(address);
   if(server.sin_addr.s_addr == -1)
   {
      struct hostent *hp = gethostbyname(address);
      if(!hp)
      {
         tcpip_error = tcp_h_errno();
         socket_error = sock_errno();

         if(tcpip_error > 0)
            displayError("TCP/IP Error: %s", h_strerror(tcpip_error));
         else if(socket_error > 0)
         {
            if(socket_error != SOCEINTR)
               displayError("Socket Error: %s", sock_strerror(socket_error));
         }
         else
            displayError("Unknown error");
         return false;
      }
      else
         server.sin_addr.s_addr = *((unsigned long *) hp->h_addr);
   }
   server.sin_port = htons(port);

/* bind doesn't work? kinda weird when you think it's should be called */
//   if( !bind(sock, (struct sockaddr*) &server, sizeof(server)) &&
//       !::connect(sock, (struct sockaddr*) &server, sizeof(server)))
   if(!::connect(sock, (struct sockaddr*) &server, sizeof(server)))
      online = true;
   else
   {
      online = false;
      socket_error = sock_errno();
      if(socket_error != SOCEINTR)
         displayError("Socket Error: %s", sock_strerror(socket_error));
   }

   return online;
}

bool tcpip_socket::close()
{
   online = false;
   return !soclose(sock);
}

char *tcpip_socket::gets(char *buffer, int size)
{
   int pos = 0;
   int stop = 0;
   int read;

   while(pos < size-1 && !stop)
   {
      read = this->read(buffer+pos, 1);
      if(read == 0 || read == -1) stop = 1;
      else
      {
         if(buffer[pos] == '\n') stop = 1;
         pos += read;
      }
   }

   buffer[pos] = '\0';

   if(read == -1)
      return NULL;
   else
      return buffer;
}

int tcpip_socket::write(char *buffer, int size)
{
   int pos = 0;
   int stop = 0;
   int sent;

   socket_error = 0;

   while(pos < size && !stop)
   {
      sent = send(sock, buffer+pos, size-pos, 0);
      if(sent == 0)
      {
         online = false;
         stop = 1;
      }
      else if(sent == -1)
      {
         online = false;
         stop = 1;
         socket_error = sock_errno();
         if(socket_error != SOCEINTR)
            displayError("Socket Error: %s", sock_strerror(socket_error));
      }
      else
         pos += sent;
   }

   writeToLog(buffer, size);

   if(sent == -1)
      return -1;
   else
      return pos;
}

int tcpip_socket::read(char *buffer, int size)
{
   int read;

   socket_error = 0;

   read = recv(sock, buffer, size, 0);
   if(read == 0) online = false;
   else if(read == -1)
   {
      online = false;
      socket_error = sock_errno();
      if(socket_error != SOCEINTR)
         displayError("Socket Error: %s", sock_strerror(socket_error));
   }

   writeToLog(buffer, read);

   return read;
}
