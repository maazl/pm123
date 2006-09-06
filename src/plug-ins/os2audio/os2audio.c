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

/* OS/2 RealTime DART Engine for PM123 */

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

//#define DEBUG 2

#include <debuglog.h>

#include <format.h>
#include <output_plug.h>
#include <decoder_plug.h>
#include <plugin.h>
#include "os2audio.h"


static void load_ini(void);
static void save_ini(void);

int device = 0;
int lockdevice = 0;
int numbuffers = 32; /* total audio buffers, _bare_ minimum = 4 (cuz of prio boost check) */
int kludge48as44 = 0;
int force8bit = 0;

typedef struct
{
   MCI_MIX_BUFFER  *NextBuffer;
   int filepos;
   void *a; /* stores OS2AUDIO */
} BUFFERINFO;

typedef struct
{
   unsigned char mmerror[160];
   ULONG playingpos;

   int device;
   int lockdevice;
   int numbuffers; /* total audio buffers, _bare_ minimum = 4 (cuz of prio boost check) */
   int kludge48as44;
   int force8bit;
   int buffersize;
   int volume;
   int doamp;
   float amplifier;

   unsigned short boostclass, normalclass;
   signed   short boostdelta, normaldelta;
   TIB *mainthread; /* thread info to set thread priority */

   /* audio buffers */
   ULONG ulMCIBuffers;

   MCI_AMP_OPEN_PARMS  maop;
   MCI_MIXSETUP_PARMS  mmp;
   MCI_BUFFER_PARMS    mbp;
   MCI_GENERIC_PARMS   mgp;
   MCI_SET_PARMS       msp;
   MCI_STATUS_PARMS    mstatp;
   MCI_MIX_BUFFER      *MixBuffers;

   ULONG zero;  /* this is 128 for 8 bit unsigned */

   BUFFERINFO *bufferinfo;

   HEV dataplayed;
   ULONG resetcount;

   MCI_MIX_BUFFER *tobefilled, *playingbuffer, playedbuffer;
   void *pBufferplayed;

   BOOL nomoredata,nobuffermode,justtrashed, opened;

   OUTPUT_PARAMS original_info; // to open the device

   BOOL pausestate;
} OS2AUDIO;


static LONG APIENTRY DARTEvent(ULONG ulStatus, MCI_MIX_BUFFER *PlayedBuffer, ULONG ulFlags)
{
   OS2AUDIO *a = (OS2AUDIO*) ((BUFFERINFO *) PlayedBuffer->ulUserParm)->a;

   switch(ulFlags)
   {
      case MIX_STREAM_ERROR | MIX_WRITE_COMPLETE:  /* error occur in device */

      if ( ulStatus == ERROR_DEVICE_UNDERRUN)
         /* Write buffers to rekick off the amp mixer. */
         a->mmp.pmixWrite( a->mmp.ulMixHandle,
                        a->MixBuffers,
                        a->ulMCIBuffers );
      break;

   case MIX_WRITE_COMPLETE:                     /* for playback  */

      /* GUS driver workaround */
//      a->mstatp.ulItem = MCI_STATUS_POSITION;
      mciSendCommand( a->maop.usDeviceID,
                           MCI_STATUS,
                           MCI_STATUS_ITEM | MCI_WAIT,
                           &a->mstatp,
                           0 );

      a->playingbuffer = ((BUFFERINFO *) PlayedBuffer->ulUserParm)->NextBuffer;

      /* the next four lines are only useful to audio_playing_samples() */
      if(a->pBufferplayed != NULL)
      {
         memcpy(&a->playedbuffer,PlayedBuffer,sizeof(a->playedbuffer));
         a->playedbuffer.ulTime = a->mstatp.ulReturn;
         a->playedbuffer.pBuffer = a->pBufferplayed;
         memcpy(a->playedbuffer.pBuffer, PlayedBuffer->pBuffer, PlayedBuffer->ulBufferLength);
      }

      /* just too bad, the decoder fell behind... here we just keep the
         buffer to be filled in front of the playing one so that when the
         decoder kicks back in, we'll hear it in at the right time */
      if(a->tobefilled == a->playingbuffer)
      {
         a->tobefilled = ((BUFFERINFO *) a->playingbuffer->ulUserParm)->NextBuffer;
         a->nomoredata = TRUE;

         WinPostMsg(a->original_info.hwnd,WM_OUTPUT_OUTOFDATA,0,0);
      }
      else
      {
         a->playingpos = ((BUFFERINFO *) a->playingbuffer->ulUserParm)->filepos;

         /* if we're about to be short of decoder's data
            (2nd ahead buffer not filled), let's boost its priority! */
         if(a->mainthread != NULL &&
            a->tobefilled == ( (BUFFERINFO *) ((BUFFERINFO *) a->playingbuffer->ulUserParm)->NextBuffer->ulUserParm)->NextBuffer)
         {  DEBUGLOG(("DARTEvent: priority boost (%d, %d, %d).\n", a->boostclass,a->boostdelta,a->mainthread->tib_ptib2->tib2_ultid));
            DosSetPriority(PRTYS_THREAD,a->boostclass,a->boostdelta,a->mainthread->tib_ptib2->tib2_ultid);
         }
      }

      /* empty the played buffer in case it doesn't get filled back */
      memset(PlayedBuffer->pBuffer,a->zero,PlayedBuffer->ulBufferLength);

      DosPostEventSem(a->dataplayed);

      if(a->opened)  // set to FALSE before actual device close
      {
         a->mmp.pmixWrite( a->mmp.ulMixHandle,
                           PlayedBuffer /* will contain new data */,
                           1 );
      }
      break;

   } /* end switch */

   return( TRUE );

} /* end DARTEvent */


