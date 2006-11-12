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
 *
 * The main source file to cddaplay.dll plug-in
 */

#define  INCL_BASE
#define  INCL_WIN
#include <os2.h>
#define  DECODER_PLUGIN_LEVEL 1

#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <types.h>
#include <nerrno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <process.h>

#ifndef TCPV40HDRS
#include <unistd.h>
#include <arpa/inet.h>
#endif

#include <inimacro.h>
#include <format.h>
#include <decoder_plug.h>
#include <plugin.h>

#include "readcd.h"
#include <utilfct.h>
#include "tcpipsock.h"
#include "http.h"
#include "cddb.h"
#include "cdda.h"
#include "cddarc.h"

#include <debuglog.h>

#define  WM_SAVE         (WM_USER+1)
#define  WM_UPDATE_DONE  (WM_USER+2)

static unsigned long negative_hits[1024];
static unsigned int  nh_head = 0;

// used for the PM123 decoder functions
typedef struct _CDDAPLAY
{
   CD_drive CD;
   FORMAT_INFO formatinfo;

   int (PM123_ENTRYP output_play_samples)(void *a, const FORMAT_INFO *format, const char *buf,int len, int posmarker);
   void *a; /* only to be used with the precedent function */
   int buffersize;

   HEV play,ok;
   char drive[4];
   int track;
   BOOL stop,rew,ffwd;
   int jumpto;
   int status;
   int decodertid;

   void (PM123_ENTRYP error_display)(char *);

   HWND hwnd;

   int last_length; // keeps the last length in memory to calls to
                    // decoder_length() remain valid after the file is closed

} CDDAPLAY;

static HWND dlghwnd = NULLHANDLE; // configuration dialog
static HWND nethwnd = NULLHANDLE; // network window dialog
static TID  tupdate = 0;

#define CHUNK 27

// used internally to manage CDDB and saved in cddaplay.ini file
typedef struct
{
   char *CDDBServers[128];
   char *HTTPServers[128];

   BOOL isCDDBSelect[128];
   BOOL isHTTPSelect[128];

   int numCDDB;
   int numHTTP;

   BOOL useCDDB;
   BOOL useHTTP;
   BOOL tryAllServers;
   char proxyURL[1024];
   char email[1024];
   
   char cddrive[4];          /* Default CD drive.                      */

} CDDA_SETTINGS;

CDDA_SETTINGS settings = {0};  // configuration settings


static void TFNENTRY decoder_thread(void *arg)
{
   CDDAPLAY *c = (CDDAPLAY *) arg;
   ULONG resetcount;

   while(1)
   {
      char *buffer = NULL, *startbuffer = NULL;
      int extrabytes = 0; // bytes left in the buffer from the last read
      ULONG lastsector,cursector,startsector;

      DosWaitEventSem(c->play, SEM_INDEFINITE_WAIT);
      DosResetEventSem(c->play,&resetcount);

      c->status = DECODER_STARTING;
      buffer = (char*)malloc(CHUNK*2352);
      extrabytes = 0;
      startbuffer = buffer;

      c->last_length = -1;
      DosPostEventSem(c->ok);

      if( !c->CD.open(c->drive) || !c->CD.readCDInfo() || !c->CD.fillTrackInfo())
      {
         WinPostMsg(c->hwnd,WM_PLAYERROR,0,0);
         c->status = DECODER_STOPPED;
         continue;
      }

      c->formatinfo.samplerate = 44100;
      c->formatinfo.channels = c->CD.getTrackInfo(c->track)->channels;
      c->formatinfo.bits = 16;
      c->formatinfo.format = 1; /* standard PCM */

      c->jumpto = -1;
      c->rew = c->ffwd = 0;
      c->stop = 0;

      lastsector = CD_drive::getLBA(c->CD.getTrackInfo(c->track)->end);
      startsector = cursector = CD_drive::getLBA(c->CD.getTrackInfo(c->track)->start);

// not good for playlists, what to do?
#if 0
      /* spin up the drive */
      c->CD.readSectors((CDREADLONGDATA*)buffer,1,cursector);
      DosSleep(2000);
#endif

      c->status = DECODER_PLAYING;
      c->last_length = decoder_length(c);

      while(cursector < lastsector && !c->stop)
      {
         int written;

         if(extrabytes < c->buffersize)
         {
            if(extrabytes)
               memmove(buffer,startbuffer,extrabytes);

            ULONG readLength = CHUNK-extrabytes/2352-(extrabytes%2352 ? 1 : 0);

            if(cursector+readLength > lastsector) readLength = lastsector-cursector;

            if(!c->CD.readSectors( (CDREADLONGDATA*) (buffer+extrabytes), readLength, cursector))
            {
               WinPostMsg(c->hwnd,WM_PLAYERROR,0,0);
               break;
            }

            cursector += readLength;
            startbuffer = buffer;
            extrabytes += readLength*2352;
         }

         written = c->output_play_samples(c->a, &c->formatinfo, startbuffer, c->buffersize,
            (int)((double)(cursector-startsector)*2352*1000/
                          (c->formatinfo.samplerate*c->formatinfo.channels*c->formatinfo.bits/8)));

         if(written < c->buffersize)
         {
            WinPostMsg(c->hwnd,WM_PLAYERROR,0,0);
            break;
         }

         startbuffer += c->buffersize;
         extrabytes -= c->buffersize;

         if(c->jumpto >= 0)
         {
            cursector = startsector + c->jumpto;
            c->jumpto = -1;
            extrabytes = 0;
            WinPostMsg(c->hwnd,WM_SEEKSTOP,0,0);
         }
         /* putting extrabytes 0 for rew and ffwd overloads the CD drive */
         if(c->rew)
         {
            cursector -= (c->formatinfo.samplerate/4*c->formatinfo.channels*
                         (c->formatinfo.bits/8))/2352;
         }
         if(c->ffwd)
         {
            cursector += (c->formatinfo.samplerate/4*c->formatinfo.channels*
                         (c->formatinfo.bits/8))/2352;
         }
      }
      free(buffer); buffer = NULL;
      c->status = DECODER_STOPPED;
      c->CD.close();

      WinPostMsg(c->hwnd,WM_PLAYSTOP,0,0);
      DosPostEventSem(c->ok);
   }
}

int PM123_ENTRY decoder_init(void **C)
{
   CDDAPLAY *c;

   *C = malloc(sizeof(CDDAPLAY));
   c = (CDDAPLAY *)*C;
   memset(c,0,sizeof(CDDAPLAY));

   DosCreateEventSem(NULL,&c->play,0,FALSE);
   DosCreateEventSem(NULL,&c->ok,0,FALSE);

   c->decodertid = _beginthread(decoder_thread,0,64*1024,(void *) c);
   if(c->decodertid != -1)
      return c->decodertid;
   else
   {
      DosCloseEventSem(c->play);
      DosCloseEventSem(c->ok);
      free(c);
      return -1;
   }
}

BOOL PM123_ENTRY decoder_uninit(void *C)
{
   CDDAPLAY *c = (CDDAPLAY *) C;
   int decodertid = c->decodertid;

   DosCloseEventSem(c->play);
   DosCloseEventSem(c->ok);

   free(c);

   return !DosKillThread(decodertid);
}


