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

#include "cddb.h"

void displayError(char *fmt, ...);
void displayMessage(char *fmt, ...);
void writeToLog(char *buffer, int size);

// used internally to manage CDDB and saved in cddaplay.ini file
typedef struct
{
   char *CDDBServers[128];
   char *HTTPServers[128];

   BOOL isCDDBSelect[128];
   BOOL isHTTPSelect[128];

   int numCDDB;
   int numHTTP;

   BOOL useCDDB;
   BOOL useHTTP;
   BOOL tryAllServers;
   char proxyURL[1024];
   char email[1024];
   
   char cddrive[4];          /* Default CD drive.                      */

} CDDA_SETTINGS;

extern CDDA_SETTINGS settings; // read-only!


#define NOSERVER 0
#define CDDB     1
#define HTTP     2

ULONG getNextCDDBServer(char *server, ULONG size, SHORT *index);
void getUserHost(char *user, int sizeuser, char *host, int sizehost);

/* for multiple match dialog */
struct FUZZYMATCHCREATEPARAMS
{
   CDDBQUERY_DATA *matches, chosen;
};
// window procedure for the fuzzy match dialog
MRESULT EXPENTRY wpMatch(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2);
