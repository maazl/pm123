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

#include <cpp/event.h>
//#include <utilfct.h>
#include <cpp/mutex.h>
#include <cpp/smartptr.h>
#include <cpp/xstring.h>
#include <cpp/queue.h>
#include <cpp/cpputil.h>
#include <cpp/container.h>
#include <cpp/url123.h>

#include <stdlib.h>
#include <time.h>

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
  enum InfoFlags // must be aligned to InfoType
  { IF_None     = 0,
    IF_Format   = 0x01,// applies to GetInfo().format
    IF_Tech     = 0x02,// applies to GetInfo().tech
    IF_Meta     = 0x04,// applies to GetInfo().meta
    IF_Phys     = 0x08,// applies to GetInfo().phys
    IF_Rpl      = 0x10,// applies to GetInfo().rpl
    IF_Other    = 0x20,// applies to GetDecoder() and PlayableCollection::GetNext()
    IF_Status   = 0x40,// applies to GetStatus() and IsModified()
    IF_All      = IF_Format|IF_Tech|IF_Meta|IF_Phys|IF_Rpl|IF_Other|IF_Status
  };
  // Parameters for InfoChange Event
  struct change_args
  { Playable&         Instance;
    InfoFlags         Flags; // Bitvector of type InfoFlags
    change_args(Playable& inst, InfoFlags flags) : Instance(inst), Flags(flags) {}
  };
  class Lock;
  friend class Lock;
  class Lock
  { Playable&  P;   // Playable object
   private:    // non-copyable
               Lock(const Lock&);
    void       operator=(const Lock&);
   public:
               Lock(Playable& p) : P(p)      { p.Mtx.Request(); }
               ~Lock();
  };
 protected:
  // C++ version of DECODER_INFO2
  class DecoderInfo : public DECODER_INFO2
  {private:
    FORMAT_INFO2      Format;
    TECH_INFO         TechInfo;
    META_INFO         MetaInfo;
    PHYS_INFO         PhysInfo;
    RPL_INFO          RplInfo;
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
  char                Decoder[13];     // 12345678.123\0 (OS/2 module names cannot exceed 8.3) 
  InfoFlags           InfoValid;       // Bitvector of type InfoFlags
  InfoFlags           InfoChangeFlags; // Bitvector with stored events
 private: // ... except for this ones
  volatile unsigned   InfoRequest;     // Bitvector with requested information
  volatile unsigned   InfoRequestLow;  // Bitvector with requested low priority information
  volatile unsigned   InService;       // 1 = unused, 0 = in service by a worker, <0 queue entries skipped
  Mutex               Mtx;             // protect this instance

 private: // non-copyable
  Playable(const Playable&);
  void operator=(const Playable&);
 protected:
  // Create Playable object. Preset some info types according to ca.
  // Each infotype in ca may be NULL, indicating that the desired information is not yet known.
  // For non NULL info blocks in ca the apropriate bits in InfoValid are set.
  Playable(const url123& url, const DECODER_INFO2* ca = NULL);
  // Update the structure components and return the required InfoChange Flags or 0 if no change has been made.
  // Calling this Functions with NULL resets the Information to its default value.
  // This does not reset the InfoValid bits.
  void                UpdateInfo(const FORMAT_INFO2* info);
  void                UpdateInfo(const TECH_INFO* info);
  void                UpdateInfo(const META_INFO* info);
  void                UpdateInfo(const PHYS_INFO* info);
  void                UpdateInfo(const RPL_INFO* info);
  // Update all kind of information.
  void                UpdateInfo(const DECODER_INFO2* info, InfoFlags what);
  void                UpdateStatus(PlayableStatus stat);
 private:
  // Raise the InfoChange event if required.
  // This function is automatically called before releasing the Mutex by the current thread.
  // Normally 
  void                RaiseInfoChange();
 public:
  virtual             ~Playable();
  // Check whether a given URL is to be initialized as playlist.
  static bool         IsPlaylist(const url123& URL);
  // Get URL
  const url123&       GetURL() const      { return URL; }
  // RTTI by the back door. (dynamic_cast<> would be much nicer, but this is not supported by icc 3.0.)
  virtual Flags       GetFlags() const;
  // Check whether the current instance is locked by the current thread.
  bool                IsMine() const      { return Mtx.GetStatus() > 0; }
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
  inline InfoFlags    CheckInfo(InfoFlags what) const; // can't define here because of missing FLAGSATTRIBUTE(InfoFlags).
  // This function is called to populate the info fields.
  // The parameter what is the requested information. The function returns the retrieved information.
  // The retrieved information must not be less than the requested information. But it might be more.
  // This is a mutable Function when the info-fields are not initialized yet (late initialization).
  // Normally this function is called automatically. However, you may want to force a synchronuous refresh.
  // When overiding this function you MUST call Playable::LoadInfo with the flags you want to return at last.
  // This atomically resets the matching bits in the InfoRequest fields and sets them in InfoValid.
  // Playable::LoadInfo is abstract but implemented. 
  virtual InfoFlags   LoadInfo(InfoFlags what) = 0;
  // Ensure that the info fields are loaded from the ressource.
  // This function may block until the data is available.
  // To be reliable you shound be in synchronized context.
  void                EnsureInfo(InfoFlags what);
  // Call LoadInfo asynchronuously. This is a service of Playable.
  // When the requested information is availabe the InfoChange event is raised.
  // But this is done only if the information really has changed.
  // Of course yo have to register the eventhandler before the call to LoadInfoAsync.
  // The parameter lowpri shedules the rquest at low priority.
  // This is useful for prefetching.
  void                LoadInfoAsync(InfoFlags what, bool lowpri = false);
  // Call LoadInfo asynchronuously if the requested information is not yet available.
  // The function returns the flags of the requested information that is immediately available.
  // For all the missing bits you should use the InfoChange event to get the requested information.
  // If no information is yet available, the function retorns IF_None.
  // Of course yo have to register the eventhandler before the call to EnsureInfoAsync.
  // If EnsureInfoAsync returned true the InfoChange event is not raised.
  // The parameter lowpri shedules the rquest at low priority.
  // This is useful for prefetching.
  InfoFlags           EnsureInfoAsync(InfoFlags what, bool lowpri = false);

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
 public: // VAC++ 3.0 requires base types to be public
  typedef int_ptr<Playable> QEntry;
 private:
  // Priority enhanced Queue
  class worker_queue : public queue<QEntry>
  { EntryBase* HPTail; // Tail of high priority items
   public:
    // Remove item from queue
    void       CommitRead(qentry* qp);
    // Write item with priority
    void       Write(const QEntry& data, bool lowpri);
    void       Purge()     { queue<QEntry>::Purge(); HPTail = NULL; }
    #ifdef DEBUG
    // Read the current head
    void       DumpQ() const;
    qentry*    RequestRead() { DumpQ(); return queue<QEntry>::RequestRead(); }
    #endif
  };

 private:
  static worker_queue      WQueue;
  static int*              WTids;         // Thread IDs of the workers (type int[])
  static size_t            WNumWorkers;   // number of workers in the above list
  static bool              WTermRq;       // Termination Request to Worker

 private:
  // Worker thread (free function because of IBM VAC restrictions)
  friend void TFNENTRY PlayableWorker(void*);
 public:
  // Initialize worker
  static void              Init();
  // Destroy worker
  static void              Uninit();

 // Repository
 private:
  static sorted_vector<Playable, const char*> RPInst;
  static Mutex             RPMutex;
  static clock_t           LastCleanup;   // Time index of last cleanup run
  clock_t                  LastAccess;    // Time index of last access to this instance (used by Cleanup)
 private:
  #ifdef DEBUG
  static void              RPDebugDump();
  #endif
  static void              DetachObjects(const vector<Playable>& list);
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
  static int_ptr<Playable> GetByURL(const url123& URL, const DECODER_INFO2* ca = NULL);
  // Cleanup unused items from the repository
  // One call to Cleanup deletes all unused items that are not requested since the /last/ call to Cleanup.
  // So the distance between the calls to Cleanup defines the minimum cache lifetime.
  static void              Cleanup();
  // clears the repository
  // Used items may stay alive until their reference count goes to zero.
  static void              Clear();     
};
// Flags Attribute for StatusFlags
FLAGSATTRIBUTE(Playable::InfoFlags);


inline Playable::InfoFlags Playable::CheckInfo(InfoFlags what) const
{ return what & ~InfoValid;
}


// Unique sorted set of Playable objects
// This class does not take ownership of the Playable objects.
// So you have to ensure that the Playable objects are held by another int_ptr instance
// as long as they are in this collection.
struct PlayableSet
: public sorted_vector<Playable, Playable>,
  public IComparableTo<PlayableSet>
{ static const PlayableSet Empty; // empty instance
                           PlayableSet();
  #ifdef DEBUG
                           ~PlayableSet() { DEBUGLOG(("PlayableSet(%p)::~PlayableSet()\n", this)); }
  xstring                  DebugDump() const;
  #endif
  virtual int              compareTo(const PlayableSet& r) const;
  // returns true if and only if all elements in this set are also in r.
  bool                     isSubsetOf(const PlayableSet& r) const;
};


/* Class representing exactly one song.
 */
class Song : public Playable
{public:
  Song(const url123& URL, const DECODER_INFO2* ca = NULL);

  virtual InfoFlags        LoadInfo(InfoFlags what);
};


#endif

