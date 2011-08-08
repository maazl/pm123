/*
 * Copyright 2008-2011 Marcel Mueller
 * Copyright 2007 Dmitry A.Steklenev <glass@ptv.ru>
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

#define  INCL_PM
#define  INCL_DOS
#define  INCL_ERRORS

#include "mpg123.h"
#include "dialog.h"

#include <charset.h>
#include <decoder_plug.h>
#include <debuglog.h>
#include <snprintf.h>
#include <eautils.h>
#include <debuglog.h>
#include <os2.h>

#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <process.h>
#include <errno.h>
#include <ctype.h>
#include <fileutil.h>

PLUGIN_CONTEXT Ctx = {0};

#define load_prf_value(var) \
  Ctx.plugin_api->profile_query(#var, &var, sizeof var)

#define save_prf_value(var) \
  Ctx.plugin_api->profile_write(#var, &var, sizeof var)


Config cfg;

const Config Config::def =
{ TAG_READ_ID3V2_AND_ID3V1
, 1004 // ISO8859-1
, TRUE
, TAG_SAVE_ID3V1_WRITE
, TAG_SAVE_ID3V2_ONDEMANDSPC
, 1004
, ID3V2_ENCODING_UTF8
, 512
};

void Config::load()
{ reset();
  load_prf_value( tag_read_type           );
  load_prf_value( tag_id3v1_charset       );
  load_prf_value( tag_read_id3v1_autoch   );
  load_prf_value( tag_save_id3v1_type     );
  load_prf_value( tag_save_id3v2_type     );
  load_prf_value( tag_read_id3v2_charset  );
  load_prf_value( tag_save_id3v2_encoding );
}

void Config::save() const
{ save_prf_value( tag_read_type           );
  save_prf_value( tag_id3v1_charset       );
  save_prf_value( tag_read_id3v1_autoch   );
  save_prf_value( tag_save_id3v1_type     );
  save_prf_value( tag_save_id3v2_type     );
  save_prf_value( tag_read_id3v2_charset  );
  save_prf_value( tag_save_id3v2_encoding );
}


/***************************************************************************************
 *
 *  class ID3
 *
 **************************************************************************************/

PLUGIN_RC ID3::Open(const char* mode)
{ ASSERT(!XFile);
  XFile = xio_fopen(Filename, mode);
  if (XFile == NULL)
  { LastError.sprintf("Unable open file %s:\n%s", Filename.cdata(), xio_strerror(errno));
    Ctx.plugin_api->error_display(LastError);
    return PLUGIN_NO_READ;
  }

 return PLUGIN_OK;
}

void ID3::Close()
{ if (XFile)
  { xio_fclose(XFile);
    XFile = NULL;
  }
}

void ID3::ReadTags(ID3V1_TAG& tagv1, ID3V2_TAG*& tagv2)
{ tagv2 = id3v2_get_tag(XFile, ID3V2_GET_NONE);
  tagv1.ReadTag(XFile);
}

PLUGIN_RC ID3::UpdateTags(ID3V1_TAG* tagv1, ID3V2_TAG* tagv2, DSTRING& savename)
{
  size_t len = Filename.length();
  char* savename2 = savename.allocate(len);
  memcpy(savename2, Filename, len);
  savename2[len-1] = '~';

  bool copy = false;
  if (tagv2)
  { if (tagv2 != DELETE_ID3V2)
    { int set_rc = id3v2_set_tag(XFile, tagv2, savename);
      if( set_rc == 1 ) {
        copy = true;
      } else if ( set_rc == -1 ) {
        LastError.sprintf("Failed to update ID3v2 tag: %s", xio_strerror(xio_errno()));
        return PLUGIN_ERROR;
      }
    } else
    { if (id3v2_wipe_tag(XFile, savename) == 0) {
        copy = true;
      } else {
        LastError.sprintf("Failed to remove ID3v2 tag: %s", xio_strerror(xio_errno()));
        return PLUGIN_ERROR;
      }
    }
  }

  XFILE* save;
  if (copy) {
    if ((save = xio_fopen(savename, "r+bU")) == NULL) {
      LastError.sprintf("Failed to open temporary file %s: %s", savename.cdata(), xio_strerror(xio_errno()));
      return PLUGIN_ERROR;
    }
  } else {
    save = XFile;
    savename.reset();
  }

  if (tagv1)
  { if (tagv1 != DELETE_ID3V1)
    { tagv1->MakeValid();
      if (tagv1->WriteTag(save) != 0)
        LastError.sprintf("Failed to write ID3v1 tag: %s", xio_strerror(xio_errno()));
    } else
    { if (ID3V1_TAG::WipeTag(save) != 0)
        LastError.sprintf("Failed to remove ID3v1 tag: %s", xio_strerror(xio_errno()));
    }
  }

  if (copy)
    xio_fclose(save);

  return LastError ? PLUGIN_ERROR : PLUGIN_OK;
}


/***************************************************************************************
 *
 *  class MPG123
 *
 **************************************************************************************/

vector<MPG123> MPG123::Instances(5);
Mutex MPG123::InstMutex;

MPG123::MPG123()
: MPEG(NULL)
, XSave(NULL)
, LastLength(-1)
, LastSize(-1)
{}
MPG123::MPG123(const DSTRING& filename)
: ID3(filename)
, MPEG(NULL)
, XSave(NULL)
, FirstFrameLen(0)
, LastLength(-1)
, LastSize(-1)
{}

