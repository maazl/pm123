/*
 * Copyright 2009-2011 M.Mueller
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


#include "123_util.h"
#include "infobundle.h"
#include "playableset.h"
#include "playable.h"
#include "xstring_api.h"

#include <stddef.h>
#include <stdio.h>
#include <math.h>

#include <debuglog.h>


char* PM123Time::toString(char* buf, PM123_TIME time)
{ char* cp = buf;
  if (time < 0)
    *cp++ = '-';
  time = fabs(time);
  unsigned long secs = (unsigned long)time;
  if (secs >= 86400)
    sprintf(cp, "%lu:%02lu:%02lu", secs / 86400, secs / 3600 % 60, secs / 60 % 60);
  else if (secs > 3600)
    sprintf(cp, "%lu:%02lu", secs / 3600, secs / 60 % 60);
  else
    sprintf(cp, "%lu", secs / 60);
  cp += strlen(cp);
  // Seconds with fractional part if any.
  time -= secs;
  ASSERT(time >= 0 && time < 1);
  if (time)
    sprintf(cp+2, "%f", time); // Hack to remove the leading 0.
  sprintf(cp, ":%02ld", secs % 60);
  if (time)
  { cp += 3;
    *cp = '.';
    // discard trailing 0's
    cp += strlen(cp);
    while (*--cp == '0')
      *cp = 0;
  }
  else
    cp[3] = 0;
  return buf;
}


/****************************************************************************
*
*  class PhysInfo
*
****************************************************************************/

const PHYS_INFO PhysInfo::Empty = PHYS_INFO_INIT;

void PhysInfo::Assign(const volatile PHYS_INFO& r)
{ filesize   = r.filesize;
  tstmp      = r.tstmp;
  attributes = r.attributes;
}

/****************************************************************************
*
*  class TechInfo
*
****************************************************************************/

const TECH_INFO TechInfo::Empty = TECH_INFO_INIT;

void TechInfo::Reset()
{ samplerate = -1;
  channels   = -1;
  attributes = TATTR_NONE;
  info.reset();
  format.reset();
  decoder.reset();
}

void TechInfo::Assign(const TECH_INFO& r)
{ (TECH_INFO&)*this = r;
}
void TechInfo::Assign(const volatile TECH_INFO& r)
{ samplerate = r.samplerate;
  channels   = r.channels;
  attributes = r.attributes;
  info       = r.info;
  format     = r.format;
  decoder    = r.decoder;
}

bool TechInfo::CmpAssign(const TECH_INFO& r)
{ return memcmpcpy(this, &r, offsetof(TECH_INFO, info)) != ~0U
    | xstring_cmpassign(&info,    r.info.cdata())
    | xstring_cmpassign(&format,  r.format.cdata())
    | xstring_cmpassign(&decoder, r.decoder.cdata());
}


/****************************************************************************
*
*  class ObjInfo
*
****************************************************************************/

const OBJ_INFO ObjInfo::Empty = OBJ_INFO_INIT;

void ObjInfo::Assign(const volatile OBJ_INFO& r)
{ songlength = r.songlength;
  bitrate    = r.bitrate;
  num_items  = r.num_items;
}

/****************************************************************************
*
*  class MetaInfo
*
****************************************************************************/

const META_INFO MetaInfo::Empty = META_INFO_INIT;

bool MetaInfo::IsInitial() const
{ return !title && !artist && !album && !year && !comment && !genre && !copyright
      && track_gain >= -1000 && track_peak <= -1000 && album_gain <= -1000 && album_peak <= -1000;
}

bool MetaInfo::Equals(const META_INFO& r) const
{ return title == r.title && artist == r.artist && album == r.album && year == r.year
      && comment == r.comment && genre == r.genre &&track == r.track && copyright == r.copyright
      && track_gain == r.track_gain && track_peak == r.track_peak
      && album_gain == r.album_gain && album_peak == r.album_peak;
}

