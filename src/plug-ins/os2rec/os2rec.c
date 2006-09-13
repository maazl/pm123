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

/* OS/2 recording decoder
 * URL syntax:  record://device?[samplerate=44100][&channels=1|2][&shared=1]
 */

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
#include <errorstr.h>

#include <format.h>
#include <decoder_plug.h>
#include <plugin.h>
//#include "os2rec.h"

//#define DEBUG 2

#include <debuglog.h>


static void load_ini(void);
static void save_ini(void);

// confinuration parameters
typedef struct
{  char        device[64];
   BOOL        lockdevice;
   FORMAT_INFO format;
   int         numbuffers;
   int         buffersize;
} PARAMETERS;

static PARAMETERS defaults =
{  "0",
   FALSE,
   { sizeof(FORMAT_INFO), 44100, 2, 16, WAVE_FORMAT_PCM },
   0, // default
   0  // default
};


typedef struct
{
   // setup parameters
   PARAMETERS params;

   int  (PM123_ENTRYP output_play_samples)( void* a, const FORMAT_INFO* format, const char* buf, int len, int posmarker );
   void* a;           /* only to be used with the precedent function */
   int   audio_buffersize;
   void (PM123_ENTRYP error_display)( char* );
   void (PM123_ENTRYP info_display )( char* );
   HEV   playsem;     /* this semaphore is reseted when DECODER_PLAY is requested
                         and is posted on stop */
   HWND  hwnd;        /* commodity for PM interface, decoder must send a few
                         messages to this handle */

   // MCI data
   int                 device;
   ULONG               MixHandle;
   PMIXERPROC          pmixRead;
   MCI_MIX_BUFFER*     MixBuffers;
   MCI_BUFFER_PARMS    bpa;
 
   // buffer queue
   MCI_MIX_BUFFER*     QHead;
   MCI_MIX_BUFFER*     QTail;
   HMTX                QMutex;
   HEV                 QSignal;
 
   // decoder thread
   ULONG               tid;
   BOOL                terminate;
   
   // status
   BOOL                opened;
   BOOL                mcibuffers;
   BOOL                playing;
 
} OS2RECORD;


static BOOL
parseURL( const char* url, PARAMETERS* params )
{ int i,j,k;
  const char* cp;
  if (strnicmp(url, "record://", 9) != 0)
    return FALSE;
  // extract device
  i = strcspn(url+9, "\\/?");
  if (cp == NULL || i >= sizeof params->device)
    return FALSE;
  if (sscanf(url+9, "%d%n", &j, &k) == 1 && k == i)
  { // bare device number
    sprintf(params->device, MCI_DEVTYPE_AUDIO_AMPMIX_NAME"%02d", j);
  } else
  { memcpy(params->device, url+9, i);
    params->device[i] = 0;
  }
  // fetch parameters
  switch (url[9+i])
  {default: // = case 0
    return TRUE;
   case '\\':
   case '/':
    return url[9+i+1] == 0;
   case '?':
    cp = url+9+i;
  }
  // parse options
  while (*cp != 0)
  { char key[16];
    char value[32];
    ++cp;
    // extract
    i = strcspn(cp, "&");
    j = strcspn(cp, "&=");
    if (i == j)
    { value[0] = 0;
    } else
    { k = i-j-1;
      if (k >= sizeof value)
        k = sizeof value -1;
      memcpy(value, cp+j+1, k);
      value[k] = 0;
    }
    if (j >= sizeof key)
      j = sizeof key -1;
    memcpy(key, cp, j);
    key[j] = 0;
    cp += i;
    DEBUGLOG(("os2rec:parseURL: option %s=%s\n", key, value));
    // parse keys
    if (stricmp(key, "saplerate") == 0)
    { if (sscanf(value, "%i%n", &i, &j) != 1 || j != strlen(value) || i <= 0)
        return FALSE; // syntax error
      params->format.samplerate = i;
    } else
    if (stricmp(key, "channels") == 0)
    { if (sscanf(value, "%i%n", &i, &j) != 1 || j != strlen(value) || i <= 0)
        return FALSE; // syntax error
      params->format.channels = i;
    } else
    if (stricmp(key, "mono") == 0)
    { params->format.channels = 1;
    } else
    if (stricmp(key, "stereo") == 0)
    { params->format.channels = 2;
    } else
    if (stricmp(key, "bits") == 0)
    { if (sscanf(value, "%i%n", &i, &j) != 1 || j != strlen(value) || i <= 0)
        return FALSE; // syntax error
      params->format.bits = i;
    } else
    if (stricmp(key, "shared") == 0)
    { if (stricmp(value, "no") == 0 || strcmp(value, "0") == 0)
        params->lockdevice = FALSE;
       else if (stricmp(value, "yes") == 0 || strcmp(value, "1") == 0)
        params->lockdevice = TRUE;
       else
        return FALSE; // bad value
    } else // bad param
      return FALSE;
  }
  return TRUE;  
}