MPG123::~MPG123()
{ Close();
}

void MPG123::TrackInstance()
{ Mutex::Lock lock(InstMutex);
  Instances.append() = this;
}

void MPG123::EndTrackInstance()
{ Mutex::Lock lock(InstMutex);
  for (MPG123*const* dpp = Instances.end(); dpp-- != Instances.begin(); )
    if (*dpp == this)
      Instances.erase(dpp);
}

inline off_t MPG123::Time2Sample(PM123_TIME time)
{ return (off_t)(floor(time * Tech.samplerate + .5));
}

static const char* const ch_modes[] = { "Stereo", "Joint-Stereo", "Dual-Channel", "Mono" };
static inline const char* ch_mode(mpg123_mode mode)
{ return (unsigned)mode < sizeof ch_modes / sizeof *ch_modes ? ch_modes[mode] : "?";
}

static const char vbr_modes[][4] = { "CBR", "VBR", "ABR" };
static inline const char* vbr_mode(mpg123_vbr mode)
{ return (unsigned)mode < sizeof vbr_modes / sizeof *vbr_modes ? vbr_modes[mode] : "";
}

int MPG123::ReadTechInfo()
{ mpg123_frameinfo info;
  int rc = mpg123_info(MPEG, &info);
  DEBUGLOG(("MPG123::ReadTechInfo - mpg123_info: %i\n", rc));
  if (rc != MPG123_OK)
  { LastError = mpg123_strerror(MPEG);
    return -1;
  }
  int bitrate = info.bitrate * 1000;
  if (FirstFrameLen == 0)
    FirstFrameLen = info.framesize;
  switch (info.vbr)
  {case MPG123_VBR:
    if (LastSize >= 0 && LastLength >= 0)
    { long size = LastSize - FirstFrameLen; // Do not count info frame
      mpg123_id3v1* tagv1;
      mpg123_id3v2* tagv2;
      if (mpg123_id3(MPEG, &tagv1, &tagv2) == MPG123_OK)
      { if (tagv1 && ((ID3V1_TAG*)tagv1)->IsValid())
          size -= sizeof(ID3V1_TAG); // Do not count ID3V1 tag size
        if (tagv2)
          size -= tagv2->taglen; // Do not count ID3V2 tag size
      }
      bitrate = (int)floor(size * 8. / LastLength * info.rate + .5);
    }
    break;
   case MPG123_ABR:
    bitrate = info.abr_rate * 1000;
   default:;
  }
  Tech.samplerate = info.rate;
  Tech.channels = (info.mode != MPG123_M_MONO) +1;
  Tech.attributes = TATTR_SONG|TATTR_WRITABLE|TATTR_STORABLE;
  Tech.info.sprintf("%i kb/s %s, %.1f kHz, %s", bitrate/1000, vbr_mode(info.vbr), info.rate/1000., ch_mode(info.mode));
  Tech.format.sprintf("MP%u", info.layer);
  return bitrate;
}

bool MPG123::UpdateStreamLength()
{ long size = xio_fsize(XFile);
  if (LastSize != size)
  { LastSize = size;
    if (size > 0)
    { mpg123_set_filesize(MPEG, size);
      off_t length = mpg123_length(MPEG);
      if (LastLength != length)
      { LastLength = length;
        return true;
      }
    }
  }
  return false;
}

PLUGIN_RC MPG123::Open(const char* mode)
{
  PLUGIN_RC ret = ID3::Open(mode);
  if (ret != PLUGIN_OK)
    return ret;

  int rc;
  MPEG = mpg123_new(NULL, &rc);
  if (MPEG == NULL)
  { ID3::Close();
    // TODO: Map Error codes?
    return PLUGIN_ERROR;
  }

  RASSERT(mpg123_replace_reader_handle(MPEG, &Decoder::FRead, &Decoder::FSeek, NULL) == MPG123_OK);
  // set some options
  RASSERT(mpg123_param(MPEG, MPG123_ADD_FLAGS, MPG123_FUZZY|MPG123_PLAIN_ID3TEXT, 0) == MPG123_OK);
  // now open the stream
  TrackInstance();
  if ((rc = mpg123_open_handle(MPEG, this)) != 0)
  { Close();
    LastError.sprintf("Unable open source with mpg123:\n%s\n%s", Filename.cdata(), mpg123_strerror(MPEG));
    Ctx.plugin_api->error_display(LastError);
    return PLUGIN_NO_PLAY;
  }

  rc = mpg123_read(MPEG, NULL, 0, NULL);
  if (rc != MPG123_OK && rc != MPG123_NEW_FORMAT)
  { LastError.sprintf("Unable read source with mpg123:\n%s\n%s", Filename.cdata(), mpg123_strerror(MPEG));
    return PLUGIN_NO_PLAY;
  }
  UpdateStreamLength();
  ReadTechInfo();

  return PLUGIN_OK;
}

void MPG123::Close()
{ if (MPEG)
  { EndTrackInstance();
    mpg123_delete(MPEG);
    MPEG = NULL;
  }
  ID3::Close();
}

ssize_t MPG123::FRead(void* that, void* buffer, size_t size)
{
  #define this ((Decoder*)that)
  size = xio_fread(buffer, 1, size, this->XFile);
  if (this->XSave && size > 0)
  { if (this->XSave)
      xio_fwrite(buffer, 1, size, this->XSave);
  }
  return size;
  #undef this
}

