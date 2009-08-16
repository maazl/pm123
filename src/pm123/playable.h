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
#include <cpp/mutex.h>
#include <cpp/smartptr.h>
#include <cpp/xstring.h>
#include <cpp/queue.h>
#include <cpp/cpputil.h>
#include <cpp/container/sorted_vector.h>
#include <cpp/container/inst_index.h>
#include <cpp/url123.h>

#include <stdlib.h>
#include <time.h>

#include <decoder_plug.h>


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
: public Iref_count,
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
    IF_Other    = 0x20,// applies to GetStatus(), GetDecoder() and PlayableCollection::GetNext()
    IF_Usage    = 0x40,// applies to IsInUse() and IsModified()
                       // This flag is for events only. Usage information is always available. 
    IF_All      = IF_Format|IF_Tech|IF_Meta|IF_Phys|IF_Rpl|IF_Other
  };
  CLASSFLAGSATTRIBUTE(Playable::InfoFlags);
  // Status of Playable Objects and PlayableInstance.
  enum Status
  { STA_Unknown, // The Status of the object is not yet known
    STA_Invalid, // the current object cannot be used
    STA_Valid    // The object is valid
  };

  // Parameters for InfoChange Event
  // An Event with Change and Load == IF_None is fired just when the playable instance dies.
  // Eventhandlers should not access instance in this case.
  struct change_args
  { Playable&         Instance; // Related Playable object.
    InfoFlags         Changed;  // Bitvector with changed information.
    InfoFlags         Loaded;   // Bitvector with loaded information. This may not contain bits not in Change.
    change_args(Playable& inst, InfoFlags changed, InfoFlags loaded) : Instance(inst), Changed(changed), Loaded(loaded) {}
  };

  // Class to lock Playable Object against modification.
  // This Object must not be held for a longer time. The must not be done any I/O in between.
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

  // Class to wait for a desired information.
  // This is something like a conditional variable.
  class WaitInfo
  {private:
    InfoFlags  Filter;
    Event      EventSem;
    class_delegate<WaitInfo, const change_args> Deleg;
   private:
    void       InfoChangeEvent(const change_args& args);
   public:
    // Create a WaitInfo Semaphore that is posted once all information specified by Filter
    // is loaded.
    // To be safe the constructor of this class must be invoked while the mutex
    // of the Playable object is held.
               WaitInfo(Playable& inst, InfoFlags filter);
               ~WaitInfo();
    // Wait until all requested information is loaded or an error occurs.
    // the function returns false if the given time elapsed or the Playable
    // object died.
    bool       Wait(long ms = -1)         { EventSem.Wait(ms); return Filter == 0; }
  };
  
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
  
  typedef char        DecoderName[13];    // 12345678.123\0 (OS/2 module names cannot exceed 8.3) 

 private:
  const url123        URL;
  Mutex               Mtx;                // protect this instance
 private: // The following vars are protected by the mutex
  DecoderInfo         Info;
  Status              Stat;
  bool                InUse;
  DecoderName         Decoder;
  // The fields InfoChange and InfoLoaded hold the flags for the next InfoChange event.
  // They are collected until OnReleaseMutex is called which raises the event.
  // Valid combinations of bits for each kind of information.
  // Loaded | Changed | Meaning
  // =======|=========|=================================================================
  //    0   |    0    | Information is untouched since the last InfoChange event.
  //    1   |    0    | Information has been updated but the value did not change.
  //    0   |    1    | INVALID! - InfoLoaded never contains less bits than InfoChanged.
  //    1   |    1    | Information has been updated and changed. 
  InfoFlags           InfoChanged;     // Bitvector with stored events
  InfoFlags           InfoLoaded;      // Bitvector with recently updated information
  // The bits in InfoMask are enabled to be raised at OnReleaseMutex.
  InfoFlags           InfoMask;
 private: // ... except for this ones
  // The InfoValid InfoConfirmed bit voctors defines the reliability status of the
  // certain information components. See InfoFlags for the kind of information.
  // Valid | Confirmed | Meaning
  // ======|===========|=================================================================
  //   0   |     0     | The information is not available.
  //   1   |     0     | The information is available but may be outdated.
  //   0   |     1     | INVALID!
  //   1   |     1     | The information is confirmed.
  // Bits in InfoValid are never reset. So reading a set bit without locking the mutex is safe.
  InfoFlags           InfoValid;
  InfoFlags           InfoConfirmed;
  // The Fields InfoRequest, InfoRequestLow and InfoInService define the request status of the information.
  // The following bit combinations are valid for each kind of information.
  // Rq.Low | Request | InService | Meaning
  // =======|=========|===========|===============================================================
  //    0   |    0    |     0     | The Information is stable.
  //    1   |    0    |     0     | The Information is requested asynchroneously at low priority.
  //    x   |    1    |     0     | The Information is requested asynchroneously at high priority.
  //    x   |    x    |     1     | The Information is currently retrieved.
  volatile unsigned   InfoRequest;     // Bitvector with requested information
  volatile unsigned   InfoRequestLow;  // Bitvector with requested low priority information
  // The InfoInService Bits protect the specified information. While a bit is set
  // the according information must not be modified by anything else but the thread
  // that set the bit. This is the same than an array of mutextes, but does not eat
  // that nuch resources.
  volatile unsigned   InfoInService;   // Bitvector with currently retrieving information   

 private: // non-copyable
  Playable(const Playable&);
  void operator=(const Playable&);
 protected:
  // Create Playable object. Preset some info types according to ca.
  // Each infotype in ca may be NULL, indicating that the desired information is not yet known.
  // For non NULL info blocks in ca the apropriate bits in InfoValid are set.
  Playable(const url123& url, const DECODER_INFO2* ca = NULL);
  // Request informations for update. This sets the apropriate bits in InfoInService.
  // The function returns the bits /not/ previously set. The caller must not update
  // other information than the returned bits.
  // It is valid to extend an active update by another call to BeginUpdate with additional flags.
  InfoFlags           BeginUpdate(InfoFlags what);
  // Completes the update requested by BeginUpdate.
  // The parameter what /must/ be the return value of BeginUpdate from the same thread.
  // You also may split the EndUpdate for different kind of information.
  // However, you must pass disjunctive flags at all calls.
  void                EndUpdate(InfoFlags what);
  // Notify about the update of some kind of information.
  // If changed is true the InfoChanged bits are also set.
  // If confirmed is true the InfoConfirmed bits are also set.
  // The function must be called in synchronized context.
  void                ValidateInfo(InfoFlags what, bool changed, bool confirmed);
  // Invalidate some kind of information.
  // This does not reset the InfoValid bits, but it resets the InfoConfirmed bits.
  // The reload parameter forces the invalidated information to be reloaded at low priority.
  void                InvalidateInfo(InfoFlags what, bool reload = false);
  // Update the structure components and set the required InfoChange Flags or 0 if no change has been made.
  // Calling this Functions with NULL resets the information to its default value.
  // This does not reset the InfoValid bits.
  // Once a playable object is constructed this function must not be called
  // unless the specified bits are successfully requested by BeginUpdate
  // and the current object is synchronized by Mtx.
  void                UpdateFormat(const FORMAT_INFO2* info, bool confirmed = true);
  void                UpdateTech(const TECH_INFO* info, bool confirmed = true);
  void                UpdateMeta(const META_INFO* info, bool confirmed = true);
  void                UpdatePhys(const PHYS_INFO* info, bool confirmed = true);
  void                UpdateRpl(const RPL_INFO* info, bool confirmed = true);
  void                UpdateOther(Status stat, bool meta_write, const char* decoder, bool confirmed = true);
  // Update all kind of information.
  void                UpdateInfo(Status stat, const DECODER_INFO2* info, const char* decoder, InfoFlags what, bool confirmed = true);
  void                UpdateInUse(bool inuse);
  // This function is called to populate the info fields.
  // The parameter what is the requested information. The function may retrieve more information
  // than requested. In this case you must call BeginUpdate for the additional bits
  // as soon as you know that you retrieve more information. And only if this call to
  // BeginUpdate grants you to update the desired information you are allowed to Update the
  // specified field by calling the appropriate UpdateInfo function.
  // If a part of the information retrieval is time-consuming and another part is not,
  // DoLoadInfo should call EndUpdate with the apropriate bits after the fast information
  // has been retrieved. 
  // The function must exactly return the updated information including additional bits,
  // that are granted by BeginUpdate, and excluding the bits already passed to EndUpdate.
  // In the easiest way no expicit calls to BeginUpdate and EndUpdate are required.
  virtual InfoFlags   DoLoadInfo(InfoFlags what) = 0;
 private:
  // This function is called by Playable::Lock::~Lock() when the Mutex Mtx is about to be released.
  void                OnReleaseMutex();
 public:
  virtual             ~Playable();
  // Check whether a given URL is to be initialized as playlist.
  static bool         IsPlaylist(const url123& URL);
  // Get URL
  const url123&       GetURL() const      { return URL; }
  // RTTI by the back door. (dynamic_cast<> would be much nicer, but this is not supported by icc 3.0.)
  virtual Flags       GetFlags() const;
  // Check whether the current instance is locked by the current thread.
  //bool                IsMine() const      { return Mtx.GetStatus() > 0; }
  // Return Status of the current object
  Status              GetStatus() const   { return Stat; }
  // Return true if the current object is marked as in use.
  bool                IsInUse() const     { return InUse; }
  // Mark the object as used (or not)
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
  InfoFlags           CheckInfo(InfoFlags what) const { return what & (InfoFlags)~InfoValid; }
  // This function Load some information immediately.
  // The parameter what is the requested information.
  void                LoadInfo(InfoFlags what);
  // Ensure that the info fields are loaded from the ressource.
  // This function may block until the data is available.
  // To be reliable you shound be in synchronized context.
  // If confirmed is true, only information that is well known to be valid
  // is returned. Information from some caching or that has been invalidated
  // will be retrieved anew.
  void                EnsureInfo(InfoFlags what, bool confirmed = false);
  // Call LoadInfo asynchronuously. This is a service of Playable.
  // When the requested information is availabe the InfoChange event is raised.
  // Note that requesting information not neccessarily causes a Changed flag to be set.
  // Only the Loaded bits are guaranteed.
  // Of course yo have to register the eventhandler before the call to LoadInfoAsync.
  // The parameter lowpri shedules the rquest at low priority.
  // This is useful for prefetching.
  void                LoadInfoAsync(InfoFlags what, bool lowpri = false);
  // Call LoadInfo asynchronuously if the requested information is not yet available.
  // The function returns the flags of the requested information that is immediately available.
  // If the parameter confirmed is true the information is also requested asynchroneously,
  // if it is currently only available as unconfirmed.
  // For all the missing bits you should wait for the matching InfoChange event
  // to get the requested information. Note that requesting information not neccessarily
  // causes a Changed flag to be set. Only the Loaded bits are guaranteed.
  // If no information is yet available, the function returns IF_None.
  // Of course yo have to register the eventhandler before the call to EnsureInfoAsync.
  // If EnsureInfoAsync returned all requested bits the InfoChange event is not raised
  // as result of this call. The parameter lowpri shedules the rquest at low priority.
  // This is useful for prefetching.
  InfoFlags           EnsureInfoAsync(InfoFlags what, bool lowpri = false, bool confirmed = false);

  // Query the current InfoRequest state. This is only reliable in a critical section.
  // But it may be called from outside for debugging or informational purposes.
  void                QueryRequestState(InfoFlags& high, InfoFlags& low, InfoFlags& inservice)
                      { high = (InfoFlags)InfoRequest; low = (InfoFlags)(InfoRequestLow & ~high); inservice = (InfoFlags)InfoInService; }

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
 public:
  typedef int_ptr<Playable> QEntry;
 private:
  // Priority enhanced Queue
  class worker_queue : public queue<QEntry>
  { EntryBase* HPTail;    // Tail of high priority items
    Event      HPEvEmpty; // Event for High priority items
   public:
    // Remove item from queue
    qentry*    RequestRead(bool lowpri)
               { return lowpri ? queue<QEntry>::RequestRead() : (qentry*)queue_base::RequestRead(HPEvEmpty, HPTail); }
    void       CommitRead(qentry* qp);
    void       RollbackRead(qentry* entry)
               { queue<QEntry>::RollbackRead(entry); HPEvEmpty.Set(); }
    // Write item with priority
    void       Write(const QEntry& data, bool lowpri);
    void       Purge()     { queue<QEntry>::Purge(); HPTail = NULL; }
    #ifdef DEBUG_LOG
    // Read the current head
    void       DumpQ() const;
    #endif
  };

 private:
  static worker_queue      WQueue;
  static int*              WTids;         // Thread IDs of the workers (type int[])
  static size_t            WNumWorkers;   // number of workers in the above list
  static size_t            WNumDlgWorkers;// number of workers in the above list
  static bool              WTermRq;       // Termination Request to Worker

 private:
  // Worker threads
  static void              PlayableWorker(bool lowpri);
  friend void TFNENTRY     PlayableWorkerStub(void* arg);
 public:
  // Initialize worker
  static void              Init();
  // Destroy worker
  static void              Uninit();
  // Inspect worker queue
  // The callback function is called once for each queue item. But be careful,
  // this is done from synchronized context.
  static void QueueTraverse(void (*action)(const queue<QEntry>::qentry& entry, void* arg), void* arg)
  { WQueue.ForEach(action, arg); }

 // Repository
 private:
  static sorted_vector<Playable, const char*> RPInst;
  static Mutex             RPMutex;
  static clock_t           LastCleanup;   // Time index of last cleanup run
  clock_t                  LastAccess;    // Time index of last access to this instance (used by Cleanup)
 private:
  #ifdef DEBUG_LOG
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