int PM123_ENTRY
decoder_init( void **A )
{
   OS2RECORD* a = (OS2RECORD*)(*A = malloc(sizeof(OS2RECORD)));
   DEBUGLOG(("os2rec:decoder_init(%p) - %p\n", A, *A));

   memset(a,0,sizeof(*a));

   // load stuff
   a->MixBuffers    = NULL;
   a->QMutex        = NULLHANDLE;
   a->QSignal       = NULLHANDLE;
   a->tid           = (ULONG)-1;
   a->terminate     = FALSE;
   a->opened        = FALSE;
   a->mcibuffers    = FALSE;
   a->playing       = FALSE;

   return 0;
}

ULONG PM123_ENTRY
decoder_uninit( void *a )
{
   DEBUGLOG(("os2rec:decoder_uninit(%p)\n", a));
   free(a);

   return 0;
}

ULONG PM123_ENTRY
decoder_support( char* ext[], int* size )
{
  if( size )
    *size = 0;

  return DECODER_OTHER;
}

ULONG PM123_ENTRY
decoder_fileinfo( const char* filename, DECODER_INFO* info )
{ PARAMETERS params = defaults;
  if (!parseURL( filename, &params ))
    return 200;
  info->format       = params.format;
  info->songlength   = -1;
  info->junklength   = 0;
  info->mpeg         = 0;
  info->layer        = 0;
  info->mode         = params.format.channels > 1 ? DECODER_MODE_STEREO : DECODER_MODE_SINGLE_CHANNEL;
  info->modext       = 0; // ??
  info->bpf          = 0; // ??
  info->bitrate      = params.format.bits * params.format.samplerate * params.format.channels / 1000;
  info->extention    = 0; // ??
  info->startsector  = 0; // no track
  info->endsector    = 0; // no track
  info->numchannels  = 0; // no MOD
  info->numpatterns  = 0; // no MOD
  info->numpositions = 0; // no MOD
  sprintf(info->tech_info, "%d bit, %f kHz, %s", params.format.bits, params.format.samplerate/1000.,
                                                 params.format.channels == 1 ? "mono" : "stereo");
  sprintf(info->title, "Audio device: %.32s", params.device);
  info->artist[0]    = 0;
  info->album[0]     = 0;
  info->year[0]      = 0;
  info->comment[0]   = 0;
  info->genre[0]     = 0;
  return 0;
}

/* Strictly speaking the folowing functions are not required to be implemented.
 * But, older PM123 versions before 1.40 import them anyway. So let's keep compatible.
 */
ULONG PM123_ENTRY
decoder_trackinfo( const char* drive, int track, DECODER_INFO* info ) {
  return 200;
}

ULONG PM123_ENTRY
decoder_cdinfo( const char* drive, DECODER_CDINFO* info ) {
  return 100;
}


/********** MMOS2 stuff ****************************************************/ 

