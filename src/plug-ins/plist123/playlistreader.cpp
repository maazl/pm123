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


#include "playlistreader.h"
#include "plist123.h"
#include <fileutil.h>
#include <charset.h>
#include <xio.h>
#include <stdio.h>
#include <ctype.h>


/****************************************************************************
*
*  Internal worker classes (PIMPL)
*
****************************************************************************/

class LSTReader : public PlaylistReader
{public:
  LSTReader(const char* url, XFILE* source) : PlaylistReader(url, source) {}
  static LSTReader*         Sniffer(const char* url, XFILE* source);
  virtual const xstring&    GetFormat() const;
 protected:
  virtual bool              ParseLine(char* line);
};

class PLSReader : public PlaylistReader
{public:
  PLSReader(const char* url, XFILE* source) : PlaylistReader(url, source) {}
  static PLSReader*         Sniffer(const char* url, XFILE* source);
  virtual const xstring&    GetFormat() const;
 protected:
  virtual bool              ParseLine(char* line);
};

class M3UReader : public PlaylistReader
{public:
  M3UReader(const char* url, XFILE* source) : PlaylistReader(url, source) {}
  static M3UReader*         Sniffer(const char* url, XFILE* source);
  virtual const xstring&    GetFormat() const;
 protected:
  virtual bool              ParseLine(char* line);
};

class M3U8Reader : public M3UReader
{private:
  static const char         Hdr[];
  char                      DecodedLine[4096];
 public:
  M3U8Reader(const char* url, XFILE* source)              : M3UReader(url, source) {}
  static M3U8Reader*        Sniffer(const char* url, XFILE* source);
  virtual const xstring&    GetFormat() const;
 protected:
  virtual bool              ParseLine(char* line);
};

/****************************************************************************
*
*  class PlaylistReader
*
****************************************************************************/

PlaylistReader::PlaylistReader(const char* url, XFILE* source)
: URL(url),
  Source(source),
  Cb(NULL),
  CbParam(NULL)
{ Info.phys = &Phys;
  Info.tech = &Tech;
  Info.obj  = &Obj;
  Info.attr = &Attr;
  Info.meta = NULL;
  Info.rpl  = &Rpl;
  Info.drpl = &Drpl;
  Info.item = &Item;
}

bool PlaylistReader::Parse(DECODER_INFO_ENUMERATION_CB cb, void* param)
{ Cb = cb;
  CbParam = param;
  Count = 0;
  Reset();

  char line[4096];
  while (*line = 0, xio_fgets(line, sizeof line, Source))
  { strchomp(line);
    ParseLine(line);
  }
  // Create last item if any
  Create();
  if (xio_ferror(Source) != 0)
    return false;
  return true;
}

void PlaylistReader::Reset()
{ DEBUGLOG(("plist123:PlaylistReader::Reset()\n"));
  Url.reset();
  Phys.filesize = -1;
  Phys.tstmp = -1;
  Phys.attributes = PATTR_NONE;
  Tech.samplerate = -1;
  Tech.channels = -1;
  Tech.attributes = TATTR_NONE;
  Tech.info.reset();
  Tech.format.reset();
  Tech.decoder.reset();
  Obj.songlength = -1;
  Obj.bitrate = -1;
  Obj.num_items = -1;
  Attr.ploptions = PLO_NONE;
  Attr.at.reset();
  Rpl.songs = -1;
  Rpl.lists = -1;
  Rpl.invalid = -1;
  Rpl.unknown = -1;
  Drpl.totallength = -1;
  Drpl.unk_length = -1;
  Drpl.totalsize = -1;
  Drpl.unk_size = -1;
  Item.alias.reset();
  Item.start.reset();
  Item.stop.reset();
  Item.pregap = -1;
  Item.postgap = -1;
  Item.gain = -1000;
  Cached = INFO_NONE;
  Override = INFO_NONE;
}

void PlaylistReader::Create()
{ DEBUGLOG(("plist123:PlaylistReader::Create() - %s, %x/%x\n", Url.cdata(), Cached, Override));
  if (Url)
  { (*Cb)(CbParam, Url, &Info, Cached, Override|Cached);
    ++Count;
  }
}

