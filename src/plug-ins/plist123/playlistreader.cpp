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
#include <cpp/container/stringmap.h>
#include <cpp/xstring.h>
#include <charset.h>
#include <strutils.h>
#include <xio.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>


/****************************************************************************
*
*  Internal worker classes (PIMPL)
*
****************************************************************************/

/** Read playlists of formats that can be written too. */
class WritablePlaylistReader : public PlaylistReader
{protected:
  WritablePlaylistReader(const char* url, XFILE* source) : PlaylistReader(url, source) {}
 public:
  virtual bool              Parse(const INFO_BUNDLE* info, DECODER_INFO_ENUMERATION_CB cb, void* param);
};

/** Read PM123 native playlist files */
class LSTReader : public WritablePlaylistReader
{public:
  static const xstringconst Format;
 private:
  LSTReader(const char* url, XFILE* source) : WritablePlaylistReader(url, source) {}
 protected:
  virtual bool              ParseLine(char* line);
 public:
  static LSTReader*         Sniffer(const char* url, XFILE* source);
  virtual const xstring&    GetFormat() const;
};

/** Read WarpVision playlist files. */
class PLSReader : public PlaylistReader
{public:
  static const xstringconst Format;
 private:
  PLSReader(const char* url, XFILE* source) : PlaylistReader(url, source) {}
 protected:
  virtual bool              ParseLine(char* line);
 public:
  static PLSReader*         Sniffer(const char* url, XFILE* source);
  virtual const xstring&    GetFormat() const;
};

/** Read WinAmp playlist files. */
class M3UReader : public WritablePlaylistReader
{public:
  static const xstringconst Format;
 protected:
  M3UReader(const char* url, XFILE* source) : WritablePlaylistReader(url, source) {}
 protected:
  virtual bool              ParseLine(char* line);
 public:
  static M3UReader*         Sniffer(const char* url, XFILE* source);
  virtual const xstring&    GetFormat() const;
};

/** Read WinAmp Unicode playlist files. */
class M3U8Reader : public M3UReader
{public:
  static const xstringconst Format;
 private:
  static const char         Hdr[];
  char                      DecodedLine[4096];
 private:
  M3U8Reader(const char* url, XFILE* source)              : M3UReader(url, source) {}
 public:
  static M3U8Reader*        Sniffer(const char* url, XFILE* source);
  virtual const xstring&    GetFormat() const;
 protected:
  virtual bool              ParseLine(char* line);
};

