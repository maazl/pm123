/*
 * Copyright 2008-2011 M.Mueller
 * Copyright 2006 Dmitry A.Steklenev
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

#define  INCL_DOS
#define  INCL_ERRORS
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <share.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>
#include <strutils.h>
#include <eautils.h>

#include "xio_file.h"
#include "xio_url.h"

#include <debuglog.h>

#ifdef XIO_SERIALIZE_DISK_IO

  Mutex serialize;

  // Serializes all read and write disk operations. This is
  // improve performance of poorly implemented filesystems (OS/2 version
  // of FAT32 for example).

  #define FILE_REQUEST_DISK( x ) if( x->s_serialized ) { serialize.Request(); }
  #define FILE_RELEASE_DISK( x ) if( x->s_serialized ) { serialize.Release(); }
#else
  #define FILE_REQUEST_DISK( x )
  #define FILE_RELEASE_DISK( x )
#endif

/* Map OS/2 APIRET errors to C errno. */
static int map_os2_errors(APIRET rc)
{
  switch (rc) {
    case NO_ERROR:
      return 0;
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_NOT_DOS_DISK:
    case ERROR_CANNOT_MAKE:
    case ERROR_FILENAME_EXCED_RANGE:
      return ENOENT;
    case ERROR_TOO_MANY_OPEN_FILES:
      return EMFILE;
    case ERROR_ACCESS_DENIED:
    case ERROR_SHARING_VIOLATION:
    case ERROR_PIPE_BUSY:
    case ERROR_LOCK_VIOLATION:
    case ERROR_WRITE_PROTECT:
      return EACCES;
    case ERROR_INVALID_ACCESS:
    case ERROR_INVALID_PARAMETER:
    case ERROR_INVALID_FUNCTION:
    case ERROR_NEGATIVE_SEEK:
      return EINVAL;
    case ERROR_INVALID_HANDLE:
      return EBADF;
    case ERROR_DISK_FULL:
      return ENOSPC;
    default: // can't help
      return EINVAL;
  }
}

#ifdef XIO_SERIALIZE_DISK_IO
static int
is_singletasking_fs( const char* filename )
{
  if( isalpha( filename[0] ) && filename[1] == ':' )
  {
    // Return-data buffer should be large enough to hold FSQBUFFER2
    // and the maximum data for szName, szFSDName, and rgFSAData
    // Typically, the data isn't that large.

    BYTE fsqBuffer[ sizeof( FSQBUFFER2 ) + ( 3 * CCHMAXPATH )] = { 0 };
    PFSQBUFFER2 pfsqBuffer = (PFSQBUFFER2)fsqBuffer;

    ULONG  cbBuffer        = sizeof( fsqBuffer );
    PBYTE  pszFSDName      = NULL;
    UCHAR  szDeviceName[3] = "x:";
    APIRET rc;

    szDeviceName[0] = filename[0];

    rc = DosQueryFSAttach( szDeviceName,    // Logical drive of attached FS
                           0,               // Ignored for FSAIL_QUERYNAME
                           FSAIL_QUERYNAME, // Return data for a Drive or Device
                           pfsqBuffer,      // Returned data
                          &cbBuffer );      // Returned data length

    // On successful return, the fsqBuffer structure contains
    // a set of information describing the specified attached
    // file system and the DataBufferLen variable contains
    // the size of information within the structure.

    if( rc == NO_ERROR ) {
      pszFSDName = pfsqBuffer->szName + pfsqBuffer->cbName + 1;
      if( stricmp( pszFSDName, "FAT32" ) == 0 ) {
        DEBUGLOG(( "xio123: detected %s filesystem for file %s, serialize access.\n", pszFSDName, filename ));
        return 1;
      } else {
        DEBUGLOG(( "xio123: detected %s filesystem for file %s.\n", pszFSDName, filename ));
      }
    }
  }
  return 0;
}
#endif

/* Opens the file specified by filename. Returns 0 if it
   successfully opens the file. A return value of -1 shows an error. */
