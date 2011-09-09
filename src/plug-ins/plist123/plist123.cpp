/*
 * Copyright 2009-2009 M.Mueller
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

#define PLUGIN_INTERFACE_LEVEL 3
#define INCL_PM
#define INCL_BASE
#include <os2.h>
#include <stdlib.h>
#include <string.h>

#include <format.h>
#include <decoder_plug.h>
#include <plugin.h>
#include "plist123.h"
#include "playlistreader.h"
#include "playlistwriter.h"

#include <fileutil.h>
#include <xio.h>
#include <cpp/smartptr.h>

#include <debuglog.h>


PLUGIN_CONTEXT Ctx;

int DLLENTRY plugin_init(const PLUGIN_CONTEXT* ctx)
{ Ctx = *ctx;
  return PLUGIN_OK;
}



// TODO:
//static void load_ini(void);
//static void save_ini(void);

// confinuration parameters
/*typedef struct
{  char        device[64];
   BOOL        lockdevice;
   FORMAT_INFO format;
   ULONG       connector;
   int         numbuffers;
   int         buffersize;
} PARAMETERS;

static PARAMETERS defaults =
{  MCI_DEVTYPE_AUDIO_AMPMIX_NAME"00",
   FALSE,
   { sizeof(FORMAT_INFO), 44100, 2, 16, WAVE_FORMAT_PCM },
   MCI_LINE_IN_CONNECTOR,
   0, // default
   0  // default
};*/


static const DECODER_FILETYPE filetypes[] =
{ { EACAT_PLAYLIST, EATYPE_PM123,   "*.lst",  DECODER_PLAYLIST|DECODER_WRITABLE }
, { EACAT_PLAYLIST, EATYPE_WINAMP,  "*.m3u",  DECODER_PLAYLIST|DECODER_WRITABLE }
, { EACAT_PLAYLIST, EATYPE_WINAMPU, "*.m3u8", DECODER_PLAYLIST|DECODER_WRITABLE }
, { EACAT_PLAYLIST, EATYPE_WVISION, "*.pls",  DECODER_PLAYLIST }
//, { EACAT_PLAYLIST, EATYPE_MS,      "*.mpl",  DECODER_PLAYLIST } untested anyway
};

ULONG DLLENTRY
decoder_support(const DECODER_FILETYPE** types, int* count)
{
  *types = filetypes;
  *count = sizeof filetypes / sizeof *filetypes;

  return DECODER_FILENAME|DECODER_URL;
}



ULONG DLLENTRY
decoder_fileinfo(const char* url, int* what, const INFO_BUNDLE* info,
                 DECODER_INFO_ENUMERATION_CB cb, void* param)
{
  *what |= INFO_PHYS|INFO_META; // always inclusive
 
  XFILE* file = xio_fopen(url, "rU");
  if (file == NULL)
  { info->tech->info = xio_strerror(xio_ferror(file));
    return PLUGIN_NO_READ;
  }
  info->phys->filesize = xio_fsize(file);
  // TODO: File Timestamp
  if (is_file(url))
    info->phys->attributes |= PATTR_WRITABLE; // TODO: read-only flag

  ULONG ret = PLUGIN_OK;
  if (*what & (INFO_CHILD|INFO_TECH|INFO_OBJ))
  { *what |= INFO_CHILD|INFO_TECH|INFO_OBJ;
    sco_ptr<PlaylistReader> reader(PlaylistReader::SnifferFactory(url, file));
    if (reader == NULL)
    { info->tech->info = "Unrecognized playlist type.";
      ret = PLUGIN_NO_PLAY;
    } else
    { info->tech->attributes |= TATTR_PLAYLIST;
      info->tech->format = reader->GetFormat();
      if (!reader->Parse(cb, param))
        ret = PLUGIN_GO_FAILED;
      else
        info->obj->num_items = reader->GetCount();
    }
  }
  
  xio_fclose(file);
  return ret;
}


ULONG DLLENTRY decoder_savefile(const char* url, const char* format, int what, const INFO_BUNDLE* info,
                                DECODER_SAVE_ENUMERATION_CB cb, void* param)
{ sco_ptr<PlaylistWriter> writer(PlaylistWriter::FormatFactory(url, format));
  if (writer == NULL)
    return PLUGIN_UNSUPPORTED;

  XFILE* file = xio_fopen(url, "wU");
  if (file == NULL)
    return PLUGIN_NO_READ;

  writer->Init(info, what, file);
  
  PlaylistWriter::Item item;
  while ((*cb)(param, &item.Url, &item.Info, &item.Valid, &item.Override) == PLUGIN_OK)
    writer->AppendItem(item);
  
  // Destroy writer before close
  writer = NULL;
  xio_fclose(file);
  return PLUGIN_OK;
}