ULONG PM123_ENTRY decoder_command(void *C, ULONG msg, DECODER_PARAMS *params)
{
   CDDAPLAY *c = (CDDAPLAY *) C;
   ULONG resetcount;

   switch(msg)
   {
      case DECODER_PLAY:
         if(c->status == DECODER_STOPPED)
         {
            c->drive[0] = params->drive[0];
            c->drive[1] = ':';
            c->drive[2] = 0;
            c->track = params->track;
            DosResetEventSem(c->ok,&resetcount);
            DosPostEventSem(c->play);
            if(DosWaitEventSem(c->ok, 10000) == 640)
            {
               c->status = DECODER_STOPPED;
               DosKillThread(c->decodertid);
               c->decodertid = _beginthread(decoder_thread,0,64*1024,(void *) c);
               return 102;
            }
         }
         else
            return 101;
         break;

      case DECODER_STOP:
         if(c->status != DECODER_STOPPED)
         {
            DosResetEventSem(c->ok,&resetcount);
            c->stop = TRUE;
            if(DosWaitEventSem(c->ok, 10000) == 640)
            {
               c->status = DECODER_STOPPED;
               DosKillThread(c->decodertid);
               c->decodertid = _beginthread(decoder_thread,0,64*1024,(void *) c);
               return 102;
            }
         }
         else
            return 101;
         break;

      case DECODER_FFWD:
         c->ffwd = params->ffwd;
         break;

      case DECODER_REW:
         c->rew = params->rew;
         break;

      case DECODER_JUMPTO:
         c->jumpto = (int)(((double)c->formatinfo.samplerate*params->jumpto/1000)*
                                    c->formatinfo.channels*(c->formatinfo.bits/8)/2352);
         break;

      case DECODER_EQ:
         return 1;

      case DECODER_SETUP:
         c->output_play_samples = params->output_play_samples;
         c->a = params->a;
         c->buffersize = params->audio_buffersize;
         c->error_display = params->error_display;
         c->CD.error_display = params->error_display;
         c->hwnd = params->hwnd;
         break;
   }
   return 0;
}

ULONG PM123_ENTRY decoder_length(void *C)
{
   CDDAPLAY *c = (CDDAPLAY *) C;

   if(c->status == DECODER_PLAYING)
      return (int)((double)c->CD.getTrackInfo(c->track)->size*1000/(c->formatinfo.samplerate*
                   c->formatinfo.channels*c->formatinfo.bits/8));
   else
      return c->last_length;
}

ULONG PM123_ENTRY decoder_status(void *C)
{
   CDDAPLAY *c = (CDDAPLAY *) C;
   return c->status;
}

ULONG PM123_ENTRY decoder_fileinfo(const char *filename, DECODER_INFO *info)
{
   return 200;
}

// we might need a mutex if this gets out of hands
static CD_drive lastCD;
static CDAUDIODISKINFODATA lastCDInfo = {0};
static CDDB_socket *lastCDDBSocket = NULL;
static CDDBQUERY_DATA lastQueryData = {0};

// this is REALLY slow if size is small and the window is visible...
void appendToMLE(HWND MLEhwnd, char *buffer, int size)
{
   ULONG iMLESize;
   if(size == -1) size = strlen(buffer);

   /* Set the MLE import-export buffer */
   WinSendMsg(MLEhwnd,
              MLM_SETIMPORTEXPORT,
              MPFROMP(buffer),
              MPFROMSHORT((USHORT) size));

   /* Set MLM_FORMAT for line feed EOL */
   WinSendMsg(MLEhwnd,
              MLM_FORMAT,
              MPFROMSHORT(MLFIE_NOTRANS),
              MPFROMLONG(0));

   /* Find out how much text is in the MLE */
   iMLESize = (ULONG) WinSendMsg(MLEhwnd,
                                 MLM_QUERYFORMATTEXTLENGTH,
                                 MPFROMLONG(0),
                                 MPFROMLONG(-1));

   /* Append the new text */
   WinSendMsg(MLEhwnd,
              MLM_IMPORT,
              MPFROMP(&iMLESize),
              MPFROMLONG(size));


   iMLESize = (ULONG) WinSendMsg(MLEhwnd,
                                 MLM_QUERYFORMATTEXTLENGTH,
                                 MPFROMLONG(0),
                                 MPFROMLONG(-1));

   WinSendMsg(MLEhwnd,
              MLM_SETSEL,
              MPFROMLONG(iMLESize),
              MPFROMLONG(iMLESize));
}

void writeToLog(char *buffer, int size)
{
   HWND MLEhwnd = WinWindowFromID(nethwnd,MLE_NETLOG);
   appendToMLE(MLEhwnd, buffer, size);
}

void displayMessage(char *fmt, ...)
{
   HWND MLEhwnd = WinWindowFromID(nethwnd,MLE_NETLOG);

   va_list args;
   char buffer[2048];

   va_start(args, fmt);
   vsprintf(buffer,fmt,args);
   va_end(args);

   strcat(buffer,"\n");

   appendToMLE(MLEhwnd, buffer, -1);
}

void displayError(char *fmt, ...)
{
   HWND MLEhwnd = WinWindowFromID(nethwnd,MLE_NETLOG);

   va_list args;
   char buffer[2048] = "Error: ";

   va_start(args, fmt);
   vsprintf(strchr(buffer,0),fmt,args);
   va_end(args);

   strcat(buffer,"\n");

   appendToMLE(MLEhwnd, buffer, -1);
}

#define NOSERVER 0
#define CDDB     1
#define HTTP     2

ULONG getNextCDDBServer(char *server, ULONG size, SHORT *index)
{
   if(settings.useHTTP)
   {
      (*index)++;
      if(!settings.tryAllServers)
      {
         while(*index < settings.numHTTP && !settings.isHTTPSelect[*index])
            (*index)++;
      }

      if(*index < settings.numHTTP)
      {
         strlcpy(server, settings.HTTPServers[*index], size);
         return HTTP;
      }
      else
         return NOSERVER;
   }
   else
   {
      (*index)++;
      if(!settings.tryAllServers)
      {
         while(*index < settings.numCDDB && !settings.isCDDBSelect[*index])
            (*index)++;
      }

      if(*index < settings.numCDDB)
      {
         strlcpy(server, settings.CDDBServers[*index], size);
         return CDDB;
      }
      else
         return NOSERVER;
   }
}

void getUserHost(char *user, int sizeuser, char *host, int sizehost)
{
   char *buffer = strdup(settings.email);
   char *hostpos = strchr(buffer,'@');
   *hostpos++ = 0;

   strlcpy(user,buffer,sizeuser);
   strlcpy(host,hostpos,sizehost);

   free(buffer);
}

/* for multiple match dialog */
struct FUZZYMATCHCREATEPARAMS
{
   CDDBQUERY_DATA *matches, chosen;
};