static void MciError(OS2AUDIO *a, ULONG ulError)
{
   unsigned char mmerror[128];
   unsigned char buffer[128];
   ULONG rc;

   rc = mciGetErrorString(ulError, buffer, sizeof(buffer));

   if (rc == MCIERR_SUCCESS)
      sprintf(mmerror,"MCI Error %d: %s\n",LOUSHORT(ulError),buffer);
   else
      sprintf(mmerror,"MCI Error %d: Cannot query error message.\n",LOUSHORT(rc));

   a->original_info.error_display(mmerror);

   WinPostMsg(a->original_info.hwnd,WM_PLAYERROR,0,0);
}

static ULONG output_set_volume(void *A, char setvolume, float setamplifier)
{
   OS2AUDIO *a = (OS2AUDIO *) A;

   if(setamplifier != 1.0)
   {
      a->doamp = TRUE;
      a->amplifier = setamplifier;
   }
   else
      a->doamp = FALSE;

   if(setvolume > 100) setvolume = 100;
   a->volume = setvolume; /* useful when device is closed and reopened */
   if(a->maop.usDeviceID)
   {
      MCI_SET_PARMS msp = {0};
      msp.ulAudio = MCI_SET_AUDIO_ALL;
      msp.ulLevel = setvolume;

      mciSendCommand(a->maop.usDeviceID, MCI_SET,
                     MCI_WAIT | MCI_SET_AUDIO | MCI_SET_VOLUME,
                     &msp, 0);
   }
   return 0;
}

static ULONG PM123_ENTRY output_pause(void *A, BOOL pause)
{
   OS2AUDIO *a = (OS2AUDIO *) A;

   if(a->maop.usDeviceID)
   {
      if(pause)
         mciSendCommand(a->maop.usDeviceID, MCI_PAUSE,
                        MCI_WAIT,
                        &a->mgp, 0);
      else
         mciSendCommand(a->maop.usDeviceID, MCI_RESUME,
                        MCI_WAIT,
                        &a->mgp, 0);

      a->pausestate = pause;
   }
   return 0;
}

ULONG PM123_ENTRY output_init(void **A)
{
   OS2AUDIO *a;
   DEBUGLOG(("output_init\n"));

   *A = malloc(sizeof(OS2AUDIO));
   a = (OS2AUDIO *) *A;
   memset(a,0,sizeof(*a));

   // load stuff
   a->device = 0;
   a->lockdevice = 0;
   a->numbuffers = 32;
   a->kludge48as44 = 1;
   a->force8bit = 0;
   a->opened = FALSE;
   a->nomoredata = TRUE;
   a->playingbuffer = NULL;
   a->volume = 100;

   return 0;
}


