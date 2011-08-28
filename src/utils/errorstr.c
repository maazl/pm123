/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp�  <rosmo@sektori.com>
 *
 * Copyright 2004 Dmitry A.Steklenev <glass@ptv.ru>
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

/* Uses error constants in nerror.h (16bit stack, the 32 bit stack
   already has a sock_strerror() ) for TCP/IP in OS/2 and returns
   the string in the comments */

#if defined(__IBMC__) || defined(__WATCOMC__)
  #include <errno.h>
  #include <nerrno.h>
#else
  #include <sys/errno.h>
#endif

#include <string.h>
#include <stdio.h>
#include <netdb.h>

#define  INCL_DOS
#define  INCL_ERRORS

#include "strutils.h"
#include "snprintf.h"
#include <os2.h>

#if defined(TCPV40HDRS)
const char*
sock_strerror( int socket_errno )
{
  switch( socket_errno )
  {
    case SOCEPERM           : return "Not owner.";
    case SOCESRCH           : return "No such process.";
    case SOCEINTR           : return "Interrupted system call.";
    case SOCENXIO           : return "No such device or address.";
    case SOCEBADF           : return "Bad file number.";
    case SOCEACCES          : return "Permission denied.";
    case SOCEFAULT          : return "Bad address.";
    case SOCEINVAL          : return "Invalid argument.";
    case SOCEMFILE          : return "Too many open files.";
    case SOCEPIPE           : return "Broken pipe.";

    case SOCEOS2ERR         : return "Operating system error.";

    case SOCEWOULDBLOCK     : return "Operation would block.";
    case SOCEINPROGRESS     : return "Operation now in progress.";
    case SOCEALREADY        : return "Operation already in progress.";
    case SOCENOTSOCK        : return "Socket operation on non-socket.";
    case SOCEDESTADDRREQ    : return "Destination address required.";
    case SOCEMSGSIZE        : return "Message too long.";
    case SOCEPROTOTYPE      : return "Protocol wrong type for socket.";
    case SOCENOPROTOOPT     : return "Protocol not available.";
    case SOCEPROTONOSUPPORT : return "Protocol not supported.";
    case SOCESOCKTNOSUPPORT : return "Socket type not supported.";
    case SOCEOPNOTSUPP      : return "Operation not supported on socket.";
    case SOCEPFNOSUPPORT    : return "Protocol family not supported.";
    case SOCEAFNOSUPPORT    : return "Address family not supported by protocol family.";
    case SOCEADDRINUSE      : return "Address already in use.";
    case SOCEADDRNOTAVAIL   : return "Can't assign requested address.";
    case SOCENETDOWN        : return "Network is down.";
    case SOCENETUNREACH     : return "Network is unreachable.";
    case SOCENETRESET       : return "Network dropped connection on reset.";
    case SOCECONNABORTED    : return "Software caused connection abort.";
    case SOCECONNRESET      : return "Connection reset by peer.";
    case SOCENOBUFS         : return "No buffer space available.";
    case SOCEISCONN         : return "Socket is already connected.";
    case SOCENOTCONN        : return "Socket is not connected.";
    case SOCESHUTDOWN       : return "Can't send after socket shutdown.";
    case SOCETOOMANYREFS    : return "Too many references: can't splice.";
    case SOCETIMEDOUT       : return "Connection timed out.";
    case SOCECONNREFUSED    : return "Connection refused.";
    case SOCELOOP           : return "Too many levels of symbolic links.";
    case SOCENAMETOOLONG    : return "File name too long.";
    case SOCEHOSTDOWN       : return "Host is down.";
    case SOCEHOSTUNREACH    : return "No route to host.";
    case SOCENOTEMPTY       : return "Directory not empty.";

    default:
      return "";
  }
}
#endif

const char*
h_strerror( int tcpip_errno )
{
  switch( tcpip_errno )
  {
    case HOST_NOT_FOUND  : return "Host not found.";
    case TRY_AGAIN       : return "Server failure.";
    case NO_RECOVERY     : return "Non-recoverable error.";
    case NO_DATA         : return "Valid name, no data record of requested type.";

    default:
      return "";
  }
}

#ifdef __IBMC__
/* Primary purpose of this function is replacement of strerror()
   to avoid problems with usage of a message file at compiling by the
   IBM VAC++. */