off_t MPG123::FSeek(void* that, off_t offset, int seekmode)
{
  #define this ((Decoder*)that)
  return xio_fseek(this->XFile, offset, seekmode);
  #undef this
}

void MPG123::FillPhysInfo(PHYS_INFO& phys)
{ DEBUGLOG(("MPG123(%p{%s})::FillPhysInfo(&%p)\n", this, Filename.cdata(), &phys));
  phys.filesize = LastSize;
  XSTAT st;
  if (xio_fstat(XFile, &st) == 0)
  { phys.tstmp = st.mtime;
    if (!(st.attr & S_IAREAD))
      phys.attributes = PATTR_WRITABLE;
  }
}

inline bool MPG123::FillTechInfo(TECH_INFO& tech, OBJ_INFO& obj)
{ DEBUGLOG(("MPG123(%p{%s})::FillTechInfo(&%p, &%p)\n", this, Filename.cdata(), &tech, &obj));
  obj.bitrate = ReadTechInfo();
  if (obj.bitrate < 0)
  { tech.info = LastError;
    return false;
  }
  tech = Tech;
  obj.songlength = (double)LastLength / Tech.samplerate;
  return true;
}

void static copy_id3v1_string(const mpg123_id3v1* tag, ID3V1_TAG_COMP id, DSTRING& result, ULONG codepage)
{ if (!result)
    ((ID3V1_TAG*)tag)->GetField(id, result, codepage);
}

void static copy_id3v1_tag(META_INFO& info, const mpg123_id3v1* tag)
{ if (!tag)
    return;

  ULONG codepage = cfg.tag_read_id3v1_autoch
    ? ((ID3V1_TAG*)tag)->DetectCodepage(cfg.tag_id3v1_charset)
    : cfg.tag_id3v1_charset;

  copy_id3v1_string(tag, ID3V1_TITLE,   info.title,   codepage);
  copy_id3v1_string(tag, ID3V1_ARTIST,  info.artist,  codepage);
  copy_id3v1_string(tag, ID3V1_ALBUM,   info.album,   codepage);
  copy_id3v1_string(tag, ID3V1_YEAR,    info.year,    codepage);
  copy_id3v1_string(tag, ID3V1_COMMENT, info.comment, codepage);
  copy_id3v1_string(tag, ID3V1_GENRE,   info.genre,   codepage);
  copy_id3v1_string(tag, ID3V1_TRACK,   info.track,   codepage);
}

static void copy_id3v2_string(const ID3V2_TAG* tag, ID3V2_ID id, DSTRING& result)
{
  ID3V2_FRAME* frame = NULL;
  char buffer[256];

  if ( !result
      && ( frame = id3v2_get_frame( tag, id, 1 )) != NULL
      && id3v2_get_string_ex( frame, buffer, sizeof buffer, cfg.tag_read_id3v2_charset ) )
    result = buffer;
}

static void copy_id3v2_tag(META_INFO& info, const ID3V2_TAG* tag)
{ if (!tag)
    return;

  ID3V2_FRAME* frame;
  char buffer[128];
  int  i;

  if( tag ) {
    copy_id3v2_string(tag, ID3V2_TIT2, info.title);
    copy_id3v2_string(tag, ID3V2_TPE1, info.artist);
    copy_id3v2_string(tag, ID3V2_TALB, info.album);
    copy_id3v2_string(tag, ID3V2_TCON, info.genre);
    copy_id3v2_string(tag, ID3V2_TDRC, info.year);

    for( i = 1; ( frame = id3v2_get_frame( tag, ID3V2_COMM, i )) != NULL ; i++ )
    {
      id3v2_get_description( frame, buffer, sizeof( buffer ));

      // Skip iTunes specific comment tags.
      if( strnicmp( buffer, "iTun", 4 ) != 0 ) {
        if (id3v2_get_string_ex( frame, buffer, sizeof buffer, cfg.tag_read_id3v2_charset ))
          info.comment = buffer;
        break;
      }
    }

    copy_id3v2_string( tag, ID3V2_TCOP, info.copyright );
    copy_id3v2_string( tag, ID3V2_TRCK, info.track );

    for( i = 1; ( frame = id3v2_get_frame( tag, ID3V2_RVA2, i )) != NULL ; i++ )
    {
      float gain = 0;
      unsigned char* data = (unsigned char*)frame->fr_data;

      // Format of RVA2 frame:
      //
      // Identification          <text string> $00        // Type of channel         $xx
      // Volume adjustment       $xx xx
      // Bits representing peak  $xx
      // Peak volume             $xx (xx ...)

      id3v2_get_description( frame, buffer, sizeof( buffer ));

      // Skip identification string.
      data += strlen((char*)data ) + 1;
      // Search the master volume.
      while( data - (unsigned char*)frame->fr_data < frame->fr_size ) {
        if( *data == 0x01 ) {
          gain = (float)((signed char)data[1] << 8 | data[2] ) / 512;
          break;
        } else {
          data += 3 + (( data[3] + 7 ) / 8 );
        }
      }
      if( gain != 0 ) {
        if( stricmp( buffer, "album" ) == 0 ) {
          info.album_gain = gain;
        } else {
          info.track_gain = gain;
        }
      }
    }

    for( i = 1; ( frame = id3v2_get_frame( tag, ID3V2_TXXX, i )) != NULL ; i++ )
    {
      id3v2_get_description( frame, buffer, sizeof( buffer ));

      if( stricmp( buffer, "replaygain_album_gain" ) == 0 ) {
        info.album_gain = atof( id3v2_get_string_ex( frame, buffer, sizeof( buffer ), 1004));
      } else if( stricmp( buffer, "replaygain_album_peak" ) == 0 ) {
        info.album_peak = atof( id3v2_get_string_ex( frame, buffer, sizeof( buffer ), 1004));
      } else if( stricmp( buffer, "replaygain_track_gain" ) == 0 ) {
        info.track_gain = atof( id3v2_get_string_ex( frame, buffer, sizeof( buffer ), 1004));
      } else if( stricmp( buffer, "replaygain_track_peak" ) == 0 ) {
        info.track_peak = atof( id3v2_get_string_ex( frame, buffer, sizeof( buffer ), 1004));
      }
    }
  }
}
static void copy_id3v2_tag(META_INFO& info, const mpg123_id3v2* tagv2)
{ if (!tagv2 || !tagv2->taglen)
    return;
  ID3V2_TAG* tag = id3v2_load_tag((char*)tagv2->tagdata, tagv2->taglen, ID3V2_GET_NOCHECK);
  copy_id3v2_tag(info, tag);
}