void MetaInfo::Reset()
{ title    .reset();
  artist   .reset();
  album    .reset();
  year     .reset();
  comment  .reset();
  genre    .reset();
  track    .reset();
  copyright.reset();
  track_gain = -1000;
  track_peak = -1000;
  album_gain = -1000;
  album_peak = -1000;
}

void MetaInfo::Assign(const META_INFO& r)
{ (META_INFO&)*this = r;
}
void MetaInfo::Assign(const volatile META_INFO& r)
{ title      = r.title;
  artist     = r.artist;
  album      = r.album;
  year       = r.year;
  comment    = r.comment;
  genre      = r.genre;
  track      = r.track;
  copyright  = r.copyright;
  track_gain = r.track_gain;
  track_peak = r.track_peak;
  album_gain = r.album_gain;
  album_peak = r.album_peak;
}

bool MetaInfo::CmpAssign(const META_INFO& r)
{ return xstring_cmpassign(&title,     r.title.cdata())
       | xstring_cmpassign(&artist,    r.artist.cdata())
       | xstring_cmpassign(&album,     r.album.cdata())
       | xstring_cmpassign(&year,      r.year.cdata())
       | xstring_cmpassign(&comment,   r.comment.cdata())
       | xstring_cmpassign(&genre,     r.genre.cdata())
       | xstring_cmpassign(&track,     r.track.cdata())
       | xstring_cmpassign(&copyright, r.copyright.cdata())
       | memcmpcpy(&track_gain, &r.track_gain, sizeof *this - offsetof(META_INFO, track_gain)) != ~0U;
}

bool operator==(const META_INFO& l, const META_INFO& r)
{ return l.title   == r.title
      && l.artist  == r.artist
      && l.album   == r.album
      && l.year    == r.year
      && l.comment == r.comment
      && l.genre   == r.genre
      && l.track   == r.track
      && l.copyright  == r.copyright
      && l.track_gain == r.track_gain
      && l.track_peak == r.track_peak
      && l.album_gain == r.album_gain
      && l.album_peak == r.album_peak;
}


/****************************************************************************
*
*  class AttrInfo
*
****************************************************************************/

const ATTR_INFO AttrInfo::Empty = ATTR_INFO_INIT;

void AttrInfo::Assign(const volatile ATTR_INFO& r)
{ ploptions = r.ploptions;
  at        = r.at;
}

bool AttrInfo::CmpAssign(const ATTR_INFO& r)
{ bool ret = ploptions != r.ploptions
           | xstring_cmpassign(&at, r.at.cdata());
  ploptions = r.ploptions;
  return ret;
}


/****************************************************************************
*
*  class RplInfo
*
****************************************************************************/

const RPL_INFO RplInfo::Empty = RPL_INFO_INIT;

RplInfo& RplInfo::operator+=(const volatile RPL_INFO& r)
{ songs   += r.songs;
  lists   += r.lists;
  invalid += r.invalid;
  unknown += r.unknown;
  return *this;
}

RplInfo& RplInfo::operator-=(const volatile RPL_INFO& r)
{ songs   -= r.songs;
  lists   -= r.lists;
  invalid -= r.invalid;
  unknown -= r.unknown;
  return *this;
}

void RplInfo::Assign(const volatile RPL_INFO& r)
{ songs   = r.songs;
  lists   = r.lists;
  invalid = r.invalid;
  unknown = r.unknown;
}


/****************************************************************************
*
*  class DrplInfo
*
****************************************************************************/

const DRPL_INFO DrplInfo::Empty = DRPL_INFO_INIT;

DrplInfo& DrplInfo::operator+=(const volatile DRPL_INFO& r)
{ totallength += r.totallength;
  unk_length  += r.unk_length;
  totalsize   += r.totalsize;
  unk_size    += r.unk_size;
  return *this;
}