// window procedure for the fuzzy match dialog
MRESULT EXPENTRY wpMatch(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{
   FUZZYMATCHCREATEPARAMS *data = (FUZZYMATCHCREATEPARAMS *) WinQueryWindowULong(hwnd,QWL_USER);

   switch(msg)
   {
      case WM_INITDLG:
      {
         HWND focusHwnd = (HWND) mp1;
         int  i = 0;
         char title[128];

         data = (FUZZYMATCHCREATEPARAMS *) mp2;

         WinSetFocus( HWND_DESKTOP, focusHwnd );
         WinSetWindowULong( hwnd, QWL_USER, (ULONG)data );
         do_warpsans( hwnd );

         while(data->matches[i].discid_cddb)
         {
            // cddb is always ISO-8859-1 by definition unless we implement protocol level 6
            ch_convert( 1004, data->matches[i].title, CH_CP_NONE, title, sizeof(title));
            int inserted = lb_add_item(hwnd,LB_MATCHES,data->matches[i].title);
            lb_set_handle(hwnd, LB_MATCHES,inserted,&data->matches[i]);
            i++;
         }
         return 0;
      }

      case WM_COMMAND:
      {
         switch(SHORT1FROMMP(mp1))
         {
            case DID_OK:
            {
               int selected = lb_selected(hwnd, LB_MATCHES, LIT_FIRST);
               if(selected != LIT_NONE)
                  memcpy(&data->chosen,lb_get_handle(hwnd,LB_MATCHES,selected),sizeof(CDDBQUERY_DATA));
               WinPostMsg(hwnd,WM_CLOSE,0,0);
            }
            case DID_CANCEL:
               WinPostMsg(hwnd,WM_CLOSE,0,0);
         }
         return 0;
      }
   }
   return WinDefDlgProc(hwnd,msg,mp1,mp2);
}


void loadCDDBInfo(void)
{
   BOOL  foundCached = FALSE;
   HINI  INIhandle;
   BOOL  success = FALSE;
   char  server[1024];
   ULONG serverType;
   SHORT index = -1;

   unsigned int i;

   delete lastCDDBSocket; lastCDDBSocket = NULL;
   lastCDDBSocket = new CDDB_socket;

   memset(lastQueryData.category, 0, sizeof(lastQueryData.category));
   memset(lastQueryData.title, 0, sizeof(lastQueryData.title));
   lastQueryData.discid_cd = lastCDDBSocket->discid(&lastCD);
   lastQueryData.discid_cddb = 0;

   for( i = 0; i < sizeof( negative_hits ) / sizeof( negative_hits[0] ); i++ ) {
     if( negative_hits[i] == lastQueryData.discid_cd ) {
       displayMessage( "Found cached negative hit: %08x", lastQueryData.discid_cd );
       goto end;
     }
   }

   if((INIhandle = open_module_ini()) != NULLHANDLE)
   {
      ULONG cdlistSize;
      char *cdlistBuffer;

      if(PrfQueryProfileSize(INIhandle, "CDInfo", NULL, &cdlistSize) && cdlistSize > 0)
      {
         cdlistBuffer = (char *) calloc(cdlistSize, 1);
         char *next_discid = cdlistBuffer;

         PrfQueryProfileData(INIhandle, "CDInfo", NULL, cdlistBuffer, &cdlistSize);
         while(next_discid[0] != '\0' && !foundCached)
         {
            unsigned long discid = 0;
            sscanf(next_discid, "%lx[^\r\n]", &discid);

            if(discid == lastQueryData.discid_cd) // found it cached!
            {
               ULONG cdinfoSize = 0;
               char *cdinfoBuffer = NULL;

               if(PrfQueryProfileSize(INIhandle, "CDInfo", next_discid, &cdinfoSize) && cdinfoSize > 0)
               {
                  cdinfoBuffer = (char *) calloc(cdinfoSize, 1);

                  if(PrfQueryProfileString(INIhandle, "CDInfo", next_discid, "", cdinfoBuffer, cdinfoSize))
                  {
                     foundCached = TRUE;
                     lastCDDBSocket->parse_read_reply(cdinfoBuffer);
                  }
               }
               free(cdinfoBuffer);
            }
            next_discid = strchr(next_discid,'\0')+1;
         }
         free(cdlistBuffer);
      }
      close_ini(INIhandle);
   }

   if(foundCached)
      return;

   // otherwize, go on the Internet

   if( settings.useCDDB ) {
     do
     {
        serverType = getNextCDDBServer(server,sizeof(server),&index);
        if(serverType != NOSERVER)
        {
           if(serverType == CDDB)
           {
              displayMessage("Contacting: %s", server);

              char host[512]=""; int port=8880;
              sscanf(server,"%[^:\n\r]:%d",host,&port);
              if(host[0])
                 lastCDDBSocket->tcpip_socket::connect(host,port);
           }
           else
           {
              char host[512], path[512], *slash = NULL, *http = NULL;

              http = strstr(server,"http://");
              if(http)
                 slash = strchr(http+7,'/');
              if(slash)
              {
                 strlcpy(host,server,slash-server);
                 strcpy(path,slash);
                 if(settings.proxyURL)
                    displayMessage("Contacting: %s", settings.proxyURL);
                 else
                    displayMessage("Contacting: %s", server);
                 lastCDDBSocket->connect(host,settings.proxyURL,path);
              }
           }

           if(!lastCDDBSocket->isOnline())
           {
              if(lastCDDBSocket->getSocketError() == SOCEINTR)
                 goto end;
              else
                 continue;
           }
           displayMessage("Looking at CDDB Server Banner at %s", server);
           if(!lastCDDBSocket->banner_req())
           {
              if(lastCDDBSocket->getSocketError() == SOCEINTR)
                 goto end;
              else
                 continue;
           }
           displayMessage("Handshaking with CDDB Server %s", server);
           char user[128]="",host[128]="";
           getUserHost(user,sizeof(user),host,sizeof(host));
           if(!lastCDDBSocket->handshake_req(user,host))
           {
              if(lastCDDBSocket->getSocketError() == SOCEINTR)
                 goto end;
              else
                 continue;
           }

           displayMessage("Looking for CD information on %s", server);
           int rc = lastCDDBSocket->query_req(&lastCD, &lastQueryData);
           if(!rc)
           {
              if(lastCDDBSocket->getSocketError() == SOCEINTR)
                 goto end;
              else
                 continue;
           }
           else if(rc == -1)
              continue; /* no match for this server */
           else if(rc == COMMAND_MORE)
           {
              FUZZYMATCHCREATEPARAMS fuzzyParams = {0};
              HMODULE module;
              char modulename[128];
              int i = 0;

              fuzzyParams.matches = (CDDBQUERY_DATA *) malloc(sizeof(CDDBQUERY_DATA));

              while(lastCDDBSocket->get_query_req(&fuzzyParams.matches[i]))
              {
                 fuzzyParams.matches[i].discid_cd = lastQueryData.discid_cd;
                 i++;
                 fuzzyParams.matches = (CDDBQUERY_DATA *) realloc(fuzzyParams.matches, (i+1)*sizeof(CDDBQUERY_DATA));
              }
              memset(&fuzzyParams.matches[i],0,sizeof(fuzzyParams.matches[i]));
              getModule(&module,modulename,128);
              WinDlgBox(HWND_DESKTOP, HWND_DESKTOP, wpMatch, module, DLG_MATCH, &fuzzyParams);
              memcpy(&lastQueryData,&fuzzyParams.chosen,sizeof(CDDBQUERY_DATA));
              free(fuzzyParams.matches);
           }
           /* else there is only one match */

           if(lastQueryData.discid_cddb != 0)
           {
              displayMessage("Requesting CD information from %s", server);
              if(!lastCDDBSocket->read_req(lastQueryData.category, lastQueryData.discid_cddb))
              {
                 if(lastCDDBSocket->getSocketError() == SOCEINTR)
                    goto end;
                 else
                    continue;
              }
              else
                 success = TRUE;
           }
        }
     } while(serverType != NOSERVER && !success);
   }

   if( !success && settings.useCDDB ) {
     negative_hits[ nh_head++ ] = lastQueryData.discid_cd;
     if( nh_head >= sizeof( negative_hits ) / sizeof( negative_hits[0] )) {
       nh_head = 0;
     }
   }

   end:

   if(success)
   {
      // let's cache it!
      if((INIhandle = open_module_ini()) != NULLHANDLE)
      {
         char discid[32];
         sprintf(discid, "%08lx",lastQueryData.discid_cd);

         PrfWriteProfileString(INIhandle, "CDInfo", discid, lastCDDBSocket->get_raw_reply());

         close_ini(INIhandle);
      }
   }
   else
   {
      delete lastCDDBSocket;
      lastCDDBSocket = NULL;

      memset(lastQueryData.category, 0, sizeof(lastQueryData.category));
      memset(lastQueryData.title, 0, sizeof(lastQueryData.title));
      lastQueryData.discid_cddb = 0;
   }

}

ULONG PM123_ENTRY decoder_trackinfo(const char *drive, int track, DECODER_INFO *info)
{
   char *temp;

   memset(info,0,sizeof(*info));
   info->size = sizeof(*info);

   if(lastCD.getDriveLetter() == NULL || drive[0] != (lastCD.getDriveLetter())[0])
   {
      if(!lastCD.open(drive) || !lastCD.readCDInfo() || !lastCD.fillTrackInfo())
         return 100;

      delete lastCDDBSocket; lastCDDBSocket = NULL;
      lastCDDBSocket = new CDDB_socket;

      memset(lastQueryData.category, 0, sizeof(lastQueryData.category));
      memset(lastQueryData.title, 0, sizeof(lastQueryData.title));
      lastQueryData.discid_cddb = 0;
   }
   else
   {
      if(!lastCD.readCDInfo())
         return 100;

      if(memcmp(lastCD.getCDInfo(), &lastCDInfo, sizeof(lastCDInfo)) != 0)
      {
         if(!lastCD.fillTrackInfo())
            return 100;

         delete lastCDDBSocket; lastCDDBSocket = NULL;
         lastCDDBSocket = new CDDB_socket;

         memset(lastQueryData.category, 0, sizeof(lastQueryData.category));
         memset(lastQueryData.title, 0, sizeof(lastQueryData.title));
         lastQueryData.discid_cddb = 0;
      }
   }

   // maybe the user wants to retry, so we always need to try to load it
   if(lastQueryData.discid_cddb == 0)
      loadCDDBInfo();

   lastCDInfo = *lastCD.getCDInfo();

   CDTRACKINFO *trackinfo = lastCD.getTrackInfo(track);
   if(trackinfo == NULL)
      return 100;

   // data track
   if(trackinfo->data)
      return 200;

   info->songlength = (int)((double)lastCD.getTrackInfo(track)->size*1000/
          (44100*lastCD.getTrackInfo(track)->channels*2));

   info->startsector = CD_drive::getLBA(lastCD.getTrackInfo(track)->start);
   info->endsector = CD_drive::getLBA(lastCD.getTrackInfo(track)->end);

   info->format.samplerate = 44100;
   info->format.bits = 16;
   info->format.channels = lastCD.getTrackInfo(track)->channels;
   info->bitrate = info->format.samplerate * info->format.bits * info->format.channels / 1000;
   info->format.format = 1; /* standard PCM */

   strcpy(info->tech_info,"True CD Quality");

   if(lastCDDBSocket != NULL)
   {
      temp = lastCDDBSocket->get_track_title(track-1,0);
      if(temp != NULL)
         strlcpy(info->title, temp, 128);

      temp = lastCDDBSocket->get_disc_title(0);
      if(temp != NULL)
         strlcpy(info->artist, temp, 128);

      temp = lastCDDBSocket->get_disc_title(1);
      if(temp != NULL)
         strlcpy(info->album, temp, 128);

      info->year[0] = 0;
      temp = lastCDDBSocket->get_disc_title(2);
      if(temp != NULL)
         strlcpy(info->comment, temp, 128);
      strcat(info->comment, " ");
      temp = lastCDDBSocket->get_track_title(track-1,1);
      if(temp != NULL)
         strlcat(info->comment, temp, 128);

      strlcpy(info->genre, lastQueryData.category, 128);
   }

//   lastCD.close();

   return 0;
}

ULONG PM123_ENTRY decoder_cdinfo(const char *drive, DECODER_CDINFO *info)
{
   CD_drive CD;

   CD.open(drive);
   CD.readCDInfo();

   info->sectors = CD_drive::getLBA(CD.getCDInfo()->leadOutAddress);
   info->firsttrack = CD.getCDInfo()->firstTrack;
   info->lasttrack = CD.getCDInfo()->lastTrack;

   CD.close();

   return 0;
}

ULONG PM123_ENTRY decoder_support(char *ext[], int *size)
{
   if(size)
      *size = 0;

   return DECODER_TRACK;
}

void save_ini()
{
   HINI INIhandle;
   int  i;

   if((INIhandle = open_module_ini()) != NULLHANDLE)
   {
      save_ini_value(INIhandle,settings.useCDDB);
      save_ini_value(INIhandle,settings.useHTTP);
      save_ini_value(INIhandle,settings.tryAllServers);
      save_ini_string(INIhandle,settings.proxyURL);
      save_ini_string(INIhandle,settings.email);
      save_ini_string(INIhandle,settings.cddrive);

      PrfWriteProfileData(INIhandle, "CDDBServers", NULL, NULL, 0);
      PrfWriteProfileData(INIhandle, "HTTPServers", NULL, NULL, 0);

      for(i = 0; i < settings.numCDDB; i++)
      {
         PrfWriteProfileData(INIhandle, "CDDBServers", settings.CDDBServers[i], (void*) &settings.isCDDBSelect[i], 4);
      }

      for(i = 0; i < settings.numHTTP; i++)
      {
         PrfWriteProfileData(INIhandle, "HTTPServers", settings.HTTPServers[i], (void*) &settings.isHTTPSelect[i], 4);
      }

      close_ini(INIhandle);
   }
}

void load_ini()
{
   HINI INIhandle;

   for(int i = 0; i < 128; i++)
   {
      free(settings.CDDBServers[i]); settings.CDDBServers[i] = NULL;
      free(settings.HTTPServers[i]); settings.HTTPServers[i] = NULL;

      settings.isCDDBSelect[i] = FALSE;            
      settings.isHTTPSelect[i] = FALSE;
   }

   settings.numCDDB       = 0;
   settings.numHTTP       = 0;
   settings.useCDDB       = TRUE;
   settings.useHTTP       = FALSE;
   settings.tryAllServers = TRUE;
   settings.proxyURL[0]   = 0;
   settings.cddrive[0]    = 0;

   strcpy( settings.email,"someone@somewhere.com" );

   if((INIhandle = open_module_ini()) != NULLHANDLE)
   {
      ULONG bufferSize;
      char *buffer;

      load_ini_value ( INIhandle, settings.useCDDB );
      load_ini_value ( INIhandle, settings.useHTTP );
      load_ini_value ( INIhandle, settings.tryAllServers );
      load_ini_string( INIhandle, settings.proxyURL, sizeof settings.proxyURL );
      load_ini_string( INIhandle, settings.email,    sizeof settings.email );
      load_ini_string( INIhandle, settings.cddrive,  sizeof settings.cddrive );

      if(PrfQueryProfileSize(INIhandle, "CDDBServers", NULL, &bufferSize) && bufferSize > 0)
      {
         int i;
         buffer = (char *) calloc(bufferSize, 1);
         char *next_string = buffer;

         PrfQueryProfileData(INIhandle, "CDDBServers", NULL, buffer, &bufferSize);

         for( i = 0; i < 128; i++)
         {
            if(next_string[0] == '\0')
               break;
            settings.CDDBServers[i] = strdup(next_string);

            BOOL isSelected = FALSE;
            ULONG isSelectedSize = 4;
            if(PrfQueryProfileData(INIhandle, "CDDBServers", next_string, (void*) &isSelected, &isSelectedSize))
               if(isSelected)
                  settings.isCDDBSelect[i] = TRUE;

            next_string = strchr(next_string,'\0') + 1;
         }
         settings.numCDDB = i;
         free(buffer);
      } else {
        settings.CDDBServers[0] = strdup( "freedb.freedb.org:8880" );
        settings.CDDBServers[1] = strdup( "at.freedb.org:8880" );
        settings.CDDBServers[2] = strdup( "au.freedb.org:8880" );
        settings.CDDBServers[3] = strdup( "ca.freedb.org:8880" );
        settings.CDDBServers[4] = strdup( "de.freedb.org:8880" );
        settings.CDDBServers[5] = strdup( "es.freedb.org:8880" );
        settings.CDDBServers[6] = strdup( "fi.freedb.org:8880" );
        settings.CDDBServers[7] = strdup( "ru.freedb.org:8880" );
        settings.CDDBServers[8] = strdup( "uk.freedb.org:8880" );
        settings.CDDBServers[9] = strdup( "us.freedb.org:8880" );
        settings.numCDDB = 10;
      }

      if(PrfQueryProfileSize(INIhandle, "HTTPServers", NULL, &bufferSize) && bufferSize > 0)
      {
         int i;
         buffer = (char *) calloc(bufferSize, 1);
         char *next_string = buffer;

         PrfQueryProfileData(INIhandle, "HTTPServers", NULL, buffer, &bufferSize);

         for(i = 0; i < 128; i++)
         {
            if(next_string[0] == '\0')
               break;
            settings.HTTPServers[i] = strdup(next_string);

            BOOL isSelected = FALSE;
            ULONG isSelectedSize = 4;
            if(PrfQueryProfileData(INIhandle, "HTTPServers", next_string, (void*) &isSelected, &isSelectedSize))
               if(isSelected)
                  settings.isHTTPSelect[i] = TRUE;

            next_string = strchr(next_string,'\0') + 1;
         }
         settings.numHTTP = i;
         free(buffer);
      } else {
        settings.HTTPServers[0] = strdup( "http://freedb.freedb.org:80/~cddb/cddb.cgi" );
        settings.HTTPServers[1] = strdup( "http://at.freedb.org:80/~cddb/cddb.cgi" );
        settings.HTTPServers[2] = strdup( "http://au.freedb.org:80/~cddb/cddb.cgi" );
        settings.HTTPServers[3] = strdup( "http://ca.freedb.org:80/~cddb/cddb.cgi" );
        settings.HTTPServers[4] = strdup( "http://de.freedb.org:80/~cddb/cddb.cgi" );
        settings.HTTPServers[5] = strdup( "http://es.freedb.org:80/~cddb/cddb.cgi" );
        settings.HTTPServers[6] = strdup( "http://fi.freedb.org:80/~cddb/cddb.cgi" );
        settings.HTTPServers[7] = strdup( "http://ru.freedb.org:80/~cddb/cddb.cgi" );
        settings.HTTPServers[8] = strdup( "http://uk.freedb.org:80/~cddb/cddb.cgi" );
        settings.HTTPServers[9] = strdup( "http://us.freedb.org:80/~cddb/cddb.cgi" );
        settings.numHTTP = 10;
      }

      close_ini(INIhandle);
   }
}

static VOID TFNENTRY
cddb_update( void* arg )
{
   char  server[512];
   ULONG serverType;
   SHORT index = LIT_FIRST;
   HAB   hab   = WinInitialize( 0 );
   HMQ   hmq   = WinCreateMsgQueue( hab, 0 );
   HWND  hwnd  = (HWND)arg;

   CDDB_socket *cddbSocket = new CDDB_socket;

   do
   {
      serverType = getNextCDDBServer(server,sizeof(server),&index);
      if(serverType != NOSERVER)
      {
         if(serverType == CDDB)
         {
            displayMessage("Contacting: %s", server);

            char host[512]=""; int port=8880;
            sscanf(server,"%[^:\n\r]:%d",host,&port);
            if(host[0])
               cddbSocket->tcpip_socket::connect(host,port);
         }
         else
         {
            char host[512], path[512], *slash = NULL, *http = NULL;

            http = strstr(server,"http://");
            if(http)
               slash = strchr(http+7,'/');
            if(slash)
            {
               strlcpy(host,server,slash-server);
               strcpy(path,slash);
               if(strlen(settings.proxyURL) > 0)
                  displayMessage("Contacting: %s", settings.proxyURL);
               else
                  displayMessage("Contacting: %s", server);
               cddbSocket->connect(host,settings.proxyURL,path);
            }
            else
               displayError("Invalid URL: %s", server);
         }

         if(!cddbSocket->isOnline())
         {
            if(cddbSocket->getSocketError() == SOCEINTR)
               goto end;
            else
               continue;
         }
         displayMessage("Looking at CDDB Server Banner at %s", server);
         if(!cddbSocket->banner_req())
         {
            if(cddbSocket->getSocketError() == SOCEINTR)
               goto end;
            else
               continue;
         }
         displayMessage("Handshaking with CDDB Server %s", server);
         char user[1024],host[1024];
         getUserHost(user,sizeof(user),host,sizeof(host));
         if(!cddbSocket->handshake_req(user,host))
         {
            if(cddbSocket->getSocketError() == SOCEINTR)
               goto end;
            else
               continue;
         }
         displayMessage("Requesting Sites from CDDB Server %s", server);
         if(!cddbSocket->sites_req())
         {
            if(cddbSocket->getSocketError() == SOCEINTR)
               goto end;
            else
               continue;
         }

         displayMessage("Reading Sites from CDDB Server %s", server);
         int newservertype;
         while((newservertype = cddbSocket->get_sites_req(server, sizeof(server))) != NOSERVER)
         {
            if(newservertype == CDDB)
            {
               if(lb_search(hwnd, LB_CDDBSERVERS, LIT_FIRST, server) < 0)
                  lb_add_item(hwnd, LB_CDDBSERVERS, server);
            }
            else if(newservertype == HTTP)
            {
               if(lb_search(hwnd, LB_HTTPCDDBSERVERS, LIT_FIRST, server) < 0)
                  lb_add_item(hwnd, LB_HTTPCDDBSERVERS, server);
            }
         }
      }

   } while(serverType != NOSERVER);

   end:

   delete cddbSocket;
   cddbSocket = NULL;

   WinPostMsg( hwnd, WM_UPDATE_DONE, 0, 0 );
   WinDestroyMsgQueue( hmq );
   WinTerminate( hab );
   _endthread();
}


/****************************************************************************
*
*  GUI stuff
*
****************************************************************************/

/* Processes messages of the dialog of addition of CD tracks. */
static MRESULT EXPENTRY
add_tracks_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg ) {
    case WM_CONTROL:
      if( SHORT1FROMMP(mp1) == CB_DRIVE && SHORT2FROMMP(mp1) == CBN_EFCHANGE ) {
        WinPostMsg( hwnd, WM_COMMAND,
                    MPFROMSHORT( PB_REFRESH ), MPFROM2SHORT( CMDSRC_OTHER, FALSE ));
      }
      break;

    case WM_COMMAND:
      if( COMMANDMSG(&msg)->cmd == PB_REFRESH )
      {
        char cdurl[32] = "cdda:///";
        BOOL multiselect = WinQueryWindowULong( hwnd, QWL_USER );
        int  i;

        DECODER_CDINFO cdinfo;
        DECODER_INFO   trackinfo;

        WinQueryDlgItemText( hwnd, CB_DRIVE, sizeof( cdurl )-8, cdurl+8 );
        WinSendDlgItemMsg( hwnd, LB_TRACKS, LM_DELETEALL, 0, 0 );

        if( decoder_cdinfo( cdurl+8, &cdinfo ) == 0 ) {
          if( cdinfo.firsttrack ) {
            for( i = cdinfo.firsttrack; i <= cdinfo.lasttrack; i++ ) {
              //sprintf( cdurl+10, "/Track %02d", i );
              if ( decoder_trackinfo( cdurl+8, i, &trackinfo) == 0 &&
                   *trackinfo.title != 0 ) {
                WinSendDlgItemMsg( hwnd, LB_TRACKS, LM_INSERTITEM,
                                   MPFROMSHORT( LIT_END ), MPFROMP( trackinfo.title ));
              } else {
                sprintf( cdurl+10, "/Track %02d", i );
                WinSendDlgItemMsg( hwnd, LB_TRACKS, LM_INSERTITEM,
                                   MPFROMSHORT( LIT_END ), MPFROMP( cdurl+11 ));
              }
            }
            if( multiselect ) {
              for( i = cdinfo.firsttrack; i <= cdinfo.lasttrack; i++ ) {
                WinSendDlgItemMsg( hwnd, LB_TRACKS, LM_SELECTITEM,
                                   MPFROMSHORT( i - cdinfo.firsttrack ),
                                   MPFROMLONG( TRUE ));
              }
            } else {
              WinSendDlgItemMsg( hwnd, LB_TRACKS, LM_SELECTITEM,
                                 MPFROMSHORT( 0 ), MPFROMLONG( TRUE ));
            }
          }
        }

        return 0;
      }
      break;

    case WM_ADJUSTWINDOWPOS:
      {
        PSWP pswp = (PSWP)mp1;
        DEBUGLOG2(("amp_add_tracks_dlg_proc: WM_ADJUSTWINDOWPOS: {%x, %d %d, %d %d}\n",
          pswp->fl, pswp->x, pswp->y, pswp->cx, pswp->cy));

        if ( pswp->fl & SWP_SIZE )
        { 
          SWP swpold;
          WinQueryWindowPos( hwnd, &swpold );
          if ( pswp->cx < 280 ) {
            pswp->cx = 280;
            pswp->x = swpold.x;
          }
          if ( pswp->cy < 250 ) {
            pswp->cy = 250;
            pswp->y = swpold.y;
          }
        }
      }
      break;

    case WM_WINDOWPOSCHANGED:
      {
        PSWP pswp = (PSWP)mp1;
        DEBUGLOG2(("amp_add_tracks_dlg_proc: WM_WINDOWPOSCHANGED: {%x, %d %d, %d %d} {%d %d, %d %d}\n",
          pswp->fl, pswp->x, pswp->y, pswp->cx, pswp->cy, pswp[1].x, pswp[1].y, pswp[1].cx, pswp[1].cy));
        
        if ( (pswp[0].fl & SWP_SIZE) && pswp[1].cx ) {
          HWND hwnd_listbox = WinWindowFromID( hwnd, LB_TRACKS );
          // move/resize all controls
          HENUM henum = WinBeginEnumWindows( hwnd );
          LONG dx = pswp[0].cx - pswp[1].cx;
          LONG dy = pswp[0].cy - pswp[1].cy;
          SWP swp_temp;
          
          for (;;) {
            HWND hwnd_child = WinGetNextWindow( henum );
            if ( hwnd_child == NULLHANDLE ) break;
            WinQueryWindowPos( hwnd_child, &swp_temp );
            if ( hwnd_child != hwnd_listbox ) {
              WinSetWindowPos( hwnd_child, NULLHANDLE, swp_temp.x, swp_temp.y + dy, 0, 0, SWP_MOVE );
            } else {
              WinSetWindowPos( hwnd_listbox, NULLHANDLE, 0, 0, swp_temp.cx + dx, swp_temp.cy + dy, SWP_SIZE );
            }
          }
          WinEndEnumWindows( henum );
        }
      }
      break;
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Adds CD tracks to the playlist or load one to the player. */
static ULONG
load_wizzard( HWND owner, char* select, ULONG size, BOOL multiselect )
{
  HFILE   hcdrom;
  ULONG   action;
  HWND    hwnd;
  HMODULE mod;
  #if DEBUG
  char* select_old = select;
  #endif
  DEBUGLOG(("cddaplay:load_wizzard(%p, %p, %d, %d)\n", owner, select, size, multiselect));  

  getModule( &mod, NULL, 0 );
  hwnd = WinLoadDlg( HWND_DESKTOP, owner, add_tracks_dlg_proc, mod, DLG_TRACK, 0 );
  DEBUGLOG(("cddaplay:load_wizzard: hwnd=%p\n", hwnd));  

  if( hwnd == NULLHANDLE ) {
    return 500;
  }

  WinSetWindowULong( hwnd, QWL_USER, multiselect );
  do_warpsans( hwnd );

  if( multiselect ) {
    WinSetWindowBits( WinWindowFromID( hwnd, LB_TRACKS ), QWL_STYLE, 1, LS_MULTIPLESEL );
    WinSetWindowBits( WinWindowFromID( hwnd, LB_TRACKS ), QWL_STYLE, 1, LS_EXTENDEDSEL );
    WinSetWindowText( hwnd, "Add Tracks" );
  } else {
    WinSetWindowText( hwnd, "Load Track" );
  }

  if( DosOpen( "\\DEV\\CD-ROM2$", &hcdrom, &action, 0,
               FILE_NORMAL, OPEN_ACTION_OPEN_IF_EXISTS,
               OPEN_SHARE_DENYNONE | OPEN_ACCESS_READONLY, NULL ) == NO_ERROR )
  {
    struct {
      USHORT count;
      USHORT first;
    } cd_info;

    char  drive[3] = "X:";
    ULONG len = sizeof( cd_info );
    ULONG i;

    if( DosDevIOCtl( hcdrom, 0x82, 0x60, NULL, 0, NULL,
                             &cd_info, len, &len ) == NO_ERROR )
    {
      for( i = 0; i < cd_info.count; i++ ) {
        drive[0] = 'A' + cd_info.first + i;
        WinSendDlgItemMsg( hwnd, CB_DRIVE, LM_INSERTITEM,
                           MPFROMSHORT( LIT_END ), MPFROMP( drive ));
      }
    }
    DosClose( hcdrom );

    if( *settings.cddrive ) {
      WinSetDlgItemText( hwnd, CB_DRIVE, settings.cddrive );
    } else {
      WinSendDlgItemMsg( hwnd, CB_DRIVE, LM_SELECTITEM,
                         MPFROMSHORT( 0 ), MPFROMSHORT( TRUE ));
    }
  }

  action = WinProcessDlg( hwnd ) == DID_OK ? 0 : 300;
  if( action == 0 ) {
    SHORT selected =
          SHORT1FROMMR( WinSendDlgItemMsg( hwnd, LB_TRACKS, LM_QUERYSELECTION,
                        MPFROMSHORT( LIT_FIRST ), 0 ));

    WinQueryDlgItemText( hwnd, CB_DRIVE, sizeof( settings.cddrive ), settings.cddrive );

    while( selected != LIT_NONE ) {
      DEBUGLOG(("cddaplay:load_wizzard: selected = %d, size = %d\n", selected, size));
      if( size < 20 ) {
        action = 100;
        break;
      }
      int len = sprintf( select, "cd:///%s\\Track %02d", settings.cddrive, selected+1 ) +1;
      size -= len;
      select += len;
      if ( !multiselect )
        break; // OS/2 seems to return always the same item in single select mode
      selected = SHORT1FROMMR( WinSendDlgItemMsg( hwnd, LB_TRACKS, LM_QUERYSELECTION,
                               MPFROMSHORT( selected ), 0 ));
    }
    // Double zero termination
    *select = 0;
  }
  WinDestroyWindow( hwnd );
  
  DEBUGLOG(("cddaplay:load_wizzard: %d - %s\n", action, select_old));
  return action;
}

/* load wizzard, no multiple selection */
static ULONG PM123_ENTRY load_wizzard_1( HWND owner, char* select, ULONG size )
{ return load_wizzard( owner, select, size, FALSE );
} 

const DECODER_WIZZARD wizzard_1 =
{ NULL,
  "~Track...\tAlt+T",
  't', AF_CHAR | AF_ALT,
  &load_wizzard_1
};

/* load wizzard with multiple selection */
static ULONG PM123_ENTRY load_wizzard_2( HWND owner, char* select, ULONG size )
{ return load_wizzard( owner, select, size, TRUE );
} 

const DECODER_WIZZARD wizzard_2 =
{ NULL,
  "~Tracks...\tAlt+T",
  't', AF_CHAR | AF_ALT,
  &load_wizzard_2
};

/* plug-in entry point */
const DECODER_WIZZARD* PM123_ENTRY decoder_getwizzard( BOOL multiselect )
{ return multiselect ? &wizzard_2 : &wizzard_1;
}


MRESULT EXPENTRY NetworkDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg ) {
    case WM_INITDLG:
    {
      RECTL rect;
      WinQueryWindowRect( hwnd, &rect );
      WinCalcFrameRect( hwnd, &rect, TRUE );
      WinSetWindowPos ( WinWindowFromID( hwnd, MLE_NETLOG ),
                        NULLHANDLE,
                        rect.xLeft,
                        rect.yBottom,
                        rect.xRight - rect.xLeft,
                        rect.yTop - rect.yBottom, SWP_SIZE | SWP_MOVE );
      break;
    }

    case WM_DESTROY:
      nethwnd = NULLHANDLE;
      break;

    case WM_SYSCOMMAND:
      if( SHORT1FROMMP(mp1) == SC_CLOSE ) {
        WinShowWindow( hwnd, FALSE );
        return 0;
      }
      break;
  }

  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

