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

/* PM123 WAV Output plug-in */

#define INCL_OS2MM
#define INCL_PM
#define INCL_DOS
#include <os2.h>
#include <os2me.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <utilfct.h>

#include <format.h>
#include <output_plug.h>
#include <decoder_plug.h>
#include <plugin.h>
#include "wavout.h"
#include "..\wavplay\wav.h"

void load_ini(void);
void save_ini(void);

char outpath[CCHMAXPATH] = {0};

#if 0

HMODULE thisModule = 0;
char *thisModulePath = NULL;

#if __cplusplus
extern "C" {
#endif

int _CRT_init(void);
void _CRT_term(void);

BOOL _System _DLL_InitTerm(ULONG hModule, ULONG flag)
{
   if(flag == 0)
   {
      if(_CRT_init() == -1)
         return FALSE;

      getModule(&thisModule,&thisModulePath);
   }
   else
      _CRT_term();

   return TRUE;
}

#if __cplusplus
}
#endif

#endif

typedef struct
{
   BOOL opened;
   HEV pause;
   int playingpos;

   char filename[CCHMAXPATH]; // filled by setup
   char fullpath[CCHMAXPATH]; // completed by open with outpath

   char *buffer;
   WAV wavfile;
   OUTPUT_PARAMS original_info; // to open the device

} WAVOUT;


ULONG output_set_volume(void *A, char setvolume, float setamplifier)
{
   return 0;
}

ULONG _System output_pause(void *A, BOOL pause)
{
   WAVOUT *a = (WAVOUT *) A;

   ULONG resetcount;

   if(pause)
      DosResetEventSem(a->pause, &resetcount);
   else
      DosPostEventSem(a->pause);

   return 0;
}

ULONG _System output_init(void **A)
{
   WAVOUT *a;

   *A = malloc(sizeof(WAVOUT));
   a = (WAVOUT *) *A;
   memset(a,0,sizeof(*a));

   DosCreateEventSem(NULL,&a->pause,0,TRUE);

   return 0;
}


ULONG _System output_close(void *A);

ULONG output_open(WAVOUT *a)
{
   OUTPUT_PARAMS *ai = &a->original_info;

   int rc;
   char *lastchar;

   // new filename, even if we didn't explicity get a close()!
   if(a->opened)
      output_close(a);

   a->playingpos = 0;
   free(a->buffer);
   a->buffer = (char *) malloc(ai->buffersize);
   DosPostEventSem(a->pause);

   strncpy(a->fullpath,outpath,sizeof(a->fullpath)-3);
   lastchar = strchr(a->fullpath,0)-1;
   if(*lastchar != '\\' && *lastchar != '/')
   {
      *(lastchar+1) = '\\';
      *(lastchar+2) = 0;
   }
   strncat(a->fullpath,a->filename,sizeof(a->fullpath)-strlen(a->fullpath)-1);

   rc = a->wavfile.open(a->fullpath, CREATE, ai->formatinfo.samplerate,
          ai->formatinfo.channels, ai->formatinfo.bits, ai->formatinfo.format);

   if(rc != 0)
   {
      char temp[512];
      sprintf(temp, "Could not open WAV file %s",a->fullpath);
      (*ai->error_display)(_strerror(temp));
      return errno;
   }

   a->opened = TRUE;

   return 0;
}

ULONG _System output_close(void *A)
{
   WAVOUT *a = (WAVOUT *) A;

   ULONG resetcount;

   if(a->opened == FALSE)
      return 0;

   a->wavfile.close();
   a->opened = FALSE;
   DosResetEventSem(a->pause,&resetcount);
   free(a->buffer);
   a->buffer = NULL;

   return 0;
}

ULONG _System output_uninit(void *A)
{
   WAVOUT *a = (WAVOUT *) A;

   DosCloseEventSem(a->pause);
   free(a->buffer);
   free(a);

   return 0;
}