static LONG APIENTRY DARTEvent(ULONG ulStatus, MCI_MIX_BUFFER *PlayedBuffer, ULONG ulFlags)
{  OS2RECORD *a = (OS2RECORD*)PlayedBuffer->ulUserParm;
   DEBUGLOG(("os2rec:DARTEvent(%u, %p, %u)\n", ulStatus, PlayedBuffer, ulFlags));

   switch(ulFlags)
   {case MIX_STREAM_ERROR | MIX_READ_COMPLETE:  // error occur in device
      DEBUGLOG(("os2rec:DARTEvent: error %u\n", ulStatus)); 
      /*if ( ulStatus == ERROR_DEVICE_UNDERRUN)
         // Write buffers to rekick off the amp mixer.
         a->mmp.pmixWrite( a->mmp.ulMixHandle,
                        a->MixBuffers,
                        a->ulMCIBuffers );*/
      break;

    case MIX_READ_COMPLETE:                     // for recording
      // post buffer in the queue
      PlayedBuffer->ulUserParm = 0; // Queue end marker
      DosRequestMutexSem(a->QMutex, SEM_INDEFINITE_WAIT);
      if (a->QHead == NULL)
         a->QHead = a->QTail = PlayedBuffer;
       else
         a->QTail->ulUserParm = (ULONG)PlayedBuffer;
      DosReleaseMutexSem(a->QMutex);
      // notify decoder thread
      DosPostEventSem(a->QSignal);        
      break;
   } // end switch
   return 0;

} /* end DARTEvent */

static void
DecoderThread( void* arg )
{  OS2RECORD *a = (OS2RECORD*)arg;
   ULONG rc;
   MCI_MIX_BUFFER* current;
   DEBUGLOG(("os2rec:DecoderThread(%p)\n", a));
   
   for(;;)
   {  // Wait for signal
      {  ULONG ul;
         DosWaitEventSem(a->QSignal, SEM_INDEFINITE_WAIT);
         DEBUGLOG(("os2rec:DecoderThread: signal! - %d\n", a->terminate));
         if (a->terminate)
            return;
         DosResetEventSem(a->QSignal, &ul);
      }
      // detach Queue
      {  PTIB ptib;
         ULONG priority;
         // We have to switch to time critical priority before we can enter the mutex.
         // Get old Priority
         DosGetInfoBlocks(&ptib, NULL);
         priority = ptib->tib_ptib2->tib2_ulpri;
         DosSetPriority(PRTYS_THREAD, PRTYC_TIMECRITICAL, -31, 0);
         DosRequestMutexSem(a->QMutex, SEM_INDEFINITE_WAIT);
         current = a->QHead;
         a->QHead = NULL;
         a->QTail = NULL;
         DosReleaseMutexSem(a->QMutex);
         // restore priority
         DosSetPriority(PRTYS_THREAD, (priority << 8 & 0xFF), priority & 0xFF, 0);
      }
      if (a->terminate)
         return;
      // pass queue content to output
      while (current != NULL)
      {  MCI_MIX_BUFFER* next;
         ULONG done = 0;
         int byterate = a->params.format.bits/8 * a->params.format.samplerate;
         DEBUGLOG(("os2rec:DecoderThread: now at %p %u %u\n", current, current->ulBufferLength, current->ulTime));
         // pass buffer content
         while (done < current->ulBufferLength)
         {  ULONG plen = current->ulBufferLength - done;
            if (plen > a->audio_buffersize)
               plen = a->audio_buffersize;
            (*a->output_play_samples)( a->a, &a->params.format,
              (char*)current->pBuffer + done, plen, current->ulTime + done*1000/byterate );
            if (a->terminate)
               return;
            done += plen;
         }
         
         next = (MCI_MIX_BUFFER*)current->ulUserParm;
         // pass buffer back to DART
         current->ulUserParm = (ULONG)a;
         rc = (USHORT)(*a->pmixRead)( a->MixHandle, current, 1 );
         if (rc != NO_ERROR)
            DEBUGLOG(("os2rec:DecoderThread: pmixRead failed %d\n", rc));
         // next buffer
         current = next;
      }
      DEBUGLOG(("os2rec:DecoderThread: done!\n"));
   }
}