MRESULT EXPENTRY OffDBDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   switch(msg)
   {
      case WM_INITDLG:
      {
         HINI INIhandle;
         CDDB_socket CDDBSocket;

         if((INIhandle = open_module_ini()) != NULLHANDLE)
         {
            ULONG cdlistSize;
            char *cdlistBuffer;

            if(PrfQueryProfileSize(INIhandle, "CDInfo", NULL, &cdlistSize) && cdlistSize > 0)
            {
               cdlistBuffer = (char *) calloc(cdlistSize, 1);
               char *next_discid = cdlistBuffer;

               PrfQueryProfileData(INIhandle, "CDInfo", NULL, cdlistBuffer, &cdlistSize);
               while(next_discid[0] != '\0')
               {
                  unsigned long discid = 0;
                  ULONG cdinfoSize = 0;
                  char *cdinfoBuffer = NULL;

                  sscanf(next_discid, "%lx[^\r\n]", &discid);

                  if(PrfQueryProfileSize(INIhandle, "CDInfo", next_discid, &cdinfoSize) && cdinfoSize > 0)
                  {
                     cdinfoBuffer = (char *) calloc(cdinfoSize, 1);

                     if(PrfQueryProfileString(INIhandle, "CDInfo", next_discid, "", cdinfoBuffer, cdinfoSize))
                     {
                        char temp[2048];
                        SHORT id;

                        // cddb is always ISO-8859-1 by definition unless we implement protocol level 6
                        ch_convert( 1004, cdinfoBuffer, CH_CP_NONE, cdinfoBuffer, cdinfoSize );
                        CDDBSocket.parse_read_reply(cdinfoBuffer);
                        sprintf(temp,"%s: %s (discid: %s)", CDDBSocket.get_disc_title(0), CDDBSocket.get_disc_title(1),next_discid);
                        id = lb_add_item(hwnd,LB_CDINFO,temp);
                        lb_set_handle(hwnd,LB_CDINFO,id,strdup(next_discid));
                     }
                  }
                  free(cdinfoBuffer);

                  next_discid = strchr(next_discid,'\0')+1;
               }
               free(cdlistBuffer);
            }
            close_ini(INIhandle);
         }

         break;
      }

      case WM_DESTROY:
      {
         int i;
         int count = lb_size(hwnd, LB_CDINFO);
         for(i = 0; i < count; i++)
         {
            free(lb_get_handle(hwnd,LB_CDINFO,i));
            lb_set_handle(hwnd,LB_CDINFO,i,NULL);
         }
         break;
      }

      case WM_COMMAND:
         switch(SHORT1FROMMP(mp1))
         {
            case PB_DELETE:
            {
               HINI INIhandle;
               if((INIhandle = open_module_ini()) != NULLHANDLE)
               {
                  int i;
                  int count = lb_size(hwnd, LB_CDINFO);

                  for(i = 0; i < count; i++)
                  {
                     if(lb_selected(hwnd, LB_CDINFO, i-1) == i)
                     {
                        char *discid = (char *) lb_get_handle(hwnd,LB_CDINFO,i);
                        if(discid != NULL)
                           PrfWriteProfileData(INIhandle, "CDInfo", discid, NULL, 0);
                     }
                  }
                  close_ini(INIhandle);

                  for(i = 0; i < count; i++)
                  {
                     free( lb_get_handle(hwnd,LB_CDINFO,i));
                     lb_set_handle(hwnd,LB_CDINFO,i,NULL);
                  }
                  lb_remove_all(hwnd, LB_CDINFO);

                  WinPostMsg(hwnd,WM_INITDLG,MPFROMHWND(hwnd),NULL);

               }
               return 0;
            }

            default:
               break;
         }

   }

   return WinDefDlgProc(hwnd, msg, mp1, mp2);

}