int _System output_play_samples(void *A, FORMAT_INFO *format, char *buf,int len, int posmarker)
{
   WAVOUT *a = (WAVOUT *) A;
   int written;

   PPIB ppib;
   PTIB ptib;

   // Sets priority to idle. Normal and especially not boost priority are needed
   // or desired here, but that wouldn't be the case for real-time output
   // plug-ins.
   DosGetInfoBlocks(&ptib,&ppib);
   DosSetPriority(PRTYS_THREAD,1,31,ptib->tib_ptib2->tib2_ultid);

   // hum this is not going to work since a->filename is not set
   if(!a->opened)
   {
      a->original_info.formatinfo = *format;
      if(output_open(a) != 0)
         return 0;
   }

   DosWaitEventSem(a->pause,-1);

   memcpy(a->buffer,buf,len);
   a->playingpos = posmarker;
   if(memcmp(format, &a->original_info.formatinfo, sizeof(FORMAT_INFO)) != 0)
      (*a->original_info.info_display)("Warning: WAV data currently being written is in a different\n"
                      "format than the opened format.  Generation of a probably\n"
                      "invalid WAV file.");

   written = a->wavfile.writeData(buf,len);

   if(written < len)
   {
      char temp[512];
      sprintf(temp,"Could not write to filename %s",a->fullpath);
      (*a->original_info.error_display)(_strerror(temp));
      a->wavfile.close();
      WinPostMsg(a->original_info.hwnd,WM_PLAYERROR,0,0);
   }

   WinPostMsg(a->original_info.hwnd,WM_OUTPUT_OUTOFDATA,0,0);

   return written;
}

ULONG _System output_playing_samples(void *A, FORMAT_INFO *info, char *buf, int len)
{
   WAVOUT *a = (WAVOUT *) A;

   *info = a->original_info.formatinfo;
   memcpy(buf,a->buffer,len);

   return 0;
}

ULONG _System output_playing_pos(void *A)
{
   WAVOUT *a = (WAVOUT *) A;
   return a->playingpos;
}

#if 0
void _System output_trash_buffers(void *A, ULONG temp_playingpos)
{
   ;
}
#endif

BOOL _System output_playing_data(void *A)
{
   return FALSE;
}


ULONG _System output_command(void *A, ULONG msg, OUTPUT_PARAMS *info)
{
   WAVOUT *a = (WAVOUT *) A;
   ULONG rc = 0;

   switch(msg)
   {
      case OUTPUT_OPEN:
      {
         char *urlbegin = strstr(info->filename,"://");
         char *lastslash = strrchr(info->filename,'/');
         char *lastdot;
         if(urlbegin != NULL &&
            (lastslash == urlbegin+2 || *(lastslash+1) == 0))
         {
            strcpy(a->filename, urlbegin+3);
            lastslash = strrchr(a->filename,'/');
            if(lastslash != NULL)
               *lastslash = 0;
         }
         else
         {
            if(urlbegin == NULL)
               urlbegin = info->filename;
            else
               urlbegin+=3;

            lastslash = strrchr(urlbegin,'\\') > strrchr(urlbegin,'/') ?
                        strrchr(urlbegin,'\\') : strrchr(urlbegin,'/');

            if(lastslash == NULL)
               strcpy(a->filename,urlbegin);
            else
               strcpy(a->filename,lastslash+1);

            lastdot = strrchr(a->filename,'.');

            if(lastdot != NULL)
               *lastdot = 0;
         }

         strcat(a->filename,".wav");

         rc = output_open(a);
         break;
      }

      case OUTPUT_CLOSE:
         rc = output_close(a);
         break;
      case OUTPUT_VOLUME:
         //rc = output_set_volume(a, info->volume, info->amplifier);
         break;
      case OUTPUT_PAUSE:
         rc = output_pause(a, info->pause);
         break;
      case OUTPUT_SETUP:
         // make sure no important information is modified here if currently
         // opened when using another thread (ie.: always_hungry = FALSE)
         // for output which is not the case here.
         a->original_info = *info;
         info->always_hungry = TRUE;
         break;

      case OUTPUT_TRASH_BUFFERS:
         //output_trash_buffers(a,info->temp_playingpos);
         break;
      case OUTPUT_NOBUFFERMODE:
         //a->nobuffermode = info->nobuffermode;
         break;
      default:
         rc = 1;
   }

   return rc;
}



