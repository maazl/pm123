/*
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

#define  INCL_PM
#define  INCL_DOSDEVIOCTL
#define  INCL_DOS
#define  INCL_ERRORS
#include <os2.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "readcd.h"
#include "debuglog.h"

/* Converts MSF address to the LBA format. */
static ULONG
cd_to_LBA( MSF msf ) {
  return msf.minutes * 4500 + msf.seconds * 75 + msf.frames;
}

/* Opens a specified CD drive. Returns the operating system error code.
   Also, a NULL *disc value indicates an error. */
APIRET
cd_open( char* drive, CDHANDLE** disc )
{
  ULONG  ulAction;
  APIRET rc;

  *disc = calloc( 1, sizeof( CDHANDLE ));

  if( *disc ) {
    if(( rc = DosOpen( drive, &(*disc)->handle, &ulAction, 0,
                       FILE_NORMAL, OPEN_ACTION_OPEN_IF_EXISTS,
                       OPEN_SHARE_DENYNONE | OPEN_ACCESS_READONLY | OPEN_FLAGS_DASD,
                       NULL )) != NO_ERROR )
    {
      free( *disc );
      *disc = NULL;
    }
  } else {
    rc = ERROR_NOT_ENOUGH_MEMORY;
  }

  return rc;
}

/* Closes a handle to a CD drive.
   Returns the operating system error code. */
APIRET cd_close( CDHANDLE* disc )
{
  APIRET rc = DosClose( disc->handle );

  free( disc );
  return rc;
}

/* Returns the first and last track numbers and the LBA address
   for the lead-out track. */
static APIRET
cd_drive_info( HFILE handle, CDINFO* info )
{
  ULONG  ulParmLen;
  ULONG  ulDataLen;
  APIRET rc;

  GETAUDIODISK_DATA data;

  rc = DosDevIOCtl( handle, IOCTL_CDROMAUDIO, CDROMAUDIO_GETAUDIODISK,
                    "CD01", 4, &ulParmLen, &data, sizeof(data), &ulDataLen );

  if( rc == NO_ERROR ) {
    info->track_first      = data.track_first;
    info->track_last       = data.track_last;
    info->lead_out_address = cd_to_LBA( data.lead_out_address );
    info->track_count      = info->track_last - info->track_first + 1;
    info->time             = 0;
    info->discid           = 0;

    memset( &info->track_info, 0, sizeof( info->track_info ));
  }

  return rc;
}

/* Returns the LBA address for the starting point of the track, and the
   track-control information for that track. */
static APIRET
cd_track_info( HFILE handle, CDINFO* info )
{
  ULONG  ulParmLen;
  ULONG  ulDataLen;
  UCHAR  i;
  APIRET rc;

  GETAUDIOTRACK_PARM parm;
  GETAUDIOTRACK_DATA data;

  for( i = info->track_first; i <= info->track_last; i++ )
  {
    memcpy( parm.signature, "CD01", 4 );
    parm.track = i;

    rc = DosDevIOCtl( handle, IOCTL_CDROMAUDIO, CDROMAUDIO_GETAUDIOTRACK,
                      &parm, sizeof( GETAUDIOTRACK_PARM ), &ulParmLen,
                      &data, sizeof( GETAUDIOTRACK_DATA ), &ulDataLen  );

    if( rc != NO_ERROR ) {
      return rc;
    }

    // It is necessary to keep in mind presence of a two-second pause
    // in the beginning of a track (150 frames).

    info->track_info[i].start    = cd_to_LBA( data.address ) - 150;
    info->track_info[i].is_data  = (data.info & 0x40) ? TRUE : FALSE;
    info->track_info[i].channels = (data.info & 0x80) ? 4 : 2;
  }

  info->track_info[i].start = info->lead_out_address;

  for( i = info->track_first; i <= info->track_last; i++ )
  {
    info->track_info[i].end  = info->track_info[i+1].start - 1;
    info->track_info[i].size = ( info->track_info[i].end - info->track_info[i].start ) * CDFRAME_SIZE;

    DEBUGLOG(( "cddaplay: track %02d start %010d end %010d (LBA)\n",
                i, info->track_info[i].start, info->track_info[i].end ));
  }

  // Generates the CDDB/freedb disc ID.
  for( i = info->track_first; i <= info->track_last; i++ )
  {
    ULONG t = ( info->track_info[i].start + 150 ) / 75;

    while( t > 0 ) {
      info->discid += t % 10;
      t /= 10;
    }
  }

  info->time   = ( info->lead_out_address - info->track_info[0].start - 150 ) / 75;
  info->discid = info->discid % 0xFF << 24 | info->time << 8 | info->track_count;

  return NO_ERROR;
}

/* Returns the CD info.
   Returns the operating system error code. */
APIRET
cd_info( CDHANDLE* disc, CDINFO* info )
{
  APIRET rc;

  if(( rc = cd_drive_info( disc->handle, info )) != NO_ERROR ) {
    return rc;
  }

  return cd_track_info( disc->handle, info );
}

/* Returns the 7-bytes of the Universal Product Code. This is a unique
   code identifying the disc. The UPC is 13 successive BCD digits (4
   bits each) followed by 12 bits set to 0. Returns the operating system
   error code. If no UPC was encoded on the disc, returns
   ERROR_NOT_SUPPORTED. */
