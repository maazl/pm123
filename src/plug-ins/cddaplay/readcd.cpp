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

/* CD drive functions */

#define INCL_PM
#define INCL_DOSDEVIOCTL
#define INCL_DOS
#include <os2.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <utilfct.h>
#include <snprintf.h>
#include "readcd.h"
#include "cdda.h"
#include "cddarc.h"

#include <debuglog.h>


const char TAG[4] = {'C','D','0','1'};


/* Simple thread-safe class to manage list of negative hits without a dependency to STL
 */
class ULset
{
   private:
      unsigned long list[1024];
      unsigned int  count;
      HMTX Mtx;

   protected:
      unsigned long find(unsigned long ul);

   public:
      ULset() : count(0) { DosCreateMutexSem(NULL, &Mtx, 0, FALSE); }
      ~ULset() { DosReleaseMutexSem(Mtx); }

      BOOL lookup(unsigned long ul);
      BOOL store(unsigned long ul);
      BOOL remove(unsigned long ul);

} negative_hits;

unsigned long ULset::find(unsigned long ul)
{  // binary search
   unsigned int l = 0;
   unsigned int r = count;

   while (l < r)
   {
      unsigned long m = (r+l) >> 1;
      if (ul <= list[m])
         r = m;
      else
         l = m+1;
   }

   return l;
}

BOOL ULset::lookup(unsigned long ul)
{
   DosRequestMutexSem(Mtx, SEM_INDEFINITE_WAIT);
   unsigned long p = find(ul);
   BOOL r = p < count && list[p] == ul;
   DosReleaseMutexSem(Mtx);
   return r;
}

BOOL ULset::store(unsigned long ul)
{
   BOOL r = FALSE;
   DosRequestMutexSem(Mtx, SEM_INDEFINITE_WAIT);
   if (count < sizeof list / sizeof *list) // list full?
   {
      unsigned long p = find(ul);
      if (p >= count || list[p] != ul) // duplicate?
      {  // insert
         memmove(list + p, list + p +1, (count - p) * sizeof *list);
         count++;
         list[p] = ul;
         r = TRUE;
      }
   }
   DosReleaseMutexSem(Mtx);
   return r;
}

BOOL ULset::remove(unsigned long ul)
{
   BOOL r = FALSE;
   DosRequestMutexSem(Mtx, SEM_INDEFINITE_WAIT);
   unsigned long p = find(ul);
   if (p < count && list[p] == ul)
   {  // delete
      count--;
      memmove(list + p +1, list + p, (count - p) * sizeof *list);
      r = TRUE;
   }
   DosReleaseMutexSem(Mtx);
   return r;
}


void CD_drive::updateError(char *fmt, ...)
{
   DEBUGLOG2(("cddaplay:CD_drive(%p)::updateError(%s)\n", this, fmt));
   va_list args;
   va_start(args, fmt);
   free(error);
   size_t len = vsnprintf(NULL, 0, fmt, args) +1;
   DEBUGLOG2(("cddaplay:CD_drive::updateError(%s) - %u\n", fmt, len));
   if (len > 1024)
      len = 1024;
   error = (char*)malloc(len);
   vsnprintf(error, len, fmt, args);
   DEBUGLOG(("cddaplay:CD_drive(%p)::updateError(%s)\n", this, error));
   va_end(args);
}


CD_drive::CD_drive(char drive)
{
   DEBUGLOG(("cddaplay:CD_drive(%p)::CD_drive(%c)\n", this, drive));
   driveLetter[0] = drive;
   driveLetter[1] = ':';
   driveLetter[2] = 0;
   opened = FALSE;
   memset(&cdInfo,0,sizeof(cdInfo));
   trackInfo = NULL;
   memset(&querydata, 0, sizeof querydata);
   error = NULL;
   instCount = 0;
   exclusive = FALSE;
   DosCreateMutexSem(NULL, &instMtx, 0, FALSE);
}

CD_drive::~CD_drive()
{
   DEBUGLOG(("cddaplay:CD_drive(%p)::~CD_drive()\n", this));
   free(trackInfo);
   if (opened)
     close();
   free(error);
   DosCloseMutexSem(instMtx);
}