static ULONG MciError(OS2RECORD *a, ULONG ulError, const char* location)
{
   unsigned char mmerror[180];
   ULONG rc;
   int len;
   
   sprintf(mmerror, "MCI Error %d at %.30s: ", ulError, location);
   len = strlen(mmerror);

   rc = (USHORT)mciGetErrorString(ulError, mmerror+len, sizeof mmerror - len);
   mmerror[sizeof mmerror-1] = 0;
   if (rc != MCIERR_SUCCESS)
      sprintf(mmerror+len, "Cannot query error message: %d.\n", rc);

   (*a->error_display)(mmerror);

   return ulError;
}

static ULONG OS2Error(OS2RECORD *a, ULONG ulError, const char* location)
{
   unsigned char mmerror[1024];
   ULONG rc;
   int len;
   
   sprintf(mmerror, "OS/2 Error %d at %.30s: ", ulError, location);
   len = strlen(mmerror);

   os2_strerror(ulError, mmerror+len, sizeof mmerror - len);
   mmerror[sizeof mmerror-1] = 0;

   (*a->error_display)(mmerror);

   return ulError;
}


static ULONG output_close(OS2RECORD *a)
{
   ULONG rc;
   MCI_GENERIC_PARMS gpa = {0};

   // terminat request to decoder thread
   a->terminate = TRUE;
   if (a->QSignal != NULLHANDLE)
      DosPostEventSem(a->QSignal);
   
   // stop playback
   if ( a->playing )
   {  rc = (USHORT)mciSendCommand( a->device, MCI_STOP, MCI_WAIT, &gpa, 0 );
      if (rc != NO_ERROR)
         DEBUGLOG(("os2rec:output_close: MCI_STOP - %d\n", rc));
      a->playing = false;
   }
   
   // join decoder thread
   rc = DosWaitThread( &a->tid, DCWW_WAIT );
   if (rc != NO_ERROR)
      DEBUGLOG(("os2rec:output_close: DosWaitThread - %d\n", rc));

   // release queue handles
   if ( a->QSignal != NULLHANDLE );  
   {  rc = DosCloseEventSem(a->QSignal);
      if (rc != NO_ERROR)
         DEBUGLOG(("os2rec:output_close: DosCloseEventSem QSignal - %d\n", rc));
      a->QSignal = NULLHANDLE;
   }
   if ( a->QMutex != NULLHANDLE )
   {  rc = DosCloseMutexSem(a->QMutex);
      if (rc != NO_ERROR)
         DEBUGLOG(("os2rec:output_close: DosCloseMutexSem QMutex - %d\n", rc));
      a->QMutex = NULLHANDLE;
   }

   // free mix buffers
   if ( a->mcibuffers )
   {  rc = (USHORT)mciSendCommand( a->device, MCI_BUFFER, MCI_WAIT|MCI_DEALLOCATE_MEMORY, &a->bpa, 0 );
      if ( rc != MCIERR_SUCCESS )
         DEBUGLOG(("os2rec:output_close: MCI_BUFFER - %d\n", rc));
      a->mcibuffers = FALSE;
   }

   free(a->MixBuffers);
   a->MixBuffers = NULL;

   // close device
   if ( a->opened )
   {  rc = (USHORT)mciSendCommand( a->device, MCI_CLOSE, MCI_WAIT, &gpa, 0 );
      if ( rc != MCIERR_SUCCESS )
         DEBUGLOG(("os2rec:output_close: MCI_CLOSE - %d\n", rc));
      a->opened = FALSE;
   }
   
   a->terminate = FALSE;
   return 0;
}

