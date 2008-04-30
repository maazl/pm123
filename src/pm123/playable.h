/*
 * Copyright 2007-2008 Marcel Mueller
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


#ifndef PLAYABLE_H
#define PLAYABLE_H

#include <stdlib.h>

#include <cpp/event.h>
//#include <utilfct.h>
#include <cpp/mutex.h>
#include <cpp/smartptr.h>
#include <cpp/xstring.h>
#include <cpp/queue.h>
#include <cpp/cpputil.h>
#include <cpp/container.h>
#include <cpp/url123.h>

#include <decoder_plug.h>


/* Status of Playable Objects and PlayableInstance.
 */
enum PlayableStatus
{ STA_Unknown, // The Status of the object is not yet known
  STA_Invalid, // the current object cannot be used
  STA_Normal,  // no special status
  STA_Used     // the current object is actively in use
};

/* Abstract class to support any playable object.
 *
 * This class instances are unique for each URL.
 *
 * All instances of this class are owned by the class itself and must not be deleted.
 * You should only hold instances in the smart pointer int_ptr<Playable>. Otherwise
 * the referred object may be deleted in background. But it is allowed to call
 * a function with parameter Playable* (e.g. member functions) while you have an active
 * instance of int_ptr<Playable> unless the function keeps a local copy of the pointer
 * after it returns. It is also allowed to pass the result of a function returning
 * int_ptr<Playable> directly to a function taking Playable* with the same restriction
 * because temporary objects are always kept until the next statement.
 * If a function that receives Playable* wants to keep the reference after returning
 * it must create a int_ptr<Playable> object. This will work as expected.
 */