BOOL CD_drive::open()
{
   DEBUGLOG(("cddaplay:CD_drive(%p)::open()\n", this));
   ULONG ulAction;
   ULONG rc;

   if (opened)
      DosClose(hCDDrive);

   clearError();

   rc = DosOpen(driveLetter, &hCDDrive, &ulAction, 0,
                FILE_NORMAL, OPEN_ACTION_OPEN_IF_EXISTS,
                OPEN_SHARE_DENYNONE | OPEN_ACCESS_READONLY |
                OPEN_FLAGS_DASD, NULL);

   if(rc)
   {
      updateError("DosOpen failed with return code %d opening drive %s",rc,driveLetter);
      return FALSE;
   }

   opened = TRUE;
   return TRUE;
}

BOOL CD_drive::close()
{
   DEBUGLOG(("cddaplay:CD_drive(%p)::close()\n", this));
   if (opened)
   {
      ULONG rc;
      rc = DosClose(hCDDrive);
      opened = FALSE;
      if(rc)
      {
         updateError("DosClose failed with return code %d closing drive",rc);
         return FALSE;
      }
   }

   return TRUE;
}

BOOL CD_drive::readCDInfo()
{
   DEBUGLOG(("cddaplay:CD_drive(%p)::readCDInfo()\n", this));
   ULONG ulParamLen;
   ULONG ulDataLen;
   ULONG rc;
   char tag[4];

   if (!opened)
      return FALSE;

   memset(&cdInfo, 0, sizeof cdInfo);

   memcpy(tag,TAG,4);
   rc = DosDevIOCtl(hCDDrive, IOCTL_CDROMAUDIO, CDROMAUDIO_GETAUDIODISK,
                    tag, 4, &ulParamLen, &cdInfo,
                    sizeof(cdInfo), &ulDataLen);
   if(rc)
   {  updateError("DosDevIOCtl failed with return code 0x%x reading CD info",rc);
      return FALSE;
   }
   else
   {  DEBUGLOG(("cddaplay:CD_drive()::readCDInfo: TRUE {%u,%u,{%u,%u,%u,%u}}\n",
         cdInfo.firstTrack, cdInfo.lastTrack, cdInfo.leadOutAddress.frame, cdInfo.leadOutAddress.second, cdInfo.leadOutAddress.minute, cdInfo.leadOutAddress.empty));
      return TRUE;
   }
}

BOOL CD_drive::readTrackInfo(char track, CDTRACKINFO *trackInfo2)
{
   DEBUGLOG(("cddaplay:CD_drive(%p)::readTrackInfo()\n", this));
   ULONG ulParamLen;
   ULONG ulDataLen;
   ULONG rc;
   BOOL returnBool = FALSE;

   if(!opened)
      return FALSE;

   CDAUDIOTRACKINFOPARAM trackParam;
   CDAUDIOTRACKINFODATA  trackInfo[2];

   memcpy(trackParam.signature,TAG,4);

   trackParam.trackNum = track;
   rc = DosDevIOCtl(hCDDrive, IOCTL_CDROMAUDIO, CDROMAUDIO_GETAUDIOTRACK,
                    &trackParam, sizeof(trackParam),
                    &ulParamLen, &trackInfo[0],
                    sizeof(trackInfo[0]), &ulDataLen);
   if(rc)
      updateError("DosDevIOCtl failed with return code 0x%x reading track %d info",rc,track);
   else
   {
      trackParam.trackNum = track+1;
      rc = 0;
      if(trackParam.trackNum <= cdInfo.lastTrack)
      {
         rc = DosDevIOCtl(hCDDrive, IOCTL_CDROMAUDIO, CDROMAUDIO_GETAUDIOTRACK,
                          &trackParam, sizeof(trackParam),
                          &ulParamLen, &trackInfo[1],
                          sizeof(trackInfo[1]), &ulDataLen);

         if(rc)
            updateError("DosDevIOCtl failed with return code 0x%x",rc);
      }
      else
          trackInfo[1].address = cdInfo.leadOutAddress;

      if(!rc)
      {
         ULONG LBA[2];
         MSF length;

         LBA[0] = getLBA(trackInfo[0].address);
         LBA[1] = getLBA(trackInfo[1].address);

         /* -150 because we want length, not an address */
         length = getMSF(LBA[1]-LBA[0]-150);

         trackInfo2->start = trackInfo[0].address;
         trackInfo2->end = trackInfo[1].address;
         trackInfo2->length = length;
         trackInfo2->size = (LBA[1]-LBA[0])*2352;
         trackInfo2->data = (trackInfo[0].info & 0x40) ? TRUE : FALSE;
         trackInfo2->channels = (trackInfo[0].info & 0x80) ? 4 : 2;
         trackInfo2->number = track;

         returnBool = TRUE;
      }
   }
   return returnBool;
}


