/*  
 * Copyright 2009-2011 Marcel Mueller
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


#ifndef INFOBUNDLE_H
#define INFOBUNDLE_H

#include <format.h>
#include <decoder_plug.h>

#include <cpp/cpputil.h>
#include <cpp/xstring.h>

#include <strutils.h>

#include <string.h>


class PlayableSetBase;

FLAGSATTRIBUTE(PHYS_ATTRIBUTES);
FLAGSATTRIBUTE(TECH_ATTRIBUTES);
FLAGSATTRIBUTE(PL_OPTIONS);
FLAGSATTRIBUTE(INFOTYPE);
FLAGSATTRIBUTE(DECODERSTATE);
FLAGSATTRIBUTE(DECODERMETA);

struct PM123Time
{ PM123_TIME     Value;
                 operator PM123_TIME&()           { return Value; }
                 operator PM123_TIME() const      { return Value; }
  static char*   toString(char* buf, PM123_TIME time);
  static xstring toString(PM123_TIME time)        { char buf[32]; return toString(buf, time); }
         xstring toString() const                 { return toString(Value); }
         bool    parse(const char* string);
};

/** C++ version of INFO_BUNDLE and it's sub structures
 */
struct PhysInfo : public PHYS_INFO
{ static const PHYS_INFO Empty;
  PhysInfo()                                      : PHYS_INFO(Empty) {}
  PhysInfo(const PhysInfo& r)                     : PHYS_INFO(r) {} // = delete would be nice
  PhysInfo(const PHYS_INFO& r)                    : PHYS_INFO(r) {}
  PhysInfo(const volatile PHYS_INFO& r)           { Assign(r); }
  PhysInfo& operator=(const PhysInfo& r)          { Assign(r); return *this; } // = delete would be nice
  PhysInfo& operator=(const PHYS_INFO& r)         { Assign(r); return *this; }
  PhysInfo& operator=(const volatile PHYS_INFO& r){ Assign(r); return *this; }
  void Reset()                                    { (PHYS_INFO&)*this = Empty; }
  void Assign(const PHYS_INFO& r)                 { (PHYS_INFO&)*this = r; }
  void Assign(const volatile PHYS_INFO& r);
  // Assign and return true on change.
  bool CmpAssign(const PHYS_INFO& r)              { return memcmpcpy(this, &r, sizeof *this) != ~0U; }
};

struct TechInfo : public TECH_INFO
{ static const TECH_INFO Empty;
  TechInfo()                                      : TECH_INFO(Empty) {}
  TechInfo(const TechInfo& r)                     { Assign(r); } // = delete would be nice
  TechInfo(const TECH_INFO& r)                    { Assign(r); }
  TechInfo(const volatile TECH_INFO& r)           { Assign(r); }
  TechInfo& operator=(const TechInfo& r)          { Assign(r); return *this; } // = delete would be nice
  TechInfo& operator=(const TECH_INFO& r)         { Assign(r); return *this; }
  TechInfo& operator=(const volatile TECH_INFO& r){ Assign(r); return *this; }
  void Reset();
  void Assign(const TECH_INFO& r);
  void Assign(const volatile TECH_INFO& r);
  // Assign and return true on change.
  bool CmpAssign(const TECH_INFO& r);
};

struct ObjInfo : public OBJ_INFO
{ static const OBJ_INFO Empty;
  ObjInfo()                                       : OBJ_INFO(Empty) {}
  ObjInfo(const ObjInfo& r)                       : OBJ_INFO(r) {} // = delete would be nice
  ObjInfo(const OBJ_INFO& r)                      : OBJ_INFO(r) {}
  ObjInfo(const volatile OBJ_INFO& r)             { Assign(r); }
  ObjInfo& operator=(const ObjInfo& r)            { Assign(r); return *this; } // = delete would be nice
  ObjInfo& operator=(const OBJ_INFO& r)           { Assign(r); return *this; }
  ObjInfo& operator=(const volatile OBJ_INFO& r)  { Assign(r); return *this; }
  void Reset()                                    { (OBJ_INFO&)*this = Empty; }
  void Assign(const OBJ_INFO& r)                  { (OBJ_INFO&)*this = r; }
  void Assign(const volatile OBJ_INFO& r);
  // Assign and return true on change.
  bool CmpAssign(const OBJ_INFO& r)               { return memcmpcpy(this, &r, sizeof *this) != ~0U; }
};

struct MetaInfo : public META_INFO
{ static const META_INFO Empty;
  MetaInfo()                                      : META_INFO(Empty) {}
  MetaInfo(const MetaInfo& r)                     { Assign(r); } // = delete would be nice
  MetaInfo(const META_INFO& r)                    { Assign(r); }
  MetaInfo(const volatile META_INFO& r)           { Assign(r); }
  MetaInfo& operator=(const MetaInfo& r)          { Assign(r); return *this; } // = delete would be nice
  MetaInfo& operator=(const META_INFO& r)         { Assign(r); return *this; }
  MetaInfo& operator=(const volatile META_INFO& r){ Assign(r); return *this; }
  bool IsInitial() const;
  bool Equals(const META_INFO& r) const;
  void Reset();
  void Assign(const META_INFO& r);
  void Assign(const volatile META_INFO& r);
  // Assign and return true on change.
  bool CmpAssign(const META_INFO& r);
};
bool operator==(const META_INFO& l, const META_INFO& r);
inline bool operator!=(const META_INFO& l, const META_INFO& r) { return !(l == r); }