int DLLENTRY plugin_query(PLUGIN_QUERYPARAM *param)
{
   param->type = PLUGIN_DECODER;
   param->author = AUTHOR_STRING;
   param->desc = DESCRIPTION_STRING;
   param->configurable = FALSE;
   param->interface = PLUGIN_INTERFACE_LEVEL;

   //load_ini();
   return 0;
}


/********** GUI stuff ******************************************************/

/*HWND dlghwnd = 0;

static MRESULT EXPENTRY ConfigureDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
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
         int i, numdevice = output_get_devices(NULL, 0);

         WinSendMsg(WinWindowFromID(hwnd, CB_DEVICE),
                  LM_INSERTITEM,
                  MPFROMSHORT(LIT_END),
                  MPFROMP("Default"));

         for(i = 1; i <= numdevice; i++)
         {
            char temp[256];
            output_get_devices(temp, i);
            WinSendMsg(WinWindowFromID(hwnd, CB_DEVICE),
                     LM_INSERTITEM,
                     MPFROMSHORT(LIT_END),
                     MPFROMP(temp));
         }

         WinSendMsg(WinWindowFromID(hwnd, CB_DEVICE),
                  LM_SELECTITEM,
                  MPFROMSHORT(device),
                  MPFROMSHORT(TRUE));


         WinSendDlgItemMsg(hwnd, CB_SHARED, BM_SETCHECK, MPFROMSHORT(!lockdevice), 0);


         WinSendMsg(WinWindowFromID(hwnd, SB_BUFFERS),
                 SPBM_SETLIMITS,
                 MPFROMLONG(200),
                 MPFROMLONG(5));

         WinSendMsg(WinWindowFromID(hwnd, SB_BUFFERS),
                 SPBM_SETCURRENTVALUE,
                 MPFROMLONG(numbuffers),
                 0);

         WinSendDlgItemMsg(hwnd, CB_8BIT, BM_SETCHECK, MPFROMSHORT(force8bit), 0);
         WinSendDlgItemMsg(hwnd, CB_48KLUDGE, BM_SETCHECK, MPFROMSHORT(kludge48as44), 0);
         break;
      }

      case WM_COMMAND:
         switch(SHORT1FROMMP(mp1))
         {
            case DID_OK:
               device = LONGFROMMR(WinSendMsg(WinWindowFromID(hwnd, CB_DEVICE),
                                           LM_QUERYSELECTION,
                                           MPFROMSHORT(LIT_FIRST),
                                           0));

               lockdevice = ! (BOOL)WinSendDlgItemMsg(hwnd, CB_SHARED, BM_QUERYCHECK, 0, 0);

               WinSendMsg(WinWindowFromID(hwnd, SB_BUFFERS),
                       SPBM_QUERYVALUE,
                       MPFROMP(&numbuffers),
                       MPFROM2SHORT(0, SPBQ_DONOTUPDATE));

               force8bit = (BOOL)WinSendDlgItemMsg(hwnd, CB_8BIT, BM_QUERYCHECK, 0, 0);
               kludge48as44 = (BOOL)WinSendDlgItemMsg(hwnd, CB_48KLUDGE, BM_QUERYCHECK, 0, 0);
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

int DLLENTRY plugin_configure(HWND hwnd, HMODULE module)
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

         WinSetFocus(HWND_DESKTOP,WinWindowFromID(dlghwnd,CB_DEVICE));
         WinShowWindow(dlghwnd, TRUE);
      }
   }
   else
      WinFocusChange(HWND_DESKTOP, dlghwnd, 0);

   return 0;
}

#define INIFILE "os2audio.ini"

static void save_ini()
{
   HINI INIhandle;

   if((INIhandle = open_module_ini()) != NULLHANDLE)
   {
      save_ini_value(INIhandle,device);
      save_ini_value(INIhandle,lockdevice);
      save_ini_value(INIhandle,numbuffers);
      save_ini_value(INIhandle,force8bit);
      save_ini_value(INIhandle,kludge48as44);

      close_ini(INIhandle);
   }
}

static void load_ini()
{
   HINI INIhandle;

   device = 0;
   lockdevice = 0;
   numbuffers = 32;
   force8bit = 0;
   kludge48as44 = 0;

   if((INIhandle = open_module_ini()) != NULLHANDLE)
   {
      load_ini_value(INIhandle,device);
      load_ini_value(INIhandle,lockdevice);
      load_ini_value(INIhandle,numbuffers);
      load_ini_value(INIhandle,force8bit);
      load_ini_value(INIhandle,kludge48as44);

      close_ini(INIhandle);
   }
}
*/