void MPG123::FillMetaInfo(META_INFO& meta)
{ DEBUGLOG(("MPG123(%p{%s})::FillMetaInfo(&%p)\n", this, Filename.cdata(), &meta));
  char buffer[256];
  if (xio_get_metainfo(XFile, XIO_META_TITLE, buffer, sizeof buffer) && *buffer)
    meta.title = buffer;
  if (xio_get_metainfo(XFile, XIO_META_GENRE, buffer, sizeof buffer) && *buffer)
    meta.genre = buffer;
  if (xio_get_metainfo(XFile, XIO_META_NAME, buffer, sizeof buffer) && *buffer)
    meta.comment = buffer;

  if (cfg.tag_read_type == TAG_READ_NONE)
    return;

  mpg123_id3v1* tagv1;
  mpg123_id3v2* tagv2;
  if (mpg123_id3(MPEG, &tagv1, &tagv2) != MPG123_OK)
    return;

  switch (cfg.tag_read_type)
  { case TAG_READ_ID3V2_AND_ID3V1:
      copy_id3v2_tag(meta, tagv2);
      copy_id3v1_tag(meta, tagv1);
      break;
    case TAG_READ_ID3V1_AND_ID3V2:
      copy_id3v1_tag(meta, tagv1);
      copy_id3v2_tag(meta, tagv2);
      break;
    case TAG_READ_ID3V2_ONLY:
      copy_id3v2_tag(meta, tagv2);
      break;
    case TAG_READ_ID3V1_ONLY:
      copy_id3v1_tag(meta, tagv1);
      break;
    default:; // avoid warning
  }
}

DSTRING MPG123::ReplaceFile(const char* srcfile, const char* dstfile)
{
  DSTRING errmsg;

  // Suspend all active instances of the updated file.
  Mutex::Lock lock(InstMutex);

  // TODO: won't work
  off_t* resumepoints = (off_t*)alloca(Instances.size() * sizeof *resumepoints);
  memset(resumepoints, -1, Instances.size() * sizeof *resumepoints);

  size_t i;
  for (i = 0; i < Instances.size(); i++)
  { MPG123& w = *Instances[i];
    w.DecMutex.Request();

    if (w.MPEG && stricmp(w.Filename, dstfile) == 0)
    { DEBUGLOG(("mpg123:Decoder::ReplaceFile: suspend currently used file: %s\n", w.Filename.cdata()));
      resumepoints[i] = mpg123_tell(w.MPEG);
      w.Close();
    } else
      w.DecMutex.Release();
  }

  // Replace file.
  srcfile = surl2file(srcfile);
  dstfile = surl2file(dstfile);
  if (unlink(dstfile) == 0)
  { DEBUGLOG(("mpg123:Decoder::ReplaceFile: deleted %s, replacing by %s\n", dstfile, srcfile));
    if (rename(srcfile, dstfile) != 0)
      errmsg = "Critical error! Failed to rename temporary file.";
  } else
  { errmsg = "Failed to delete old file.";
    DEBUGLOG(("mpg123:Decoder::ReplaceFile: failed to delete %s (rc = %i), rollback %s\n", dstfile, errno, srcfile));
    unlink(srcfile);
  }

  // Resume all suspended instances of the updated file.
  for (i = 0; i < Instances.size(); i++)
  { if (resumepoints[i] == -1)
      continue;
    MPG123& w = *Instances[i];
    DEBUGLOG(("mpg123:Decoder::ReplaceFile: resumes currently used file: %s\n", w.Filename.cdata()));
    if (w.Open("rbXU") == 0)
      mpg123_seek(w.MPEG, resumepoints[i], SEEK_SET);
    w.DecMutex.Release();
  }

  return errmsg;
}


/***************************************************************************************
 *
 *  class Decoder
 *
 **************************************************************************************/

Decoder::Decoder()
: DecTID(-1)
, Status(DECODER_STOPPED)
, UpdateMeta(false)
{}

Decoder::~Decoder()
{ Stop();
}