MRESULT EXPENTRY ConfigureDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   int i;

   switch(msg)
   {
      case WM_DESTROY:
         if( tupdate ) {
            DosKillThread(  tupdate );
            DosWaitThread( &tupdate, DCWW_WAIT );
         }
         WinSendMsg( hwnd, WM_UPDATE_DONE, 0, 0 );
         dlghwnd = 0;
         break;

      case WM_CLOSE:
         WinDestroyWindow(hwnd);
         break;

      case WM_INITDLG:

         WinSendDlgItemMsg(hwnd, CB_USECDDB, BM_SETCHECK, MPFROMSHORT(settings.useCDDB), 0);
         WinSendDlgItemMsg(hwnd, CB_USEHTTP, BM_SETCHECK, MPFROMSHORT(settings.useHTTP), 0);
         WinSendDlgItemMsg(hwnd, CB_TRYALL, BM_SETCHECK, MPFROMSHORT(settings.tryAllServers), 0);
         WinSetDlgItemText(hwnd, EF_PROXY, settings.proxyURL);
         WinSetDlgItemText(hwnd, EF_EMAIL, settings.email);

         for(i = 0; i < settings.numCDDB; i++)
         {
            lb_add_item(hwnd,LB_CDDBSERVERS,settings.CDDBServers[i]);
            if(settings.isCDDBSelect[i])
               lb_select(hwnd,LB_CDDBSERVERS,i);
         }

         for(i = 0; i < settings.numHTTP; i++)
         {
            lb_add_item(hwnd,LB_HTTPCDDBSERVERS,settings.HTTPServers[i]);
            if(settings.isHTTPSelect[i])
               lb_select(hwnd,LB_HTTPCDDBSERVERS,i);
         }

         WinPostMsg( hwnd, WM_CONTROL, MPFROM2SHORT( CB_USECDDB, BN_CLICKED ),
                                       MPFROMHWND( WinWindowFromID( hwnd, CB_USECDDB )));
         break;

      case WM_SAVE:
      {
         int i;

         settings.useCDDB = (BOOL)WinSendDlgItemMsg(hwnd, CB_USECDDB, BM_QUERYCHECK, 0, 0);
         settings.useHTTP = (BOOL)WinSendDlgItemMsg(hwnd, CB_USEHTTP, BM_QUERYCHECK, 0, 0);
         settings.tryAllServers = (BOOL)WinSendDlgItemMsg(hwnd, CB_TRYALL, BM_QUERYCHECK, 0, 0);
         WinQueryDlgItemText(hwnd, EF_PROXY, 1023, settings.proxyURL);
         WinQueryDlgItemText(hwnd, EF_EMAIL, 1023, settings.email);

         for( i = 0; i < 128; i++)
         {
            free(settings.CDDBServers[i]); settings.CDDBServers[i] = NULL;
            free(settings.HTTPServers[i]); settings.HTTPServers[i] = NULL;

            settings.isCDDBSelect[i] = FALSE;
            settings.isHTTPSelect[i] = FALSE;
         }

         settings.numCDDB = 0;
         settings.numHTTP = 0;

         int count = lb_size(hwnd, LB_CDDBSERVERS);
         if(count > 0)
         {
            if(count > 128) count = 128;
            settings.numCDDB = count;

            for(i = 0; i < count; i++)
            {
               int size = lb_get_item_size(hwnd,LB_CDDBSERVERS,i);
               settings.CDDBServers[i] = (char *) malloc(size+1);
               lb_get_item(hwnd, LB_CDDBSERVERS, i, settings.CDDBServers[i], size+1);
               if(lb_selected(hwnd, LB_CDDBSERVERS, i-1) == i)
                  settings.isCDDBSelect[i] = TRUE;
            }
         }

         count = lb_size(hwnd, LB_HTTPCDDBSERVERS);
         if(count > 0)
         {
            if(count > 128) count = 128;
            settings.numHTTP = count;

            for(i = 0; i < count; i++)
            {
               int size = lb_get_item_size(hwnd,LB_HTTPCDDBSERVERS,i);
               settings.HTTPServers[i] = (char *) malloc(size+1);
               lb_get_item(hwnd, LB_HTTPCDDBSERVERS, i, settings.HTTPServers[i], size+1);
               if(lb_selected(hwnd, LB_HTTPCDDBSERVERS, i-1) == i)
                  settings.isHTTPSelect[i] = TRUE;
            }
         }

         save_ini();
      }

      case WM_CONTROL:
        if( SHORT1FROMMP(mp1) == CB_USECDDB &&
          ( SHORT2FROMMP(mp1) == BN_CLICKED || SHORT2FROMMP(mp1) == BN_DBLCLICKED ))
        {
          BOOL use = WinQueryButtonCheckstate( hwnd, CB_USECDDB );

          WinEnableControl( hwnd, PB_UPDATE,  use );
          WinEnableControl( hwnd, CB_USEHTTP, use );
          WinEnableControl( hwnd, CB_TRYALL,  use );
          return 0;
        }
        break;

      case WM_UPDATE_DONE:
        tupdate = 0;
        WinEnableControl( hwnd, DID_OK,     TRUE );
        WinEnableControl( hwnd, PB_ADD1,    TRUE );
        WinEnableControl( hwnd, PB_DELETE1, TRUE );
        WinEnableControl( hwnd, PB_ADD2,    TRUE );
        WinEnableControl( hwnd, PB_DELETE2, TRUE );
        WinEnableControl( hwnd, PB_UPDATE,  TRUE );
        WinEnableControl( hwnd, CB_USEHTTP, TRUE );
        WinEnableControl( hwnd, CB_TRYALL,  TRUE );
        WinEnableControl( hwnd, CB_USECDDB, TRUE );
        return 0;

      case WM_COMMAND:
         switch(SHORT1FROMMP(mp1))
         {
            char buffer[512];
            int  nextitem;

            case DID_OK:
               WinSendMsg(hwnd,WM_SAVE,0,0);
            case DID_CANCEL:
               WinDestroyWindow(hwnd);
               break;

            case PB_ADD1:
               WinQueryDlgItemText( hwnd, EF_NEWSERVER, sizeof(buffer), buffer );
               if(*buffer)
                  lb_add_item(hwnd, LB_CDDBSERVERS, buffer);
               break;

            case PB_ADD2:
               WinQueryDlgItemText( hwnd, EF_NEWSERVER, sizeof(buffer), buffer );
               if(*buffer)
                  lb_add_item(hwnd, LB_HTTPCDDBSERVERS, buffer);
               break;

            case PB_DELETE1:
               nextitem = lb_selected(hwnd, LB_CDDBSERVERS, LIT_FIRST);
               while(nextitem != LIT_NONE)
               {
                  lb_remove_item(hwnd, LB_CDDBSERVERS, nextitem);
                  nextitem = lb_selected(hwnd, LB_CDDBSERVERS, LIT_FIRST);
               }
               break;

            case PB_DELETE2:
               nextitem = lb_selected(hwnd, LB_HTTPCDDBSERVERS, LIT_FIRST);
               while(nextitem != LIT_NONE)
               {
                  lb_remove_item(hwnd, LB_HTTPCDDBSERVERS, nextitem);
                  nextitem = lb_selected(hwnd, LB_HTTPCDDBSERVERS, LIT_FIRST);
               }
               break;

            case PB_UPDATE:
               WinEnableControl( hwnd, DID_OK,     FALSE );
               WinEnableControl( hwnd, PB_ADD1,    FALSE );
               WinEnableControl( hwnd, PB_DELETE1, FALSE );
               WinEnableControl( hwnd, PB_ADD2,    FALSE );
               WinEnableControl( hwnd, PB_DELETE2, FALSE );
               WinEnableControl( hwnd, PB_UPDATE,  FALSE );
               WinEnableControl( hwnd, CB_USEHTTP, FALSE );
               WinEnableControl( hwnd, CB_TRYALL,  FALSE );
               WinEnableControl( hwnd, CB_USECDDB, FALSE );
               WinSendMsg( hwnd, WM_SAVE, 0, 0 );
               tupdate = _beginthread( cddb_update, NULL, 65535, (void*)hwnd );
               break;

            case PB_NETWIN:
               WinShowWindow(nethwnd,TRUE);
               WinFocusChange(HWND_DESKTOP, nethwnd, 0);
               break;

            case PB_OFFDB:
               {
                  HMODULE module;
                  char modulename[128];

                  getModule(&module,modulename,128);
                  WinDlgBox(HWND_DESKTOP, hwnd, OffDBDlgProc, module, DLG_OFFDB, NULL);
                  break;
               }
         }
         return 0;
   }

   return WinDefDlgProc(hwnd, msg, mp1, mp2);
}

