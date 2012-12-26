/*
 * Copyright 2011-2011 M.Mueller
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

#define INCL_DOSERROR
#include "dirscan.h"

#include <eautils.h>
#include <strutils.h>
#include <xio.h>
#include <cpp/smartptr.h>
#include <cpp/algorithm.h>


PLUGIN_RC DirScan::InitUrl(const char* url)
{
  Recursive    = false;
  AllFiles     = false;
  Hidden       = false;
  Pattern.reset();
  FoldersFirst = false;
  Items.clear();

  // Check whether it is a folder
  const char* params = strchr(url, '?');
  if (!params)
    params = url + strlen(url);
  // now params points to the parameters starting with '?' or to '\0' if none.

  // Can only play folder URLs
  if (params[-1] != '/')
    return PLUGIN_NO_PLAY;

  // copy url without parameters
  Params = params;
  BasePathLen = params - url;
  Path.reserve(_MAX_PATH+25); // typically sufficient size
  Path.append(url, BasePathLen -1); // Discard trailing slash for now

  if (Path[7U] == '/')
    DosPath = Path.cdata() + 8;
  else
  { DosPath = Path.cdata() + 5;
    if (DosPath[1] == '|')
      DosPath[1] = ':';
  }

  // Parse parameters if any.
  while (*params)
  { const char* key = ++params;
    size_t len = strcspn(params, "&");
    params += len;
    const char* value = strnchr(key, '=', len);
    if (value)
    { len = value - key;
      ++value;
    }
    // Now [key..key+len] is the key and [value..Params] is the value.
    switch (len)
    {case 12:
      if (strnicmp(key, "foldersfirst", 12) == 0)
        FoldersFirst = true;
      break;
     case 9:
      if (strnicmp(key, "recursive", 9) == 0)
        Recursive = true;
      break;
     case 8:
      if (strnicmp(key, "allfiles", 8) == 0)
        AllFiles = true;
      break;
     case 7:
      if (strnicmp(key, "pattern", 7) == 0)
        if (value)
          Pattern.assign(value, params-value);
      break;
     case 6:
      if (strnicmp(key, "hidden", 6) == 0)
        Hidden = true;
      break;
    }
  }

  DEBUGLOG(("foldr123:DirScan::InitUrl(%s): Rec=%u, All=%u, Hid=%u, Pat=%s, Path=%s",
    url, Recursive, AllFiles, Hidden, Pattern.cdata(), Path.cdata()));
  return PLUGIN_OK;
}

time_t DirScan::ConvertOS2FTime(FDATE date, FTIME time)
{ struct tm value = {0};
  value.tm_year = date.year + 80;
  value.tm_mon = date.month -1;
  value.tm_mday = date.day;
  value.tm_hour = time.hours;
  value.tm_min = time.minutes;
  value.tm_sec = time.twosecs << 1;
  return mktime(&value);
}

PLUGIN_RC DirScan::GetInfo(const INFO_BUNDLE* info)
{
  FILESTATUS3 stat;
  APIRET rc = DosQueryPathInfo(DosPath, FIL_STANDARD, &stat, sizeof(stat));
  if (rc != NO_ERROR)
  { // Errors of DosQueryPathInfo are considered to be permanent.
    char buf[256];
    os2_strerror(rc, buf, sizeof buf);
    info->tech->info = buf;
    info->phys->attributes = PATTR_INVALID;
    info->rpl->invalid = 1;
    return PLUGIN_NO_READ;
  }
  info->phys->tstmp = ConvertOS2FTime(stat.fdateLastWrite, stat.ftimeLastWrite);
  info->phys->filesize = stat.cbFile;
  info->phys->attributes = stat.attrFile & FILE_READONLY ? PATTR_NONE : PATTR_WRITABLE;
  info->tech->attributes = TATTR_PLAYLIST;
  return PLUGIN_OK;
}

bool DirScan::IsDot(const char* name, size_t len)
{ switch (len)
  {case 2:
    if (name[1] != '.')
      break;
   case 1:
    if (name[0] == '.')
      return true;
  }
  return false;
}

void DirScan::Scan()
{ DEBUGLOG(("foldr123:DirScan::Scan()\n"));
  // Seek for all files.
  Path.append('/');
  if (Pattern)
    Path.append(Pattern);
  else
    Path.append('*');
  // allocate output buffer
  sco_arr<char> buffer(65534);
  HDIR hdir = HDIR_CREATE;
  ULONG flags = FILE_ARCHIVED|FILE_READONLY
              | Recursive * FILE_DIRECTORY
              | Hidden * (FILE_HIDDEN|FILE_SYSTEM);
  if (AllFiles)
  { ULONG count = 1000; // Hmm...
    APIRET rc = DosFindFirst(DosPath, &hdir, flags, buffer.get(), buffer.size(), &count, FIL_STANDARD);
    DEBUGLOG(("foldr123:DirScan::Scan: DosFindFirst(%s,, %x): %i, %u\n", DosPath, flags, rc, count));
    while (rc == 0)
    { // process result package
      FILEFINDBUF3* fb = (FILEFINDBUF3*)buffer.get();
      while (count--)
      { if (!IsDot(fb->achName, fb->cchName))
          Items.append() = new Entry(fb->achName, fb->cchName, fb->attrFile, fb->cbFile, ConvertOS2FTime(fb->fdateLastWrite, fb->ftimeLastWrite));
        // next entry
        if (!fb->oNextEntryOffset)
          break;
        fb = (FILEFINDBUF3*)((char*)fb + fb->oNextEntryOffset);
      }
      // next package
      count = 1000;
      rc = DosFindNext(hdir, buffer.get(), buffer.size(), &count);
    }
  } else
  { // prepare GEA2LIST
    EAOP2* const eaop = (EAOP2*)buffer.get();
    eaop->fpGEA2List = (GEA2LIST*)alloca(sizeof(GEA2LIST) +5);
    eaop->fpGEA2List->cbList = sizeof(GEA2LIST) +5;
    eaop->fpGEA2List->list[0].oNextEntryOffset = 0;
    eaop->fpGEA2List->list[0].cbName = 5;
    memcpy(eaop->fpGEA2List->list[0].szName, ".type", 6);
    eaop->fpFEA2List = NULL;
    eaop->oError = 0;
    ULONG count = 1000; // Hmm...
    APIRET rc = DosFindFirst(DosPath, &hdir, flags, buffer.get(), buffer.size(), &count, FIL_QUERYEASFROMLIST);
    DEBUGLOG(("foldr123:DirScan::Scan: DosFindFirst(%s,, %x, [ea]): %i, %u\n", DosPath, flags, rc, count));
    while (rc == 0)
    { // process result package
      FILEFINDBUF3FEA2LIST* fb = (FILEFINDBUF3FEA2LIST*)(eaop +1);
      while (count--)
      { const char* const name = (char*)&fb->cbList + fb->cbList +1;
        const size_t name_len = *((unsigned char*)&fb->cbList + fb->cbList);
        if (IsDot(name, name_len))
          goto next;
        // File?
        if (!(fb->attrFile & FILE_DIRECTORY))
        { char buf[XIO_MAX_FILETYPE];
          const char* eadata = NULL;
          // Has .type EA?
          if (fb->cbList > sizeof(FEA2))
          { const USHORT* eas = (const USHORT*)(fb->list[0].szName + fb->list[0].cbName +1);
            USHORT eatype = *eas++;
            *buf = 0;
            eadecode(buf, sizeof buf, eatype, &eas);
            eadata = buf;
          }
          Path.erase(BasePathLen);
          Path.append(name, name_len);
          if (!Ctx.plugin_api->obj_supported(Path, eadata))
            goto next;
        }
        Items.append() = new Entry(name, name_len, fb->attrFile, fb->cbFile, ConvertOS2FTime(fb->fdateLastWrite, fb->ftimeLastWrite));
       next:
        // next entry
        if (!fb->oNextEntryOffset)
          break;
        fb = (FILEFINDBUF3FEA2LIST*)((char*)fb + fb->oNextEntryOffset);
      }
      // next package
      count = 1000;
      rc = DosFindNext(hdir, buffer.get(), buffer.size(), &count);
    }
  }
  DosFindClose(hdir);
}

/* binary compatible to strnicmp
static int StandardComparer(const DirScan::Entry* l, const DirScan::Entry* r)
{ return stricmp(l->Path, r->Path);
}*/