DrplInfo& DrplInfo::operator-=(const volatile DRPL_INFO& r)
{ totallength -= r.totallength;
  unk_length  -= r.unk_length;
  totalsize   -= r.totalsize;
  unk_size    -= r.unk_size;
  return *this;
}

void DrplInfo::Assign(const volatile DRPL_INFO& r)
{ totallength = r.totallength;
  unk_length  = r.unk_length;
  totalsize   = r.totalsize;
  unk_size    = r.unk_size;
}


/****************************************************************************
*
*  class ItemInfo
*
****************************************************************************/

const ITEM_INFO ItemInfo::Empty = ITEM_INFO_INIT;

bool ItemInfo::IsInitial() const
{ return !alias && !start && !stop && pregap == 0 && postgap == 0 && gain == 0;
}

void ItemInfo::Reset()
{ alias.reset();
  start.reset();
  stop .reset();
  pregap  = 0;
  postgap = 0;
  gain    = 0;
}

void ItemInfo::Assign(const ITEM_INFO& r)
{ (ITEM_INFO&)*this = r;
}
void ItemInfo::Assign(const volatile ITEM_INFO& r)
{ alias   = r.alias;
  start   = r.start;
  stop    = r.stop;
  pregap  = r.pregap;
  postgap = r.postgap;
  gain    = r.gain;
}

bool ItemInfo::CmpAssign(const ITEM_INFO& r)
{ bool ret = xstring_cmpassign(&alias, r.alias.cdata())
           | xstring_cmpassign(&start, r.start.cdata())
           | xstring_cmpassign(&stop,  r.stop.cdata())
           | pregap  != r.pregap
           | postgap != r.postgap
           | gain    != r.gain;
  pregap  = r.pregap;
  postgap = r.postgap;
  gain    = r.gain;
  return ret;
}
  
bool operator==(const ITEM_INFO& l, const ITEM_INFO& r)
{ return l.alias == r.alias
      && l.start == r.start
      && l.stop  == r.stop
      && l.pregap  == r.pregap
      && l.postgap == r.postgap
      && l.gain    == r.gain;
}


/****************************************************************************
*
*  class DocoderInfo
*
****************************************************************************/

void DecoderInfo::Reset()
{ Phys.Reset();
  Tech.Reset();
  Obj .Reset();
  Meta.Reset();
  Attr.Reset();
}


/****************************************************************************
*
*  class AggregateInfo
*
****************************************************************************/

/*AggregateInfo::AggregateInfo(const AggregateInfo& r)
: Rpl(r.Rpl),
  Drpl(r.Drpl),
  Exclude(r.Exclude),
  Revision(r.Revision)
{}*/

AggregateInfo& AggregateInfo::operator=(const AggregateInfo& r)
{ Rpl  = r.Rpl;
  Drpl = r.Drpl;
  Revision = r.Revision;
  return *this;
}

AggregateInfo& AggregateInfo::operator=(const volatile AggregateInfo& r)
{ Rpl  = r.Rpl;
  Drpl = r.Drpl;
  Revision = r.Revision;
  return *this;
}


/****************************************************************************
*
*  class AllInfo
*
****************************************************************************/

AllInfo::AllInfo()
: AggregateInfo(PlayableSetBase::Empty) // This bundle contains always the root aggregate without exclusions
{}
AllInfo::AllInfo(const INFO_BUNDLE& r)
: AggregateInfo(PlayableSetBase::Empty) // This bundle contains always the root aggregate without exclusions
{ Assign(r);
}
AllInfo::AllInfo(const INFO_BUNDLE_CV& r)
: AggregateInfo(PlayableSetBase::Empty) // This bundle contains always the root aggregate without exclusions
{ Assign(r);
}

void AllInfo::Reset()
{ DecoderInfo::Reset();
  AggregateInfo::Reset();
  Item.Reset();
}