BOOL CD_drive::fillTrackInfo()
{
   int i,e;

   free(trackInfo);
   trackInfo = (CDTRACKINFO *)malloc(getCount() * sizeof(*trackInfo));

   e = 0;
   for(i = cdInfo.firstTrack; i <= cdInfo.lastTrack; i++)
      if(!readTrackInfo(i, &trackInfo[e++]))
         return FALSE;

   return TRUE;
}


BOOL CD_drive::play(char track)
{
   ULONG ulParamLen;
   ULONG ulDataLen;
   ULONG rc;
   BOOL returnBool = FALSE;
   CDPLAYAUDIOPARAM playParam;
   CDTRACKINFO trackInfo;

   memcpy(playParam.signature,TAG,4);
   playParam.addressingMode = MODE_MSF;

   if(!readTrackInfo(track, &trackInfo))
       return FALSE;
   playParam.start = trackInfo.start;
   playParam.end = trackInfo.end;

   rc = DosDevIOCtl(hCDDrive, IOCTL_CDROMAUDIO, CDROMAUDIO_PLAYAUDIO,
                    &playParam, sizeof(playParam),
                    &ulParamLen, NULL,
                    0, &ulDataLen);
   if(rc)
      updateError("DosDevIOCtl failed with return code 0x%x playing track %d",rc,track);
   else
      returnBool = TRUE;

   return returnBool;
}

BOOL CD_drive::stop()
{
   ULONG ulParamLen;
   ULONG ulDataLen;
   ULONG rc;
   BOOL returnBool = FALSE;
   char tag[4];

   memcpy(tag,TAG,4);

   rc = DosDevIOCtl(hCDDrive, IOCTL_CDROMAUDIO, CDROMAUDIO_STOPAUDIO,
                    tag, 4, &ulParamLen, NULL, 0, &ulDataLen);
   if(rc)
      updateError("DosDevIOCtl failed with return code 0x%x stopping playing.",rc);
   else
      returnBool = TRUE;
   return returnBool;
}

BOOL CD_drive::readSectors(CDREADLONGDATA data[], ULONG number, ULONG start)
{
   ULONG ulParamLen;
   ULONG ulDataLen;
   ULONG rc;
   BOOL returnBool = FALSE;
   CDREADLONGPARAM readParam = {0};

   memcpy(readParam.signature,TAG,4);
   readParam.addressingMode = MODE_LBA;
   readParam.numberSectors = number;
   readParam.startSector = start;

   rc = DosDevIOCtl(hCDDrive, IOCTL_CDROMDISK, CDROMDISK_READLONG,
                    &readParam, sizeof(readParam), &ulParamLen,
                    data, sizeof(*data)*number, &ulDataLen);

   if(rc)
      updateError("DosDevIOCtl failed with return code 0x%x reading sector %d-%d",rc,start,start+number-1);
   else
      returnBool = TRUE;
   return returnBool;
}

BOOL CD_drive::getUPC(char UPC[7])
{
   ULONG rc;
   BOOL returnBool = FALSE;
   CDGETUPCDATA getUPCData = {0};
   char tag[4];
   ULONG ulParamLen = sizeof(tag);
   ULONG ulDataLen = sizeof(getUPCData);

   memcpy(tag,TAG,4);

   rc = DosDevIOCtl(hCDDrive, IOCTL_CDROMDISK, CDROMDISK_GETUPC,
                    tag, ulParamLen, &ulParamLen,
                    &getUPCData, ulDataLen, &ulDataLen);

   if(rc)
      updateError("DosDevIOCtl failed with return code 0x%x getting UPC info.",rc);
   else
   {
      memcpy(UPC,getUPCData.UPC,sizeof(getUPCData.UPC));
      returnBool = TRUE;
   }
   return returnBool;
}