/** Read CUE sheets. */
class CUEReader : public PlaylistReader
{public:
  static const xstringconst Format;
 private:
  enum TokenFlags
  { TF_None,
    TF_Sniffer ///< This token is allowed at the start of the file.
  };
  struct TokenData
  { bool       (CUEReader::*Parser)(char* args);
    TokenFlags              Flags;
  };
  typedef strmap<12,TokenData> MapEntry;
 private:
  static const MapEntry     TokenMap[];
  PlaylistReaderInfo*       CurrentTrack;
  META_INFO*                CurrentMeta;
  double                    CurrentPos;
  xstring                   FileFormat;
  bool                      HaveIndex;
  PlaylistReaderInfo*       LastTrack;
  vector_own<PlaylistReaderInfo> Tracks;
 private:
  CUEReader(const char* url, XFILE* source);
  static void               GetString(xstring& dst, char*& src);
  static void               Transcode(xstring& value);
  static bool               ParseMSF(double& dst, const char* src);
  bool                      File(char* args);
  bool                      Index(char* args);
  bool                      Performer(char* args);
  bool                      Postgap(char* args);
  bool                      Pregap(char* args);
  bool                      Rem(char* args);
  bool                      Title(char* args);
  bool                      Track(char* args);
 public:
  static CUEReader*         Sniffer(const char* url, XFILE* source);
  virtual const xstring&    GetFormat() const;
 protected:
  virtual bool              ParseLine(char* line);
  virtual bool              Parse(const INFO_BUNDLE* info, DECODER_INFO_ENUMERATION_CB cb, void* param);
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
{}

bool PlaylistReader::Parse(const INFO_BUNDLE* info, DECODER_INFO_ENUMERATION_CB cb, void* param)
{ Info = info;
  Cb = cb;
  CbParam = param;
  Reset();

  info->tech->attributes |= TATTR_PLAYLIST;
  info->tech->format = GetFormat();
  info->obj->num_items = 0;

  char line[4096];
  while (*line = 0, xio_fgets(line, sizeof line, Source))
  { strchomp(line);
    ParseLine(line);
  }
  // Create last item if any
  Create(*this);
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
  Meta.title.reset();
  Meta.artist.reset();
  Meta.album.reset();
  Meta.year.reset();
  Meta.comment.reset();
  Meta.genre.reset();
  Meta.track.reset();
  Meta.copyright.reset();
  Meta.track_gain = -1000;
  Meta.track_peak = -1000;
  Meta.album_gain = -1000;
  Meta.album_peak = -1000;
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
  Reliable = INFO_NONE;
  Override = INFO_NONE;
}

void PlaylistReader::Create(PlaylistReaderInfo& info)
{ DEBUGLOG(("plist123:PlaylistReader(%p)::Create(&%p{%s ... %x,%x,%x})\n",
    this, &info, info.Url.cdata(), info.Cached, info.Reliable, info.Override));
  INFO_BUNDLE infobundle = { &info.Phys, &info.Tech, &info.Obj, &info.Meta, &info.Attr, &info.Rpl, &info.Drpl, &info.Item };
  if (info.Url)
  { (*Cb)(CbParam, info.Url, &infobundle, info.Override|(info.Cached&~info.Reliable), info.Override|info.Reliable);
    ++Info->obj->num_items;
  }
}

PlaylistReader* PlaylistReader::SnifferFactory(const char* url, XFILE* source)
{ // The following implementation uses the implicit property of the XIO library,
  // that you can seek back on transient data streams also as long as
  // it does not exceed the buffer.
  PlaylistReader* ret = M3U8Reader::Sniffer(url, source);
  if (ret)
    goto ok;
  xio_rewind(source);
  ret = M3UReader::Sniffer(url, source);
  if (ret)
    goto ok;
  xio_rewind(source);
  ret = PLSReader::Sniffer(url, source);
  if (ret)
    goto ok;
  xio_rewind(source);
  ret = LSTReader::Sniffer(url, source);
  if (ret)
    goto ok;
  xio_rewind(source);
  ret = CUEReader::Sniffer(url, source);
 ok:
  DEBUGLOG(("PlaylistReader::SnifferFactory: %p\n", ret));
  return ret;
}


bool WritablePlaylistReader::Parse(const INFO_BUNDLE* info, DECODER_INFO_ENUMERATION_CB cb, void* param)
{ bool ret = PlaylistReader::Parse(info, cb, param);
  info->tech->attributes |= TATTR_WRITABLE;
  return ret;
}


/* class LSTReader */

const xstringconst LSTReader::Format(EATYPE_PM123);

const xstring& LSTReader::GetFormat() const
{ return Format;
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
    { Create(*this);
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
    } else if (memcmp(line+1, "AT ", 3) == 0) // at
    { Attr.at = line+6;
      Override |= INFO_ATTR;
    } else if (memcmp(line+1, "OPT ", 4) == 0) // playlist options
    { if (strstr(line+5, "alt"))
        Attr.ploptions |= PLO_ALTERNATION;
      if (strstr(line+5, "noshuffle"))
        Attr.ploptions |= PLO_NO_SHUFFLE;
      else if (strstr(line+5, "shuffle"))
        Attr.ploptions |= PLO_SHUFFLE;
      Override |= INFO_ATTR;
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
    { Create(*this);
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
      ParseDbl(Obj.songlength, tokens[4]);

      ParseInt(Rpl.songs, tokens[5]);
      ParseDbl(Drpl.totalsize, tokens[6]);

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
    Create(*this);
    Reset();
    break;
  }
  return true;
}


/* class PLSReader */

const xstringconst PLSReader::Format(EATYPE_WVISION);

const xstring& PLSReader::GetFormat() const
{ return Format;
}

PLSReader* PLSReader::Sniffer(const char* url, XFILE* source)
{ DEBUGLOG(("PLSReader::Sniffer(%s, %p)\n", url, source));
  char buffer[50];
  if (xio_fread(buffer, 1, sizeof buffer -1, source) < 11)
    return NULL;
  buffer[sizeof buffer -1] = 0; // ensure termination
  char* cp = buffer;
  cp += strspn(buffer, " \t\r\n");
  if (strnicmp(cp, "[playlist]", 10) != 0)
    return NULL;
  // rewind after header
  xio_fseek(source, cp + 10 - buffer, XIO_SEEK_SET);
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
  { Create(*this);
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

const xstringconst M3UReader::Format(EATYPE_WINAMP);

const xstring& M3UReader::GetFormat() const
{ return Format;
}

M3UReader* M3UReader::Sniffer(const char* url, XFILE* source)
{ DEBUGLOG(("M3UReader::Sniffer(%s, %p)\n", url, source));
  char buffer[10];
  xio_fgets(buffer, sizeof buffer, source);
  if (xio_ftell(source) < 7)
    return NULL;
  if (memcmp(buffer, "#EXTM3U", 7) != 0)
  { // Accept non extended M3U also in case of the M3U extension.
    char ext[6];
    sfext(ext, url, sizeof ext);
    if (stricmp(ext, ".m3u") != 0)
      return NULL;
    xio_rewind(source);
  }
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
    Create(*this);
    Reset();
  }
  return true;
}

/* class M3U8Reader */

const xstringconst M3U8Reader::Format(EATYPE_WINAMPU);

const xstring& M3U8Reader::GetFormat() const
{ return Format;
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


/* class CUEReader */

const xstringconst CUEReader::Format(EATYPE_CUESHEET);

const xstring& CUEReader::GetFormat() const
{ return Format;
}

const CUEReader::MapEntry CUEReader::TokenMap[] =
{ { "CATALOG"   , { NULL,                   TF_Sniffer } }
, { "CDTEXTFILE", { NULL,                   TF_Sniffer } }
, { "FILE",       { &CUEReader::File,       TF_Sniffer } }
, { "FLAGS",      { NULL,                   TF_None    } }
, { "INDEX",      { &CUEReader::Index,      TF_None    } }
, { "ISRC",       { NULL,                   TF_Sniffer } }
, { "PERFORMER",  { &CUEReader::Performer,  TF_Sniffer } }
, { "POSTGAP",    { &CUEReader::Postgap,    TF_None    } }
, { "PREGAP",     { &CUEReader::Pregap,     TF_None    } }
, { "REM",        { &CUEReader::Rem,        TF_Sniffer } }
, { "SONGWRITER", { NULL,                   TF_Sniffer } }
, { "TITLE",      { &CUEReader::Title,      TF_Sniffer } }
, { "TRACK",      { &CUEReader::Track,      TF_Sniffer } }
};

CUEReader* CUEReader::Sniffer(const char* url, XFILE* source)
{ DEBUGLOG(("CUEReader::Sniffer(%s, %p)\n", url, source));
  char buffer[12];
  xio_fgets(buffer, sizeof buffer, source);
  const MapEntry* mp = mapsearcha(TokenMap, buffer);
  if (mp == NULL || (mp->Val.Flags & TF_Sniffer) == 0 || !isspace(buffer[strlen(mp->Str)]))
    return NULL;
  xio_rewind(source);
  return new CUEReader(url, source);
}

CUEReader::CUEReader(const char* url, XFILE* source)
: PlaylistReader(url, source)
, Tracks(99)
{}

bool CUEReader::ParseLine(char* line)
{ DEBUGLOG(("CUEReader(%p)::ParseLine(\"%s\")\n", this, line));
  // strip line
  line = blank_strip(line);
  // Empty line?
  if (!*line)
    return true;
  // Identify command
  const MapEntry* mp = mapsearcha(TokenMap, line);
  if (mp == NULL)
    return false; // unknown command
  size_t len = strlen(mp->Str);
  if (!isspace(line[len]))
    return false; // unknown command with only valid prefix
  if (mp->Val.Parser == NULL)
    return true; // command does not care us
  return (this->*mp->Val.Parser)(line + len);
}

bool CUEReader::Parse(const INFO_BUNDLE* info, DECODER_INFO_ENUMERATION_CB cb, void* param)
{
  CurrentTrack = NULL;
  CurrentMeta = info->meta;
  CurrentPos = -1;
  LastTrack = NULL;

  bool ret = PlaylistReader::Parse(info, cb, param);

  foreach (PlaylistReaderInfo,*const*, ipp, Tracks)
  {
    PlaylistReaderInfo* ip = *ipp;
    // Ignore data tracks
    if (!ip || (ip->Tech.attributes & TATTR_SONG) == 0)
      continue;

    // Copy artist from album?
    if (!ip->Meta.artist && (ip->Meta.album = Info->meta->album) != NULL)
      ip->Override |= INFO_META;
    // Copy album gain from album
    if (Info->meta->album_gain > -1000 || Info->meta->album_peak > -1000)
    { ip->Meta.album_gain = Info->meta->album_gain;
      ip->Meta.album_peak = Info->meta->album_peak;
      ip->Override |= INFO_META;
    }
    Create(*ip);
  }
  return ret;
}

void CUEReader::GetString(xstring& dst, char*& src)
{
  src += strspn(src, " \r\n");
  dst.reset();
  size_t len;
  switch (*src)
  {case 0: // end of line
    return;

   case '"': // quoted string
    dst = xstring::empty;
    ++src;
   cont:
    len = strcspn(src, "\"");
    if (src[len])
    { if (src[len+1] == '"')
      { dst = dst + xstring(src, len+1);
        src += len+2;
        goto cont;
      }
      dst = dst + xstring(src, len);
      ++src;
    }
    break;

   default: // unquoted string
    len = strcspn(src, " \r\n");
    dst.assign(src, len);
  }
  src += len;
}

void CUEReader::Transcode(xstring& value)
{ xstring result;
  char* dst = result.allocate(value.length());
  if (ch_convert(1004, value, CH_CP_NONE, dst, result.length()+1, 0))
    value = result;
  // else ... well, fallback
}

bool CUEReader::ParseMSF(double& dst, const char* src)
{ unsigned m;
  unsigned s;
  unsigned f;
  size_t len;
  if (sscanf(src, "%u:%u:%u%n", &m, &s, &f, &len) != 3 || strlen(src) != len)
    return false;
  dst = m * 60 + s + f / 75.;
  return true;
}

bool CUEReader::File(char* args)
{ DEBUGLOG(("CUEReader(%p)::File(%s)\n", this, args));
  GetString(Url, args);
  GetString(FileFormat, args);
  CurrentPos = 0;
  LastTrack = NULL;
  return true;
}

bool CUEReader::Track(char* args)
{ DEBUGLOG(("CUEReader(%p)::Track(%s)\n", this, args));
  xstring arg;
  GetString(arg, args);
  int track = 0;
  ParseInt(track, arg);
  if (track <= 0 || track > 99)
  { Reset();
    return false;
  }
  LastTrack = CurrentTrack;

  if (Tracks.size() < (unsigned)track)
    Tracks.set_size(track);
  Tracks[track-1] = CurrentTrack = new PlaylistReaderInfo(*this);
  if (!CurrentTrack->Url && LastTrack)
    CurrentTrack->Url = LastTrack->Url;

  GetString(arg, args);
  if (arg.compareToI("AUDIO") == 0)
  { CurrentTrack->Tech.samplerate = 44100;
    CurrentTrack->Tech.channels = 2;
    CurrentTrack->Tech.attributes = TATTR_SONG;
  }
  CurrentMeta = &CurrentTrack->Meta;
  CurrentMeta->track.sprintf("%02i", track);
  CurrentMeta->artist = Info->meta->artist;
  CurrentMeta->album = Info->meta->title;
  HaveIndex = false;
  Reset();
  return true;
}

static const xstringconst InfoLE("16 bit stereo PCM audio, little endian");
static const xstringconst InfoBE("16 bit stereo PCM audio, big endian");

bool CUEReader::Index(char* args)
{ DEBUGLOG(("CUEReader(%p)::Index(%s)\n", this, args));
  if (!CurrentTrack || !FileFormat)
    return false;
  if (HaveIndex || !(CurrentTrack->Tech.attributes & TATTR_SONG)) // Ignore sub indices and data tracks
    return true;

  xstring arg;
  GetString(arg, args);
  GetString(arg, args);
  ParseMSF(CurrentPos, arg);
  if (CurrentPos < 0)
    return false;
  HaveIndex = true;

  if (CurrentPos > 0)
  { unsigned sec10 = (unsigned)floor(CurrentPos) / 10;
    CurrentTrack->Item.start.sprintf("%u:%u%f", sec10/6, sec10%6, CurrentPos-sec10*10);
    CurrentTrack->Override |= INFO_ITEM;
  }

  // store some known infos
  if (FileFormat.compareToI("BINARY") == 0)
  { CurrentTrack->Tech.format = "RAW 16 le";
    CurrentTrack->Tech.info = InfoLE;
    goto bin2;
  } else if (FileFormat.compareToI("MOTOROLA") == 0)
  { CurrentTrack->Tech.format = "RAW 16 be";
    CurrentTrack->Tech.info = InfoBE;
   bin2:
    CurrentTrack->Tech.decoder = "wavplay.dll";
    CurrentTrack->Obj.bitrate = 1411200;
    CurrentTrack->Reliable |= INFO_TECH|INFO_META|INFO_ATTR;
  }

  // terminate the last track at CurrentPos
  if (LastTrack)
  { LastTrack->Item.stop = CurrentTrack->Item.start;
    LastTrack->Override |= INFO_ITEM;
    LastTrack = NULL;
  }
  return true;
}

bool CUEReader::Title(char* args)
{ DEBUGLOG(("CUEReader(%p)::Title(%s)\n", this, args));
  GetString(CurrentMeta->title, args);
  Transcode(CurrentMeta->title);
  if (CurrentTrack)
    CurrentTrack->Override |= INFO_META;
  return true;
}

bool CUEReader::Performer(char* args)
{ DEBUGLOG(("CUEReader(%p)::Performer(%s)\n", this, args));
  GetString(CurrentMeta->artist, args);
  Transcode(CurrentMeta->artist);
  if (CurrentTrack)
    CurrentTrack->Override |= INFO_META;
  return true;
}

bool CUEReader::Postgap(char* args)
{ DEBUGLOG(("CUEReader(%p)::Postgap(%s)\n", this, args));
  if (!CurrentTrack)
    return false;
  xstring arg;
  GetString(arg, args);
  double value;
  if (ParseMSF(value, arg))
  { CurrentTrack->Item.postgap = value;
    CurrentTrack->Override |= INFO_ITEM;
  }
  return true;
}

bool CUEReader::Pregap(char* args)
{ DEBUGLOG(("CUEReader(%p)::Pregap(%s)\n", this, args));
  if (!CurrentTrack)
    return false;
  xstring arg;
  GetString(arg, args);
  double value;
  if (ParseMSF(value, arg))
  { CurrentTrack->Item.pregap = value;
    CurrentTrack->Override |= INFO_ITEM;
  }
  return true;
}

bool CUEReader::Rem(char* args)
{ DEBUGLOG(("CUEReader(%p)::Rem(%s)\n", this, args));
  // Check for replay gain comments
  xstring arg;
  GetString(arg, args);
  if (!arg)
    return true;
  static const char map[][22] =
  { "REPLAYGAIN_ALBUM_GAIN"
  , "REPLAYGAIN_ALBUM_PEAK"
  , "REPLAYGAIN_TRACK_GAIN"
  , "REPLAYGAIN_TRACK_PEAK"
  };
  const char (*rp)[22] = (const char (*)[22])bsearch(arg.cdata(), map, countof(map), sizeof *map, (int (TFNENTRY*)(const void*, const void*))&stricmp);
  if (rp == NULL)
    return true;
  double value = -1000;
  GetString(arg, args);
  ParseDbl(value, arg);
  GetString(arg, args);
  if (!arg || arg.compareToI("dB") != 0)
    value = log10(value) * 20;
  switch (rp - map)
  {case 0: // album gain
    Info->meta->album_gain = value;
    break;
   case 1: // album peak
    Info->meta->album_peak = value;
    break;
   case 2: // track gain
    if (CurrentTrack)
    { CurrentMeta->track_gain = value;
      CurrentTrack->Override |= INFO_META;
    }
    break;
   case 3:
    if (CurrentTrack)
    { CurrentMeta->track_peak = value;
      CurrentTrack->Override |= INFO_META;
    }
  }
  return true;
}