#define FONT1 "9.WarpSans"
#define FONT2 "8.Helv"

int PM123_ENTRY plugin_configure(HWND hwnd, HMODULE module)
{
   if(dlghwnd == 0)
   {
      dlghwnd = WinLoadDlg(HWND_DESKTOP, HWND_DESKTOP, ConfigureDlgProc, module, DLG_CDDA, NULL);

      if(dlghwnd)
      {
         do_warpsans(dlghwnd);

//         WinSetFocus(HWND_DESKTOP,WinWindowFromID(dlghwnd,CB_FORCEMONO));
//         if(!mmx_present)
//            WinEnableWindow(WinWindowFromID(dlghwnd,CB_USEMMX),FALSE);
         WinShowWindow(dlghwnd, TRUE);
      }
   }
   else
      WinFocusChange(HWND_DESKTOP, dlghwnd, 0);

   return 0;
}

int PM123_ENTRY plugin_query(PLUGIN_QUERYPARAM *param)
{
   param->type = PLUGIN_DECODER;
   param->author = "Samuel Audet";
   param->desc = "CDDA Play 1.11";
   param->configurable = TRUE;

   load_ini();
   return 0;
}

extern "C" int INIT_ATTRIBUTE __dll_initialize( void )
{
  HMODULE hmodule;
  char    name[_MAX_PATH];

  getModule( &hmodule, name, sizeof( name ));

  // create hidden network window
  if( nethwnd == NULLHANDLE ) {
    nethwnd = WinLoadDlg( HWND_DESKTOP, HWND_DESKTOP, NetworkDlgProc, hmodule, DLG_NETWIN, NULL );

    if( nethwnd != NULLHANDLE ) {
      do_warpsans( nethwnd );
    }
  }
  return 1;
}

extern "C" int TERM_ATTRIBUTE __dll_terminate( void )
{
  if( nethwnd != NULLHANDLE ) {
     WinDestroyWindow( nethwnd );
  }
  return 1;
}

#if defined(__IBMCPP__)
unsigned long _System _DLL_InitTerm( unsigned long modhandle,
                                     unsigned long flag       )
{
  if( flag == 0 ) {
    if( _CRT_init() == -1 ) return 0UL;
    __dll_initialize();
  } else {
    __dll_terminate ();
    _CRT_term();
  }

  return 1UL;
}
#endif
