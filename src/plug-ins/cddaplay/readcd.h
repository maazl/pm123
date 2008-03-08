/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 *
 * Copyright 2006 Dmitry A.Steklenev <glass@ptv.ru>
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

#ifndef READCD_H
#define READCD_H

#ifdef __cplusplus
extern "C" {
#endif

#define MODE_LBA   0
#define MODE_MSF   1

#define MAX_TRACKS 256
#pragma pack(1)

typedef struct _MSF
{
  UCHAR frames;
  UCHAR seconds;
  UCHAR minutes;
  UCHAR hours;              /* Assumes that there are always 0 hours.            */

} MSF;

typedef struct _GETAUDIODISK_DATA
{
  UCHAR track_first;        /* First track number on the disc.                   */
  UCHAR track_last;         /* Last track number on the disc.                    */
  MSF   lead_out_address;   /* The address of the lead-out track in MSF format.  */

} GETAUDIODISK_DATA;

typedef struct _GETAUDIOTRACK_PARM
{
  UCHAR signature[4];       /* Signature string of "CD01".                       */
  UCHAR track;              /* Track number to return track information.         */

} GETAUDIOTRACK_PARM;

typedef struct _GETAUDIOTRACK_DATA
{
  MSF   address;            /* The starting address of the track in MSF format.  */
  UCHAR info;               /* The track-control information byte.               */

} GETAUDIOTRACK_DATA;

typedef struct _GETUPC_DATA
{
  UCHAR control_ADR;        /* The Track-Control Information byte.               */
  UCHAR UPC[7];             /* Universal Product Code.                           */
  UCHAR reserved;           /* Must be 0.                                        */
  UCHAR frame;              /* Frame number of current head location.            */

} GETUPC_DATA;

typedef struct _CDREADLONG_PARM
{
  UCHAR  signature[4];      /* Signature string of "CD01".                       */
  UCHAR  addressing_mode;   /* Addressing format of starting sector number.      */
  USHORT number_sectors;    /* The number of 2352-byte sectors to read.          */
  ULONG  start_sector;      /* The starting sector number of the read operation. */
  UCHAR  reserved;          /* Not used. Must be 0.                              */
  UCHAR  interleave_size;   /* Not used. Must be 0.                              */
  UCHAR  interleave_skip_factor;

} CDREADLONG_PARM;

typedef struct _CDTRACKINFO
{
  ULONG  start;             /* The starting sector number of the track.          */
  ULONG  end;               /* The ending sector number of the track.            */
  ULONG  size;              /* Size of the track in bytes.                       */
  BOOL   is_data;           /* Is it a data track.                               */
  USHORT channels;          /* The number of the audio channels.                 */

} CDTRACKINFO;

typedef struct _CDINFO
{
  UCHAR track_first;        /* First track number on the disc.                   */
  UCHAR track_last;         /* Last track number on the disc.                    */
  ULONG track_count;        /* The number of tracks on the disc.                 */
  ULONG lead_out_address;   /* The lead-out sector number.                       */
  ULONG time;               /* The length of disc in milliseconds.               */
  ULONG discid;             /* The FreeDB (CDDB) disc identifier.                */

  CDTRACKINFO track_info[MAX_TRACKS];

} CDINFO;

typedef struct _CDFRAME
{
  UCHAR sync   [  12];
  UCHAR header [   4];
  UCHAR data   [2048];
  UCHAR EDC_ECC[ 288];

} CDFRAME;

#define CDFRAME_SIZE sizeof( CDFRAME )
#pragma pack()

typedef struct _CDHANDLE
{
  HFILE   handle;     /* Handle for the disc.                               */
  CDFRAME frame;      /* Last readed disc frame.                            */
  ULONG   extrabytes; /* Number of bytes remained after last reading.       */
  ULONG   start;      /* The starting sector number of the selected track.  */
  ULONG   end;        /* The ending sector number of the selected track.    */
  ULONG   pos;        /* The current sector number of the selected track.   */
  ULONG   size;       /* Size of the selected track in bytes.               */
  USHORT  channels;   /* The number of the audio channels.                  */

} CDHANDLE;

/* Opens a specified CD drive. Returns the operating system error code.
   Also, a NULL pointer disc value indicates an error. */
APIRET cd_open( char* drive, CDHANDLE** disc );

/* Closes a handle to a CD drive.
   Returns the operating system error code. */
APIRET cd_close( CDHANDLE* disc );

/* Returns the 7-bytes of the Universal Product Code. This is a unique
   code identifying the disc. The UPC is 13 successive BCD digits (4
   bits each) followed by 12 bits set to 0. Returns the operating system
   error code. If no UPC was encoded on the disc, returns
   ERROR_NOT_SUPPORTED. */
APIRET cd_upc( CDHANDLE* disc, unsigned char* upc );

/* Returns the disc info.
   Returns the operating system error code. */
APIRET cd_info( CDHANDLE* disc, CDINFO* info );

/* Selects specified audio track for reading. */
void cd_select_track( CDHANDLE* disc, CDTRACKINFO* track );

/* Returns the current position of the file pointer. The position is
   the number of bytes from the beginning of the file. */
long cd_tell( CDHANDLE* disc );

/* Moves read pointer to a new location that is offset bytes from
   the beginning of the selected track. The offset bytes must be an
   integer product of the number of channels and of the number of bits
   in the sample. Returns the offset, in bytes, of the new position
   from the beginning of the file. A return value of -1L indicates an
   error. */
long cd_seek( CDHANDLE* disc, long offset );

/* Reads count bytes from the track into buffer. Returns the number
   of bytes placed in result. The return value 0 indicates an attempt
   to read at end-of-track. A return value -1 indicates an error. */
int cd_read_data( CDHANDLE* disc, char* result, unsigned int count );

#ifdef __cplusplus
}
#endif
#endif /* READCD_H */

