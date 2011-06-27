/*
 * Copyright (C) 2010-2011 Marcel Mueller
 * Copyright (C) 2007 Dmitry A.Steklenev
 * Copyright (C) 2000-2004 Haavard Kvaalen
 * Copyright (C) 1998, 1999, 2002 Espen Skoglund
 *
 * $Id: id3v2.c,v 1.4 2008/04/21 08:30:11 glass Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <io.h>

#include <charset.h>
#include <debuglog.h>
#include <minmax.h>

#include "id3v2/id3v2.h"
#include "id3v2/id3v2_header.h"


/* Seek `offset' bytes forward in the indicated ID3-tag. Return 0
   upon success, or -1 if an error occured. */
static int
id3v2_seek( ID3V2_TAG* id3, int offset )
{
  // Check boundary.
  if(( id3->id3_pos + offset > id3->id3_totalsize ) ||
     ( id3->id3_pos + offset < 0 )) {
    return -1;
  }

  if( offset > 0 )
  {
    // If offset is positive, we use fread() instead of fseek(). This
    // is more robust with respect to streams.

    char buf[64];
    int  r, remain = offset;

    while( remain > 0 )
    {
      int size = min( 64, remain );
      r = xio_fread( buf, 1, size, (XFILE*)id3->id3_file );
      if( r == 0 ) {
        id3->id3_error_msg = "Read failed.";
        return -1;
      }
      remain -= r;
    }
  }
  else
  {
    // If offset is negative, we ahve to use fseek(). Let us hope
    // that it works.

    if( xio_fseek( (XFILE*)id3->id3_file, offset, XIO_SEEK_CUR ) == -1 ) {
      id3->id3_error_msg = "Seeking beyond tag boundary.";
      return -1;
    }
  }
  id3->id3_pos += offset;
  return 0;
}

/* Read `size' bytes from indicated ID3-tag. If `buf' is non-NULL,
   read into that buffer. Return a pointer to the data which was
   read, or NULL upon error. */
static void* id3v2_read( ID3V2_TAG* id3, void* buf, int size )
{
  int ret;

  // Check boundary.
  if( id3->id3_pos + size > id3->id3_totalsize ) {
    return NULL;
  }

  // If buffer is NULL, we use the default buffer.
  if( buf == NULL ) {
    if( size > ID3V2_FILE_BUFSIZE ) {
      return NULL;
    }
    buf = id3->id3_filedata;
  }

  // Try reading from file.
  ret = xio_fread( buf, 1, size, (XFILE*)id3->id3_file );
  if( ret != size ) {
    id3->id3_error_msg = "Read failed.";
    return NULL;
  }

  id3->id3_pos += ret;
  return buf;
}

/* Seek `offset' bytes forward in the indicated ID3-tag. Return 0
   upon success, or -1 if an error occured. */
static int id3v2_mem_seek( ID3V2_TAG* id3, int offset )
{
  int newpos = id3->id3_pos + offset;

  if (newpos < 0 || newpos > id3->id3_totalsize)
    return -1;

  id3->id3_pos = newpos;
  return 0;
}

/* Read `size' bytes from indicated ID3-tag. If `buf' is non-NULL,
   read into that buffer. Return a pointer to the data which was
   read, or NULL upon error. */
static void* id3v2_mem_read( ID3V2_TAG* id3, void* buf, int size )
{
  // Check boundary.
  if( id3->id3_pos + size > id3->id3_totalsize ) {
    return NULL;
  }

  // If buffer is NULL, we use the default buffer.
  if( buf == NULL ) {
    if( size > ID3V2_FILE_BUFSIZE ) {
      return NULL;
    }
    buf = id3->id3_filedata;
  }

  memcpy(buf, (char*)id3->id3_file + id3->id3_pos, size);
  id3->id3_pos += size;
  return buf;
}


/* Initialize an empty ID3 tag. */
static void
id3v2_init_tag( ID3V2_TAG* id3 )
{
  // Initialize header.
  id3->id3_version   = ID3V2_VERSION;
  id3->id3_revision  = ID3V2_REVISION;
  id3->id3_flags     = ID3V2_THFLAG_USYNC | ID3V2_THFLAG_EXP;
  id3->id3_size      = 0;
  id3->id3_totalsize = 0;
  id3->id3_altered   = 1;
  id3->id3_newtag    = 1;
  id3->id3_pos       = 0;
  //id3->id3_started   = 0;

  // Initialize frames.
  id3->id3_frames = NULL;
  id3->id3_frames_count = 0;
}


/* Creates a new ID3 tag structure. Useful for creating
   a new tag. */
ID3V2_TAG*
id3v2_new_tag( void )
{
  ID3V2_TAG* id3;

  // Allocate ID3 structure. */
  id3 = (ID3V2_TAG*)calloc( 1, sizeof( ID3V2_TAG ));

  if( id3 != NULL ) {
    id3v2_init_tag( id3 );
  }

  return id3;
}

