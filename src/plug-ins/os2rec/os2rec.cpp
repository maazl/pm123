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

#define DECODER_PLUGIN_LEVEL 1
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
#include "os2rec.h"

#include <debuglog.h>


static void load_ini(void);
static void save_ini(void);

// confinuration parameters
typedef struct
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
};


typedef struct
{
   // setup parameters
   PARAMETERS          params;

   int  (DLLENTRYP output_play_samples)( void* a, const FORMAT_INFO* format, const char* buf, int len, int posmarker );
   void* a;           /* only to be used with the precedent function */
   int   audio_buffersize;
   void (DLLENTRYP error_display)( const char* );
   void (DLLENTRYP info_display )( const char* );
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


/* checks whether value is an abrevation of key with at least minlen characters length */
static BOOL
abbrev(const char* value, const char* key, int minlen)
{ int len = strlen(value);
  if (len < minlen)
    return FALSE;
  return strnicmp(value, key, len) == 0;
}

/* decompose record:/// url into params structure. Return FALSE on error. 
 */
static BOOL
parseURL( const char* url, PARAMETERS* params )
{ int i,j,k;
  const char* cp;
  if (strnicmp(url, "record:///", 10) != 0)
    return FALSE;
  url += 10;
  // extract device
  i = strcspn(url, "\\/?");
  if (i >= sizeof params->device)
    return FALSE;
  if (sscanf(url, "%d%n", &j, &k) == 1 && k == i)
  { // bare device number
    sprintf(params->device, MCI_DEVTYPE_AUDIO_AMPMIX_NAME"%02d", j);
  } else if (i != 0)
  { memcpy(params->device, url, i);
    params->device[i] = 0;
  }
  // fetch parameters
  switch (url[i])
  {default: // = case 0
    return TRUE;
   case '\\':
   case '/':
    return url[i+1] == 0;
   case '?':
    cp = url+i;
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
    if (abbrev(key, "samplerate", 4))
    { if (sscanf(value, "%i%n", &i, &j) != 1 || j != strlen(value) || i <= 0)
        return FALSE; // syntax error
      params->format.samplerate = i;
    } else
    if (abbrev(key, "channels", 2))
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
    if (abbrev(key, "bits", 3))
    { if (sscanf(value, "%i%n", &i, &j) != 1 || j != strlen(value) || i <= 0)
        return FALSE; // syntax error
      params->format.bits = i;
    } else
    if (abbrev(key, "shared", 5))
    { if (abbrev(value, "no", 1) || strcmp(value, "0") == 0)
        params->lockdevice = TRUE;
       else if (abbrev(value, "yes", 0) || strcmp(value, "1") == 0)
        params->lockdevice = FALSE;
       else
        return FALSE; // bad value
    } else
    if (abbrev(key, "input", 2))
    { if (abbrev(value, "line", 1))
        params->connector = MCI_LINE_IN_CONNECTOR;
       else if (abbrev(value, "mic", 1))
        params->connector = MCI_MICROPHONE_CONNECTOR;
       else if (abbrev(value, "digital", 1))
        params->connector = MCI_AMP_STREAM_CONNECTOR;
       else
        return FALSE;
    } else // bad param
      return FALSE;
  }
  return TRUE;  
}


int DLLENTRY
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

ULONG DLLENTRY
decoder_uninit( void *a )
{
   DEBUGLOG(("os2rec:decoder_uninit(%p)\n", a));
   free(a);

   return 0;
}

ULONG DLLENTRY
decoder_support( char* ext[], int* size )
{
  if( size )
    *size = 0;

  return DECODER_OTHER;
}

static const char*
get_connector_name(ULONG connector)
{ switch (connector)
  {case MCI_LINE_IN_CONNECTOR:
    return "line";
   case MCI_MICROPHONE_CONNECTOR:
    return "mic.";
   case MCI_AMP_STREAM_CONNECTOR:
    return "digital";
   default:
    return NULL;
  }
}

ULONG DLLENTRY
decoder_fileinfo( const char* filename, DECODER_INFO* info )
{ const char* cp;
  PARAMETERS params  = defaults;
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
  /*info->numchannels  = 0; // no MOD
  info->numpatterns  = 0; // no MOD
  info->numpositions = 0; // no MOD*/
  sprintf(info->tech_info, "%d bit, %.1f kHz, %s", params.format.bits, params.format.samplerate/1000.,
                                                 params.format.channels == 1 ? "mono" : "stereo");
  cp = get_connector_name(params.connector);
  if (cp == NULL)
    sprintf(info->title, "Record: %.32s-%d", params.device, params.connector);
   else
    sprintf(info->title, "Record: %.32s-%s", params.device, cp);
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
ULONG DLLENTRY
decoder_trackinfo( const char* drive, int track, DECODER_INFO* info ) {
  return 200;
}

ULONG DLLENTRY
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

static void TFNENTRY
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
   char mmerror[180];
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
   char mmerror[1024];
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
   // select input
   {  MCI_CONNECTOR_PARMS cpa;
      memset(&cpa, 0, sizeof cpa);

      cpa.ulConnectorType = a->params.connector;

      rc = (USHORT)mciSendCommand(a->device, MCI_CONNECTOR, MCI_WAIT|MCI_ENABLE_CONNECTOR|MCI_CONNECTOR_TYPE, &cpa, 0);
      if (rc != MCIERR_SUCCESS)
      {  output_close(a);
         return MciError(a,rc,"MCI_CONNECTOR");
      }
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

      rc = (USHORT)mciSendCommand(a->device, MCI_MIXSETUP, MCI_WAIT|MCI_MIXSETUP_INIT, &mspa, 0);
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
      a->MixBuffers = (MCI_MIX_BUFFER*)calloc(a->params.numbuffers, sizeof(*a->MixBuffers));

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


ULONG DLLENTRY decoder_command(void *A, ULONG msg, DECODER_PARAMS *info)
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
ULONG DLLENTRY
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

ULONG DLLENTRY
decoder_length( void *w )
{  return (ULONG)-1;
}


int DLLENTRY plugin_query(PLUGIN_QUERYPARAM *param)
{
   param->type = PLUGIN_DECODER;
   param->author = "Marcel Mueller";
   param->desc = "OS2 recording plug-in 1.0";
   param->configurable = FALSE;

   //load_ini();
   return 0;
}


static ULONG get_devices(MCI_SYSINFO_LOGDEVICE* info, int deviceid)
{
   char buffer[256];
   MCI_SYSINFO_PARMS mip;

   if (info)
   {  mip.pszReturn = buffer;
      mip.ulRetSize = sizeof(buffer);
      mip.usDeviceType = MCI_DEVTYPE_AUDIO_AMPMIX;
      mip.ulNumber = deviceid;
      mciSendCommand(0, MCI_SYSINFO, MCI_WAIT | MCI_SYSINFO_INSTALLNAME, &mip, 0);

      mip.ulItem = MCI_SYSINFO_QUERY_DRIVER;
      mip.pSysInfoParm = info;
      strcpy(info->szInstallName, buffer);
      mciSendCommand(0, MCI_SYSINFO, MCI_WAIT | MCI_SYSINFO_ITEM,        &mip, 0);
   } else
   {  mip.pszReturn = buffer;
      mip.ulRetSize = sizeof(buffer);
      mip.usDeviceType = MCI_DEVTYPE_AUDIO_AMPMIX;
      mciSendCommand(0, MCI_SYSINFO, MCI_WAIT | MCI_SYSINFO_QUANTITY,    &mip, 0);
   }
   return atoi(mip.pszReturn);
}


/********** GUI stuff ******************************************************/ 

struct WizzardDlgResult
{ char* url;
  ULONG size;
};

static MRESULT EXPENTRY WizzardDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{ 
  const int samprates[] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 96000};
  DEBUGLOG2(("os2rec:WizzardDlgProc(%p, %p, %p, %p)\n", hwnd, msg, mp1, mp2));
  
  switch (msg)
  {case WM_INITDLG:
    // Store data pointer
    WinSetWindowULong(hwnd, QWL_USER, (ULONG)mp2);

    // Initialize devices
    { HWND hwndctrl = WinWindowFromID(hwnd, CB_DEVICE);
      int numdevice = get_devices(NULL, 0);
      WinSendMsg(hwndctrl, LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP("Default"));
      for (int i = 1; i <= numdevice; i++)
      { MCI_SYSINFO_LOGDEVICE info;
        char temp[256];
        get_devices(&info, i);
        //DEBUGLOG(("os2rec:WizzardDlgProc:WM_INITDLG: %s %s - %s\n", info.szInstallName, info.szResourceName, defaults.device));
        sprintf(temp, "%s (%s)", info.szProductInfo, info.szVersionNumber);
        WinSendMsg(hwndctrl, LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(temp));
        // select by resource name ?
        if ( stricmp(info.szResourceName, defaults.device) == 0 ||
             (i == 0 && stricmp(MCI_DEVTYPE_AUDIO_AMPMIX_NAME"00", defaults.device) == 0) )
          WinSendMsg(hwndctrl, LM_SELECTITEM, MPFROMSHORT(i), MPFROMSHORT(TRUE));
      }
      // select by ID ?
      { const char* cp = defaults.device;
        if (strnicmp(cp, MCI_DEVTYPE_AUDIO_AMPMIX_NAME, strlen(MCI_DEVTYPE_AUDIO_AMPMIX_NAME)) == 0)
          cp += strlen(MCI_DEVTYPE_AUDIO_AMPMIX_NAME);
        int i, len;
        if (sscanf(cp, "%u%n", &i, &len) == 1 && len == strlen(cp))
          WinSendMsg(hwndctrl, LM_SELECTITEM, MPFROMSHORT(i), MPFROMSHORT(TRUE));
      }
    }
    // Initialize share mode
    WinCheckButton(hwnd, defaults.lockdevice ? RB_EXCL : RB_SHARE, TRUE);
    // Initialize input
    WinCheckButton(hwnd, defaults.connector == MCI_MICROPHONE_CONNECTOR ? RB_MIC : defaults.connector == MCI_AMP_STREAM_CONNECTOR ? RB_DIGITAL : RB_LINE, TRUE);
    // Initialize sampling rate
    { HWND hwndctrl = WinWindowFromID(hwnd, SB_SAMPRATE);
      WinSendMsg(hwndctrl, SPBM_SETMASTER,       NULLHANDLE,       0);
      WinSendMsg(hwndctrl, SPBM_SETTEXTLIMIT,    MPFROMSHORT(6),   0);
      WinSendMsg(hwndctrl, SPBM_SETLIMITS,       MPFROMLONG(200000), MPFROMLONG(1000));
      WinSendMsg(hwndctrl, SPBM_SETCURRENTVALUE, MPFROMLONG(defaults.format.samplerate), 0);
    }
    // Initialize number of channels
    WinCheckButton(hwnd, defaults.format.channels == 1 ? RB_MONO : RB_STEREO, TRUE);
    // Initialize format
    WinCheckButton(hwnd, defaults.format.bits == 8 ? RB_8BIT : defaults.format.bits == 24 ? RB_24BIT : RB_16BIT, TRUE);
    break;
    
   case WM_COMMAND:
    DEBUGLOG(("os2rec:WizzardDlgProc(%p, WM_COMMAND, %p, %p)\n", hwnd, mp1, mp2));
    switch (SHORT1FROMMP(mp1))
    {case DID_OK:
      // Construct URL
      { char url[128];
        LONG samp;
        WinSendDlgItemMsg(hwnd, SB_SAMPRATE, SPBM_QUERYVALUE, MPFROMP(&samp), 0);
        sprintf(url, "record:///%u?samp=%u&%s&bits=%u&in=%s&share=%s",
          SHORT1FROMMR(WinSendDlgItemMsg(hwnd, CB_DEVICE, LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), 0)),
          samp,
          WinQueryButtonCheckstate(hwnd, RB_MONO) ? "mono" : "stereo",
          WinQueryButtonCheckstate(hwnd, RB_8BIT) ? 8      : WinQueryButtonCheckstate(hwnd, RB_24BIT)   ? 24        : 16,
          WinQueryButtonCheckstate(hwnd, RB_MIC)  ? "mic"  : WinQueryButtonCheckstate(hwnd, RB_DIGITAL) ? "digital" : "line",
          WinQueryButtonCheckstate(hwnd, RB_EXCL) ? "no"   : "yes");
        // store result
        WizzardDlgResult* dp = (WizzardDlgResult*)WinQueryWindowULong(hwnd, QWL_USER);
        DEBUGLOG(("os2rec:WizzardDlgProc: dp=%p->{%p,%u}\n", dp, dp->url, dp->size));
        size_t len = strlen(url)+1;
        if (dp->size >= len +1)
        { memcpy(dp->url, url, len);
          dp->url[len] = 0; // double zero termination
        }
        dp->size = len +1;
      }
      break;
    }
    break;

   case WM_CONTROL:
    DEBUGLOG(("os2rec:WizzardDlgProc(%p, WM_CONTROL, %p, %p)\n", hwnd, mp1, mp2));
    switch (SHORT1FROMMP(mp1))
    {case SB_SAMPRATE:
      switch (SHORT2FROMMP(mp1))
      { int  i;
        LONG val;

       case SPBN_DOWNARROW:
        WinSendDlgItemMsg(hwnd, SB_SAMPRATE, SPBM_QUERYVALUE, MPFROMP(&val), 0);
        for (i = sizeof samprates/sizeof *samprates; --i >= 0; )
          if (samprates[i] < val)
            break;
        //DEBUGLOG(("os2rec:WizzardDlgProc: SPBN_UPARROW - %i %i %i\n", val, i, samprates[(i+sizeof samprates/sizeof *samprates) % (sizeof samprates/sizeof *samprates)]));
        WinSendDlgItemMsg(hwnd, SB_SAMPRATE, SPBM_SETCURRENTVALUE, MPFROMLONG(samprates[(i+sizeof samprates/sizeof *samprates) % (sizeof samprates/sizeof *samprates)]), 0);
        return 0;
        
       case SPBN_UPARROW:
        WinSendDlgItemMsg(hwnd, SB_SAMPRATE, SPBM_QUERYVALUE, MPFROMP(&val), 0);
        for (i = 0; i < sizeof samprates/sizeof *samprates; ++i )
          if (samprates[i] > val)
            break;
        //DEBUGLOG(("os2rec:WizzardDlgProc: SPBN_DOWNARROW - %i %i %i\n", val, i, samprates[i % (sizeof samprates/sizeof *samprates)]));
        WinSendDlgItemMsg(hwnd, SB_SAMPRATE, SPBM_SETCURRENTVALUE, MPFROMLONG(samprates[i % (sizeof samprates/sizeof *samprates)]), 0);
        return 0;
      }
      break;   
    }
    break;
  }
  return WinDefDlgProc(hwnd, msg, mp1, mp2);
}

static ULONG DLLENTRY WizzardDlg(HWND owner, char* select, ULONG size)
{ DEBUGLOG(("os2rec:WizzardDlg(%p, %p, %d)\n", owner, select, size));  
  HMODULE mod;
  getModule( &mod, NULL, 0 );

  WizzardDlgResult res = { select, size };
  switch (WinDlgBox(HWND_DESKTOP, owner, WizzardDlgProc, mod, DLG_RECWIZZARD, &res))
  {case DID_OK:
    return res.size <= size ? 0 : 100;

   case DID_CANCEL:
    return 300;

   default:
    return 500;
  }
}

const DECODER_WIZZARD wizzardentry =
{ NULL,
  "Record...",
  0, 0,
  &WizzardDlg
};

/* DLL entry point */
const DECODER_WIZZARD* DLLENTRY decoder_getwizzard( BOOL multiselect )
{ return &wizzardentry;
}

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