static ULONG output_open(OS2AUDIO *a)
{
   OUTPUT_PARAMS *ai = &a->original_info;

   ULONG rc,i;
   ULONG openflags;

   a->playingpos = 0;
   a->pausestate = FALSE;

   if(a->opened) // trash buffers?
      return 0;

   a->device = device;
   a->lockdevice = lockdevice;
   a->numbuffers = numbuffers;
   a->kludge48as44 = kludge48as44;
   a->force8bit = force8bit;

   if(ai->formatinfo.samplerate <= 0) ai->formatinfo.samplerate = 44100;
   if(ai->formatinfo.channels <= 0) ai->formatinfo.channels = 2;
   if(ai->formatinfo.format <= 0) ai->formatinfo.format = WAVE_FORMAT_PCM;
   if(ai->formatinfo.bits != 8 && !a->force8bit)
   {
      ai->formatinfo.bits = 16;
      a->zero = 0;
   }
   else
      a->zero = 128;

   /* open the mixer device */
   memset(&a->maop, 0, sizeof(a->maop));
   a->maop.usDeviceID = 0;
   a->maop.pszDeviceType = (PSZ) MAKEULONG(MCI_DEVTYPE_AUDIO_AMPMIX, a->device);

   openflags = MCI_WAIT | MCI_OPEN_TYPE_ID;
   if(!a->lockdevice) openflags |= MCI_OPEN_SHAREABLE;

   rc = mciSendCommand(0,
                       MCI_OPEN,
                       openflags,
                       &a->maop,
                       0);

   if (LOUSHORT(rc) != MCIERR_SUCCESS)
   {
      MciError(a,rc);
      a->maop.usDeviceID = 0;
      //free(a);
      return(rc);
   }

   /* Set the MCI_MIXSETUP_PARMS data structure to match the audio stream. */

   memset(&a->mmp, 0, sizeof(a->mmp));

   if(a->force8bit)
      a->mmp.ulBitsPerSample = 8;
   else
      a->mmp.ulBitsPerSample = ai->formatinfo.bits;
   a->mmp.ulFormatTag = ai->formatinfo.format; // MCI_WAVE_FORMAT_PCM;

   if(a->kludge48as44 && ai->formatinfo.samplerate == 48000)
      a->mmp.ulSamplesPerSec = 44100;
   else
      a->mmp.ulSamplesPerSec = ai->formatinfo.samplerate;
   a->mmp.ulChannels = ai->formatinfo.channels;

   /* Setup the mixer for playback of wave data */
   a->mmp.ulFormatMode = MCI_PLAY;
   a->mmp.ulDeviceType = MCI_DEVTYPE_WAVEFORM_AUDIO;
   a->mmp.pmixEvent    = DARTEvent;

   rc = mciSendCommand( a->maop.usDeviceID,
                        MCI_MIXSETUP,
                        MCI_WAIT | MCI_MIXSETUP_INIT,
                        &a->mmp,
                        0 );

   if ( LOUSHORT(rc) != MCIERR_SUCCESS )
   {
      MciError(a,rc);
      a->maop.usDeviceID = 0;
      //free(a);
      return(rc);
   }

   output_set_volume(a,a->volume,a->amplifier);

   /* Set up the BufferParms data structure and allocate
    * device buffers from the Amp-Mixer  */

   memset(&a->mbp, 0, sizeof(a->mbp));
   free(a->MixBuffers);
   free(a->bufferinfo);
   if(a->numbuffers < 5) a->numbuffers = 5;
   if(a->numbuffers > 200) a->numbuffers = 200;
   a->MixBuffers = calloc(a->numbuffers, sizeof(*a->MixBuffers));
   a->bufferinfo = calloc(a->numbuffers, sizeof(*a->bufferinfo));

   a->ulMCIBuffers = a->numbuffers;
   a->mbp.ulNumBuffers = a->ulMCIBuffers;
/*   mbp.ulBufferSize = mmp.ulBufferSize; */
   /* I don't like this... they must be smaller than 64KB or else the
      engine needs major rewrite */
   if(a->force8bit && ai->formatinfo.bits == 16)
      a->mbp.ulBufferSize = a->buffersize = ai->buffersize/2;
   else
      a->mbp.ulBufferSize = a->buffersize = ai->buffersize;
   a->mbp.pBufList = a->MixBuffers;

   rc = mciSendCommand( a->maop.usDeviceID,
                        MCI_BUFFER,
                        MCI_WAIT | MCI_ALLOCATE_MEMORY,
                        (PVOID) &a->mbp,
                        0 );

   if ( LOUSHORT(rc) != MCIERR_SUCCESS )
   {
      MciError(a,rc);
      a->maop.usDeviceID = 0;
      //free(a);
      return(rc);
   }

   a->pBufferplayed = a->playedbuffer.pBuffer = calloc(1,a->buffersize);

   a->ulMCIBuffers = a->mbp.ulNumBuffers; /* never know! */

   /* Fill all device buffers with zeros and set linked list */

   for(i = 0; i < a->ulMCIBuffers; i++)
   {
      a->MixBuffers[i].ulFlags = 0;
      a->MixBuffers[i].ulBufferLength = a->mbp.ulBufferSize;
      memset(a->MixBuffers[i].pBuffer, a->zero, a->MixBuffers[i].ulBufferLength);

      a->MixBuffers[i].ulUserParm = (ULONG) &a->bufferinfo[i];
      a->bufferinfo[i].a = a; /* useful in the DART event function */
      a->bufferinfo[i].NextBuffer = &a->MixBuffers[i+1];
   }

   a->bufferinfo[i-1].NextBuffer = &a->MixBuffers[0];

   /* Create a semaphore to know when data has been played by the DART thread */
   DosCreateEventSem(NULL,&a->dataplayed,0,FALSE);

   a->playingbuffer = &a->MixBuffers[0];
   a->tobefilled = &a->MixBuffers[1];
   a->playingpos = 0;
   a->nomoredata = TRUE;
   a->nobuffermode = FALSE;
   a->justtrashed = FALSE;

   a->boostclass = ai->boostclass;
   a->boostdelta = ai->boostdelta;
   a->normalclass = ai->normalclass;
   a->normaldelta = ai->normaldelta;

   if(a->boostclass > 4) a->boostdelta = 3;
   if(a->boostdelta > 31) a->boostdelta = 31;
   if(a->boostdelta < -31) a->boostdelta = -31;

   if(a->normalclass > 4) a->normaldelta = 3;
   if(a->normaldelta > 31) a->normaldelta = 31;
   if(a->normaldelta < -31) a->normaldelta = -31;

   a->mainthread = NULL;

   /* let's set it once */
   a->mstatp.ulItem = MCI_STATUS_POSITION;

   /* Write buffers to kick off the amp mixer. see DARTEvent() */
   rc = a->mmp.pmixWrite( a->mmp.ulMixHandle,
                       a->MixBuffers,
                       a->ulMCIBuffers );

   a->opened  = TRUE;

   return 0;
}

