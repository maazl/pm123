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

/* The WAV player plug-in for PM123 */

#define  INCL_DOS
#define  INCL_WIN
#include <os2.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <process.h>

#include <format.h>
#include <decoder_plug.h>
#include <plugin.h>
#include <utilfct.h>
#include "wav.h"

typedef struct _WAVPLAY
{
   WAV wavfile;
   FORMAT_INFO formatinfo;

   int (PM123_ENTRYP output_play_samples)(void *a, FORMAT_INFO *format,char *buf,int len, int posmarker);
   void *a; /* only to be used with the precedent function */
   int buffersize;

   HEV play,ok;
   char filename[1024];
   BOOL stop,rew,ffwd;
   int jumpto;
   int status;

   int decodertid;

   void (PM123_ENTRYP error_display)(char *);
   HWND hwnd;

   int last_length; // keeps the last length in memory to calls to
                    // decoder_length() remain valid after the file is closed
} WAVPLAY;


static void TFNENTRY decoder_thread(void *arg)
{
   WAVPLAY *w = (WAVPLAY *) arg;
   ULONG resetcount;

   while(1)
   {
      char *buffer = NULL;
      int read = 0;

      DosWaitEventSem(w->play, SEM_INDEFINITE_WAIT);
      DosResetEventSem(w->play,&resetcount);

      w->status = DECODER_STARTING;
      buffer = (char*)malloc(w->buffersize);

      w->last_length = -1;
      DosPostEventSem(w->ok);

      if(w->wavfile.open(w->filename,READ,w->formatinfo.samplerate,
            w->formatinfo.channels,w->formatinfo.bits,w->formatinfo.format))
      {
         WinPostMsg(w->hwnd,WM_PLAYERROR,0,0);
         w->status = DECODER_STOPPED;
         continue;
      }

      w->jumpto = -1;
      w->rew = w->ffwd = 0;
      w->stop = 0;

      w->status = DECODER_PLAYING;
      w->last_length = decoder_length(w);

      while((read = w->wavfile.readData(buffer,w->buffersize)) > 0 && !w->stop)
      {
         int written = w->output_play_samples(w->a, &w->formatinfo, buffer, read,
            (int)((double)w->wavfile.filepos()*1000/
                  (w->formatinfo.samplerate*w->formatinfo.channels*w->formatinfo.bits/8)));

         if(written < read)
         {
            WinPostMsg(w->hwnd,WM_PLAYERROR,0,0);
            break;
         }

         if(w->jumpto >= 0)
         {
            w->wavfile.jumpto(w->jumpto);
            w->jumpto = -1;
            WinPostMsg(w->hwnd,WM_SEEKSTOP,0,0);
         }
         if(w->rew)
         {
            w->wavfile.jumpto(w->wavfile.filepos()-
                              (w->formatinfo.samplerate/4*w->formatinfo.channels*
                               (w->formatinfo.bits/8)));
         }
         if(w->ffwd)
         {
            w->wavfile.jumpto(w->wavfile.filepos()+
                              (w->formatinfo.samplerate/4*w->formatinfo.channels*
                               (w->formatinfo.bits/8)));
         }
      }
      free(buffer); buffer = NULL;
      w->status = DECODER_STOPPED;
      w->wavfile.close();

      WinPostMsg(w->hwnd,WM_PLAYSTOP,0,0);
      DosPostEventSem(w->ok);
   }
}

int PM123_ENTRY decoder_init(void **W)
{
   WAVPLAY *w;

   *W = malloc(sizeof(WAVPLAY));
   w = (WAVPLAY *)*W;
   memset(w,0,sizeof(WAVPLAY));

   DosCreateEventSem(NULL,&w->play,0,FALSE);
   DosCreateEventSem(NULL,&w->ok,0,FALSE);

   w->decodertid = _beginthread(decoder_thread,0,64*1024,(void *) w);
   if(w->decodertid != -1)
      return w->decodertid;
   else
   {
      DosCloseEventSem(w->play);
      DosCloseEventSem(w->ok);
      free(w);
      return -1;
   }
}

BOOL PM123_ENTRY decoder_uninit(void *W)
{
   WAVPLAY *w = (WAVPLAY *) W;
   int decodertid = w->decodertid;

   DosCloseEventSem(w->play);
   DosCloseEventSem(w->ok);

   free(w);

   return !DosKillThread(decodertid);
}


