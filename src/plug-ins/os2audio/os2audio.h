/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 *
 * Copyright 2007-2008 Dmitry A.Steklenev <glass@ptv.ru>
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

#define DLG_CONFIGURE 1
#define ID_NULL       1000
#define CB_DEVICE     1010
#define CB_SHARED     1020
#define SB_BUFFERS    1030
#define CB_8BIT       1040
#define CB_48KLUDGE   1050
#define CB_RG_ENABLE  1060
#define CB_RG_TYPE    1070
#define SB_RG_PREAMP  1080
#define ST_RG_PREAMP  1090
#define SB_PREAMP     1100
#define ST_PREAMP     1110

typedef struct BUFFERINFO
{
  MCI_MIX_BUFFER*  next;
  ULONG            playingpos;
  ULONG            offset;
  struct OS2AUDIO* a;

} BUFFERINFO;

#define DEVICE_OPENING    1
#define DEVICE_OPENED     2
#define DEVICE_CLOSING    3
#define DEVICE_CLOSED     4
#define DEVICE_FAILED     5

#define OUTPUT_IS_HUNGRY 16 // buffers
#define OUTPUT_IS_SATED  32 // buffers


typedef struct OS2AUDIO
{
  ULONG playingpos;
  int   device;
  int   lockdevice;
  int   numbuffers;   /* Suggested total audio buffers.                       */
  int   filled;       /* Number of buffers that are ready to play.            */
  int   kludge48as44;
  int   force8bit;
  int   buffersize;   /* Suggested size of the audio buffers.                 */
  int   volume;
  float amplifier;
  int   zero;         /* This is 128 for 8 bit unsigned.                      */
  int   configured;

  float track_gain;   /* Defines Replay Gain values as specified at           */
  float track_peak;   /* http://www.replaygain.org                            */
  float album_gain;
  float album_peak;
  float scale;

  BOOL  trashed;      /* A next waiting portion of samples should be trashed. */
  BOOL  nomoredata;   /* The audio device don't have audio buffers for play.  */
  HEV   dataplayed;   /* Posted after playing of the next audio buffer.       */
  HMTX  mutex;        /* Serializes access to this structure.                 */
  int   status;       /* See DEVICE_* definitions.                            */

  OUTPUT_PARAMS original_info;

  TID   drivethread;  /* An identifier of the driver thread.                  */
  BOOL  boosted;      /* The priority of the driver thread is boosted.        */

  USHORT             mci_device_id; /* An identifier of the audio device.     */
  MCI_MIXSETUP_PARMS mci_mix;       /* Parameters of the audio mixer.         */
  MCI_BUFFER_PARMS   mci_buf_parms; /* Parameters of the audio buffers.       */
  MCI_MIX_BUFFER*    mci_buffers;   /* Audio buffers.                         */
  BUFFERINFO*        mci_buff_info; /* Audio buffers additional information.  */
  MCI_MIX_BUFFER*    mci_to_fill;   /* The buffer for a next portion of data. */
  MCI_MIX_BUFFER*    mci_is_play;   /* The buffer played now.                 */

} OS2AUDIO;