struct AttrInfo : public ATTR_INFO
{ static const ATTR_INFO Empty;
  AttrInfo()                                      : ATTR_INFO(Empty) {}
  AttrInfo(const AttrInfo& r)                     : ATTR_INFO(r) {} // = delete would be nice
  AttrInfo(const ATTR_INFO& r)                    : ATTR_INFO(r) {}
  AttrInfo(const volatile ATTR_INFO& r)           { Assign(r); }
  AttrInfo& operator=(const AttrInfo& r)          { Assign(r); return *this; } // = delete would be nice
  AttrInfo& operator=(const ATTR_INFO& r)         { Assign(r); return *this; }
  AttrInfo& operator=(const volatile ATTR_INFO& r){ Assign(r); return *this; }
  bool IsInitial() const                          { return !ploptions && !at; }
  bool Equals(const ATTR_INFO& r) const           { return ploptions == r.ploptions && at == r.at; }
  void Reset()                                    { (ATTR_INFO&)*this = Empty; }
  void Assign(const ATTR_INFO& r)                 { (ATTR_INFO&)*this = r; }
  void Assign(const volatile ATTR_INFO& r);
  // Assign and return true on change.
  bool CmpAssign(const ATTR_INFO& r);
};
inline bool operator==(const ATTR_INFO& l, const ATTR_INFO& r) { return l.ploptions == r.ploptions && l.at == r.at; }
inline bool operator!=(const ATTR_INFO& l, const ATTR_INFO& r) { return !(l == r); }

struct RplInfo : public RPL_INFO
{ static const RPL_INFO Empty;
  RplInfo()                                       : RPL_INFO(Empty) {}
  RplInfo(const RplInfo& r)                       : RPL_INFO(r) {} // = delete would be nice
  RplInfo(const RPL_INFO& r)                      : RPL_INFO(r) {}
  RplInfo(const volatile RPL_INFO& r)             { Assign(r); }
  RplInfo& operator=(const RplInfo& r)            { Assign(r); return *this; } // = delete would be nice
  RplInfo& operator=(const RPL_INFO& r)           { Assign(r); return *this; }
  RplInfo& operator=(const volatile RPL_INFO& r)  { Assign(r); return *this; }
  void Reset()                                    { (RPL_INFO&)*this = Empty; }
  void Assign(const RPL_INFO& r)                  { (RPL_INFO&)*this = r; }
  void Assign(const volatile RPL_INFO& r);
  // Assign and return true on change.
  bool CmpAssign(const RPL_INFO& r)               { return memcmpcpy(this, &r, sizeof *this) != ~0U; }
  int  GetSongs() const                           { return unknown ? -1 : songs; }
  int  GetLists() const                           { return unknown ? -1 : lists; }
  int  GetInvalid() const                         { return unknown ? -1 : invalid; }
  void Clear()                                    { memset(this, 0, sizeof *this); }
  RplInfo& operator+=(const volatile RPL_INFO& r);
  RplInfo& operator-=(const volatile RPL_INFO& r);
};

struct DrplInfo : public DRPL_INFO
{ static const DRPL_INFO Empty;
  DrplInfo()                                      : DRPL_INFO(Empty) {}
  DrplInfo(const DrplInfo& r)                     : DRPL_INFO(r) {} // = delete would be nice
  DrplInfo(const DRPL_INFO& r)                    : DRPL_INFO(r) {}
  DrplInfo(const volatile DRPL_INFO& r)           { Assign(r); }
  DrplInfo& operator=(const DrplInfo& r)          { Assign(r); return *this; } // = delete would be nice
  DrplInfo& operator=(const DRPL_INFO& r)         { Assign(r); return *this; }
  DrplInfo& operator=(const volatile DRPL_INFO& r){ Assign(r); return *this; }
  void      Reset()                               { (DRPL_INFO&)*this = Empty; }
  void      Assign(const DRPL_INFO& r)            { (DRPL_INFO&)*this = r; }
  void      Assign(const volatile DRPL_INFO& r);
  // Assign and return true on change.
  bool      CmpAssign(const DRPL_INFO& r)         { return memcmpcpy(this, &r, sizeof *this) != ~0U; }
  PM123_TIME GetTotalLength() const               { return unk_length ? -1. : totallength; }
  PM123_SIZE GetTotalSize() const                 { return unk_size   ? -1. : totalsize; }
  void      Clear()                               { memset(this, 0, sizeof *this); }
  DrplInfo& operator+=(const volatile DRPL_INFO& r);
  DrplInfo& operator-=(const volatile DRPL_INFO& r);
};