PROXYFUNCIMP(void DLLENTRY, Decoder)MetaCallback(int type, const char* metabuff, long pos, void* arg)
{ ((Decoder*)arg)->UpdateMeta = true;
}

// Decoding thread.
PROXYFUNCIMP(void TFNENTRY, Decoder)ThreadStub(void* arg)
{ ((Decoder*)arg)->ThreadFunc();
}

enum localstate
{ ST_READY,
  ST_EOF,
  ST_ERROR
};

void Decoder::ThreadFunc()
{
  localstate state = ST_READY;
  goto start;

 done:
  DecMutex.Release();
  return;

 wait:
  DecMutex.Release();
 start:
  DecEvent.Wait();
  DecEvent.Reset();
  DecMutex.Request();

  if ((Status & 3) == 0)
    goto done;
  if (state == ST_ERROR)
    goto wait; // Error => wait until we receive DECODER_STOP

  if (Status == DECODER_STARTING)
    Status = DECODER_PLAYING;

 nextblock:
  // Handle seek command
  if (JumpTo >= 0)
  { mpg123_seek(MPEG, Time2Sample(JumpTo), SEEK_SET);
    JumpTo = -1;
    (*OutEvent)(OutParam, DECEVENT_SEEKSTOP, NULL);
  } else if (Fast == DECFAST_FORWARD)
    mpg123_seek(MPEG, Time2Sample(.25), SEEK_CUR);
  else if (state == ST_EOF)
    goto wait;
  else if (Fast == DECFAST_REWIND)
    mpg123_seek(MPEG, Time2Sample(-.3), SEEK_CUR);
  state = ST_READY; // Recover from ST_EOF in case of seek

  short* buffer;
  DecMutex.Release();
  int count = (*OutRequestBuffer)(OutParam, &Tech, &buffer);
  DecMutex.Request();
  if ((Status & 3) == 0)
    goto done;
  if (count <= 0)
  { // Error
    (*OutEvent)(OutParam, DECEVENT_PLAYERROR, NULL);
    state = ST_ERROR;
    goto wait;
  }

  size_t done;
  int rc = mpg123_read(MPEG, (unsigned char*)buffer, count * sizeof(*buffer) * Tech.channels, &done);
  DEBUGLOG(("mpg123:Decoder::ThreadFunc mpg123_read -> %i, %u\n", rc, done));
  bool upd_len = UpdateStreamLength(); // Update stream length before ReadTechInfo
  switch (rc)
  {case MPG123_DONE:
    if (done)
    { PM123_TIME pos = (double)(mpg123_tell(MPEG) - done) / Tech.samplerate;
      (*OutCommitBuffer)(OutParam, done / sizeof(*buffer) / Tech.channels, pos);
    }
    (*OutEvent)(OutParam, DECEVENT_PLAYSTOP, NULL);
    state = ST_EOF;
    goto wait;
   default:
    LastError = mpg123_strerror(MPEG);
    (*OutEvent)(OutParam, DECEVENT_PLAYERROR, (void*)LastError.cdata());
    state = ST_ERROR;
    goto wait;
   case MPG123_NEW_FORMAT:
    // fetch format
    ReadTechInfo();
   case MPG123_OK:
     PM123_TIME pos = (double)(mpg123_tell(MPEG) - done) / Tech.samplerate;
     (*OutCommitBuffer)(OutParam, done / sizeof(*buffer) / Tech.channels, pos);
  }
  // Update stream length?
  if (upd_len)
  { OBJ_INFO obj = OBJ_INFO_INIT;
    obj.songlength = StreamLength();
    mpg123_frameinfo info;
    if (mpg123_info(MPEG, &info) == MPG123_OK)
      obj.bitrate = info.bitrate;
    (*OutEvent)(OutParam, DECEVENT_CHANGEOBJ, &obj);
  }
  // Update meta info?
  // The flag my be set while executing mpg123_read
  if (UpdateMeta)
  { UpdateMeta = false;
    META_INFO meta;
    FillMetaInfo(meta);
    (*OutEvent)(OutParam, DECEVENT_CHANGEMETA, &meta);
  }
  goto nextblock;
}


PLUGIN_RC Decoder::Setup(const DECODER_PARAMS2& par)
{ if (Status != DECODER_STOPPED)
    return PLUGIN_GO_FAILED;

  OutRequestBuffer = par.OutRequestBuffer;
  OutCommitBuffer = par.OutCommitBuffer;
  OutEvent = par.DecEvent;
  OutParam = par.A;
  Filename = par.URL;
  // Init
  PLUGIN_RC rc = Open("rbXU");
  if (rc != PLUGIN_OK)
    return rc;
  xio_set_metacallback(XFile, &Decoder::MetaCallback, this);

  return PLUGIN_OK;
}

PLUGIN_RC Decoder::Play(PM123_TIME start, DECFASTMODE fast)
{ if (Status != DECODER_STOPPED)
    return PLUGIN_GO_ALREADY;

  JumpTo = start <= 0 ? -1 : start; // After opening a new file we so are in its beginning.
  Fast = fast;
  Status = DECODER_STARTING;
  DecTID = _beginthread(PROXYFUNCREF(Decoder)ThreadStub, 0, 65535, this);
  if (DecTID == -1)
  { Status = DECODER_STOPPED;
    return PLUGIN_ERROR;
  }

  DecEvent.Set();
  return PLUGIN_OK;
}

