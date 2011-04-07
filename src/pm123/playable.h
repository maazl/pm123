/*  
 * Copyright 2007-2011 Marcel Müller
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

#include <config.h>
#include "aplayable.h"
#include "playableref.h"
#include "collectioninfocache.h"

#include <cpp/mutex.h>
#include <cpp/smartptr.h>
#include <cpp/cpputil.h>
#include <cpp/container/inst_index.h>
#include <cpp/container/sorted_vector.h>
#include <cpp/container/list.h>
#include <cpp/url123.h>

#include <time.h>


/** @brief Instance of a PlayableRef within a laylist.
 *
 * Calling non-constant methods unsynchronized will not cause the application
 * to have undefined behavior. It will only cause the PlayableInstance to have a
 * non-deterministic but valid state.
 * The relationship of a PlayableInstance to it's parent container is weak.
 * That means that the PlayableInstance may be detached from the container before it dies.
 * A detached PlayableInstance will die as soon as it is no longer used. It may not be
 * reattached to a container.
 * That also means that there is no way to get a reference to the actual parent,
 * because once you got the reference it might immediately get invalid.
 * And to avoid this, the parent must be locked, which is impossible to ensure, of course.
 * All you can do is to verify whether the current PlayableInstance belongs to a given parent.
 */
class PlayableInstance : public PlayableRef
{public:
  typedef int (*Comparer)(const PlayableInstance& l, const PlayableInstance& r);

 protected:
  /// Weak reference to the parent Collection.
  const Playable* volatile Parent;
  /// Index within the collection.
  int                      Index;

 private: // non-copyable
  PlayableInstance(const PlayableInstance& r);
  void operator=(const PlayableInstance& r);
 protected:
  PlayableInstance(const Playable& parent, APlayable& refto);
 public:
  //virtual                  ~PlayableInstance() {}
  /// Check if this instance (still) belongs to a collection.
  /// The return value true is not reliable unless the collection is locked.
  /// Calling this method with NULL will check whether the instance does no longer belong to a collection.
  /// In this case only the return value true is reliable.
  #ifdef DEBUG_LOG
  virtual bool             HasParent(const Playable* parent) const
                           { if (Parent == parent)
                               return true;
                             DEBUGLOG(("PlayableInstnace(%p)::HasParent(%p) - %p\n", this, parent, Parent));
                             return false;
                           }
  #else
  virtual bool             HasParent(const Playable* parent) const
                           { return Parent == parent; }
  #endif

  /// Returns the index within the current collection counting from 1.
  /// A return value of 0 indicates the the instance does no longer belong to a collection.
  /// The return value is only reliable while the collection is locked.
  int                      GetIndex() const    { return Index; }

  /*// Swap instance properties (fast)
  // You must not swap instances that belong to different PlayableCollection objects or
  // refer to different objects.
  // The collection must be locked when calling Swap.
  // Swap does not swap the current status.
          void             Swap(PlayableRef& r);*/

  // Compare two PlayableInstance objects by value.
  // Two instances are equal if on only if they belong to the same PlayableCollection,
  // share the same Playable object (=URL) and have the same properties values for alias and slice.
  //friend bool              operator==(const PlayableInstance& l, const PlayableInstance& r);

  // Return the logical ordering of two Playableinstances.
  // 0       = equal
  // <0      = l before r
  // >0      = l after r
  // INT_MIN = unordered (The PlayableInstances do not belong to the same collection.)
  // Note that the result is not reliable unless you hold the mutex of the parent.
  //int                      CompareTo(const PlayableInstance& r) const;
};


