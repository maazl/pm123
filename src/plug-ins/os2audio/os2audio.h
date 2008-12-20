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

#define DEVICE_OPENING    1
#define DEVICE_OPENED     2
#define DEVICE_CLOSING    3
#define DEVICE_CLOSED     4
#define DEVICE_FAILED     5


typedef struct OS2AUDIO
{
  int   device;       /*                                                      C  */
  int   lockdevice;   /*                                                      C  */
  int   numbuffers;   /* Suggested total audio buffers, bare minimum = 4      C  */
                      /* (see prio boost check).                                 */
  int   kludge48as44; /*                                                      C  */
  int   force8bit;    /*                                                      Cd */
  int   low_water_mark;/* When the number of filled buffers reaches this      Cm
                          the decoder thread is boosted.                         */
  int   high_water_mark;/* When the number of buffers reaches this level      Cm
                           the decoder thread returns to normal priority.        */
  int   buffersize;   /* Suggested size of the audio buffers.                 C  */
  int   volume;       /*                                                      C  */
  float amplifier;    /*                                                      C  */
  int   zero;         /* This is 128 for 8 bit unsigned.                      C* */
  int   configured;

  float track_gain;   /* Defines Replay Gain values as specified at           Cd */
  float track_peak;   /* http://www.replaygain.org                            Cd */
  float album_gain;   /*                                                      Cd */
  float album_peak;   /*                                                      Cd */
  float scale;        /* Scalefactor for replaygain                           D  */

  BOOL  trashed;      /* A next waiting portion of samples should be trashed. Cd */
  BOOL  nomoredata;   /* The audio device don't have audio buffers for play.  Md */
  HEV   dataplayed;   /* Posted after playing of the next audio buffer.       P* */
  HMTX  mutex;        /* Serializes access to this structure.                 Pc */
  int   status;       /* See DEVICE_* definitions.                            C* */

  OUTPUT_PARAMS original_info;      /*                                        C* */

  TID   drivethread;  /* An identifier of the driver thread.                  Dm */
  BOOL  boosted;      /* The priority of the driver thread is boosted.        Md */

  USHORT             mci_device_id; /* An identifier of the audio device.     C* */
  USHORT             reserved;      /* ensure alignment for atomic operations    */
  MCI_MIXSETUP_PARMS mci_mix;       /* Parameters of the audio mixer.         C* */
  MCI_BUFFER_PARMS   mci_buf_parms; /* Parameters of the audio buffers.       C* */
  MCI_MIX_BUFFER*    mci_buffers;   /* Audio buffers.                         C* */
  ULONG*             buf_positions; /* Playing position of the buffers.       Dm */
  int                buf_to_fill;   /* The buffer for a next portion of data. Dm */
  int                buf_is_play;   /* The buffer played now.                 Mdc*/
  ULONG              mci_time;      /* MCI time of the last played buffer     Mc */

} OS2AUDIO;

/* Sharing policy:
 *  uppercase = writer of the field
 *  lowercase = reader of the field 
 *  D/d = decoder thread
 *  M/m = MCI (DART) thread
 *  C/c = control thread (only while not playing)
 *        This may in fact be the decoder thread too if the sampling rate changes.
 *  P   = plug-in manager
 *  *   = any thread (always reader)
 * Examples:
 *  C* Once initialized by the controller, this member is read-only to the threads.
 *  Md This member is written by the DART thread and read by the decoder thread.
 *     The access is implicitely atomic.
 *  Dm This member is written by the decoder thread and read by the DART thread.
 *     The access is implicitely atomic.
 */