int XIOfile::open( const char* filename, XOFLAGS oflags )
{
  ULONG flags = 0;
  ULONG omode = OPEN_FLAGS_NOINHERIT;
  ULONG dummy;
  APIRET rc;
  DEBUGLOG(("XIOfile(%p)::open(%s, %x)\n", this, filename, oflags));

  if(( oflags & XO_WRITE ) && ( oflags & XO_READ )) {
    omode |= OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYWRITE;
  } else if( oflags & XO_WRITE ) {
    omode |= OPEN_ACCESS_WRITEONLY | OPEN_SHARE_DENYWRITE;
  } else if( oflags & XO_READ ) {
    omode |= OPEN_ACCESS_READONLY  | OPEN_SHARE_DENYNONE;
  }

  if( oflags & XO_CREATE ) {
    flags |= OPEN_ACTION_CREATE_IF_NEW;
  }
  if( oflags & XO_TRUNCATE ) {
    flags |= OPEN_ACTION_REPLACE_IF_EXISTS;
  } else {
    flags |= OPEN_ACTION_OPEN_IF_EXISTS;
  }
  
  if( !(oflags & XO_NOBUFFER) && (oflags & (XO_READ|XO_WRITE)) != (XO_READ|XO_WRITE) )
    omode |= OPEN_FLAGS_SEQUENTIAL;
  
  if( strnicmp( filename, "file:", 5 ) == 0 )
  {
    XURL* url = url_allocate( filename );
    char* p;

    if( !url->path ) {
      url_free( url );
      errno = error = ENOENT;
      return -1;
    }

    // Converts leading drive letters of the form C| to C:
    // and if a drive letter is present or we have UNC path
    // strips off the slash that precedes path. Otherwise,
    // the leading slash is used.

    /* OS/2 does not require this
    for( p = url->path; *p; p++ ) {
      if( *p == '/'  ) {
          *p = '\\';
      }
    }*/

    p = url->path;
    //DEBUGLOG(("XIOfile::open %s\n", p));
    // Convert file://server/share/path URLs to UNC path
    if ( *p != '/' ) {
      char* f = (char*)alloca( strlen( p ) + 3 );
      f[0] = f[1] = '/';
      strcpy( f + 2, p );
      filename = f;
    } else {
      // Converts leading drive letters of the form C| to C:
      // and if a drive letter is present strips off the slash that precedes
      // path. URLs starting with file://///server/share are also stripped.
      // Otherwise, the leading slash is used.
      if( isalpha( p[1] ) && ( p[2] == '|' || p[2] == ':' )) {
        p[2] = ':';
        ++p;
      } else if ( p[1] == '/' && p[2] == '/' ) {
        ++p;
      }
      char* f = (char*)alloca( strlen( p ) + 1 );
      strcpy( f, p );
      filename = f;
    }
    url_free( url );
  }

  #ifdef XIO_SERIALIZE_DISK_IO
  s_serialized = is_singletasking_fs( p );
  #endif

  FILE_REQUEST_DISK(this);
  rc = DosOpen( (PSZ)filename, &s_handle,
                &dummy, 0, FILE_NORMAL, flags, omode, NULL );
  DEBUGLOG(("XIOfile::open: hfile = %i, action = %u, rc = %i\n", s_handle, dummy, rc));
  if ( rc == NO_ERROR && oflags & XO_APPEND )
    rc = DosSetFilePtr( s_handle, 0, FILE_END, &dummy );
  FILE_RELEASE_DISK(this);

  if ( rc != NO_ERROR ) {
    errno = error = map_os2_errors( rc );
    s_handle = (HFILE)-1;
    return -1;
  } else {
    ASSERT(s_handle != (HFILE)-1);
    return 0;
  }
}

/* Reads count bytes from the file into buffer. Returns the number
   of bytes placed in result. The return value 0 indicates an attempt
   to read at end-of-file. A return value -1 indicates an error.     */
int XIOfile::read( void* result, unsigned int count )
{
  APIRET rc;
  ULONG actual = 0; // DosRead seems to dislike random numbers

  FILE_REQUEST_DISK(this);
  ASSERT(s_handle != (HFILE)-1);
  rc = DosRead( s_handle, result, count, &actual );
  FILE_RELEASE_DISK(this);

  if ( rc != NO_ERROR )
  { DEBUGLOG(("XIOfile::read: failed %lu\n", rc));
    errno = error = map_os2_errors( rc );
    return -1;
  } else if (actual == 0) {
    eof = true;
  }
  DEBUGLOG(("XIOfile::read(%p, %u): %u - hfile = %i, rc = %i\n", result, count, actual, s_handle, rc));
  return actual;
}