PlaylistReader* PlaylistReader::SnifferFactory(const char* url, XFILE* source)
{ PlaylistReader* ret = M3U8Reader::Sniffer(url, source);
  if (ret)
    goto ok;
  // The following instruction uses the implicit property of the XIO library,
  // that you can seek back on transient data streams also as long as
  // it does not exceed the buffer.
  xio_rewind(source);
  ret = M3UReader::Sniffer(url, source);
  if (ret)
    goto ok;
  // The following instruction uses the implicit property of the XIO library,
  // that you can seek back on transient data streams also as long as
  // it does not exceed the buffer.
  xio_rewind(source);
  ret = PLSReader::Sniffer(url, source);
  if (ret)
    goto ok;
  // The following instruction uses the implicit property of the XIO library,
  // that you can seek back on transient data streams also as long as
  // it does not exceed the buffer.
  xio_rewind(source);
  ret = LSTReader::Sniffer(url, source);
 ok:
  DEBUGLOG(("PlaylistReader::SnifferFactory: %p\n", ret));
  return ret;
}


/* class LSTReader */

const xstring& LSTReader::GetFormat() const
{ static const xstring format(EATYPE_PM123);
  return format;
}

LSTReader* LSTReader::Sniffer(const char* url, XFILE* source)
{ DEBUGLOG(("LSTReader::Sniffer(%s, %p)\n", url, source));
  char buffer[4];
  xio_fgets(buffer, sizeof buffer, source);
  if (xio_ftell(source) < 2)
    return NULL;
  if (buffer[0] != '#' || (buffer[1] != '\r' && buffer[1] != '\n'))
    return NULL;
  return new LSTReader(url, source);
}

bool LSTReader::ParseLine(char* line)
{ DEBUGLOG(("plist123:LSTReader::ParseLine(%s)\n", line));
  switch (line[0])
  {case '#':
    // comments close URL
    if (Url)
    { Create();
      Reset();
    }
    // prefix lines
    if (memcmp(line+1, "ALIAS ", 6) == 0) // alias name
    { Item.alias = line+7;
      Override |= INFO_ITEM;
    } else if (memcmp(line+1, "START ", 6) == 0) // slice
    { Item.start = line+7;
      Override |= INFO_ITEM;
    } else if (memcmp(line+1, "STOP ", 5) == 0) // slice
    { Item.stop = line+6;
      Override |= INFO_ITEM;
    } else if (memcmp(line+1, "GAP ", 4) == 0) // gaps
    { if (line[5] != ',') // no pregap
        sscanf(line+5, "%f", &Item.pregap);
      char* cp = strchr(line+5, ',');
      if (cp)
        sscanf(cp+1, "%f", &Item.postgap);
      Override |= INFO_ITEM;
    } else if (memcmp(line+1, "GAIN ", 5) == 0) // gain
    { sscanf(line+6, "%f", &Item.gain);
      Override |= INFO_ITEM;
    } else if (line[1] == ' ')
    { const char* cp = strstr(line+2, ", ");
      if (cp)
      { cp = strstr(cp+2, ", ");
        if (cp)
          Tech.info = cp+2;
      }
    }
    break;

   case 0: // Skip empty lines
    break;

   default:
    // close last URL
    if (Url)
    { Create();
      Reset();
    }
    Url = line;
    break;

   case '>':
    { // tokenize string
      const char* tokens[10] = {NULL};
      const char** tp = tokens;
      *tp = strtok(line+1, ",");
      while (*tp)
      { if (**tp == 0)
          *tp = NULL;
        *++tp = strtok(NULL, ",");
        if (tp == tokens + sizeof tokens/sizeof *tokens)
          break;
      }
      // 0: bitrate, 1: samplingrate, 2: channels, 3: file size, 4: total length,
      // 5: no. of song items, 6: total file size, 7: no. of items, 8: recursive, 9: list items. 
      DEBUGLOG(("Playlist::LSTReader::ParseLine: tokens %s, %s, %s, %s, %s, %s, %s, %s, %s\n",
        tokens[0], tokens[1], tokens[2], tokens[3], tokens[4], tokens[5], tokens[6], tokens[7], tokens[8]));
      // Data type conversion
      
      ParseInt(Obj.bitrate, tokens[0]);
      ParseFloat(Obj.songlength, tokens[4]);

      ParseInt(Rpl.songs, tokens[5]);
      ParseFloat(Drpl.totalsize, tokens[6]);

      /* No phys info so far because we do not have the writable flag 
      if (tokens[3])
      { Info.phys = &Phys;
        // memset(Phys, 0, sizeof Tech);
        Phys.size       = sizeof Phys;
        Phys.filesize   = atof(tokens[3]);
        Phys.num_items  = tokens[7] ? atoi(tokens[7]) : 1;
      }*/
      ParseInt(Tech.samplerate, tokens[1]);
      ParseInt(Tech.channels, tokens[2]);
      if (strchr(tokens[2], '.'))
      { switch (Tech.channels) // convert stupid modes of old PM123 versions
        {case 0:
          Tech.channels = 2;
          break;
         case 3:
          Tech.channels = 1;
          break;
        }
      }
      break;
    }

   case '<':
    // close URL
    Create();
    Reset();
    break;
  }
  return true;
}