PLUGIN_RC Decoder::Stop()
{
  if (Status == DECODER_STOPPED)
    return PLUGIN_GO_ALREADY;

  Status = DECODER_STOPPING;
  DecEvent.Set();
  wait_thread(DecTID, 5000);

  Mutex::Lock lock(DecMutex);
  Status = DECODER_STOPPED;
  DecTID = -1;

  Close();
  if (XSave)
  { xio_fclose(XSave);
    XSave = NULL;
    Savename.reset();
  }
  return PLUGIN_OK;
}

PLUGIN_RC Decoder::SetFast(DECFASTMODE fast)
{ if (Status == DECODER_STOPPED)
    return PLUGIN_GO_FAILED;
  if (Fast == fast)
    return PLUGIN_GO_ALREADY;

  Mutex::Lock lock(DecMutex);
  if (xio_can_seek(XFile)) // Support fast forward for unseekable streams?
    return PLUGIN_UNSUPPORTED;
  Fast = fast;
  return PLUGIN_OK;
}

PLUGIN_RC Decoder::Jump(PM123_TIME pos)
{ Mutex::Lock lock(DecMutex);
  JumpTo = pos;
  switch (Status)
  {case DECODER_STOPPED:
    return PLUGIN_GO_FAILED;
   case DECODER_STOPPING:
    DecEvent.Set();
   default:
    return PLUGIN_OK;
  }
}

PLUGIN_RC Decoder::Save(const DSTRING& savename)
{ Mutex::Lock lock(DecMutex);
  if (!savename)
  { // Disable save
    if (!XSave)
      return PLUGIN_GO_ALREADY;
    xio_fclose(XSave);
    XSave = NULL;
  } else
  { // Enable or change savename
    if (XSave)
    { if ( savename.length() == Savename.length()
        && memcmp(savename.cdata(), Savename.cdata(), savename.length()) == 0 )
        return PLUGIN_GO_ALREADY;
      xio_fclose(XSave);
      XSave = NULL;
      Savename.reset();
    }
    XFILE* save = xio_fopen(savename, "abU");
    if (save == NULL)
    { int err = xio_errno();
      LastError.sprintf("Could not open file to save data:\n%s\n%s",
        savename.cdata(), xio_strerror(err));
        Ctx.plugin_api->info_display(LastError);
      return PLUGIN_NO_READ;
    }
  }
  Savename = savename;
  return PLUGIN_OK;
}


/***************************************************************************************
 *
 *  Plug-in API
 *
 **************************************************************************************/

/// Init function is called when PM123 needs the specified decoder to play
/// the stream demanded by the user.
int DLLENTRY decoder_init(void** returnw)
{ *returnw = new Decoder();
  return PLUGIN_OK;
}

/// Uninit function is called when another decoder than yours is needed.
BOOL DLLENTRY decoder_uninit(void* arg)
{ delete (Decoder*)arg;
  return TRUE;
}

/// There is a lot of commands to implement for this function. Parameters
/// needed for each of the are described in the definition of the structure
/// in the decoder_plug.h file.
ULONG DLLENTRY decoder_command(void* arg, DECMSGTYPE msg, const DECODER_PARAMS2* info)
{ Decoder& w = *(Decoder*)arg;

  switch( msg )
  { case DECODER_SETUP:
      return w.Setup(*info);

    case DECODER_PLAY:
      return w.Play(info->JumpTo, info->Fast);

    case DECODER_STOP:
      return w.Stop();

    case DECODER_FFWD:
      return w.SetFast(info->Fast);

    case DECODER_JUMPTO:
      return w.Jump(info->JumpTo);

    case DECODER_SAVEDATA:
      return w.Save(info->SaveFilename);

    default:
      return PLUGIN_UNSUPPORTED;
  }
}

void DLLENTRY decoder_event(void* w, OUTEVENTTYPE event)
{
  // TODO:
}

/// Returns current status of the decoder.
ULONG DLLENTRY decoder_status(void* arg)
{ return ((Decoder*)arg)->GetStatus();
}

/// Returns number of milliseconds the stream lasts.
PM123_TIME DLLENTRY decoder_length( void* arg )
{ return ((Decoder*)arg)->StreamLength();
}

/// Returns information about specified file.
ULONG DLLENTRY decoder_fileinfo( const char* url, int* what, const INFO_BUNDLE* info,
                                 DECODER_INFO_ENUMERATION_CB, void* )
{
  DEBUGLOG(("mpg123:decoder_fileinfo(%s, *%x, %p)\n", url, *what, info));
  *what |= INFO_PHYS|INFO_ATTR|INFO_CHILD;
  MPG123 w(url);

  PLUGIN_RC rc = w.Open("rbU");
  if (rc != PLUGIN_OK)
  { info->tech->info = w.GetLastError();
    return rc;
  }

  w.FillPhysInfo(*info->phys);
  if (*what & (INFO_TECH|INFO_OBJ|INFO_META))
  { *what |= INFO_TECH|INFO_OBJ|INFO_META;
    if (!w.FillTechInfo(*info->tech, *info->obj))
      return PLUGIN_NO_PLAY;
    w.FillMetaInfo(*info->meta);
  }

  DEBUGLOG(("mpg123:decoder_fileinfo: {%.0f,%i,%x}, {%i,%i,%x,%s,%s,}, {%.3f,%i,},\n"
            "\t{%s,%s,%s,%s, %s,%s,%s,%s, %.1f,%.1f,%.1f,%.1f}\n",
    info->phys->filesize, info->phys->tstmp, info->phys->attributes,
    info->tech->samplerate, info->tech->channels, info->tech->attributes,
    info->tech->info.cdata(), info->tech->format.cdata(),
    info->obj->songlength, info->obj->bitrate,
    info->meta->title.cdata(), info->meta->artist.cdata(), info->meta->album.cdata(), info->meta->year.cdata(),
    info->meta->comment.cdata(), info->meta->genre.cdata(), info->meta->track.cdata(), info->meta->copyright.cdata(),
    info->meta->track_gain, info->meta->track_peak, info->meta->album_gain, info->meta->album_peak));
  return PLUGIN_OK;
}