int DirScan::FoldersFirstComparer(const DirScan::Entry* l, const DirScan::Entry* r)
{ if ((l->Attributes ^ r->Attributes) & FILE_DIRECTORY)
    return (r->Attributes & FILE_DIRECTORY) - (l->Attributes & FILE_DIRECTORY);
  else
    return stricmp(l->Name, r->Name);
}

static META_INFO empty_meta = META_INFO_INIT;
static ATTR_INFO empty_attr = ATTR_INFO_INIT;

int DirScan::SendResult(DECODER_INFO_ENUMERATION_CB cb, void* param)
{
  // Order by
  merge_sort(Items.begin(), Items.end(), FoldersFirst ? &FoldersFirstComparer : (int (*)(const DirScan::Entry*, const DirScan::Entry*))stricmp);

  foreach (Entry, *const*, epp, Items)
  { const Entry* ep = *epp;
    PHYS_INFO phys = PHYS_INFO_INIT;
    TECH_INFO tech = TECH_INFO_INIT;
    INFO_BUNDLE info = { &phys, &tech, NULL, &empty_meta, &empty_attr };
    phys.filesize  = ep->Size;
    phys.tstmp     = ep->Timestamp;
    int what = INFO_PHYS;
    if (ep->Attributes & FILE_DIRECTORY)
    { tech.attributes = TATTR_PLAYLIST;
      tech.decoder    = "foldr123.dll";
      what |= INFO_TECH|INFO_META|INFO_ATTR;
    } else if (!(ep->Attributes & FILE_READONLY))
      phys.attributes = PATTR_WRITABLE;
    // clac URL
    Path.erase(BasePathLen);
    Path.append(ep->Name);
    if (ep->Attributes & FILE_DIRECTORY)
    { Path.append('/');
      Path.append(Params);
    }
    (*cb)(param, Path.cdata(), &info, INFO_NONE, what);
  }

  return Items.size();
}