static ULONG device_open(OS2RECORD *a)
{
   ULONG rc,i;
   
   if (a->opened) // trash buffers?
      return 101;

   // open the mixer device
   {  MCI_OPEN_PARMS opa;
      memset(&opa, 0, sizeof opa);
      opa.pszDeviceType = a->params.device;//(PSZ)MAKEULONG(MCI_DEVTYPE_AUDIO_AMPMIX, a->device);

      rc = (USHORT)mciSendCommand(0, MCI_OPEN, a->params.lockdevice ? MCI_WAIT : MCI_WAIT|MCI_OPEN_SHAREABLE, &opa, 0); // cut rc to 16 bit...
      if (rc != MCIERR_SUCCESS)
         return MciError(a, rc, "MCI_OPEN");
      // save return values
      a->device = opa.usDeviceID;
      a->opened = TRUE;
   }
   // Set the MCI_MIXSETUP_PARMS data structure to match the audio stream.
   {  MCI_MIXSETUP_PARMS  mspa;
      memset(&mspa, 0, sizeof mspa);

      mspa.ulBitsPerSample = a->params.format.bits;
      mspa.ulFormatTag     = a->params.format.format; // MCI_WAVE_FORMAT_PCM, hopefully
      mspa.ulSamplesPerSec = a->params.format.samplerate;
      mspa.ulChannels      = a->params.format.channels;
      // Setup the mixer for playback of wave data
      mspa.ulFormatMode    = MCI_RECORD;
      mspa.ulDeviceType    = MCI_DEVTYPE_WAVEFORM_AUDIO;
      mspa.pmixEvent       = &DARTEvent;

      rc = (USHORT)mciSendCommand(a->device, MCI_MIXSETUP, MCI_WAIT | MCI_MIXSETUP_INIT, &mspa, 0);
      if (rc != MCIERR_SUCCESS)
      {  output_close(a);
         return MciError(a,rc,"MCI_MIXSETUP");
      }
      DEBUGLOG(("os2rec:device_open: MIXSETUP - %d %d %d\n", mspa.ulMixHandle, mspa.ulBufferSize, mspa.ulNumBuffers));
      // save return values
      a->MixHandle = mspa.ulMixHandle;
      a->pmixRead  = mspa.pmixRead;
      if (a->params.numbuffers == 0)
         a->params.numbuffers = mspa.ulNumBuffers;
      if (a->params.buffersize == 0)
         a->params.buffersize = mspa.ulBufferSize;
   }
   // constraints
   if (a->params.numbuffers < 5)
     a->params.numbuffers = 5;
   else if (a->params.numbuffers > 100)
     a->params.numbuffers = 100;
   a->params.buffersize -= a->params.buffersize % a->audio_buffersize;
   if (a->params.buffersize >= 65536)
     a->params.buffersize = 65535 - 65535 % a->audio_buffersize;
   if (a->params.buffersize < a->audio_buffersize)
     a->params.buffersize = a->audio_buffersize;
   // Set up the BufferParms data structure and allocate device buffers from the Amp-Mixer
   {  free(a->MixBuffers);
      a->MixBuffers = calloc(a->params.numbuffers, sizeof(*a->MixBuffers));

      memset(&a->bpa, 0, sizeof a->bpa);
      a->bpa.ulNumBuffers = a->params.numbuffers;
      a->bpa.ulBufferSize = a->params.buffersize;
      a->bpa.pBufList     = a->MixBuffers;

      rc = (USHORT)mciSendCommand( a->device, MCI_BUFFER, MCI_WAIT|MCI_ALLOCATE_MEMORY, &a->bpa, 0 );
      if ( rc != MCIERR_SUCCESS )
      {  output_close(a);
         return MciError(a,rc, "MCI_BUFFER");
      }
      a->mcibuffers = TRUE;                 
   }
   // setup queue
   rc = DosCreateMutexSem(NULL, &a->QMutex, 0, FALSE);
   if (rc != NO_ERROR)
   {  output_close(a);
      return OS2Error(a,rc,"DosCreateMutexSem QMutex");
   }
   rc = DosCreateEventSem(NULL, &a->QSignal, 0, FALSE);
   if (rc != NO_ERROR)
   {  output_close(a);
      return OS2Error(a,rc,"DosCreateEventSem QSignal");
   }
   
   a->QHead = NULL;
   a->QTail = NULL;
   
   // start decoder thread
   a->tid = _beginthread(&DecoderThread, NULL, 65536, a);
   if (a->tid == (ULONG)-1)
   {  (*a->error_display)("Failed to create decoder thread.");
      output_close(a);
      return errno;
   }
   
   // setup buffers
   {  int i;
      for (i = 0; i < a->bpa.ulNumBuffers; i++)
      {  DEBUGLOG2(("xx: %d %d\n", i, a->MixBuffers[i].ulBufferLength));
         a->MixBuffers[i].ulUserParm = (ULONG)a;
      }
   }
   
   // start recording
   (*a->pmixRead)( a->MixHandle, a->MixBuffers, a->bpa.ulNumBuffers);

   a->playing = TRUE;
   DEBUGLOG(("os2rec:decice_open: completed\n"));

   return 0;
}