static void replace_id3v2_string(ID3V2_TAG* tag, ID3V2_ID id, const char* string)
{
  ID3V2_FRAME* frame;
  if (id == ID3V2_COMM)
  { for (int i = 1; (frame = id3v2_get_frame(tag, ID3V2_COMM, i)) != NULL; i++)
    { char buffer[128];
      id3v2_get_description(frame, buffer, sizeof buffer);
      // Skip iTunes specific comment tags.
      if (strnicmp( buffer, "iTun", 4) != 0)
        break;
    }
  } else
    frame = id3v2_get_frame(tag, id, 1);

  // update frame
  if (string == NULL)
  { if (frame)
      id3v2_delete_frame(frame);
  } else
  { if (frame == NULL)
      frame = id3v2_add_frame(tag, id);
    id3v2_set_string(frame, string, cfg.tag_save_id3v2_encoding);
  }
}

BOOL ascii_check(const char* str)
{ if (str)
    while (*str)
    { if (*str < 0x20 || *str > 0x7E)
        return FALSE;
      ++str;
    }
  return TRUE;
}

ULONG DLLENTRY decoder_saveinfo(const char* url, const META_INFO* info, int haveinfo, DSTRING* errortext)
{
  // open file
  ID3 id3file(url);
  PLUGIN_RC ret = id3file.Open("r+bU");
  if (ret != PLUGIN_OK)
  { *errortext = id3file.GetLastError();
    return ret;
  }

  // read tags
  ID3V2_TAG* tagv2;
  ID3V1_TAG tagv1data = ID3V1_TAG::CleanTag;
  id3file.ReadTags(tagv1data, tagv2);

  bool have_tagv2 = true;
  if (tagv2 == NULL)
  { have_tagv2 = false;
    tagv2 = id3v2_new_tag();
  }

  // Apply changes to the tags. Note that they are not necessarily written.
  if (haveinfo & DECODER_HAVE_TITLE)
  { replace_id3v2_string( tagv2, ID3V2_TIT2, info->title);
    tagv1data.SetField(ID3V1_TITLE, info->title, cfg.tag_id3v1_charset);
  }
  if (haveinfo & DECODER_HAVE_ARTIST)
  { replace_id3v2_string( tagv2, ID3V2_TPE1, info->artist);
    tagv1data.SetField(ID3V1_ARTIST, info->artist, cfg.tag_id3v1_charset);
  }
  if (haveinfo & DECODER_HAVE_ALBUM)
  { replace_id3v2_string( tagv2, ID3V2_TALB, info->album);
    tagv1data.SetField(ID3V1_ALBUM, info->album, cfg.tag_id3v1_charset);
  }
  if (haveinfo & DECODER_HAVE_YEAR)
  { replace_id3v2_string( tagv2, ID3V2_TDRC, info->year);
    tagv1data.SetField(ID3V1_YEAR, info->year, cfg.tag_id3v1_charset);
  }
  if (haveinfo & DECODER_HAVE_GENRE)
  { replace_id3v2_string( tagv2, ID3V2_TCON, info->genre);
    tagv1data.SetField(ID3V1_GENRE, info->genre, cfg.tag_id3v1_charset);
  }
  if (haveinfo & DECODER_HAVE_COMMENT)
  { replace_id3v2_string( tagv2, ID3V2_COMM, info->comment);
    tagv1data.SetField(ID3V1_COMMENT, info->comment, cfg.tag_id3v1_charset);
  }
  if (haveinfo & DECODER_HAVE_COPYRIGHT)
  { replace_id3v2_string( tagv2, ID3V2_TCOP, info->copyright);
  }
  if (haveinfo & DECODER_HAVE_TRACK)
  { replace_id3v2_string( tagv2, ID3V2_TRCK, info->track);
    tagv1data.SetField(ID3V1_TRACK, info->track, cfg.tag_id3v1_charset);
  }
  // FIX ME: Replay gain info also must be saved.

  // Determine save mode
  META_INFO new_tag_info;
  size_t i;
  switch (cfg.tag_save_id3v2_type)
  { case TAG_SAVE_ID3V2_ONDEMANDSPC:
      copy_id3v2_tag(new_tag_info, tagv2);
      if ( ascii_check(new_tag_info.title  )
        && ascii_check(new_tag_info.artist )
        && ascii_check(new_tag_info.album  )
        && ascii_check(new_tag_info.comment) )
      goto cont;
    write:
      if (!tagv2->id3_altered)
        tagv2 = NULL;
      have_tagv2 = TRUE;
      break;

    case TAG_SAVE_ID3V2_ONDEMAND:
      copy_id3v2_tag(new_tag_info, tagv2);
    cont:
      i = 0;
      if ( (new_tag_info.title   && strlen(new_tag_info.title ) > 30)
        || (new_tag_info.artist  && strlen(new_tag_info.artist) > 30)
        || (new_tag_info.album   && strlen(new_tag_info.album ) > 30)
        || (new_tag_info.year    && strlen(new_tag_info.year  ) > 4 )
        || (new_tag_info.genre   && *new_tag_info.genre && ID3V1_TAG::GetGenre(new_tag_info.genre) == -1)
        || (new_tag_info.copyright && *new_tag_info.copyright)
        || (new_tag_info.comment && (i = strlen(new_tag_info.comment)) > 30)
        || (new_tag_info.track   && ( strlen(new_tag_info.track) > strspn(new_tag_info.track, "0123456789")
                                   || (i > 28 && *new_tag_info.track) )))
        goto write;
      // Purge ID3v2 metadata
      replace_id3v2_string( tagv2, ID3V2_TIT2, NULL );
      replace_id3v2_string( tagv2, ID3V2_TPE1, NULL );
      replace_id3v2_string( tagv2, ID3V2_TALB, NULL );
      replace_id3v2_string( tagv2, ID3V2_TDRC, NULL );
      replace_id3v2_string( tagv2, ID3V2_COMM, NULL );
      replace_id3v2_string( tagv2, ID3V2_TCON, NULL );
      replace_id3v2_string( tagv2, ID3V2_TCOP, NULL );
      replace_id3v2_string( tagv2, ID3V2_TRCK, NULL );
    case TAG_SAVE_ID3V2_WRITE:
      if ( tagv2->id3_frames_count )
        goto write;
    case TAG_SAVE_ID3V2_DELETE:
      if ( have_tagv2 )
      { tagv2 = DELETE_ID3V2;
        have_tagv2 = FALSE;
        break;
      }
    default: // case TAG_SAVE_ID3V2_UNCHANGED:
      tagv2 = NULL;
      have_tagv2 = FALSE;
  }

  ID3V1_TAG* tagv1;
  switch (cfg.tag_save_id3v1_type)
  { case TAG_SAVE_ID3V1_NOID3V2:
      if (have_tagv2)
        goto del;
    case TAG_SAVE_ID3V1_WRITE:
      if (!tagv1data.IsClean())
      { tagv1 = &tagv1data;
        break;
      }
    case TAG_SAVE_ID3V1_DELETE:
    del:
      if (tagv1data.IsValid())
      { tagv1 = DELETE_ID3V1;
        break;
      }
    default: // case TAG_SAVE_ID3V1_UNCHANGED:
      tagv1 = NULL;
  }

  // Now start the transaction.
  DSTRING savename;
  if (id3file.UpdateTags(tagv1, tagv2, savename) != PLUGIN_OK)
    *errortext = id3file.GetLastError();
  id3file.Close();

  if (!errortext && *savename)
  { // Must replace the file.
    eacopy( url, savename );
    *errortext = MPG123::ReplaceFile( savename, url );
  }
  return *errortext ? PLUGIN_ERROR : PLUGIN_OK;
}

