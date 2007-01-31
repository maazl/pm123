/*
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

#ifndef XIO_FTP_H
#define XIO_FTP_H

#include "xio.h"
#include "xio_protocol.h"

#ifndef FTPBASEERR
#define FTPBASEERR 22000
#endif

#define FTP_ANONYMOUS_USER  "anonymous"
#define FTP_ANONYMOUS_PASS  "xio123@"

#define FTP_CONNECTION_ALREADY_OPEN 125
#define FTP_OPEN_DATA_CONNECTION    150
#define FTP_OK                      200
#define FTP_FILE_STATUS             213
#define FTP_SERVICE_READY           220
#define FTP_TRANSFER_COMPLETE       226
#define FTP_PASSIVE_MODE            227
#define FTP_LOGGED_IN               230
#define FTP_FILE_ACTION_OK          250
#define FTP_DIRECTORY_CREATED       257
#define FTP_FILE_CREATED            257
#define FTP_WORKING_DIRECTORY       257
#define FTP_NEED_PASSWORD           331
#define FTP_NEED_ACCOUNT            332
#define FTP_FILE_OK                 350
#define FTP_FILE_UNAVAILABLE        450
#define FTP_SYNTAX_ERROR            500
#define FTP_BAD_ARGS                501
#define FTP_NOT_IMPLEMENTED         502
#define FTP_BAD_SEQUENCE            503
#define FTP_NOT_IMPL_FOR_ARGS       504
#define FTP_NOT_LOGGED_IN           530
#define FTP_FILE_NO_ACCESS          550
#define FTP_PROTOCOL_ERROR          999

#ifdef __cplusplus
extern "C" {
#endif

/* Initializes the ftp protocol. */
XPROTOCOL*  ftp_initialize( XFILE* x );
/* Maps the error number in errnum to an error message string. */
const char* ftp_strerror( int errnum );

#ifdef __cplusplus
}
#endif
#endif /* XIO_FTP_H */