static ULONG output_close(void *A)
{
   OS2AUDIO *a = (OS2AUDIO *) A;

   ULONG rc;

   if(a->opened == FALSE)
      return 0;

   if(!a->maop.usDeviceID)
      return 0;

   while(!a->nomoredata)
   {
      DosResetEventSem(a->dataplayed,&a->resetcount);
      DosWaitEventSem(a->dataplayed, -1);
   }

   a->opened = FALSE;
//   a->nomoredata = TRUE;

   a->playingbuffer = NULL;

   DosCloseEventSem(a->dataplayed);
   a->dataplayed = 0;

   // to be kind to other output plug-ins
   if(a->mainthread != NULL)
      DosSetPriority(PRTYS_THREAD,a->normalclass,a->normaldelta,a->mainthread->tib_ptib2->tib2_ultid);

   rc = mciSendCommand( a->maop.usDeviceID,
                        MCI_BUFFER,
                        MCI_WAIT | MCI_DEALLOCATE_MEMORY,
                        &a->mbp,
                        0 );

   if ( LOUSHORT(rc) != MCIERR_SUCCESS )
   {
      MciError(a,rc);
      return(rc);
   }

   free(a->bufferinfo);
   free(a->MixBuffers);
   a->bufferinfo = NULL;
   a->MixBuffers = NULL;

   memset(&a->mbp, 0, sizeof(a->mbp));

   rc = mciSendCommand( a->maop.usDeviceID,
                        MCI_CLOSE,
                        MCI_WAIT ,
                        &a->mgp,
                        0 );

   if ( LOUSHORT(rc) != MCIERR_SUCCESS )
   {
      MciError(a,rc);
      return(rc);
   }

   memset(&a->maop, 0, sizeof(a->maop));

   free(a->pBufferplayed);
   a->pBufferplayed = NULL;

   a->opened = FALSE;
   a->nomoredata = TRUE;

   return 0;
}