/* Writes count bytes from source into the file. Returns the number
   of bytes moved from the source to the file. The return value may
   be positive but less than count. A return value of -1 indicates an
   error */
int XIOfile::write( const void* source, unsigned int count )
{
  APIRET rc;
  ULONG actual;

  FILE_REQUEST_DISK(this);
  ASSERT(s_handle != (HFILE)-1);
  rc = DosWrite( s_handle, (PVOID)source, count, &actual );
  FILE_RELEASE_DISK(this);

  if ( rc != NO_ERROR )
  { errno = error = map_os2_errors( rc );
    return -1;
  } else {
    errno = error = 0;
    return actual;
  }
}

/* Closes the file. Returns 0 if it successfully closes the file. A
   return value of -1 shows an error. */
int XIOfile::close()
{ DEBUGLOG(("XIOfile(%p)::close() - %i\n", this, s_handle));
  if (s_handle == (HFILE)-1)
  { errno = EBADF;
    return -1;
  }

  FILE_REQUEST_DISK(this);
  APIRET rc = DosClose( s_handle );
  FILE_RELEASE_DISK(this);

  if ( rc != NO_ERROR ) {
    errno = map_os2_errors( rc );
    return -1;
  } else {
    s_handle = (HFILE)-1; // some invalid value
    return 0;
  }
}

/* Returns the current position of the file pointer. The position is
   the number of bytes from the beginning of the file. On devices
   incapable of seeking, the return value is -1L. */
long XIOfile::tell( long* offset64 )
{
  APIRET rc;
  ULONG actual;

  FILE_REQUEST_DISK(this);
  ASSERT(s_handle != (HFILE)-1);
  rc = DosSetFilePtr( s_handle, 0, FILE_CURRENT, &actual );
  FILE_RELEASE_DISK(this);

  if ( rc != NO_ERROR )
  { errno = map_os2_errors( rc );
    return -1;
  } else {
    // TODO: 64 bit support
    if (offset64)
      *offset64 = 0;
    return actual;
  }
}

/* Moves any file pointer to a new location that is offset bytes from
   the origin. Returns the offset, in bytes, of the new position from
   the beginning of the file. A return value of -1L indicates an
   error. */
long XIOfile::seek( long offset, XIO_SEEK origin, long* offset64 )
{
  ULONG  mode;
  APIRET rc;
  ULONG actual;

  switch( origin ) {
    case XIO_SEEK_SET: mode = FILE_BEGIN;   break;
    case XIO_SEEK_CUR: mode = FILE_CURRENT; break;
    case XIO_SEEK_END: mode = FILE_END;     break;
    default:
      errno = EINVAL;
      return -1L;
  }

  FILE_REQUEST_DISK(this);
  ASSERT(s_handle != (HFILE)-1);
  rc = DosSetFilePtr( s_handle, offset, mode, &actual );
  FILE_RELEASE_DISK(this);

  if ( rc != NO_ERROR )
  { errno = error = map_os2_errors( rc );
    return -1L;
  } else {
    // TODO: 64 bit support
    if (offset64)
      *offset64 = 0;
    errno = error = 0;
    eof   = false;
    return actual;
  }
}

/* Returns the size of the file. A return value of -1L indicates an
   error or an unknown size. */
long XIOfile::getsize( long* offset64 )
{
  APIRET rc;
  FILESTATUS3 fi;

  FILE_REQUEST_DISK(this);
  ASSERT(s_handle != (HFILE)-1);
  rc = DosQueryFileInfo( s_handle, FIL_STANDARD, &fi, sizeof fi );
  FILE_RELEASE_DISK(this);

  if ( rc != NO_ERROR )
  { errno = map_os2_errors( rc );
    return -1L;
  } else {
    // TODO: 64 bit support
    if (offset64)
      *offset64 = 0;
    return fi.cbFile;
  }
}