class PlayableSetBase
: public IComparableTo<PlayableSetBase>
{public:
  static const PlayableSetBase& Empty; // empty instance

 protected:
                           PlayableSetBase() {}
                           ~PlayableSetBase() {}
 public:
  #ifdef DEBUG_LOG
  xstring                  DebugDump() const;
  #endif
  virtual size_t           size() const = 0;
  virtual Playable*        operator[](size_t where) const = 0;
  virtual bool             contains(const Playable& key) const = 0;
  
  virtual int              compareTo(const PlayableSetBase& r) const;
  // returns true if and only if all elements in this set are also in r.
  bool                     isSubsetOf(const PlayableSetBase& r) const;
};

// Unique sorted set of Playable objects
// This class does not take ownership of the Playable objects!
// So you have to ensure that the Playable objects are held by another int_ptr instance
// as long as they are in this collection.
class PlayableSet
: public sorted_vector<Playable, Playable>,
  public PlayableSetBase
{public:
                           PlayableSet();
                           PlayableSet(const PlayableSetBase& r);
                           PlayableSet(const PlayableSet& r);
  #ifdef DEBUG_LOG
                           ~PlayableSet()
                           { DEBUGLOG(("PlayableSet(%p)::~PlayableSet()\n", this)); }
  #endif

  virtual size_t           size() const
                           { return sorted_vector<Playable, Playable>::size(); }
  virtual Playable*        operator[](size_t where) const
                           { return sorted_vector<Playable, Playable>::operator[](where); }
  virtual bool             contains(const Playable& key) const
                           { return sorted_vector<Playable, Playable>::find(key) != NULL; }
};

