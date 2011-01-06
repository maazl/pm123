/*
 * Copyright 2009-2010 M.Mueller
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


#include "playlistwriter.h"
#include "plist123.h"
#include <fileutil.h>
#include <charset.h>
#include <stdio.h>

#include <debuglog.h>

/****************************************************************************
*
*  Internal worker classes
*
****************************************************************************/

class LSTWriter : public PlaylistWriter
{public:
  LSTWriter(const char* list) : PlaylistWriter(list) {}
  virtual bool              Init(const INFO_BUNDLE* info, int valid, XFILE* stream);
  virtual bool              AppendItem(const Item& item);
  virtual bool              Finish();
};

class M3UWriter : public PlaylistWriter
{public:
  M3UWriter(const char* list) : PlaylistWriter(list) {}
  virtual bool              Init(const INFO_BUNDLE* info, int valid, XFILE* stream);
  virtual bool              AppendItem(const Item& item);
};

class M3U8Writer : public M3UWriter
{private:
  char                      EncodedLine[4096];
 public:
  M3U8Writer(const char* list) : M3UWriter(list) {}
  virtual bool              Init(const INFO_BUNDLE* info, int valid, XFILE* stream);
 protected:
  virtual void              Write(const char* data);
};

/****************************************************************************
*
*  class PlaylistWriter
*
****************************************************************************/

PlaylistWriter::PlaylistWriter(const char* list)
: ListUrl(list),
  ListInfo(NULL),
  Stream(NULL),
  Options(0),
  OK(false)
{}

bool PlaylistWriter::Init(const INFO_BUNDLE* info, int valid, XFILE* stream)
{ DEBUGLOG(("plst123:PlaylistWriter({%s})::Init(%p, %x, %p)\n", ListUrl, info, valid, stream));
  ListInfo = info;
  Stream = stream;
  Valid = valid;
  return OK = true;
}

void PlaylistWriter::Write(const char* data)
{ OK &= xio_fputs(data, Stream) >= 0;
}

PlaylistWriter* PlaylistWriter::FormatFactory(const char* list, const char* format)
{ DEBUGLOG(("plst123:PlaylistWriter::FormatFactory(%s, %s)\n", list, format));
  if (format)
  { if (stricmp(format, EATYPE_PM123) == 0)
      return new LSTWriter(list);
    if (stricmp(format, EATYPE_WINAMP) == 0)
      return new M3UWriter(list);
    if (stricmp(format, EATYPE_WINAMPU) == 0)
      return new M3U8Writer(list);
  }
  // no format, try extension
  char ext[6];
  sfext(ext, list, sizeof ext);
  if (stricmp(ext, ".m3u") == 0)
    return new M3UWriter(list);
  if (stricmp(ext, ".m3u8") == 0)
    return new M3U8Writer(list);
  // Default to PM123 playlist
  return new LSTWriter(list);
}

bool PlaylistWriter::Finish()
{ DEBUGLOG(("plst123:PlaylistWriter({%s})::Finish()\n", ListUrl));
  return OK;
}

/****************************************************************************
*
*  class LSTWriter
*
****************************************************************************/

bool LSTWriter::Init(const INFO_BUNDLE* info, int valid, XFILE* stream)
{ if (!PlaylistWriter::Init(info, valid, stream))
    return false;
  // Header
  Write("#\n"
        "# Playlist created with " DESCRIPTION_STRING "\n"
        "# Do not modify!\n"
        "#\n");
  return OK;
}