ULONG PM123_ENTRY decoder_command(void *W, ULONG msg, DECODER_PARAMS *params)
{
   WAVPLAY *w = (WAVPLAY *) W;
   ULONG resetcount;

   switch(msg)
   {
      case DECODER_PLAY:
         if(w->status == DECODER_STOPPED)
         {
            strlcpy(w->filename, params->filename,sizeof(w->filename));
            DosResetEventSem(w->ok,&resetcount);
            DosPostEventSem(w->play);
            if(DosWaitEventSem(w->ok, 10000) == 640)
            {
               w->status = DECODER_STOPPED;
               DosKillThread(w->decodertid);
               w->decodertid = _beginthread(decoder_thread,0,64*1024,(void *) w);
               return 102;
            }
         }
         else
            return 101;
         break;

      case DECODER_STOP:
         if(w->status != DECODER_STOPPED)
         {
            DosResetEventSem(w->ok,&resetcount);
            w->stop = TRUE;
            if(DosWaitEventSem(w->ok, 10000) == 640)
            {
               w->status = DECODER_STOPPED;
               DosKillThread(w->decodertid);
               w->decodertid = _beginthread(decoder_thread,0,64*1024,(void *) w);
               return 102;
            }
         }
         else
            return 101;
         break;

      case DECODER_FFWD:
         w->ffwd = params->ffwd;
         break;

      case DECODER_REW:
         w->rew = params->rew;
         break;

      /* I multiply by the channels and bits last because I need to fall on
         a byte which is a multiple of those or else I'll get garbage */
      case DECODER_JUMPTO:
         w->jumpto = (int)((double)w->formatinfo.samplerate*params->jumpto/1000)*
                              w->formatinfo.channels*(w->formatinfo.bits/8);
         break;

      case DECODER_EQ:
         return 1;

      case DECODER_SETUP:
         w->output_play_samples = params->output_play_samples;
         w->a = params->a;
         w->buffersize = params->audio_buffersize;
         w->error_display = params->error_display;
         w->hwnd = params->hwnd;
         break;
   }
   return 0;
}

ULONG PM123_ENTRY decoder_length(void *W)
{
   WAVPLAY *w = (WAVPLAY *) W;

   if(w->status == DECODER_PLAYING)
      w->last_length = (int)((double)w->wavfile.filelength()*1000/(w->formatinfo.samplerate*
                                     w->formatinfo.channels*w->formatinfo.bits/8));

   return w->last_length;
}

ULONG PM123_ENTRY decoder_status(void *W)
{
   WAVPLAY *w = (WAVPLAY *) W;
   return w->status;
}

ULONG PM123_ENTRY decoder_fileinfo(char *filename, DECODER_INFO *info)
{
   WAV wavfile;
   int rc;

   memset(info,0,sizeof(*info));
   info->size = sizeof(*info);

   info->size = sizeof(*info);
   info->mpeg = 0;

   rc = wavfile.open( filename,READ,info->format.samplerate,
                      info->format.channels,info->format.bits,info->format.format);
   if(rc == 0)
   {
      info->songlength = (int)((double)wavfile.filelength()*1000/
         (info->format.samplerate*info->format.channels*info->format.bits/8));

      info->bitrate = info->format.samplerate * info->format.bits * info->format.channels / 1000;
      info->mode = info->format.channels == 1 ? 3 : 0;
      sprintf(info->tech_info,"%d bits, %.1f kHz, %s",info->format.bits, (float)info->format.samplerate/1000, info->format.channels == 1 ? "Mono" : "Stereo");
   }
   wavfile.close();
   return rc;
}

ULONG PM123_ENTRY decoder_trackinfo(char *drive, int track, DECODER_INFO *info)
{
   return 200;
}

ULONG PM123_ENTRY decoder_cdinfo(char *drive, DECODER_CDINFO *info)
{
   return 100;
}


ULONG PM123_ENTRY decoder_support(char *ext[], int *size)
{
   if(size)
   {
      if(ext != NULL && *size >= 1)
      {
         strcpy(ext[0],"*.WAV");
      }
      *size = 1;
   }

   return DECODER_FILENAME;
}

int PM123_ENTRY plugin_query(PLUGIN_QUERYPARAM *param)
{
   param->type = PLUGIN_DECODER;
   param->author = "Samuel Audet";
   param->desc = "WAV Play 1.01";
   param->configurable = FALSE;
   return 0;
}