BOOL CD_drive::loadCDDBInfo(BOOL refresh)
{
   BOOL  foundCached = FALSE;
   HINI  INIhandle;
   BOOL  success = FALSE;
   char  server[1024];
   ULONG serverType;
   SHORT index = -1;

   querydata.discid_cd = cddb.discid(this);

   DEBUGLOG(("cddaplay:CD_drive(%p)::loadCDDBInfo() - discid = %x\n", this, querydata.discid_cd));

   if (!refresh && negative_hits.lookup(querydata.discid_cd)) {
      displayMessage( "Found cached negative hit: %08x", querydata.discid_cd );
      return FALSE;
   }

   if((INIhandle = open_module_ini()) != NULLHANDLE)
   {
      char discid[10];
      sprintf(discid, "%08lx",querydata.discid_cd);

      ULONG cdinfoSize = 0;
      if(PrfQueryProfileSize(INIhandle, "CDInfo", discid, &cdinfoSize) && cdinfoSize > 0)
      {  // found cached
         char *cdinfoBuffer = (char *)malloc(cdinfoSize);
         if(PrfQueryProfileString(INIhandle, "CDInfo", discid, "", cdinfoBuffer, cdinfoSize))
         {
            DEBUGLOG(("cddaplay:CD_drive()::loadCDDBInfo() - ini cache hit\n"));
            foundCached = TRUE;
            cddb.parse_read_reply(cdinfoBuffer);
         }

         free(cdinfoBuffer);
      }

      close_ini(INIhandle);
   }

   if(foundCached)
      return TRUE;

   // otherwize, go on the Internet

   if( settings.useCDDB ) {
     do
     {
        serverType = getNextCDDBServer(server,sizeof(server),&index);
        if(serverType != NOSERVER)
        {
           if(serverType == CDDB)
           {
              displayMessage("Contacting: %s", server);

              char host[512]=""; int port=8880;
              sscanf(server,"%[^:\n\r]:%d",host,&port);
              if(host[0])
                 cddb.tcpip_socket::connect(host,port);
           }
           else
           {
              char host[512], path[512], *slash = NULL, *http = NULL;

              http = strstr(server,"http://");
              if(http)
                 slash = strchr(http+7,'/');
              if(slash)
              {
                 strlcpy(host,server,slash-server);
                 strcpy(path,slash);
                 if(settings.proxyURL)
                    displayMessage("Contacting: %s", settings.proxyURL);
                 else
                    displayMessage("Contacting: %s", server);
                 cddb.connect(host,settings.proxyURL,path);
              }
           }

           if(!cddb.isOnline())
           {
              if(cddb.getSocketError() == SOCEINTR)
                 goto end;
              else
                 continue;
           }
           displayMessage("Looking at CDDB Server Banner at %s", server);
           if(!cddb.banner_req())
           {
              if(cddb.getSocketError() == SOCEINTR)
                 goto end;
              else
                 continue;
           }
           displayMessage("Handshaking with CDDB Server %s", server);
           char user[128]="",host[128]="";
           getUserHost(user,sizeof(user),host,sizeof(host));
           if(!cddb.handshake_req(user,host))
           {
              if(cddb.getSocketError() == SOCEINTR)
                 goto end;
              else
                 continue;
           }

           displayMessage("Looking for CD information on %s", server);
           int rc = cddb.query_req(this, &querydata);
           if(!rc)
           {
              if(cddb.getSocketError() == SOCEINTR)
                 goto end;
              else
                 continue;
           }
           else if(rc == -1)
              continue; /* no match for this server */
           else if(rc == COMMAND_MORE)
           {
              FUZZYMATCHCREATEPARAMS fuzzyParams = {0};
              HMODULE module;
              char modulename[128];
              int i = 0;

              fuzzyParams.matches = (CDDBQUERY_DATA *) malloc(sizeof(CDDBQUERY_DATA));

              while(cddb.get_query_req(&fuzzyParams.matches[i]))
              {
                 fuzzyParams.matches[i].discid_cd = querydata.discid_cd;
                 i++;
                 fuzzyParams.matches = (CDDBQUERY_DATA *) realloc(fuzzyParams.matches, (i+1)*sizeof(CDDBQUERY_DATA));
              }
              memset(&fuzzyParams.matches[i],0,sizeof(fuzzyParams.matches[i]));
              getModule(&module,modulename,128);
              WinDlgBox(HWND_DESKTOP, HWND_DESKTOP, wpMatch, module, DLG_MATCH, &fuzzyParams);
              memcpy(&querydata, &fuzzyParams.chosen,sizeof(CDDBQUERY_DATA));
              free(fuzzyParams.matches);
           }
           /* else there is only one match */

           if(querydata.discid_cddb != 0)
           {
              displayMessage("Requesting CD information from %s", server);
              if(!cddb.read_req(querydata.category, querydata.discid_cddb))
              {
                 if(cddb.getSocketError() == SOCEINTR)
                    goto end;
                 else
                    continue;
              }
              else
                 success = TRUE;
           }
        }
     } while(serverType != NOSERVER && !success);

     if( !success )
        negative_hits.store(querydata.discid_cd);
     else
        negative_hits.remove(querydata.discid_cd);
   }

end:
   if(success)
   {
      // let's cache it!
      if((INIhandle = open_module_ini()) != NULLHANDLE)
      {
         char discid[10];
         sprintf(discid, "%08lx",querydata.discid_cd);

         PrfWriteProfileString(INIhandle, "CDInfo", discid, cddb.get_raw_reply());

         close_ini(INIhandle);
      }
   }

   return success;
}