ULONG PM123_ENTRY output_uninit(void *a)
{
   DEBUGLOG(("output_uninit\n"));
   free(a);

   return 0;
}


int PM123_ENTRY output_play_samples(void *A, FORMAT_INFO *format, char *buf,int len, int posmarker)
{
   OS2AUDIO *a = (OS2AUDIO*)A;
   PTIB ptib;

   DEBUGLOG(("output_play_samples({%i,%i,%i,%i,%x}, %p, %i, %i)\n",
      format->size, format->samplerate, format->channels, format->bits, format->format, buf, len, posmarker));

   DosGetInfoBlocks(&ptib,NULL);

   if(ptib != a->mainthread)
   {
      a->mainthread = ptib;
      DEBUGLOG(("output_play_samples: initial priority boost (%d, %d, %d).\n", a->boostclass, a->boostdelta, a->mainthread->tib_ptib2->tib2_ultid));
      DosSetPriority(PRTYS_THREAD,a->boostclass,a->boostdelta,a->mainthread->tib_ptib2->tib2_ultid);
   }

   // set the new format structure before re-opening
   if(memcmp(format, &a->original_info.formatinfo, sizeof(FORMAT_INFO)) != 0
      || a->opened == FALSE)
   {
      ULONG rc;

      a->original_info.formatinfo = *format;
      output_close(a);
      rc = output_open(a);

      if(rc != 0)
         return 0;
   }

   if(a->nobuffermode)
   {
      // sit until only two buffers are left filled
      MCI_MIX_BUFFER *temp = a->playingbuffer;

      while(
         (a->tobefilled != (temp = ((BUFFERINFO *) temp->ulUserParm)->NextBuffer)) &&
         (a->tobefilled != (temp = ((BUFFERINFO *) temp->ulUserParm)->NextBuffer)) &&
         (a->tobefilled != (temp = ((BUFFERINFO *) temp->ulUserParm)->NextBuffer)) )
         {
            DosResetEventSem(a->dataplayed,&a->resetcount);
            DosWaitEventSem(a->dataplayed, -1);
            temp = a->playingbuffer;
         }
   }
   else
      /* if we're too quick, let's wait */
      while(a->tobefilled == a->playingbuffer)
      {
         DosResetEventSem(a->dataplayed,&a->resetcount);
         DosWaitEventSem(a->dataplayed, -1);
      }

   if(a->pausestate)
      output_pause(a,TRUE);

   if(a->justtrashed)
      a->justtrashed = FALSE;
   else
   {
      int i;

      a->nomoredata = FALSE;

      if(a->force8bit && a->original_info.formatinfo.bits != 8)
      {
         signed short *bufin = (short *) buf;
         int zero = a->zero;
         unsigned char *bufout = a->tobefilled->pBuffer;

         for(i = 0; i < len/2; i++) // we suppose 16 bit data
            bufout[i] = (bufin[i]>>8)+zero;

         a->tobefilled->ulBufferLength = len/2;
      }
      else
      {
         memcpy(a->tobefilled->pBuffer, buf, len);
         a->tobefilled->ulBufferLength = len;
      }
      ((BUFFERINFO *) a->tobefilled->ulUserParm)->filepos = posmarker;

      /* if we're out of the water (3rd ahead buffer filled),
         let's reduce our priority */
      if(a->tobefilled == ( (BUFFERINFO *) ( (BUFFERINFO *) ((BUFFERINFO *) a->playingbuffer->ulUserParm)->NextBuffer->ulUserParm)->NextBuffer->ulUserParm)->NextBuffer)
      {
         DEBUGLOG(("output_play_samples: end of priority boost (%d).\n", a->mainthread->tib_ptib2->tib2_ultid));
         DosSetPriority(PRTYS_THREAD,a->normalclass,a->normaldelta,a->mainthread->tib_ptib2->tib2_ultid);
      }

      a->tobefilled = ((BUFFERINFO *) a->tobefilled->ulUserParm)->NextBuffer;
   }

   return len;
}