static void TFNENTRY PlayableWorker(void*);
class Playable
: public Iref_Count,
  public InstanceCompareable<Playable>,
  public IComparableTo<const char*>
{public:
  enum Flags
  { None        = 0,   // This attribute implies that a cast to Song is valid.
    Enumerable  = 1,   // This attribute implies that a cast to PlayableCollection is valid.
    Mutable     = 3    // This attribute implies that a cast to PlayableCollection and Playlist is valid.
  };
  enum InfoFlags
  { IF_None     = 0,
    IF_Format   = 0x01,// applies to GetInfo().format
    IF_Tech     = 0x02,// applies to GetInfo().tech
    IF_Meta     = 0x04,// applies to GetInfo().meta
    IF_Other    = 0x08,// applies e.g. to PlayableCollection::GetNext()
    IF_Status   = 0x10,// applies to GetStatus() and IsModified()
    IF_All      = IF_Format|IF_Tech|IF_Meta|IF_Other|IF_Status
  };
  // Parameters for InfoChange Event
  struct change_args
  { Playable&         Instance;
    InfoFlags         Flags; // Bitvector of type InfoFlags
    change_args(Playable& inst, InfoFlags flags) : Instance(inst), Flags(flags) {}
  };
 protected:
  // C++ version of DECODER_INFO2
  class DecoderInfo : public DECODER_INFO2
  {private:
    FORMAT_INFO2      Format;
    TECH_INFO         TechInfo;
    META_INFO         MetaInfo;
   public:
    static const DecoderInfo InitialInfo;
   private:
    void              Init();
   public:
    // reset all fileds to their initial state
    void              Reset();
    DecoderInfo()     { Reset(); }
    // copy
    DecoderInfo(const DECODER_INFO2& r) { *this = r; }
    // assignment
    void              operator=(const DECODER_INFO2& r);
  };

 private:
  const url123        URL;
 protected: // The following vars are protected by the mutex
  DecoderInfo         Info;
  PlayableStatus      Stat;
  char                Decoder[_MAX_FNAME];
  InfoFlags           InfoValid;       // Bitvector of type InfoFlags
  InfoFlags           InfoChangeFlags; // Bitvector with stored events
  // ... except for this one
  unsigned            InfoRequest;     // Bitvector with requested information
 public:
  Mutex               Mtx;   // protect this instance

 private: // non-copyable
  Playable(const Playable&);
  void operator=(const Playable&);
 protected:
  Playable(const url123& url, const FORMAT_INFO2* ca_format = NULL, const TECH_INFO* ca_tech = NULL, const META_INFO* ca_meta = NULL);
  // Update the structure components and return the required InfoChange Flags or 0 if no change has been made.
  void                UpdateInfo(const FORMAT_INFO2* info);
  void                UpdateInfo(const TECH_INFO* info);
  void                UpdateInfo(const META_INFO* info);
  void                UpdateInfo(const DECODER_INFO2* info);
  void                UpdateStatus(PlayableStatus stat);
  // Raise the InfoChange event if required.
  // This function must be called in synchronized context.
  void                RaiseInfoChange();
 public:
  virtual ~Playable();
  // Check whether a given URL is to be initialized as playlist.
  static bool         IsPlaylist(const url123& URL);
  // Get URL
  const url123&       GetURL() const      { return URL; }
  // RTTI by the back door. (dynamic_cast<> would be much nicer, but this is not supported by icc 3.0.)
  virtual Flags       GetFlags() const;
  // Return Status of the current object
  PlayableStatus      GetStatus() const   { return Stat; }
  // Mark the object as used (or not)
  // This Function should not be called if the object's state is STA_Unknown or STA_Invalid.
  void                SetInUse(bool used);
  // Display name
  // This returns either Info.meta.title or the object name of the current URL.
  // Keep in mind that this may not return the expected value unless EnsureInfo(IF_Meta) has been called.
  virtual xstring     GetDisplayName() const;

  // Get current Object info.
  // If the required information is not yet loaded, you will get an empty structure.
  // Call EnsureInfo to prefetch the information.
  // Since the returned information is a reference to a potentially mutable structure
  // you have to syncronize the access by either locking the current instance before the call
  // or by copying the data inside a critical section unless your read access is implicitly atomic.
  // However the returned memory reference itself is guaranteed to be valid
  // as long as the current Playable instance exists. Only the content may change.
  // So there is no need to lock before the GetInfo call.
  const DECODER_INFO2& GetInfo() const    { return Info; }
  // If and only if the current object is already identified by a decoder_fileinfo call
  // then this function will return the DLL name of the encoder.
  // The result is only guaranteed to be non NULL if EnsureInfo with IF_Other has been called.
  const char*         GetDecoder() const  { return *Decoder ? Decoder : NULL; }

  // Check wether the requested information is immediately available.
  // Return the bits in what that are /not/ available.
  inline InfoFlags    CheckInfo(InfoFlags what); // can't define here because of missing FLAGSATTRIBUTE(InfoFlags).
  // This function is called to populate the info fields.
  // The parameter what is the requested information. The function returns the retrieved information.
  // The retrieved information must mot be less than the requested information. But it might be more.
  // This is a mutable Function when the info-fields are not initialized yet (late initialization).
  // Normally this function is called automatically. However, you may want to force a synchronuous refresh.
  virtual InfoFlags   LoadInfo(InfoFlags what) = 0;
  // Ensure that the info fields are loaded from the ressource.
  // This function may block until the data is available.
  // To be reliable you shound be in synchronized context.
  void                EnsureInfo(InfoFlags what) { InfoFlags i = CheckInfo(what); if (i != 0) LoadInfo(i); }
  // Call LoadInfo asynchronuously. This is a service of Playable.
  // When the requested information is availabe the InfoChange event is raised.
  // But this is done only if the information really has changed.
  // Of course yo have to register the eventhandler before the call to LoadInfoAsync.
  void                LoadInfoAsync(InfoFlags what);
  // Call LoadInfo asynchronuously if the requested information is not yet available.
  // The function returns the flags of the requested information that is immediately available.
  // For all the missing bits you should use the InfoChange event to get the requested information.
  // If no information is yet available, the function retorns IF_None.
  // Of course yo have to register the eventhandler before the call to EnsureInfoAsync.
  // If EnsureInfoAsync returned true the InfoChange event is not raised.
  InfoFlags           EnsureInfoAsync(InfoFlags what);

  // Set meta information.
  // Calling this function with meta == NULL deletes the meta information.
  void                SetMetaInfo(const META_INFO* meta);
  // Set technical information.
  void                SetTechInfo(const TECH_INFO* tech);

 public:
  // Event on info change
  // This event will always be called in synchronized context.
  event<const change_args> InfoChange;

 // asynchronuous request service
 private:
  typedef int_ptr<Playable> QEntry;

 private:
  static queue<QEntry>     WQueue;
  static int               WTid;          // Thread ID of the worker
  static bool              WTermRq;       // Termination Request to Worker

 private:
  // Worker thread (free function because of IBM VAC restrictions)
  friend void TFNENTRY PlayableWorker(void*);
 public:
  static void              Init();
  static void              Uninit();

 // Repository
 private:
  static sorted_vector<Playable, const char*> RPInst;
  static Mutex             RPMutex;
 private:
  #ifdef DEBUG
  static void              RPDebugDump();
  #endif
 public:
  virtual int              compareTo(const char*const& str) const;
  // ICC don't know using
  int                      compareTo(const Playable& r) const { return InstanceCompareable<Playable>::compareTo(r); }
  // Seek whether an URL is already loaded.
  static int_ptr<Playable> FindByURL(const char* url);
  // FACTORY! Get a new or an existing instance of this URL.
  // The optional parameters ca_* are preloaded informations.
  // This is returned by the apropriate Get* functions without the need to access the underlying data source.
  // This is used to speed up large playlists.
  static int_ptr<Playable> GetByURL(const url123& URL, const FORMAT_INFO2* ca_format = NULL, const TECH_INFO* ca_tech = NULL, const META_INFO* ca_meta = NULL);
};
// Flags Attribute for StatusFlags
FLAGSATTRIBUTE(Playable::InfoFlags);


inline Playable::InfoFlags Playable::CheckInfo(InfoFlags what)
{ return what & ~InfoValid;
}


// Unique sorted set of Playable objects
struct PlayableSet
: public sorted_vector<Playable, Playable>,
  public IComparableTo<PlayableSet>
{ static const PlayableSet Empty; // empty instance
                           PlayableSet();
  virtual int              compareTo(const PlayableSet& r) const;
  #ifdef DEBUG
  xstring                  DebugDump() const;
  #endif
};


/* Class representing exactly one song.
 */
class Song : public Playable
{public:
  Song(const url123& URL, const FORMAT_INFO2* ca_format = NULL, const TECH_INFO* ca_tech = NULL, const META_INFO* ca_meta = NULL)
   : Playable(URL, ca_format, ca_tech, ca_meta) { DEBUGLOG(("Song(%p)::Song(%s, %p, %p, %p)\n", this, URL.cdata(), ca_format, ca_tech, ca_meta)); }

  virtual InfoFlags        LoadInfo(InfoFlags what);
};


#endif