/* Cache logic
 */
CD_drive::Cache CD_drive::cache;

CD_drive::Cache::Cache()
{  memset(cache, 0, sizeof cache);
   DosCreateMutexSem(NULL, &cacheMtx, 0, FALSE);
}

CD_drive::Cache::~Cache()
{  // destroy any cached objects
   CD_drive** p = cache + sizeof(cache)/sizeof(*cache);
   while (p != cache)
      (*--p)->release();
   // destroy mutex
   DosCloseMutexSem(cacheMtx);
}


CD_drive* CD_drive::request(char drive, BOOL exclusive, BOOL refresh)
{
   DEBUGLOG(("cddaplay:CD_drive::request(%c, %u, %u)\n", drive, exclusive, refresh));
   drive &= 0xdf; // toupper
   if (drive < 'A' || drive > 'Z')
      return NULL;
   // Cache lookup
   CD_drive* drv = cache.lookup(drive-'A');
   if (drv != NULL)
   {  // cached object found!
      // Lock instance
      DosRequestMutexSem(drv->instMtx, SEM_INDEFINITE_WAIT);
      DEBUGLOG(("cddaplay:CD_drive::request - %p{%s,%u}\n", drv, drv->driveLetter, drv->exclusive));
      // Exclusive access active?
      if (drv->exclusive)
      {  // 2nd exclusive lock?
         if (exclusive)
         {  // Can't use this instance!
            cache.release();
            DEBUGLOG(("cddaplay:CD_drive::request: INTERNAL ERROR: second exclusive lock\n"));
            DosReleaseMutexSem(drv->instMtx);
            return NULL;
         }
      }
      else if (refresh)
      {  // refresh!
         // Even in case on a refresh we do not refresh the currently cached instance
         // to avoid that other references to this instance see changing metadata.
         // We create always a new instance and leave the the other instance alive
         // until it is no longer used.
         // free instance and remove cache reference
         int cnt = --drv->instCount;
         DosReleaseMutexSem(drv->instMtx);
         if (cnt == 0)
            delete drv;
         goto refresh;
      }
      // use cached instance!
      cache.release();
      goto retref;
   }
refresh:
   // Create new instance!
   drv = new CD_drive(drive);
   // Lock instance
   DosRequestMutexSem(drv->instMtx, SEM_INDEFINITE_WAIT);
   // Store 2 Cache
   cache.store(drive-'A', drv);
   drv->instCount++;
   cache.release();
   // Read CD index
   if (drv->open())
   {  drv->readCDInfo() &&
      drv->fillTrackInfo() &&
      drv->loadCDDBInfo(refresh);
      drv->close();
   }
retref:
   // Exclusive access requested?
   drv->exclusive = exclusive;
   // return reference
   drv->instCount++;
   DosReleaseMutexSem(drv->instMtx);
   DEBUGLOG(("cddaplay:CD_drive::request: %p{%i}\n", drv, drv->instCount));
   return drv;
}

void CD_drive::release(BOOL exclusive)
{
   DEBUGLOG(("cddaplay:CD_drive(%p{%i})::release(%u)\n", this, instCount, exclusive));
   // Lock instance
   DosRequestMutexSem(instMtx, SEM_INDEFINITE_WAIT);
   // reset exclusive flag?
   if (exclusive)
      this->exclusive = FALSE;
   // free instance
   int cnt = --instCount;
   DosReleaseMutexSem(instMtx);
   // last reference?
   if (cnt == 0)
      delete this;
}