/* class PLSReader */

const xstring& PLSReader::GetFormat() const
{ static const xstring format(EATYPE_WVISION);
  return format;
}

PLSReader* PLSReader::Sniffer(const char* url, XFILE* source)
{ DEBUGLOG(("PLSReader::Sniffer(%s, %p)\n", url, source));
  char buffer[14];
  xio_fgets(buffer, sizeof buffer, source);
  if (xio_ftell(source) < 11)
    return NULL;
  if (strnicmp(buffer, "[playlist]", 10) != 0)
    return NULL;
  return new PLSReader(url, source);
}

bool PLSReader::ParseLine(char* line)
{ DEBUGLOG(("plist123:PLSReader::ParseLine(%s)\n", line));
  char c = toupper(line[0]);
  if (c < 'A' && c > 'Z')
    return true; 

  const char* cp = strchr(line, '=');
  if (cp == NULL)
    return false;

  if (strnicmp(line, "File", 4) == 0)
  { Create();
    Reset();
    Url = cp+1;
  } else if (strnicmp(line, "Title", 5) == 0)
  { Item.alias = cp+1;
    Override |= INFO_ITEM;
  }
  // TODO: more info

  return true;
}

/* class M3UReader */

const xstring& M3UReader::GetFormat() const
{ static const xstring format(EATYPE_WINAMP);
  return format;
}

M3UReader* M3UReader::Sniffer(const char* url, XFILE* source)
{ DEBUGLOG(("M3UReader::Sniffer(%s, %p)\n", url, source));
  char buffer[10];
  xio_fgets(buffer, sizeof buffer, source);
  if (xio_ftell(source) < 7)
    return NULL;
  if (memcmp(buffer, "#EXTM3U", 7) != 0)
    return NULL;
  return new M3UReader(url, source);
}

bool M3UReader::ParseLine(char* line)
{ DEBUGLOG(("plist123:M3UReader::ParseLine(%s)\n", line));

  switch (line[0])
  {case 0:
   case '\r':
   case '\n':
    break;
   case '#':
    if (memcmp(line+1, "EXTINF:", 7) == 0)
    { line += 8;
      sscanf(line, "%lf", &Obj.songlength);
      const char* cp = strchr(line, ',');
      if (cp)
      { Item.alias = cp+1;
        Override |= INFO_ITEM;
      }
    }
    break;
   default:
    Url = line;
    Create();
    Reset();
  }
  return true;
}

/* class M3U8Reader */

const xstring& M3U8Reader::GetFormat() const
{ static const xstring format(EATYPE_WINAMPU);
  return format;
}

const char M3U8Reader::Hdr[] = "\xef\xbb\xbf#EXTM3U";

M3U8Reader* M3U8Reader::Sniffer(const char* url, XFILE* source)
{ DEBUGLOG(("M3U8Reader::Sniffer(%s, %p)\n", url, source));
  char buffer[14];
  xio_fgets(buffer, sizeof buffer, source);
  if (xio_ftell(source) < 8)
    return NULL;
  // With BOM?
  if (memcmp(buffer, Hdr, 10) != 0)
  { char ext[6];
    sfext(ext, url, sizeof ext);
    if (stricmp(ext, ".m3u8") != 0 || memcmp(buffer, Hdr+3, 7) != 0)
      return NULL;
  }
  return new M3U8Reader(url, source);
}

bool M3U8Reader::ParseLine(char* line)
{ DEBUGLOG(("plist123:M3U8Reader::ParseLine(%s)\n", line));
  // convert to system codepage
  if (ch_convert(1208, line, 0, DecodedLine, sizeof DecodedLine, 0) == NULL)
    return false;
  return M3UReader::ParseLine(DecodedLine);
}