/* Read the ID3 tag from the input stream. The start of the tag
   must be positioned in the next tag in the stream.  Return 0 upon
   success, or -1 if an error occured. */
static int
id3v2_read_tag( ID3V2_TAG* id3 )
{
  char*   buf;
  uint8_t padding;

  // We know that the tag will be at least this big.
  id3->id3_totalsize = ID3V2_TAGHDR_SIZE + 3;

  if( !( id3->id3_oflags & ID3V2_GET_NOCHECK )) {
    // Check if we have a valid ID3 tag.
    char* id = (char*)id3->id3_read( id3, NULL, 3 );
    if( id == NULL ) {
      return -1;
    }

    if(( id[0] != 'I' ) || ( id[1] != 'D' ) || ( id[2] != '3' )) {
      // ID3 tag was not detected.
      id3->id3_seek( id3, -3 );
      return -1;
    }
  }

  // Read ID3 tag-header.
  buf = (char*)id3->id3_read( id3, NULL, ID3V2_TAGHDR_SIZE );
  if( buf == NULL ) {
    return -1;
  }

  id3->id3_version    = buf[0];
  id3->id3_revision   = buf[1];
  id3->id3_flags      = buf[2];
  id3->id3_size       = ID3_GET_SIZE28( buf[3], buf[4], buf[5], buf[6] );
  id3->id3_totalsize += id3->id3_size;
  id3->id3_newtag     = 0;

  if( id3->id3_flags & ID3V2_THFLAG_FOOTER ) {
    id3->id3_totalsize += 10;
  }

  DEBUGLOG(( "id3v2: found ID3V2 tag version 2.%d.%d\n",
                     id3->id3_version, id3->id3_revision ));

  if(( id3->id3_version < 2 ) || ( id3->id3_version > 4 )) {
    DEBUGLOG(( "id3v2: this version of the ID3V2 tag is unsupported.\n" ));
    return -1;
  }

  // Parse extended header.
  if( id3->id3_flags & ID3V2_THFLAG_EXT ) {
    buf = (char*)id3->id3_read( id3, NULL, ID3V2_EXTHDR_SIZE );
    if( buf == NULL ) {
      return -1;
    }
  }

  // Parse frames.
  while( id3->id3_pos < id3->id3_size ) {
    if( id3v2_read_frame( id3 ) == -1 ) {
      return -1;
    }
  }

  // Like id3lib, we try to find unstandard padding (not within
  // the tag size). This is important to know when we strip
  // the tag or replace it. Another option might be looking for
  // an MPEG sync, but we don't do it.

  id3->id3_seek( id3, id3->id3_totalsize - id3->id3_pos );

  // Temporarily increase totalsize, to try reading beyong the boundary.
  ++id3->id3_totalsize;

  while( id3->id3_read( id3, &padding, sizeof( padding )) != NULL )
  {
    if( padding == 0 ) {
      ++id3->id3_totalsize;
    } else {
      id3->id3_seek( id3, -1 );
      break;
    }
  }

  // Decrease totalsize after we temporarily increased it.
  --id3->id3_totalsize;

  return 0;
}

/* Read an ID3 tag using a file pointer. Return a pointer to a
   structure describing the ID3 tag, or NULL if an error occured. */
ID3V2_TAG*
id3v2_get_tag( XFILE* file, int flags )
{
  // Allocate ID3 structure.
  ID3V2_TAG* id3 = (ID3V2_TAG*)calloc( 1, sizeof( ID3V2_TAG ));

  // Initialize access pointers.
  id3->id3_seek    = id3v2_seek;
  id3->id3_read    = id3v2_read;
  id3->id3_oflags  = flags;
  id3->id3_pos     = 0;
  id3->id3_file    = file;
  //id3->id3_started = xio_ftell( file );

  // Allocate buffer to hold read data.
  id3->id3_filedata = malloc( ID3V2_FILE_BUFSIZE );

  // Try reading ID3 tag.
  if( id3v2_read_tag( id3 ) == -1 ) {
    if( ~flags & ID3V2_GET_CREATE )
    {
      free( id3->id3_filedata );
      free( id3 );
      return NULL;
    }
    id3v2_init_tag( id3 );
  }

  return id3;
}