const char*
clib_strerror( int clib_errno )
{
  switch( clib_errno )
  {
    case EDOM           : return "Domain error.";
    case ERANGE         : return "Range error.";
    case EBADMODE       : return "The file mode parameter is not correct.";
    case EBADNAME       : return "The file name is "", a null pointer, or an invalid DDNAME.";
    case EISTEMPMEM     : return "Temporary memory files cannot be reopened.";
    case EBADSHARE      : return "The file sharing mode specified is not correct.";
    case EBUFMODE       : return "The buffering mode specified is not correct.";
    case EERRSET        : return "A previous error has occurred on the stream.";
    case EISOPEN        : return "The file is open.";
    case ENOTEXIST      : return "The file cannot be found.";
    case ENOTINIT       : return "This operation must be done before any reads, writes, or repositions.";
    case ENULLFCB       : return "The stream pointer is NULL.";
    case EOUTOFMEM      : return "There is not enough memory available to complete the operation.";
    case ESMALLBF       : return "The specified buffer size is too small.";
    case EEXIST         : return "The file already exists.";
    case ENOGEN         : return "A unique file name could not be generated.";
    case ENOSEEK        : return "The seek operation is not valid for this stream.";
    case EBADPOS        : return "The file position for the file is not valid.";
    case EBADSEEK       : return "Attempted to seek to an invalid file position.";
    case ENOENT         : return "The file or directory specified cannot be found.";
    case EACCESS        : return "The file or directory specified is read-only.";
    case EMFILE         : return "Too many open files.";
    case ENOCMD         : return "A command processor could not be found.";
    case EGETANDPUT     : return "A read operation cannot immediately follow a write operation.";
    case EPASTEOF       : return "Attempted to read past end-of-file.";
    case ENOTREAD       : return "The file is not open for reading.";
    case ETOOMANYUNGETC : return "Too many consecutive calls to ungetc.";
    case EUNGETEOF      : return "Cannot put EOF back to the stream.";
    case EPUTUNGET      : return "Cannot put a character back to the stream immediately "  \
                                 "following a write operation on the stream.";
    case ECHILD         : return "The process identifier specified for the child process " \
                                 "is not valid.";
    case EINTR          : return "The child process ended abnormally.";
    case EINVAL         : return "The action code specified is not correct.";
    case ENOEXEC        : return "Cannot run the specified file.";
    case EAGAIN         : return "Cannot start another process.";
    case EBADTYPE       : return "The stream specified is the wrong type for the operation.";
    case ENOTWRITE      : return "The file is not opened for writing.";
    case EPUTANDGET     : return "A write operation must not immediately follow a read operation.";
    case ELARGEBF       : return "The specified buffer length is too large.";
    case EBADF          : return "The file handle is not valid.";
    case EXDEV          : return "Cannot rename a file to a different device.";
    case ENOSPC         : return "There is no space left on the device.";
    case EMATH          : return "An unrecognized exception occurred in a math routine.";
    case EMODNAME       : return "The DLL specified cannot be found.";
    case EMAXATTR       : return "The value specified for blksize or lrecl is too large.";
    case EREADERROR     : return "Error in reading the C Locale Description (CLD) file.";
    case EBADATTR       : return "The value specified for blksize or lrecl conflicts  with " \
                                 "a previously set value.";
    case EILSEQ         : return "An encoding error was detected.";
    case E2BIG          : return "Argument list too long.";
    case EOS2ERR        : return "Operating system error.";

    default:
      return "Unknown error.";
  }
}
#endif

char* os2_strerror( unsigned long os2_errno, char* result, size_t size )
{ ULONG  ulMessageLength;
  APIRET rc = DosGetMessage(0, 0, result, size, os2_errno, (PSZ)"OSO001.MSG", &ulMessageLength);
  if (rc == NO_ERROR)
  { result[ulMessageLength] = 0;
    if (result[ulMessageLength-1] == '\n')
    { result[ulMessageLength-1] = 0;
      if (result[ulMessageLength-2] == '\r')
        result[ulMessageLength-2] = 0;
    }
  } else
    snprintf(result, size, "No error text is available. Error code is %06lu.", os2_errno);
  return result;
}

static void make_os2pm_error(char* result, size_t size, ERRINFO* perr)
{ size_t n = snprintf(result, size, "ERRORID %lx", perr->idError);
  if (size <= n)
    return;
  size -= n; result += n;
  if (perr->cDetailLevel)
  { USHORT* off;
    size_t i;
    strlcpy(result, ", Details: ", size);
    if (size <= 10)
      return;
    size -= 10; result += 10;
    off = (USHORT*)((char*)perr + perr->offaoffszMsg);
    for (i = 0; i < perr->cDetailLevel; ++i) 
    { const char* msg = (const char*)perr + off[i];
      if (*msg == 0)
        continue;
      n = snprintf(result, size, "  %u %s\n", off[i], msg);
      if (size <= n)
        return;
      size -= n; result += n;
    }
  }
  if (perr->offBinaryData > perr->cbFixedErrInfo)
  { const unsigned char* bp;
    strlcpy(result, "\n  ", size);
    if (size <= 3)
      return;
    size -= 3; result += 3;
    n = perr->cbFixedErrInfo - perr->offBinaryData;
    if (n > (size-1)>>1)
      n = (size-1)>>1;
    bp = ((const unsigned char*)perr + perr->offBinaryData);
    while (n)
    { sprintf(result, "%02x", *bp++);
      result += 2;
    }
    *result = 0; 
  }
}

char* os2pm_strerror( char* result, size_t size )
{ ERRINFO* perr = WinGetErrorInfo(NULLHANDLE); // Well a NULL-HAB is not as documented, but it is working.
  if (!perr)
  { if (size)
      *result = 0;
    return result;
  }
  make_os2pm_error(result, size, perr );
  WinFreeErrorInfo(perr);
  return result;
}