ULONG PM123_ENTRY decoder_command(void *A, ULONG msg, DECODER_PARAMS *info)
{  OS2RECORD *a = (OS2RECORD *)A;
   DEBUGLOG(("os2rec:decoder_command(%p, %i, %p)\n", a, msg, info));

   switch(msg)
   {case DECODER_PLAY:
      a->params = defaults;
      if (!parseURL(info->other == NULL ? info->URL : info->other, &a->params))
      {  return 100;
      }
      return device_open(a);

    case DECODER_STOP:
      return output_close(a);

    case DECODER_SETUP:
      a->output_play_samples = info->output_play_samples;
      a->a                   = info->a;
      a->audio_buffersize    = info->audio_buffersize;
      a->error_display       = info->error_display;
      a->info_display        = info->info_display;
      a->playsem             = info->playsem;
      a->hwnd                = info->hwnd;
      return 0;
         
    default:
      return 1;
   }
}

// Status interface
ULONG PM123_ENTRY
decoder_status( void *A )
{  OS2RECORD *a = (OS2RECORD *)A;
   
   if (a->terminate)
      return DECODER_STOPPED;
   if (a->playing)
      return DECODER_PLAYING;
   if (a->opened)
      return DECODER_STARTING;
   return DECODER_STOPPED;  
}

ULONG PM123_ENTRY
decoder_length( void *w )
{  return (ULONG)-1;
}


int PM123_ENTRY plugin_query(PLUGIN_QUERYPARAM *param)
{
   param->type = PLUGIN_DECODER;
   param->author = "Marcel Mueller";
   param->desc = "OS2RECORD 1.0";
   param->configurable = FALSE;

   //load_ini();
   return 0;
}


/*static ULONG output_get_devices(char *name, int deviceid)
{
   char buffer[256];
   MCI_SYSINFO_PARMS mip;

   if(deviceid && name)
   {
      MCI_SYSINFO_LOGDEVICE mid;

      mip.pszReturn = buffer;
      mip.ulRetSize = sizeof(buffer);
      mip.usDeviceType = MCI_DEVTYPE_AUDIO_AMPMIX;
      mip.ulNumber = deviceid;

      mciSendCommand(0,
                     MCI_SYSINFO,
                     MCI_WAIT | MCI_SYSINFO_INSTALLNAME,
                     &mip,
                     0);

      mip.ulItem = MCI_SYSINFO_QUERY_DRIVER;
      mip.pSysInfoParm = &mid;
      strcpy(mid.szInstallName,buffer);

      mciSendCommand(0,
                     MCI_SYSINFO,
                     MCI_WAIT | MCI_SYSINFO_ITEM,
                     &mip,
                     0);

      sprintf(name,"%s (%s)", mid.szProductInfo, mid.szVersionNumber);
   }

   mip.pszReturn = buffer;
   mip.ulRetSize = sizeof(buffer);
   mip.usDeviceType = MCI_DEVTYPE_AUDIO_AMPMIX;

   mciSendCommand(0,
                  MCI_SYSINFO,
                  MCI_WAIT | MCI_SYSINFO_QUANTITY,
                  &mip,
                  0);

   return atoi(mip.pszReturn);
} */


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

int PM123_ENTRY plugin_configure(HWND hwnd, HMODULE module)
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