ULONG PM123_ENTRY output_playing_samples(void *A, FORMAT_INFO *info, char *buf, int len)
{
   OS2AUDIO *a = (OS2AUDIO *) A;
   DEBUGLOG2(("output_playing_samples(%p, %p, %i)\n", info, buf, len));

   if(len > a->buffersize || !a->playingbuffer || !a->maop.usDeviceID) return 1;

   info->bits = a->mmp.ulBitsPerSample;
   info->samplerate = a->mmp.ulSamplesPerSec;
   info->channels = a->mmp.ulChannels;
   info->format = a->mmp.ulFormatTag;

   if(buf && len)
   {
      ULONG rc;
      int upto;

//      a->mstatp.ulItem = MCI_STATUS_POSITION;

      rc = mciSendCommand( a->maop.usDeviceID,
                           MCI_STATUS,
                           MCI_STATUS_ITEM | MCI_WAIT,
                           &a->mstatp,
                           0 );

      if ( LOUSHORT(rc) != MCIERR_SUCCESS )
      {
         MciError(a,rc);
         a->maop.usDeviceID = 0;
         return(rc);
      }

      /* this is hypocrite...
         DART returns the value in ulReturn instead of ulValue,
         also it returns in milliseconds and not MMTIME... arg */

      upto = (a->mstatp.ulReturn-a->playedbuffer.ulTime) * a->mmp.ulSamplesPerSec / 1000;
      upto *= a->mmp.ulChannels * (a->mmp.ulBitsPerSample/8);

      /* if a timing problem occurs, let's at least not crash */
      if(upto > a->playingbuffer->ulBufferLength) upto = a->playingbuffer->ulBufferLength;

      if(len <= upto)
         memcpy(buf,(char *) (a->playingbuffer->pBuffer)+upto-len, len);
      else
      {
         memcpy(buf,(char *) a->playedbuffer.pBuffer+a->playedbuffer.ulBufferLength-(len-upto),len-upto);
         memcpy(buf+(len-upto),a->playingbuffer->pBuffer,upto);
      }
   }

   return 0;
}

ULONG PM123_ENTRY output_playing_pos(void *A)
{
   OS2AUDIO *a = (OS2AUDIO *) A;
   DEBUGLOG(("output_playing_pos: %lu\n", a->playingpos));
   return a->playingpos;
}

static void output_trash_buffers(void *A, ULONG temp_playingpos)
{
   OS2AUDIO *a = (OS2AUDIO *) A;
   int i;

   if(!a->opened)
      return;

   a->justtrashed = TRUE;

   /* Fill all device buffers with zeros */
   for(i = 0; i < a->ulMCIBuffers; i++)
      memset(a->MixBuffers[i].pBuffer, a->zero, a->MixBuffers[i].ulBufferLength);

   a->tobefilled = ((BUFFERINFO *) a->playingbuffer->ulUserParm)->NextBuffer;
   a->playingpos = temp_playingpos;
   ((BUFFERINFO *) a->tobefilled->ulUserParm)->filepos = temp_playingpos;
   if(a->mainthread != NULL)
      DosSetPriority(PRTYS_THREAD,a->boostclass,a->boostdelta,a->mainthread->tib_ptib2->tib2_ultid);
   a->nomoredata = TRUE;

   // return TRUE;
}

BOOL PM123_ENTRY output_playing_data(void *A)
{
   OS2AUDIO *a = (OS2AUDIO *) A;
   DEBUGLOG2(("output_playing_data: %i\n", !a->nomoredata));
   return !a->nomoredata;
}



#if 0

/*
 * get formats for specific channel/rate parameters
 */
