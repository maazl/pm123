/*  
 * Copyright 2007-2012 Marcel Mueller
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
#include "playableinstance.h"
#include "collectioninfocache.h"

#include <cpp/mutex.h>
#include <cpp/smartptr.h>
#include <cpp/cpputil.h>
#include <cpp/container/inst_index.h>
#include <cpp/container/btree.h>
#include <cpp/container/list.h>
#include <cpp/url123.h>

#include <time.h>


enum CollectionChangeType
{ PCT_None,                // No items are added/removed from the collection
  PCT_Insert,              // Item just inserted
  PCT_Move,                // Item has just moved
  PCT_Delete,              // Item just deleted
  PCT_All                  // The collection has entirely changed
};

struct CollectionChangeArgs : PlayableChangeArgs
{ CollectionChangeType        Type;
  CollectionChangeArgs(APlayable& inst, APlayable* orig, InfoFlags loaded, InfoFlags changed, InfoFlags invalidated)
                                             : PlayableChangeArgs(inst, orig, loaded, changed, invalidated), Type(PCT_None) {}
  CollectionChangeArgs(APlayable& inst, InfoFlags loaded, InfoFlags changed)
                                             : PlayableChangeArgs(inst, loaded, changed), Type(PCT_None) {}
  CollectionChangeArgs(APlayable& inst)      : PlayableChangeArgs(inst), Type(PCT_None) {}
  CollectionChangeArgs(APlayable& inst, CollectionChangeType type, APlayable* item, InfoFlags loaded, InfoFlags changed)
  : PlayableChangeArgs(inst, item, loaded, changed, IF_None), Type(type) {}
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
: public APlayable
//,  public inst_index<Playable,xstring,&xstring::compare>
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
    static const ItemInfo   Item;            // always empty!
   public:
    MyInfo();
    ~MyInfo()                                { if (item != &Item) delete item; }
    /// @brief Override the item information of this instance.
    /// @param newitem This item information replaces the current.
    /// @ The structure takes the ownership of \c *newitem.
    /// @remarks This function does not raise any changed events.
    /// Furthermore it is not intended to be used twice on the same object.
    void                    SetItem(ItemInfo* newitem) { ASSERT(item == &Item); item = newitem; }
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
    /// Activate the event handlers
    void                    Attach()         { DEBUGLOG(("Playable::Entry(%p)::Attach()\n", this)); ASSERT(Parent); GetInfoChange() += InstDelegate; }
    /// Detach a PlayableInstance from the collection.
    /// This function must be called only by the parent collection and only while it is locked.
    void                    Detach()         { DEBUGLOG(("Playable::Entry(%p)::Detach()\n", this)); InstDelegate.detach(); Parent = NULL; Index = 0; }
    /// Update Index in Collection
    #ifdef DEBUG_LOG
    void                    SetIndex(int index) { Index = index; InvalidateDebugName(); }
    #else
    void                    SetIndex(int index) { Index = index; }
    #endif
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

 private: // Services to update the Info* variables.
  /// Raise the \c InfoChange event and include dependent information in \c Changed and \c Invalidated.
  void                      RaiseInfoChange(CollectionChangeArgs& args);
  /// @brief Update the structure components.
  /// @return return the information that has changed.
  /// @remarks Once a playable object is constructed this function must not be called
  /// unless the specified bits are successfully requested by \c BeginUpdate
  /// and the current object is synchronized by \c Mtx.
  InfoFlags                 UpdateInfo(const INFO_BUNDLE& info, InfoFlags what);

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
  /// @return Returns true if the content were not successfully committed,
  /// i.e the info has been invalidated during calculation.
  bool                      CalcRplInfo(AggregateInfo& cie, InfoState::Update& upd, PlayableChangeArgs& events, JobSet& job);
  /// @brief Replace list of children
  /// @details This function does a smart update of the current collection and return \c true
  /// if this actually caused a change in the list of children.
  /// It effectively replaces the whole content by the content of \a newcontent.
  /// However, to keep existing iterators valid as far as possible and the change event cascades
  /// small, it looks for identical or similar objects in the current collection content first.
  /// The content of the vector is destroyed in general.
  /// The function must be called from synchronized context.
  bool                      UpdateCollection(const vector<APlayable>& newcontent);
  /// InfoChange Events from the children
  void                      ChildChangeNotification(const PlayableChangeArgs& args);

 private: // Implementation of abstract info worker API.
  virtual InfoFlags         DoRequestInfo(InfoFlags& what, Priority pri, Reliability rel);
  //virtual AggregateInfo&    DoAILookup(const PlayableSetBase& exclude);
  virtual InfoFlags         DoRequestAI(const PlayableSetBase& exclude, const volatile AggregateInfo*& ai,
                                        InfoFlags& what, Priority pri, Reliability rel);
  virtual void              DoLoadInfo(JobSet& job);
  virtual xstring           DoDebugName() const;
  /// Returns the decoder NAME that can play this file and returns 0
  /// if not returns error 200 = nothing can play that.
  ULONG                     DecoderFileInfo(InfoFlags& what, INFO_BUNDLE& info, void* param);
  PROXYFUNCDEF void DLLENTRY Playable_DecoderEnumCb(void* param, const char* url,
                            const INFO_BUNDLE* info, int cached, int override );

 private:
  /// @brief Create Playable object.
  /// @details Use Factory \c GetByURL to create Playable objects.
  Playable(const url123& url);
  /// Implement \c GetOverridden private, because it makes no sense for \c Playable.
  virtual InfoFlags         GetOverridden() const;
  /// Mark the object as modified (or not)
          void              SetModified(bool modified, APlayable* origin);

 public: // public API
  virtual                   ~Playable();

  /// Assign an alias name (like 'Bookmarks') for this entity.
  void                      SetAlias(const xstring& alias);

  /// Display name
  /// @return This returns either Info.meta.title or the object name of the current URL.
  /// Keep in mind that this may not return the expected value unless \c RequestInfo(IF_Meta) has been called.
  /// Thread-safe but only reliable in synchronized context.
  virtual xstring           GetDisplayName() const;

  virtual const INFO_BUNDLE_CV& GetInfo() const;

  /// Access the InfoChange event, but only the public part.
  event_pub<const CollectionChangeArgs>& GetInfoChange() { return (event_pub<const CollectionChangeArgs>&)APlayable::GetInfoChange(); }

  /// Mark the object as used (or not)
  virtual void              SetInUse(unsigned used);
  /// Check whether the current Collection is a modified shadow of an unmodified backend.
  bool                      IsModified() const { return Modified; }

  /// Invalidate some infos, but do not reload unless this is required.
  /// @param what The kind of information that is to be invalidated.
  /// @param source playable object that caused the invalidation.
  /// This is only significant when \a what contains \c IF_Aggreg.
  /// If \a source is \c NULL, all \c CollectionInfoCache entries are invalidated.
  /// @return Return the bits in what that really caused an information to be invalidated,
  /// i.e. that have been valid before.
  /// @remarks It might look that you get not the desired result if some consumer has registered
  /// to the invalidate event and requests the information as soon as it has been invalidated.
  virtual InfoFlags         Invalidate(InfoFlags what, const Playable* source = NULL);

  /// Access to request state for diagnostic purposes (may be slow).
  virtual void              PeekRequest(RequestState& req) const;

  /// @brief Provides cached information about the current object.
  /// @param cached kind of information that should be taken as cached.
  /// The information is taken over if and only if the specified information is currently in virgin state.
  /// @param reliable kind of information that is always updated (if not locked).
  /// @remarks Information with both bits in \a cached and \a reliable are set is ignored.
  void                      SetCachedInfo(const INFO_BUNDLE& info, InfoFlags cached, InfoFlags reliable);

  /// Set new meta information.
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
  /*// Returns a string that represents a serialized version of a sub item.
  xstring                   SerializeItem(const PlayableInstance* item, SerializationOptions opt) const;*/

 public: // Editor functions
  /// Change meta information of an object.
  /// @param meta Calling this function with NULL deletes the meta information.
  /// @return returns 0 on success.
  int                       SaveMetaInfo(const META_INFO& meta, DECODERMETA haveinfo);

  /// Insert a new item before the item \a before.
  /// @param item Item to insert.
  /// @param before If the parameter before is \c NULL the item is appended.
  /// @return The function fails with returning \c NULL if the PlayableInstance before is no longer valid
  /// or the update flags cannot be locked. Otherwise it returns the newly created \c PlayableInstance.
  int_ptr<PlayableInstance> InsertItem(APlayable& item, PlayableInstance* before = NULL);
  /// Move an item inside the list.
  /// @param item Item to move within this playlist.
  /// @param before If the parameter before is \c NULL the item is moved to the end.
  /// @return The function fails with returning false if one of the PlayableInstances is no longer valid
  /// or the update flags cannot be locked.
  /// @pre item must belong to this playlist.
  bool                      MoveItem(PlayableInstance& item, PlayableInstance* before);
  /// Remove an item from the playlist.
  /// @param item Item to remove from this playlist.
  /// @return The function fails with returning false if the PlayableInstance is no longer valid
  /// or the update flags cannot be locked.
  /// @pre item must belong to this playlist.
  bool                      RemoveItem(PlayableInstance& item);
  /// Remove all items from the playlist.
  /// @return The function returns \c false if the update flags cannot be locked.
  bool                      Clear();
  /// Sort all items with a comparer.
  /// @return The function returns \c false if the update flags cannot be locked.
  bool                      Sort(ItemComparer comp);
  /// Randomize record sequence.
  /// @return The function returns \c false if the update flags cannot be locked.
  bool                      Shuffle();
  /// Save content of the current playlist under \a dest.
  /// @param dest URL of the destination where to save the content. This might be the same than the current URL or not.
  /// @param decoder Name of the decoder to use for saving. Must be valid.
  /// @param format Format to save. Depending on the decoder this might be \c NULL.
  /// @param relative Use relative paths in the playlist.
  /// @return true: succeeded.
  bool                      Save(const url123& dest, const char* decoder, const char* format, bool relative);

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
  static  int               Comparer(const xstring& l, const Playable& r);
  #ifdef DEBUG_LOG
  static  void              RPDebugDump();
  #endif
 public:
  typedef btree<Playable,const xstring,&Playable::Comparer> RepositoryType;
  /// Access the repository exclusively and read only.
  class RepositoryAccess
  { Mutex::Lock             Lock;
   public:
    RepositoryAccess()                       : Lock(RPMtx) {}
    operator const RepositoryType*() const   { return &RPIndex; }
    const RepositoryType& operator*() const  { return RPIndex; }
    const RepositoryType* operator->() const { return &RPIndex; }
  };
 private:
  /// Object repository
  static  RepositoryType    RPIndex;
  static  Mutex             RPMtx;
  static  clock_t           LastCleanup;   // Time index of last cleanup run
          clock_t           LastAccess;    // Time index of last access to this instance (used by Cleanup)
 public:
  /// Seek whether an URL is already loaded.
  static  int_ptr<Playable> FindByURL(const xstring& url);
  /// FACTORY! Get a new or an existing instance of this URL.
  static  int_ptr<Playable> GetByURL(const url123& url);
  /// Cleanup unused items from the repository
  /// @remarks One call to Cleanup deletes all unused items that are not requested since the /last/ call to Cleanup.
  /// So the distance between the calls to Cleanup defines the minimum cache lifetime.
  static  void              Cleanup();
  /// Destroy worker
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