APIRET
cd_upc( CDHANDLE* disc, unsigned char* upc )
{
  ULONG  ulParmLen;
  ULONG  ulDataLen;
  APIRET rc;

  GETUPC_DATA data = { 0 };

  rc = DosDevIOCtl( disc->handle, IOCTL_CDROMAUDIO, CDROMAUDIO_GETAUDIODISK,
                    "CD01", 4, &ulParmLen, &data, sizeof(data), &ulDataLen );

  if( rc == NO_ERROR )
  {
    memcpy( upc, data.UPC, sizeof( data.UPC ));

    if( !upc[0] && !upc[1] && !upc[2] && !upc[3] &&
        !upc[4] && !upc[5] && !upc[6] )
    {
      return ERROR_NOT_SUPPORTED;
    }
  }
  return rc;
}

/* Reads specified frames in the data buffer.
   Returns the operating system error code. */
static APIRET
cd_read_frames( HFILE handle, CDFRAME data[], ULONG number, ULONG start )
{
  ULONG ulParmLen;
  ULONG ulDataLen;

  CDREADLONG_PARM parm = {{ 0 }};
  memcpy( parm.signature, "CD01", 4 );

  parm.addressing_mode = MODE_LBA;
  parm.number_sectors  = number;
  parm.start_sector    = start;

  return DosDevIOCtl( handle, IOCTL_CDROMDISK, CDROMDISK_READLONG,
                      &parm, sizeof( parm ), &ulParmLen,
                      data,  sizeof(*data ) * number, &ulDataLen );
}

/* Selects specified audio track for reading. Returns 0 if it
   successfully selects the file. A return value of -1 shows an error. */
void
cd_select_track( CDHANDLE* disc, CDTRACKINFO* track )
{
  disc->start      = track->start;
  disc->end        = track->end;
  disc->pos        = track->start;
  disc->extrabytes = 0;
  disc->size       = track->size;
  disc->channels   = track->channels;
}

/* Moves read pointer to a new location that is offset bytes from
   the beginning of the selected track. The offset bytes must be an
   integer product of the number of channels and of the number of bits
   in the sample. Returns the offset, in bytes, of the new position
   from the beginning of the file. A return value of -1L indicates an
   error. */
long
cd_seek( CDHANDLE* disc, long offset )
{
  ULONG frame;
  ULONG bytes;

  if( !disc->channels ) {
    return -1;
  }

  offset = offset / disc->channels / 2 * disc->channels * 2;
  frame  = disc->start + ( offset / CDFRAME_SIZE );
  bytes  = offset % CDFRAME_SIZE;

  if( frame == disc->pos && disc->extrabytes ) {
    // Current frame is the same.
    disc->extrabytes = CDFRAME_SIZE - bytes;
    return offset;
  }

  if( frame > disc->end ) {
    disc->pos = disc->end + 1;
    disc->extrabytes = 0;
    return disc->size;
  }

  disc->pos = frame;

  if( bytes ) {
    if( cd_read_frames( disc->handle, &disc->frame, 1, disc->pos ) == NO_ERROR ) {
      disc->extrabytes = CDFRAME_SIZE - bytes;
    } else {
      return -1;
    }
  } else {
    disc->extrabytes = 0;
  }

  return offset;
}

/* Returns the current position of the file pointer. The position is
   the number of bytes from the beginning of the file. */
long
cd_tell( CDHANDLE* disc )
{
  long offset = ( disc->pos - disc->start ) * CDFRAME_SIZE;

  if( disc->extrabytes ) {
    offset += CDFRAME_SIZE - disc->extrabytes;
  }

  return offset;
}

/* Reads count bytes from the track into buffer. Returns the number
   of bytes placed in result. The count must be an integer product of
   the number of channels and of the number of bits in the sample.
   The return value 0 indicates an attempt to read at end-of-track.
   A return value -1 indicates an error. */
int
cd_read_data( CDHANDLE* disc, char* result, unsigned int count )
{
  int read = 0;

  if( !disc->channels || count % ( disc->channels * 2 )) {
    return -1;
  }

  if( disc->extrabytes ) {
    if( disc->extrabytes > count ) {
      memcpy( result, (char*)&disc->frame + CDFRAME_SIZE - disc->extrabytes, count );
      read = count;
    } else {
      memcpy( result, (char*)&disc->frame + CDFRAME_SIZE - disc->extrabytes, disc->extrabytes );
      read = disc->extrabytes;
      disc->pos++;
    }
    disc->extrabytes -= read;
  }

  if( read < count && disc->pos <= disc->end ) {
    int frames = ( count - read ) / CDFRAME_SIZE;

    if( disc->pos + frames > disc->end ) {
      frames = disc->end - disc->pos + 1;
    }

    if( frames ) {
      if( cd_read_frames( disc->handle, (CDFRAME*)( result + read ),
                          frames, disc->pos ) == NO_ERROR )
      {
        read += frames * CDFRAME_SIZE;
        disc->pos += frames;
      } else {
        return -1;
      }
    }
  }

  if( read < count && disc->pos <= disc->end ) {
    if( cd_read_frames( disc->handle, &disc->frame, 1, disc->pos ) == NO_ERROR ) {
      memcpy( result + read, (char*)&disc->frame, count - read );
      disc->extrabytes = CDFRAME_SIZE - count + read;
      read = count;
    } else {
      return -1;
    }
  }

  return read;
}