static time_t convert_OS2_ftime(FDATE date, FTIME time)
{ struct tm value = {0};
  value.tm_year = date.year + 80;
  value.tm_mon = date.month -1;
  value.tm_mday = date.day;
  value.tm_hour = time.hours;
  value.tm_min = time.minutes;
  value.tm_sec = time.twosecs << 1;
  return mktime(&value);
}

int XIOfile::getstat( XSTAT* st )
{ APIRET rc;
  FILESTATUS3 fi;
  EAOP2  eaop2 = {NULL};

  FILE_REQUEST_DISK(this);
  ASSERT(s_handle != (HFILE)-1);
  rc = DosQueryFileInfo(s_handle, FIL_STANDARD, &fi, sizeof fi);

  if (rc == NO_ERROR)
  { eaop2.fpGEA2List = (GEA2LIST*)alloca(sizeof(GEA2LIST) + 5);
    eaop2.fpGEA2List->cbList = sizeof(GEA2LIST) + 5;
    eaop2.fpGEA2List->list[0].oNextEntryOffset = 0;
    eaop2.fpGEA2List->list[0].cbName = 5;
    memcpy(eaop2.fpGEA2List->list[0].szName, ".type", 6);
    // Since there is nor reasonable efficient way to determine the size of one EA
    // a rather large buffer is allocated.
    eaop2.fpFEA2List = (FEA2LIST*)alloca(sizeof(FEA2LIST) + sizeof(FEA2) + 256);
    eaop2.fpFEA2List->cbList = sizeof(FEA2LIST) + sizeof(FEA2) + 256;

    if (DosQueryFileInfo(s_handle, FIL_QUERYEASFROMLIST, &eaop2, sizeof eaop2) != NO_ERROR)
      eaop2.fpFEA2List = NULL; // No .TYPE EA.
  }
  FILE_RELEASE_DISK(this);

  if ( rc != NO_ERROR )
  { errno = map_os2_errors( rc );
    return -1L;
  }
  // TODO: 64 bit support

  st->size = fi.cbFile;
  st->atime = convert_OS2_ftime(fi.fdateLastAccess, fi.ftimeLastAccess);
  st->mtime = convert_OS2_ftime(fi.fdateLastWrite, fi.ftimeLastWrite);
  st->ctime = convert_OS2_ftime(fi.fdateCreation, fi.ftimeCreation);
  st->attr = fi.attrFile; // This ftp client is always read only

  *st->type = 0;
  if (eaop2.fpFEA2List && eaop2.fpFEA2List->list[0].cbValue)
  { // Decode .TYPE EA
    // easize = eaop2.fpFEA2List->list[0].cbValue;
    const USHORT* eadata = (const USHORT*)(eaop2.fpFEA2List->list[0].szName + eaop2.fpFEA2List->list[0].cbName + 1);
    USHORT eatype = *eadata++;
    eadecode(st->type, sizeof(st->type), eatype, &eadata);
  }

  return 0;
}

/* Lengthens or cuts off the file to the length specified by size.
   You must open the file in a mode that permits writing. Adds null
   characters when it lengthens the file. When cuts off the file, it
   erases all data from the end of the shortened file to the end
   of the original file. */
int XIOfile::chsize( long size, long offset64 )
{
  APIRET rc;

  FILE_REQUEST_DISK(this);
  // TODO: 64 bit
  ASSERT(s_handle != (HFILE)-1);
  rc = DosSetFileSize( s_handle, size );
  FILE_RELEASE_DISK(this);

  if ( rc != NO_ERROR )
  { errno = map_os2_errors( rc );
    return -1L;
  } else {
    return 0;
  }
}

XSFLAGS XIOfile::supports() const
{ return
    XS_CAN_READ   | XS_CAN_WRITE | XS_CAN_READWRITE |
    XS_CAN_CREATE | XS_CAN_SEEK  | XS_CAN_SEEK_FAST;
}

XIO_PROTOCOL XIOfile::protocol() const
{ return XIO_PROTOCOL_FILE;
}

/* Cleanups the file protocol. */
XIOfile::~XIOfile()
{
  close();
}

/* Initializes the file protocol. */
XIOfile::XIOfile()
: s_handle((HFILE)-1)
{ blocksize = 32768;
}