/** @brief Class to support any playable object.
 *
 * This class instances are unique for each URL.
 * Comparing two pointers for equality is equivalent to comparing the URL for equality.
 *
 * All instances of this class are owned by the class itself and must not be deleted.
 * You should only hold instances in the smart pointer \c int_ptr<Playable>. Otherwise
 * the referred object may be deleted in background. But it is allowed to call
 * a function with parameter \c Playable* (e.g. member functions) while you have an active
 * instance of \c int_ptr<Playable> unless the function keeps a local copy of the pointer
 * after it returns. It is also allowed to pass the result of a function returning
 * \c int_ptr<Playable> directly to a function taking \c Playable* with the same restriction
 * because temporary objects are always kept until the next statement.
 * If a function that receives \c Playable* wants to keep the reference after returning
 * it must create a \c int_ptr<Playable> object. This will work as expected.
 */
class Playable
: public APlayable,
  public InstanceCompareable<Playable>,
  public IComparableTo<const char*>
{public:
  /// Options for \c SerializeItem
  enum SerializationOptions
  { SO_Default              = 0x00,
    SO_RelativePath         = 0x01,
    SO_UseUpdir             = 0x02,
    SO_IndexOnly            = 0x10
  };
  /// Comparer for \c Sort
  typedef int DLLENTRYP(ItemComparer)(const PlayableInstance* l, const PlayableInstance* r);
 private:
  class MyInfo
  : public DecoderInfo,
    public CollectionInfo,
    public INFO_BUNDLE_CV
  {private:
    static const ItemInfo   Item;           // always empty!
   public:
    MyInfo();
  };

  /// Internal representation of a PlayableInstance as linked list item.
  class Entry 
  : public PlayableInstance,
    public list_int<Entry>::entry_base
  {public:
    typedef class_delegate<Playable, const PlayableChangeArgs> IDType;
   private:
    IDType                  InstDelegate;
   public:
    Entry(Playable& parent, APlayable& refto, IDType::func_type ifn);
    virtual                 ~Entry()         { DEBUGLOG(("Playable::Entry(%p)::~Entry()\n", this)); }
    // Activate the event handlers
    void                    Attach()         { DEBUGLOG(("Playable::Entry(%p)::Attach()\n", this)); ASSERT(Parent); GetInfoChange() += InstDelegate; }
    // Detach a PlayableInstance from the collection.
    // This function must be called only by the parent collection and only while it is locked.
    void                    Detach()         { DEBUGLOG(("Playable::Entry(%p)::Detach()\n", this)); InstDelegate.detach(); Parent = NULL; Index = 0; }
    // Update Index in Collection
    void                    SetIndex(int index) { Index = index; }
  };

  class PlaylistData : public CollectionInfoCache
  {public:
    list_int<Entry>         Items;
   public:
    PlaylistData(Playable& parent) : CollectionInfoCache(parent) {}
  };

 public:
  /// The unique URL of this playable object.
  const url123              URL;
  Mutex                     Mtx;             // protect this instance
 private: // The following vars are protected by the mutex
  MyInfo                    Info;
  bool                      Modified;        // Current object has unsaved changes.
  sco_ptr<PlaylistData>     Playlist;        // Playlist informations, optional.
 private: // ... except for this ones. They are accessed atomically.
  // Internal state
  bool                      InUse;           // Current object is in use.

 private: // Services to update the Info* variables.
  /// @brief Update the structure components and set the required InfoLoaded flags.
  /// @return return the information that has changed.
  /// @details Once a playable object is constructed this function must not be called
  /// unless the specified bits are successfully requested by \c BeginUpdate
  /// and the current object is synchronized by \c Mtx.
  InfoFlags                 UpdateInfo(const INFO_BUNDLE& info, InfoFlags what);
  /// Raise InfoChange event
  void                      RaiseInfoChange(InfoFlags loaded, InfoFlags changed);
 private: // Services for playlist functions
  void                      EnsurePlaylist();
  /// Create an new Playlist item.
  Entry*                    CreateEntry(APlayable& refto);
  /// @brief Insert a new entry into the list.
  /// @remarks The list must be locked when calling this Function.
  void                      InsertEntry(Entry* entry, Entry* before);
  /// @brief Move a entry inside the list.
  /// @return The function returns false if the move is a no-op.
  /// @remarks The list must be locked when calling this Function.
  bool                      MoveEntry(Entry* entry, Entry* before);
  /// @brief Remove an entry from the list.
  /// @remarks The list must be locked before calling this Function.
  void                      RemoveEntry(Entry* entry);
  /// @brief Renumber the entries in the range [from,to[ starting with \a index.
  /// @details If \a to is NULL all remaining entries are renumbered.
  /// @remarks The list must be locked before calling this Function.
  void                      RenumberEntries(Entry* from, const Entry* to, int index);

  /// Calculate the recursive playlist information.
  /// @param cie Information to calculate.
  /// @param upd Information to obtain. Only \c IF_Aggreg flags are handled by this method.
  /// After completion the retrieved information in \a upd is comitted.
  /// @param events If an information is successfully retrieved the appropriate bits in \c events.Loaded should be set.
  /// If this caused a change the \c events.Changed bits should be set too.
  /// @param job Schedule requests to other objects that are required to complete this request at priority \c job.Pri.
  /// If a information from another object is required to complete the request these objects should be added
  /// to the dependency list by calling \c job.Depends.Add. This causes a reschedule of the request,
  /// once the dependencies are completed.
  void                      CalcRplInfo(CollectionInfo& cie, InfoState::Update& upd, PlayableChangeArgs& events, JobSet& job);
  /// @brief Replace list of children
  /// @details This function does a smart update of the current collection and return \c true
  /// if this actually caused a change in the list of children.
  /// It effectively replaces the whole content by the content of \a newcontent.
  /// However, to keep existing iterators valid as far as possible and the change event cascades
  /// small, it looks for identical or similar objects in the current collection content first.
  /// The content of the vector is destroyed in general.
  /// The function must be called from synchronized context.
  bool                      UpdateCollection(const vector<PlayableRef>& newcontent);
  /// InfoChange Events from the children
  void                      ChildChangeNotification(const PlayableChangeArgs& args);

 private:
  virtual InfoFlags         DoRequestInfo(InfoFlags& what, Priority pri, Reliability rel);
  virtual AggregateInfo&    DoAILookup(const PlayableSetBase& exclude);
  virtual InfoFlags         DoRequestAI(AggregateInfo& ai, InfoFlags& what, Priority pri, Reliability rel);
  virtual void              DoLoadInfo(JobSet& job);
  // Callback function of dec_fileinfo
  PROXYFUNCDEF void DLLENTRY Playable_DecoderEnumCb(void* param, const char* url,
                            const INFO_BUNDLE* info, int cached, int override );

 private:
  /// @brief Create Playable object.
  /// @details Use Factory \c GetByURL to create Playable objects.
  Playable(const url123& url);
  /// Implement \c GetOverridden private, because it makes no sense for \c Playable.
  virtual InfoFlags         GetOverridden() const;
 public:
  virtual                   ~Playable();

  /// Display name
  /// @return This returns either Info.meta.title or the object name of the current URL.
  /// Keep in mind that this may not return the expected value unless \c RequestInfo(IF_Meta) has been called.
  /// Thread-safe but only reliable in synchronized context.
  virtual xstring           GetDisplayName() const;

  virtual const INFO_BUNDLE_CV& GetInfo() const;

  /// Return true if the current object is marked as in use.
  virtual bool              IsInUse() const;
  /// Mark the object as used (or not)
  virtual void              SetInUse(bool used);
  /// Check whether the current Collection is a modified shadow of an unmodified backend.
  bool                      IsModified() const { return Modified; }

  /// @brief Provides cached information about the current object.
  /// @details The information is taken over if and only if the specified information is currently in virgin state.
  void                      SetCachedInfo(const INFO_BUNDLE& info, InfoFlags what);

  /// Set meta information.
  /// @param meta Calling this function with NULL deletes the meta information.
  void                      SetMetaInfo(const META_INFO* meta);
 
  // Iterate over the collection. While the following functions are atomic and therefore thread-safe
  // the iteration itself is not because the collection may change at any time.
  // Calling these function is not valid until the information IF_Other is loaded.
  /// Get previous item of this collection. Passing NULL will return the last item.
  /// @return The function returns NULL if there are no more items.
  int_ptr<PlayableInstance> GetPrev(const PlayableInstance* cur) const;
  /// Get next item of this collection. Passing NULL will return the first item.
  /// @return The function returns NULL if there are no more items.
  int_ptr<PlayableInstance> GetNext(const PlayableInstance* cur) const;
  /// Returns a string that represents a serialized version of a sub item.
  xstring                   SerializeItem(const PlayableInstance* item, SerializationOptions opt) const;

 public: // Editor functions
  /// Insert a new item before the item \a before.
  /// @param before If the parameter before is \c NULL the item is appended.
  /// @return The function fails with returning \c NULL if the PlayableInstance before is no longer valid
  /// or the update flags cannot be locked. Otherwise it returns the newly created \c PlayableInstance.
  int_ptr<PlayableInstance> InsertItem(APlayable& item, PlayableInstance* before = NULL);
  /// Move an item inside the list.
  /// @param before If the parameter before is \c NULL the item is moved to the end.
  /// @return The function fails with returning false if one of the PlayableInstances is no longer valid
  /// or the update flags cannot be locked.
  bool                      MoveItem(PlayableInstance* item, PlayableInstance* before);
  /// Remove an item from the playlist.
  /// @return The function fails with returning false if the PlayableInstance is no longer valid
  /// or the update flags cannot be locked.
  bool                      RemoveItem(PlayableInstance* item);
  /// Remove all items from the playlist.
  /// @return The function returns \c false if the update flags cannot be locked.
  bool                      Clear();
  /// Sort all items with a comparer.
  /// @return The function returns \c false if the update flags cannot be locked.
  bool                      Sort(ItemComparer comp);
  /// Randomize record sequence.
  /// @return The function returns \c false if the update flags cannot be locked.
  bool                      Shuffle();

 private: // Internal dispatcher functions
  virtual const Playable&   DoGetPlayable() const;
  // Revoke visibility of GetPlayable, since calling GetPlayable on type Playable
  // is always a no-op. So the compiler will tell us.
  void                      GetPlayable() const;
 private: // Explicit interface implementations
  virtual int_ptr<Location> GetStartLoc() const;
  virtual int_ptr<Location> GetStopLoc() const;

 // Repository
 private:
  static  sorted_vector<Playable, const char*> RPInst;
  static  Mutex             RPMutex;
  static  clock_t           LastCleanup;   // Time index of last cleanup run
          clock_t           LastAccess;    // Time index of last access to this instance (used by Cleanup)
 private:
  #ifdef DEBUG_LOG
  static  void              RPDebugDump();
  #endif
  static  void              DetachObjects(const vector<Playable>& list);
 public:
  virtual int               compareTo(const char*const& str) const;
  // ICC don't know using
  int                       compareTo(const Playable& r) const { return InstanceCompareable<Playable>::compareTo(r); }
  // Seek whether an URL is already loaded.
  static  int_ptr<Playable> FindByURL(const char* url);
  // FACTORY! Get a new or an existing instance of this URL.
  // The optional parameters ca_* are preloaded informations.
  // This is returned by the appropriate Get* functions without the need to access the underlying data source.
  // This is used to speed up large playlists.
  static  int_ptr<Playable> GetByURL(const url123& URL);
  // Cleanup unused items from the repository
  // One call to Cleanup deletes all unused items that are not requested since the /last/ call to Cleanup.
  // So the distance between the calls to Cleanup defines the minimum cache lifetime.
  static  void              Cleanup();
  // Destroy worker
  static  void              Uninit();
};


// Due to the nature of the repository comparing instances is equivalent
// to comparing the pointers, because the key of different instances
// MUST be different and there are no two instances with the same key.
inline bool operator==(const Playable& l, const Playable& r)
{ return &l == &r;
}
inline bool operator!=(const Playable& l, const Playable& r)
{ return &l != &r;
}


#endif