void AllInfo::Assign(const INFO_BUNDLE& r)
{ Phys.Assign(*r.phys);
  Tech.Assign(*r.tech);
  Obj .Assign(*r.obj);
  Meta.Assign(*r.meta);
  Attr.Assign(*r.attr);
  Rpl .Assign(*r.rpl);
  Drpl.Assign(*r.drpl);
  Item.Assign(*r.item);
}

void AllInfo::Assign(const INFO_BUNDLE_CV& r)
{ Phys.Assign(*r.phys);
  Tech.Assign(*r.tech);
  Obj .Assign(*r.obj);
  Meta.Assign(*r.meta);
  Attr.Assign(*r.attr);
  Rpl .Assign(*r.rpl);
  Drpl.Assign(*r.drpl);
  Item.Assign(*r.item);
}


/****************************************************************************
*
*  class InfoBundle
*
****************************************************************************/

void InfoBundle::Init()
{ phys = &Phys;
  tech = &Tech;
  obj  = &Obj;
  meta = &Meta;
  attr = &Attr;
  rpl  = &Rpl;
  drpl = &Drpl;
  item = &Item;
}

void InfoBundle::Reset()
{ Init();
  AllInfo::Reset();
}

InfoBundle::InfoBundle()
{ Init();
}
InfoBundle::InfoBundle(const InfoBundle& r)
: AllInfo((AllInfo&)r)
{ Init();
}

/*void InfoBundle::operator=(const INFO_BUNDLE& r)
{ Reset();
  // version stable copy
  if (r.tech)
    memcpy(&TechInfo.size+1, &r.tech.size+1, min(sizeof TechInfo, r.tech->size) - sizeof TechInfo.size);
  if (r.meta)
    memcpy(&MetaInfo.size+1, &r.meta.size+1, min(sizeof MetaInfo, r.meta->size) - sizeof MetaInfo.size);
  if (r.phys)
    memcpy(&PhysInfo.size+1, &r.phys.size+1, min(sizeof PhysInfo, r.phys->size) - sizeof PhysInfo.size);
  if (r.attr)
    memcpy(&AttrInfo.size+1, &r.attr.size+1, min(sizeof AttrInfo, r.attr->size) - sizeof AttrInfo.size);
  if (r.rpl)
    memcpy(&RplInfo.size +1, &r.rpl.size +1, min(sizeof RplInfo,  r.rpl->size)  - sizeof RplInfo.size );
  if (r.item)
    memcpy(&ItemInfo.size+1, &r.item.size+1, min(sizeof ItemInfo, r.item->size) - sizeof ItemInfo.size);
}*/

/*static InfoFlags InfoBundle::ContainsInfo(const INFO_BUNDLE& info)
{ return !!info->phys * IF_Phys
       | !!info->tech * IF_Tech
       | !!info->song * IF_Song
       | !!info->meta * IF_Meta
       | !!info->attr * IF_Attr
       | !!info->rpl  * IF_Rpl
       | !!info->drpl * IF_Drpl
       | !!info->item * IF_Item;
}*/

/****************************************************************************
*
*  class MaskedInfoBundle
*
****************************************************************************/

/*MaskedInfoBundle::MaskedInfoBundle(const IFOBUNLE& info, InfoFlags mask)
{ DEBUGLOG(("MaskedInfoBundle(%p)::MaskedInfoBundle(&%p, %x)\n", this, &info, mask));
  memset((INFO_BUNDLE*)this, 0, sizeof(INFO_BUNDLE));
  if (mask & IF_Phys)
    phys = info.phys;
  if (mask & IF_Tech)
    tech = info.tech;
  if (mask & IF_Obj)
    obj  = info.obj;
  if (mask & IF_Meta)
    meta = info.meta;
  if (mask & IF_Attr)
    attr = info.attr;
  if (mask & IF_Rpl)
    rpl  = info.rpl;
  if (mask & IF_Drpl)
    drpl = info.drpl;
  if (mask & IF_Item)
    item = info.item; 
}*/