HWND dlghwnd = 0;

MRESULT EXPENTRY ConfigureDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   HWND hFilename = WinWindowFromID(hwnd,EF_FILENAME);
   switch(msg)
   {
      case WM_DESTROY:
         dlghwnd = 0;
         break;

      case WM_CLOSE:
         WinDestroyWindow(hwnd);
         break;

      case WM_INITDLG:
      {
         WinSendMsg(hFilename,EM_SETTEXTLIMIT,MPFROMSHORT(512),0);
         WinSetWindowText(hFilename,outpath);

         break;
      }

      case WM_COMMAND:
         switch(SHORT1FROMMP(mp1))
         {
            case PB_BROWSE:
            {
               SWP swp;
               POINTL pos;
               char path[CCHMAXPATH];

               WinQueryWindowPos(hwnd,&swp);
               pos.x = swp.x+20;
               pos.y = swp.y+20;

               WinQueryWindowText(hFilename,sizeof(path),path);
               if(open_dir_browser(hwnd,pos,"Select output WAV directory:",path,sizeof(path)))
               {
                  char *lastchar = strchr(path,0)-1;
                  if(*lastchar == ':')
                  {
                     *(lastchar+1) = '\\';
                     *(lastchar+2) = 0;
                  }
                  WinSetWindowText(hFilename,path);
               }

               return 0;
            }

            case DID_OK:
               WinQueryWindowText(hFilename,sizeof(outpath),outpath);
               save_ini();
            case DID_CANCEL:
               WinDestroyWindow(hwnd);
               break;
         }
         break;
   }

   return WinDefDlgProc(hwnd, msg, mp1, mp2);
}

#define FONT1 "9.WarpSans"
#define FONT2 "8.Helv"

void _System plugin_configure(HWND hwnd, HMODULE module)
{
   if(dlghwnd == 0)
   {
      LONG fontcounter = 0;
      dlghwnd = WinLoadDlg(HWND_DESKTOP, HWND_DESKTOP, ConfigureDlgProc, module, 1, NULL);

      if(dlghwnd)
      {
         HPS hps;

         hps = WinGetPS(HWND_DESKTOP);
         if(GpiQueryFonts(hps, QF_PUBLIC,strchr(FONT1,'.')+1, &fontcounter, 0, NULL))
            WinSetPresParam(dlghwnd, PP_FONTNAMESIZE, strlen(FONT1)+1, FONT1);
         else
            WinSetPresParam(dlghwnd, PP_FONTNAMESIZE, strlen(FONT2)+1, FONT2);
         WinReleasePS(hps);

         WinSetFocus(HWND_DESKTOP,WinWindowFromID(dlghwnd,EF_FILENAME));
         WinShowWindow(dlghwnd, TRUE);
      }
   }
   else
      WinFocusChange(HWND_DESKTOP, dlghwnd, 0);
}

void save_ini()
{
   HINI INIhandle;

   if((INIhandle = open_module_ini()) != NULLHANDLE)
   {
      save_ini_string(INIhandle,outpath);
      close_ini(INIhandle);
   }
}

void load_ini()
{
   HINI INIhandle;

   outpath[0] = 0;

   if((INIhandle = open_module_ini()) != NULLHANDLE)
   {
      load_ini_string(INIhandle,outpath,sizeof(outpath));
      close_ini(INIhandle);
   }
}

void _System plugin_query(PLUGIN_QUERYPARAM *param)
{
   param->type = PLUGIN_OUTPUT;
   param->author = "Samuel Audet";
   param->desc = "WAVOUT 1.02";
   param->configurable = TRUE;

   load_ini();
}