ID3V2_TAG* id3v2_load_tag( char* tagdata, size_t taglen, int flags )
{
  // Allocate ID3 structure.
  ID3V2_TAG* id3 = (ID3V2_TAG*)calloc( 1, sizeof( ID3V2_TAG ));

  // Initialize access pointers.
  id3->id3_seek    = id3v2_mem_seek;
  id3->id3_read    = id3v2_mem_read;
  id3->id3_oflags  = flags;
  id3->id3_pos     = 0;
  id3->id3_file    = tagdata;
  id3->id3_totalsize = taglen;
  //id3->id3_started = xio_ftell( file );

  // Allocate buffer to hold read data.
  id3->id3_filedata = malloc( ID3V2_FILE_BUFSIZE );

  // Try reading ID3 tag.
  if( id3v2_read_tag( id3 ) == -1 ) {
    if( ~flags & ID3V2_GET_CREATE )
    {
      free( id3->id3_filedata );
      free( id3 );
      return NULL;
    }
    id3v2_init_tag( id3 );
  }

  return id3;
}

/* Free all resources associated with the ID3 tag. */
void id3v2_free_tag( ID3V2_TAG* id3 )
{
  free( id3->id3_filedata );
  id3v2_delete_all_frames( id3 );
  free( id3 );
}

/* When altering a file, some ID3 tags should be discarded. As the ID3
   library has no means of knowing when a file has been altered
   outside of the library, this function must be called manually
   whenever the file is altered. */
int id3v2_alter_tag( ID3V2_TAG* id3 )
{
  // List of frame classes that should be discarded whenever the
  // file is altered.

  static const ID3V2_ID discard_list[] = {
    ID3V2_ETCO, ID3V2_EQUA, ID3V2_MLLT, ID3V2_POSS, ID3V2_SYLT,
    ID3V2_SYTC, ID3V2_RVAD, ID3V2_TENC, ID3V2_TLEN, ID3V2_TSIZ,
    0
  };

  // Go through list of frame types that should be discarded.
  for (size_t i = 0; i < sizeof discard_list / sizeof * discard_list; ++i)
  { ID3V2_ID id = discard_list[i];
    // Discard all frames of that type.
    ID3V2_FRAME* fr;
    while(( fr = id3v2_get_frame( id3, id, 1 )) != NULL ) {
      id3v2_delete_frame( fr );
    }
  }

  return 0;
}

/* Remove the ID3 tag from the file. Returns 0 upon success,
   or -1 if an error occured. */
int
id3v2_wipe_tag( XFILE* file, const char* savename )
{
  ID3V2_TAG* old_id3;
  int        old_totalsize;
  XFILE*     save;
  char*      buffer;
  int        rc = 0;

  // Figure out how large the current tag is.
  xio_rewind( file );
  old_id3 = id3v2_get_tag( file, 0 );

  if( old_id3 ) {
    // We use max to make sure an erroneous tag doesn't confuse us.
    old_totalsize = max( old_id3->id3_totalsize, 0 );
    id3v2_free_tag( old_id3 );
  } else {
    return 0;
  }

  if(( buffer = (char*)calloc( 1, ID3V2_FILE_BUFSIZE )) == NULL ) {
    return -1;
  }

  if(( save = xio_fopen( savename, "wbU" )) == NULL ) {
    free( buffer );
    return -1;
  }

  // And now, copy all the data to the new file.
  if( xio_fseek( file, old_totalsize, XIO_SEEK_SET ) != -1 ) {
    size_t bytes;
    while(( bytes = xio_fread( buffer, 1, ID3V2_FILE_BUFSIZE, file )) > 0 ) {
      if( xio_fwrite( buffer, 1, bytes, save ) != bytes ) {
        break;
      }
    }
    rc = xio_ferror( save ) ? -1 : 0;
  } else {
    rc = -1;
  }

  xio_fclose( save );
  free( buffer );
  return rc;
}

/* Write the ID3 tag to the indicated file descriptor. Return 0
   upon success, or -1 if an error occured. */
static int
id3v2_write_tag( XFILE* file, ID3V2_TAG* id3 )
{
  int  i;
  char buf[ID3V2_TAGHDR_SIZE];

  ID3V2_FRAME* fr;

  // Write tag header.

  buf[0] = ID3V2_VERSION;
  buf[1] = ID3V2_REVISION;
  buf[2] = id3->id3_flags;

  ID3_SET_SIZE28( id3->id3_size, buf[3], buf[4], buf[5], buf[6] );

  if( xio_fwrite( "ID3", 1, 3, file ) != 3 ) {
    return -1;
  }
  if( xio_fwrite( buf, 1, ID3V2_TAGHDR_SIZE, file ) != ID3V2_TAGHDR_SIZE ) {
    return -1;
  }

  // TODO: Write extended header.

#if 0
  if( id3->id3_flags & ID3_THFLAG_EXT ) {
    id3_exthdr_t exthdr;
  }
#endif

  for( i = 0; i < id3->id3_frames_count; i++ )
  {
    char fhdr[ID3V2_FRAMEHDR_SIZE];

    fr = id3->id3_frames[i];

    // TODO: Support compressed headers, encoded
    // headers, and grouping info.

    *(ID3V2_ID*)fhdr = fr->fr_desc->fd_id;

    #if ID3V2_VERSION < 4
      fhdr[4] = ( fr->fr_raw_size >> 24 ) & 0xFF;
      fhdr[5] = ( fr->fr_raw_size >> 16 ) & 0xFF;
      fhdr[6] = ( fr->fr_raw_size >> 8  ) & 0xFF;
      fhdr[7] = ( fr->fr_raw_size       ) & 0xFF;
    #else
      ID3_SET_SIZE28( fr->fr_raw_size, fhdr[4], fhdr[5], fhdr[6], fhdr[7] )
    #endif

    fhdr[8] = ( fr->fr_flags    >> 8  ) & 0xFF;
    fhdr[9] = ( fr->fr_flags          ) & 0xFF;

    if( xio_fwrite( fhdr, 1, sizeof( fhdr ), file ) != sizeof( fhdr )) {
      return -1;
    }

    if( xio_fwrite( fr->fr_raw_data, 1, fr->fr_raw_size, file ) != fr->fr_raw_size ) {
      return -1;
    }
  }
  return 0;
}