bool LSTWriter::AppendItem(const Item& item)
{ DEBUGLOG(("plist123:LSTWriter(%p)::AppendItem({%s, %p, %x, %x})\n", this,
    item.Url.cdata(), item.Info, item.Valid, item.Override));
  register const INFO_BUNDLE& info = *item.Info;
  // alias
  if (info.item->alias)
  { Write("#ALIAS ");
    Write(info.item->alias);
    Write("\n");
  }
  // slice
  if (info.item->start)
  { Write("#START ");
    Write(info.item->start);
    Write("\n");
  }
  if (info.item->stop)
  { Write("#STOP ");
    Write(info.item->stop);
    Write("\n");
  }
  // location
  if (info.attr->at)
  { Write("#AT ");
    Write(info.attr->at);
    Write("\n");
  }
  // Total playing time
  PM123_TIME songlen = -2;
  if (item.Valid & INFO_TECH)
  { if (info.tech->attributes & TATTR_PLAYLIST)
    { if (item.Valid & INFO_OBJ)
        songlen = info.obj->songlength;
    } else
    { if (item.Valid & INFO_DRPL)
        songlen = info.drpl->totallength;
    }
  }
  // comment
  if (item.Valid & (INFO_PHYS|INFO_TECH|INFO_OBJ))
  { char buf[50];
    Write("# ");
    if (!(item.Valid & INFO_PHYS) || info.phys->filesize < 0)
      Write("n/a, ");
    else
    { sprintf(buf, "%.0lf kiB, ", info.phys->filesize/1024.);
      Write(buf);
    }
    if (songlen < 0)
      Write("n/a, ");
    else
    { unsigned long s = (unsigned long)songlen;
      if (s < 60)
        sprintf(buf, "%lu s, ", s);
      else if (s < 3600)
        sprintf(buf, "%lu:%02lu, ", s/60, s%60);
      else
        sprintf(buf, "%lu:%02lu:%02lu, ", s/3600, s/60%60, s%60);
      Write(buf);
    }
    if (!(item.Valid & INFO_OBJ) || info.obj->bitrate < 0)
      Write("n/a, ");
    else
    { sprintf(buf, "%i, ", info.obj->bitrate);
      Write(buf);
    }
    Write(info.tech->info);
    Write("\n");
  }
  // Object name
  { const char* cp = item.Url;
    if (strnicmp(cp,"file://", 7) == 0)
    { cp += 5;
      if (cp[2] == '/')
        cp += 3;
    }
    Write(cp);
    Write("\n");
  }
  // tech info
  if (item.Valid & (INFO_PHYS|INFO_TECH|INFO_OBJ))
  { char buf[128];
    // 0: bitrate, 1: samplingrate, 2: channels, 3: file size, 4: total length,
    // 5: no. of song items, 6: total file size, 7: no. of items, 8: recursive, 9: no. of list items.
    strcpy(buf, ">,");
    if (item.Valid & INFO_OBJ)
      sprintf(buf + 1, "%i,", info.obj->bitrate);
    if (item.Valid & INFO_TECH)
      sprintf(buf + strlen(buf), "%i,%i,", info.tech->samplerate, info.tech->channels);
    else
      strcat(buf + 2, ",,");
    if (item.Valid & INFO_PHYS)
      sprintf(buf + strlen(buf), "%.0f,", info.phys->filesize);
    else
      strcat(buf + 4, ",");
    if (songlen > -2)
    { sprintf(buf + strlen(buf), "%.3f", songlen);
      if ((info.tech->attributes & TATTR_PLAYLIST) && info.drpl->unk_length > 0)
        sprintf(buf + strlen(buf), "-%i", info.drpl->unk_length);
    }
    Write(buf);
    // Playlists only...
    if ((item.Valid & INFO_TECH) && (info.tech->attributes & TATTR_PLAYLIST))
    { strcpy(buf, ",");
      if (item.Valid && INFO_RPL)
      { sprintf(buf + 1, ",%i", info.rpl->totalsongs);
        if (info.rpl->unk_songs > 0)
          sprintf(buf + strlen(buf), "-%i", info.rpl->unk_songs);
      } else
        strcpy(buf + 1, ",");
      if (item.Valid && INFO_DRPL)
      { sprintf(buf + strlen(buf), ",%.0f", info.drpl->totalsize);
        if (info.drpl->unk_size > 0)
          sprintf(buf + strlen(buf), "-%i", info.drpl->unk_size);
      } else
        strcat(buf + 2, ",");
      if (item.Valid & INFO_OBJ)
        sprintf(buf + strlen(buf), ",%i", info.obj->num_items);
      else
        strcat(buf + 3, ",");
      if (item.Valid & INFO_RPL)
      { sprintf(buf + strlen(buf), ",%i", info.rpl->recursive);
        if (info.rpl->unk_recurs > 0)
          sprintf(buf + strlen(buf), "-%i", info.rpl->unk_recurs);
        sprintf(buf + strlen(buf), ",%i", info.rpl->totallists);
        if (info.rpl->unk_lists > 0)
          sprintf(buf + strlen(buf), "-%i", info.rpl->unk_lists);
      } else
        strcat(buf + 4, ",,");
      Write(buf);
    }
    Write("\n");
  }
  return OK;
}

bool LSTWriter::Finish()
{ // Footer
  Write("# End of playlist\n");
  return PlaylistWriter::Finish();
}


/****************************************************************************
*
*  class M3UWriter
*
****************************************************************************/

bool M3UWriter::Init(const INFO_BUNDLE* info, int valid, XFILE* stream)
{ if (!PlaylistWriter::Init(info, valid, stream))
    return false;
  // Header
  Write("#EXTM3U\n");
  return OK;
}

bool M3UWriter::AppendItem(const Item& item)
{ DEBUGLOG(("plist123:M3UWriter(%p)::AppendItem({%s...})\n", this, item.Url.cdata()));
  // EXTINF
  register const INFO_BUNDLE& info = *item.Info;
  char buf[20];
  if ((item.Valid & (INFO_OBJ|INFO_META)) || ((item.Override & INFO_ITEM) && info.item->alias))
  { if (item.Valid & INFO_OBJ)
    { sprintf(buf, "#EXTINF:%.0f,", info.obj->songlength);
      Write(buf);
    } else
      Write("#EXTINF:,");
    if ((item.Override & INFO_ITEM) && info.item->alias)
      Write(info.item->alias);
    else
    { // Default to Artist - Title
      if (info.meta->artist)
      { Write(info.meta->artist);
        Write(" - ");
      }
      Write(info.meta->title);
    }
    Write("\n");
  }
  // Object name
  { const char* cp = item.Url;
    if (strnicmp(cp,"file://", 7) == 0)
    { cp += 5;
      if (cp[2] == '/')
        cp += 3;
    }
    Write(cp);
    Write("\n");
  }
  return OK;
}

/****************************************************************************
*
*  class M3U8Writer
*
****************************************************************************/

bool M3U8Writer::Init(const INFO_BUNDLE* info, int valid, XFILE* stream)
{ if (!PlaylistWriter::Init(info, valid, stream))
    return false;
  // Header
  // TODO: BOM optional
  Write("\xef\xbb\xbf#EXTM3U\n");
  return OK;
}

void M3U8Writer::Write(const char* data)
{ // convert to UTF-8
  if (ch_convert(0, data, 1208, EncodedLine, sizeof EncodedLine, 0) == NULL)
    OK = false;
  else
    M3UWriter::Write(EncodedLine);
}