int PM123_ENTRY output_get_formats(OUTPUT_PARAMS *ai)
{
   int fmts = 0;
   ULONG rc;
   MCI_MIXSETUP_PARMS mmptemp = {0};

   mmp.ulDeviceType = MCI_DEVTYPE_WAVEFORM_AUDIO;
   mmp.pmixEvent    = DARTEvent;

   mmptemp.ulFormatMode = MCI_PLAY;
   mmptemp.ulSamplesPerSec = ai->rate;
   mmptemp.ulChannels = ai->channels;

   mmptemp.ulFormatTag = MCI_WAVE_FORMAT_PCM;
   mmptemp.ulBitsPerSample = 16;
   rc = mciSendCommand( maop.usDeviceID,
                        MCI_MIXSETUP,
                        MCI_WAIT | MCI_MIXSETUP_QUERYMODE,
                        &mmptemp,
                        0 );
   if((LOUSHORT(rc) == MCIERR_SUCCESS) && (rc != 0x4000)) /* undocumented */
      fmts = fmts | AUDIO_FORMAT_SIGNED_16;

   mmptemp.ulFormatTag = MCI_WAVE_FORMAT_PCM;
   mmptemp.ulBitsPerSample = 8;
   rc = mciSendCommand( maop.usDeviceID,
                        MCI_MIXSETUP,
                        MCI_WAIT | MCI_MIXSETUP_QUERYMODE,
                        &mmptemp,
                        0 );
   if((LOUSHORT(rc) == MCIERR_SUCCESS) && (rc != 0x4000)) /* undocumented */
      fmts = fmts | AUDIO_FORMAT_UNSIGNED_8;

   mmptemp.ulFormatTag = MCI_WAVE_FORMAT_ALAW;
   mmptemp.ulBitsPerSample = 8;
   rc = mciSendCommand( maop.usDeviceID,
                        MCI_MIXSETUP,
                        MCI_WAIT | MCI_MIXSETUP_QUERYMODE,
                        &mmptemp,
                        0 );
   if((LOUSHORT(rc) == MCIERR_SUCCESS) && (rc != 0x4000)) /* undocumented */
      fmts = fmts | AUDIO_FORMAT_ALAW_8;

   mmptemp.ulFormatTag = MCI_WAVE_FORMAT_MULAW;
   mmptemp.ulBitsPerSample = 8;
   rc = mciSendCommand( maop.usDeviceID,
                        MCI_MIXSETUP,
                        MCI_WAIT | MCI_MIXSETUP_QUERYMODE,
                        &mmptemp,
                        0 );
   if((LOUSHORT(rc) == MCIERR_SUCCESS) && (rc != 0x4000)) /* undocumented */
      fmts = fmts | AUDIO_FORMAT_ULAW_8;

   return fmts;
}

int PM123_ENTRY output_rate_best_match(OUTPUT_PARAMS *ai)
{
   return 0;
}

#endif

static ULONG output_get_devices(char *name, int deviceid)
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
}


ULONG PM123_ENTRY output_command(void *A, ULONG msg, OUTPUT_PARAMS *info)
{
   OS2AUDIO *a = (OS2AUDIO *) A;
   ULONG rc = 0;
   DEBUGLOG(("output_command(%i, %p)\n", msg, info));

   switch(msg)
   {
      case OUTPUT_OPEN:
         rc = output_open(a);
         break;
      case OUTPUT_CLOSE:
         rc = output_close(a);
         break;
      case OUTPUT_VOLUME:
         rc = output_set_volume(a, info->volume, info->amplifier);
         break;
      case OUTPUT_PAUSE:
         rc = output_pause(a, info->pause);
         break;
      case OUTPUT_SETUP:
         if(!a->opened) // otherwize, information on the current session are modified
            a->original_info = *info;
         info->always_hungry = FALSE;
         break;
      case OUTPUT_TRASH_BUFFERS:
         output_trash_buffers(a,info->temp_playingpos);
         break;
      case OUTPUT_NOBUFFERMODE:
         a->nobuffermode = info->nobuffermode;
         break;
      default:
         rc = 1;
   }

   return rc;
}


/********** GUI stuff ******************************************************/ 

HWND dlghwnd = 0;

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

int PM123_ENTRY plugin_query(PLUGIN_QUERYPARAM *param)
{
   param->type = PLUGIN_OUTPUT;
   param->author = "Samuel Audet";
   param->desc = "OS2AUDIO 1.20";
   param->configurable = TRUE;

   load_ini();
   return 0;
}