struct ItemInfo : public ITEM_INFO
{ static const ITEM_INFO Empty;
  ItemInfo()                                      : ITEM_INFO(Empty) {}
  ItemInfo(const ItemInfo& r)                     { Assign(r); } // = delete would be nice
  ItemInfo(const ITEM_INFO& r)                    { Assign(r); }
  ItemInfo(const volatile ITEM_INFO& r)           { Assign(r); }
  ItemInfo& operator=(const ItemInfo& r)          { Assign(r); return *this; } // = delete would be nice
  ItemInfo& operator=(const ITEM_INFO& r)         { Assign(r); return *this; }
  ItemInfo& operator=(const volatile ITEM_INFO& r){ Assign(r); return *this; }
  bool      IsInitial() const;
  void      Reset();
  void      Assign(const ITEM_INFO& r);
  void      Assign(const volatile ITEM_INFO& r);
  // Assign and return true on change.
  bool      CmpAssign(const ITEM_INFO& r);
};
bool operator==(const ITEM_INFO& l, const ITEM_INFO& r);
inline bool operator!=(const ITEM_INFO& l, const ITEM_INFO& r) { return !(l == r); }

struct DecoderInfo
{ PhysInfo               Phys;
  TechInfo               Tech;
  ObjInfo                Obj;
  MetaInfo               Meta;
  AttrInfo               Attr;
  // reset all fields to their initial state
  void                   Reset();
};

struct AggregateInfo
{ RplInfo                Rpl;
  DrplInfo               Drpl;
  const PlayableSetBase& Exclude; // Virtual base class supplied from creator or derived class.
 protected:
  unsigned               Revision;
 public:
                         //AggregateInfo() : Revision(0) {}
                         AggregateInfo(const PlayableSetBase& exclude) : Exclude(exclude), Revision(0) {}
                         //AggregateInfo(const AggregateInfo& r);
  void                   Reset()                  { Rpl.Reset(); Drpl.Reset(); }
  AggregateInfo&         operator=(const AggregateInfo& r);
  AggregateInfo&         operator=(const volatile AggregateInfo& r);
  // reset all fields to their initial state
  void                   Clear()                  { Rpl.Clear(); Drpl.Clear(); }
  AggregateInfo&         operator+=(const AggregateInfo& r) { Rpl += r.Rpl; Drpl += r.Drpl; return *this; }
  AggregateInfo&         operator+=(const volatile AggregateInfo& r) { Rpl += r.Rpl; Drpl += r.Drpl; return *this; }
  AggregateInfo&         operator-=(const AggregateInfo& r) { Rpl -= r.Rpl; Drpl -= r.Drpl; return *this; }
  AggregateInfo&         operator-=(const volatile AggregateInfo& r) { Rpl -= r.Rpl; Drpl -= r.Drpl; return *this; }
  unsigned               GetRevision() const      { return Revision; }
  unsigned               NextRevision()           { return Revision++; }
};

// TODO: remove this class
class AllInfo
: public DecoderInfo
, public AggregateInfo
{public:
  ItemInfo               Item;
 public:
  // reset all fields to their initial state
  void                   Reset();
  void                   Assign(const INFO_BUNDLE& r);
  void                   Assign(const INFO_BUNDLE_CV& r);
  AllInfo();
  AllInfo(const INFO_BUNDLE& r);
  AllInfo(const INFO_BUNDLE_CV& r);
  AllInfo& operator=(const INFO_BUNDLE& r)        { Assign(r); return *this; }
  AllInfo& operator=(const INFO_BUNDLE_CV& r)     { Assign(r); return *this; }
  // Returns a bit vector with all valid pointers in info.
  // This will never return IF_Child or any of IF_Basic. 
  //static InfoFlags            ContainsInfo(const INFO_BUNDLE& info);
};

class InfoBundle
: public INFO_BUNDLE
, public AllInfo
{private:
  void                   Init();
 public:
  // reset all fields to their initial state
  void                   Reset();
                         InfoBundle();
  // Copy
                         InfoBundle(const InfoBundle& r);
  // Reference
  //                       InfoBundle(const INFO_BUNDLE& r) { *this = r; }
  // Assignment
  //void                   operator=(const INFO_BUNDLE& r);
  operator const INFO_BUNDLE_CV&() const { return *(INFO_BUNDLE_CV*)(INFO_BUNDLE*)this; }
 
  // Returns a bit vector with all valid pointers in info.
  // This will never return IF_Child or any of IF_Basic. 
  //static InfoFlags       ContainsInfo(const INFO_BUNDLE& info);
};

/* Facade that sets all info types not in mask to NULL. */
/*class MaskedInfoBundle : public INFO_BUNDLE
{public:
  MaskInfoBundle(const INFO_BUNDLE& info, InfoFlags mask);
};*/

#endif