// Unique sorted set of Playable objects.
// The ownership of the content is held by this class.
class OwnedPlayableSet
: public sorted_vector_int<Playable, Playable>,
  public PlayableSetBase
{public:
                           OwnedPlayableSet();
                           OwnedPlayableSet(const PlayableSetBase& r);
                           OwnedPlayableSet(const OwnedPlayableSet& r);
  #ifdef DEBUG_LOG
                           ~OwnedPlayableSet()
                           { DEBUGLOG(("OwnedPlayableSet(%p)::~OwnedPlayableSet()\n", this)); }
  #endif

  virtual size_t           size() const
                           { return sorted_vector_int<Playable, Playable>::size(); }
  virtual Playable*        operator[](size_t where) const
                           { return sorted_vector_int<Playable, Playable>::operator[](where); }
  virtual bool             contains(const Playable& key) const
                           { return sorted_vector_int<Playable, Playable>::find(key) != NULL; }
};

/* Class representing exactly one song.
 */
class Song : public Playable
{protected:
  virtual InfoFlags        DoLoadInfo(InfoFlags what);
 public:
  Song(const url123& URL, const DECODER_INFO2* ca = NULL);
  // Save meta info, return result from dec_saveinfo
  ULONG                    SaveMetaInfo(const META_INFO& info, int haveinfo);
};


#endif