/* Writes the ID3 tag to the file. Returns 0 upon success, returns 1 if
   the ID3 tag is saved to savename or returns -1 if an error occured. */
int
id3v2_set_tag( XFILE* file, ID3V2_TAG* id3, const char* savename )
{
  ID3V2_TAG* old_id3;
  int        old_totalsize;
  int        new_totalsize;
  int        old_startdata;
  int        i;
  XFILE*     save = NULL;
  char*      buffer;
  size_t     remaining;
  int        rc = 0;

  if(( buffer = (char*)calloc( 1, ID3V2_FILE_BUFSIZE )) == NULL ) {
    return -1;
  }

  // Figure out how large the current tag is.
  xio_rewind( file );
  old_id3 = id3v2_get_tag( file, 0 );

  if( old_id3 ) {
    // We use max to make sure an erroneous tag doesn't confuse us.
    old_totalsize = max( old_id3->id3_totalsize, 0 );
    old_startdata = old_totalsize;
    id3v2_free_tag( old_id3 );
  } else {
    char data[4];
    old_totalsize = 0;
    old_startdata = 0;

    if( xio_fread( data, 1, 4, file ) == 4 ) {
      if( data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F' ) {
        while( xio_fread( data + 3, 1, 1, file ) == 1 ) {
          if( data[0] == 'd' && data[1] == 'a' && data[2] == 't' && data[3] == 'a' ) {
            old_startdata = xio_ftell( file );
            DEBUGLOG(( "id3v2: skipped RIFF header up to %d.\n", old_totalsize ));
            break;
          }
          data[0] = data[1];
          data[1] = data[2];
          data[2] = data[3];
        }
      }
    }
  }

  // Figure out how large the new tag will be.
  new_totalsize = ID3V2_TAGHDR_SIZE + 3;

  for( i = 0; i < id3->id3_frames_count; i++ ) {
    new_totalsize += id3->id3_frames[i]->fr_raw_size + ID3V2_FRAMEHDR_SIZE;
  }

  new_totalsize += 0;   // no extended header, no footer, no padding.
  id3->id3_flags = 0;

  // Determine whether we need to rewrite the file to make place for the new tag.
  if( new_totalsize <= old_totalsize ) {
    save = file;
    new_totalsize = old_totalsize;
  } else {
    if(( save = xio_fopen( savename, "wbU" )) == NULL ) {
      free( buffer );
      return -1;
    }
    // Use padding.
    new_totalsize += 1024;
  }

  // Zero-out the ID3v2 tag area.
  xio_rewind( save );
  for( remaining = new_totalsize; remaining > 0; remaining -= min( remaining, ID3V2_FILE_BUFSIZE )) {
    xio_fwrite( buffer, 1, min( remaining, ID3V2_FILE_BUFSIZE ), save );
  }

  // Write the new tag.
  id3->id3_size = new_totalsize - ID3V2_TAGHDR_SIZE - 3;
  xio_rewind( save );

  if( id3v2_write_tag( save, id3 ) == -1 ) {
    rc = -1;
  }

  if( save != file && rc == 0 )
  {
    size_t bytes;

    // And now, copy all the data to the new file.
    if( xio_fseek( file, old_startdata, XIO_SEEK_SET ) != -1 ) {
      if( xio_fseek( save, new_totalsize, XIO_SEEK_SET ) != -1 ) {
        while(( bytes = xio_fread( buffer, 1, ID3V2_FILE_BUFSIZE, file )) > 0 ) {
          if( xio_fwrite( buffer, 1, bytes, save ) != bytes ) {
            break;
          }
        }
      }
      rc = xio_ferror( save ) ? -1 : 1;
    } else {
      rc = -1;
    }
  }

  if( save != file ) {
    xio_fclose( save );
  }
  free( buffer );
  return rc;
}