static const DECODER_FILETYPE filetypes[] =
{ { "Digital Audio", "MP1", "*.mp1", DECODER_FILENAME|DECODER_URL|DECODER_SONG|DECODER_METAINFO }
, { "Digital Audio", "MP2", "*.mp2", DECODER_FILENAME|DECODER_URL|DECODER_SONG|DECODER_METAINFO }
, { "Digital Audio", "MP3", "*.mp3", DECODER_FILENAME|DECODER_URL|DECODER_SONG|DECODER_METAINFO }
};

ULONG DLLENTRY decoder_support(const DECODER_FILETYPE** types, int* count)
{ *types = filetypes;
  *count = sizeof filetypes / sizeof *filetypes;
  return DECODER_FILENAME|DECODER_URL|DECODER_SONG|DECODER_METAINFO;
}

/* Returns information about plug-in. */
int DLLENTRY plugin_query( PLUGIN_QUERYPARAM* param )
{ param->type         = PLUGIN_DECODER;
  param->author       = "Samuel Audet, Dmitry A.Steklenev";
  param->desc         = "MP3 Decoder 2.0";
  param->configurable = TRUE;
  param->interface    = PLUGIN_INTERFACE_LEVEL;
  return 0;
}

/* init plug-in */
int DLLENTRY plugin_init( const PLUGIN_CONTEXT* ctx )
{ Ctx = *ctx;
  dialog_init();
  cfg.load();
  mpg123_init();
  return PLUGIN_OK;
}

int DLLENTRY plugin_deinit( int unload )
{ if (unload)
    mpg123_exit();
  return PLUGIN_OK;
}

#if 0 // defined(__IBMC__)
unsigned long _System _DLL_InitTerm( unsigned long modhandle,
                                     unsigned long flag )
{
  if( flag == 0 ) {
    if( _CRT_init() == -1 ) {
      return 0UL;
    }
    return __dll_initialize();
  } else {
    __dll_terminate();
    #ifdef __DEBUG_ALLOC__
    _dump_allocated( 0 );
    #endif
    _CRT_term();
    return 1UL;
  }
}
#endif
