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


#ifndef PLAYABLECOLLECTION_H
#define PLAYABLECOLLECTION_H

#include "playable.h"

#include <cpp/event.h>
#include <xio.h>
#include <cpp/smartptr.h>
#include <cpp/xstring.h>
#include <cpp/cpputil.h>
#include <cpp/container/sorted_vector.h>
#include <cpp/container/list.h>
#include <cpp/url123.h>

#include <decoder_plug.h>

#include <format.h>


class PlayableCollection;
class SongIterator;
class PlayableInstance;


/* Playable object together with a start and stop position.
 * Objects of this class may either be reference counted, managed by a int_ptr
 * or used as temporaries.
 */
class PlayableSlice : public Iref_Count
{protected:
  const int_ptr<Playable>  RefTo;
 private:
  xstring                  Alias;
 protected:
  // Start and Stop are owned by PlayableSlice. But since SongIterator is not yet a complete type
  // we have to deal with that manually. Furthermore Start and Stop MUST have RefTo as root,
  // but they must not use *this as root to avoid cyclic references.
  SongIterator*            Start;
  SongIterator*            Stop;
 private: // not assignable
  void                     operator=(const PlayableSlice& r);

 public:
  PlayableSlice(Playable* pp);
  PlayableSlice(const PlayableSlice& r);
  PlayableSlice(const url123& url, const xstring& alias = xstring());
  PlayableSlice(const url123& url, const xstring& alias, const char* start, const char* stop);
  virtual                  ~PlayableSlice();
  // Get't the referenced content.
  // This Pointer is valid as long as the PlayableInstance exist and does not change.
  Playable*                GetPlayable() const { return RefTo; }
  // Swap instance properties (fast)
  // You must not swap instances that belong to different Playable objects.
  virtual void             Swap(PlayableSlice& r);
  // Start and Stop position
  const SongIterator*      GetStart() const    { return Start; }
  const SongIterator*      GetStop() const     { return Stop; }
  // The functions take the ownership of the SongIterator.
  virtual void             SetStart(SongIterator* iter);
  virtual void             SetStop (SongIterator* iter);
  // Aliasname
  xstring                  GetAlias() const    { return Alias; }
  virtual void             SetAlias(const xstring& alias);
  // Usage status
  virtual void             SetInUse(bool used);
  #ifdef DEBUG
  xstring                  DebugName() const;
  #endif
};

/*class PlayableInstRef : public PlayableSlice
{private:
  const int_ptr<PlayableInstance> InstRef;
 public:
  PlayableInstRef(PlayableInstance* ref);
  virtual PlayableInstance* GetPlayableInstance() const;

};*/

/* Instance to a Playable object. While the Playable objects are unique per URL
 * the PlayableInstances are not.

 * In contrast to Playable PlayableInstance is not thread-safe.
 * It is usually protected by the collection it belongs to.
 * But calling non-constant methods unsynchronized will not cause the application
 * to have undefined behaviour. It will only cause the PlayableInstance to have a
 * non-deterministic but valid state.
 * The relationship of a PlayableInstance to it's container is weak.
 * That means that the PlayableInstance may be detached from the container before it dies.
 * A detached PlayableInstance will die as soon as it is no longer used. It will never be
 * reattached to a container.
 * That also means that there is no way to get a reference to the actual parent,
 * because once you got the reference it might immediately get invalid.
 * And to avoid this, the parent must be locked, which is impossible to ensure, of course.
 * All you can do is to verify wether the current PlayableInstance belongs to a given parent
 * while this parent is locked with the method IsParent. This is e.g. useful to ensure
 * that a PlayableInstance is still valid before it is removed from the collection.
 */
class PlayableInstance : public PlayableSlice
{public:
  // Parameters for StatusChange Event
  enum StatusFlags
  { SF_None    = 0,
    SF_InUse   = 0x01,
    SF_Alias   = 0x02,
    SF_Slice   = 0x04,
    SF_All     = SF_InUse|SF_Alias|SF_Slice
  };
  struct change_args
  { PlayableInstance&      Instance;
    StatusFlags            Flags; // Bitvector of type StatusFlags
    change_args(PlayableInstance& inst, StatusFlags flags) : Instance(inst), Flags(flags) {}
  };
 private:
  enum SliceBorder
  { SB_Start               = 1,
    SB_Stop                = -1
  };

 protected:
  PlayableCollection*      Parent; // Weak reference to the parent Collection
 private:
  bool                     InUse;

 private:
  PlayableInstance(const PlayableInstance& r);
  void operator=(const PlayableInstance& r);
 private:
  // Compares two slice borders taking NULL iterators by default as
  // at the start, if type is SB_Start, or at the end if type is SB_Stop.
  // Otherwise the same as SongIterator::CopareTo.
  static int               CompareSliceBorder(const SongIterator* l, const SongIterator* r, SliceBorder type);
 protected:
  PlayableInstance(PlayableCollection& parent, Playable* playable);
 public:
  virtual                  ~PlayableInstance() {}
  // Check if this instance (still) belongs to a collection.
  // The return value true is not reliable unless the collection is locked.
  // Calling this method with NULL will check whether the instance does no longer belog to a collection.
  // In this case only the return value true is reliable.
  #ifdef DEBUG
  bool                     IsParent(const PlayableCollection* parent) const
                           { if (Parent == parent)
                               return true;
                             DEBUGLOG(("PlayableInstnace(%p)::IsParent(%p) - %p\n", this, parent, Parent));
                             return false;
                           }
  #else
  bool                     IsParent(const PlayableCollection* parent) const { return Parent == parent; }
  #endif
  // Display name
  xstring                  GetDisplayName() const;

  // Swap instance properties (fast)
  // You must not swap instances that belong to different PlayableCollection objects.
  // The collection must be locked when calling Swap.
  // Swap does not swap the current status.
  virtual void             Swap(PlayableSlice& r);

  // Play position
  // The functions take the ownership of the SongIterator.
  virtual void             SetStart(SongIterator* iter);
  virtual void             SetStop (SongIterator* iter);
  // Aliasname
  virtual void             SetAlias(const xstring& alias);

  // In use status changes of a PlayableInstance are automatically reflected
  // to the underlying Playable object, but /not/ the other way.
  bool                     IsInUse() const     { return InUse; }
  // Change status of this instance.
  virtual void             SetInUse(bool used);

  // event on status change
  event<const change_args> StatusChange;
  // Compare two PlayableInstance objects by value.
  // Two instances are equal if on only if they belong to the same PlayableCollection,
  // share the same Playable object (=URL) and have the same properties values for alias and slice.
  friend bool              operator==(const PlayableInstance& l, const PlayableInstance& r);
  // Return the logicalordering of two Playableinstances.
  // 0  = equal
  // -1 = l before r
  // 1  = l after r
  // 4  = unordered (The PlayableInstances do not belong to the same collection.)
  int                      CompareTo(const PlayableInstance& r) const;
};
// Flags Attribute for StatusFlags
FLAGSATTRIBUTE(PlayableInstance::StatusFlags);


/* Abstract class representing a collection of PlayableInstances.
 */
class PlayableCollection : public Playable
{public:
  enum change_type
  { Insert, // item just inserted
    Move,   // item just moved
    Delete  // item about to be deleted
  };
  enum serialization_options
  { SerialDefault      = 0x00,
    SerialRelativePath = 0x01,
    SerialUseUpdir     = 0x02,
    SerialIndexOnly    = 0x10
  };
  enum save_options
  { SaveDefault        = 0x00,
    SaveRelativePath   = 0x01,
    SaveUseUpdir       = 0x02,
    SaveAsM3U          = 0x10
  };
  struct change_args
  { PlayableCollection&       Collection;
    PlayableInstance&         Item;
    change_type               Type;
    change_args(PlayableCollection& coll, PlayableInstance& item, change_type type)
    : Collection(coll), Item(item), Type(type) {}
  };
  typedef int (*ItemComparer)(const PlayableInstance* l, const PlayableInstance* r);
  // Information on PlayableCollection.
  struct CollectionInfo
  { T_TIME                    Songlength; // Tech info
    T_SIZE                    Filesize;   // Tech info
    int                       Items;      // PL info
    PlayableSet               Excluded;   // PL info

    void                      Add(T_TIME songlength, double filesize, int items);
    void                      Add(const CollectionInfo& ci);
    void                      Reset();
                              CollectionInfo()       { Reset(); }
  };
 protected:
  // internal representation of a PlayableInstance as linked list.
  struct Entry : public PlayableInstance, public list_int<Entry>::entry_base
  { typedef class_delegate<PlayableCollection, const Playable::change_args> TDType;
    typedef class_delegate<PlayableCollection, const PlayableInstance::change_args> IDType;
    TDType TechDelegate;
    IDType InstDelegate;
    Entry(PlayableCollection& parent, Playable* playable, TDType::func_type tfn, IDType::func_type ifn);
    virtual                   ~Entry() { DEBUGLOG(("PlayableCollection::Entry(%p)::~Entry()\n", this)); }
    // Activate the event handlers
    void                      Attach();
    // Detach a PlayableInstance from the collection.
    // This function must be called only by the parent collection and only while it is locked.
    void                      Detach();
  };
  // The object list is implemented as a doubly linked list to keep the iterators valid on modifications.
  typedef list_int<Entry>     EntryList;
  // CollectionInfo CacheEntry
  struct CollectionInfoEntry
  : public PlayableSet
  { CollectionInfo            Info;
    InfoFlags                 Valid; // only IF_Tech and IF_Pl are significant here
    CollectionInfoEntry(const PlayableSetBase& r) : PlayableSet(r), Valid(IF_None) { DEBUGLOG(("PlayableCollection::CollectionInfoEntry(%p)::CollectionInfoEntry({%u,...})\n", this, r.size())); }
    #ifdef DEBUG
    ~CollectionInfoEntry()    { DEBUGLOG(("PlayableCollection::CollectionInfoEntry(%p)::~CollectionInfoEntry()\n", this)); }
    #endif
  };

 private:
  EntryList                   List;
  bool                        Modified;
 protected:
  xstring                     ErrorMsg; // can store error from last DoLoadInfo call. Only valid if status is STA_Invalid.

 private:
  // Cache with subenumeration infos
  // This object is protected by the mutex below.
  sorted_vector<CollectionInfoEntry, PlayableSetBase> CollectionInfoCache;
  // One Mutex to protect CollectionInfoCache of all instances.
  // You must not aquire another Mutex while holding this one.
  static Mutex                CollectionInfoCacheMtx;
/* private: // Entries used by BeginRefresh/EndRefresh ...
  // List of Playable objects that are removed, added or their PlayableInstance is modified.
  // This list does not hold the ownership of the Playable objects inside.
  // So you must not dereference them anymore. However, comparing the pointers for instance
  // equality is mostly safe, since Playable objects are unique with respect to their primary key.
  // In the worst case another object is invalidated accidentially. 
  PlayableSet                 ModifiedSubitems;*/

 private:
  // Invalidate CollectionInfoCache by single object.
  void                        InvalidateCIC(InfoFlags what, Playable& p);
  // Invalidate CollectionInfoCache by multiple objects.
  /*void                        InvalidateCIC(InfoFlags what, const PlayableSetBase& set);*/
  // This is called by the InfoChange events of the children.
  void                        ChildInfoChange(const Playable::change_args& args);
  // This is called by the StatusChange events of the children.
  void                        ChildInstChange(const PlayableInstance::change_args& args);
  // Internal subfunction to UpdateList.
  bool                        UpdateListCore(Entry* cur_new, Entry*& first_new);
  // Internal subfunction to UpdateList.
  bool                        PurgeUntil(Entry* keep_from);
 protected:
  // Prefetch some information to avoid deadlocks in GetCollectionInfo.
  void                        PrefetchSubInfo(const PlayableSetBase& excluding, bool incl_songs);
  // Create new entry and make the path absolute if required.
  Entry*                      CreateEntry(const char* url, const DECODER_INFO2* ca = NULL) const;
  // Update the modified flag
  void                        UpdateModified(bool modified);
  // Update the list of entries. The update is done smooth, keeping as much entries
  // as possible. And throwing away the new ones in doubt.
  // The functions return false if the operation was a no-op.
  void                        UpdateList(EntryList& newlist);
  void                        UpdateList(const vector<PlayableInstance>& newlist);
  // Subfunction to LoadInfo which really populates the linked list.
  // The return value indicates whether the object is valid.
  virtual bool                LoadList(EntryList& list, PHYS_INFO& phys) = 0;
  // Load Information from URL
  // This implementation is only the framework. It reqires LoadList() for it's work.
  virtual InfoFlags           DoLoadInfo(InfoFlags what);
 protected: // modification functions
  // Insert a new entry into the list.
  // The list must be locked before calling this Function.
  void                        InsertEntry(Entry* entry, Entry* before);
  // Append a new entry at the end of the list.
  // The list must be locked before calling this Function.
  void                        AppendEntry(Entry* entry) { InsertEntry(entry, NULL); }
  // Move a entry inside the list.
  // The list must be locked before calling this Function.
  // The function returns false if the move is a no-op.
  bool                        MoveEntry(Entry* entry, Entry* before);
  // Remove an entry from the list.
  // The list must be locked before calling this Function.
  void                        RemoveEntry(Entry* entry);
  // Save to stream as PM123 playlist format
  bool                        SaveLST(XFILE* of, bool relative);
  // Save to stream as WinAmp playlist format
  bool                        SaveM3U(XFILE* of, bool relative);
  // Constructor with defaults if available.
  PlayableCollection(const url123& URL, const DECODER_INFO2* ca = NULL);
 public:
  virtual                     ~PlayableCollection();
  // RTTI by the back door.
  virtual Flags               GetFlags() const;
  // Check whether the current Collection is a modified shadow of a unmodified backend.
  // Calling Save() will turn the state into unmodified.
  bool                        IsModified() const { return Modified; }

  // Iterate over the collection. While the following functions are atomic and therefore thread-safe
  // the iteration itself is not because the collection may change at any time.
  // Calling these function is not valid until the information IF_Other is loaded.
  // Get previous item of this collection. Passing NULL will return the last item.
  // The function returns NULL if ther are no more items.
  virtual int_ptr<PlayableInstance> GetPrev(const PlayableInstance* cur) const;
  // Get next item of this collection. Passing NULL will return the first item.
  // The function returns NULL if ther are no more items.
  virtual int_ptr<PlayableInstance> GetNext(const PlayableInstance* cur) const;
  // Returns a string that represents a serialized version of a subitem.
  // If index_only is true the string will be of the form "[index]". This is not recommended.
  xstring                     SerializeItem(const PlayableInstance* cur, serialization_options opt) const;
  // Returns the item that is adressed by the string str.
  // If the string is not valid or the collection is empty the function returns NULL.
  int_ptr<PlayableInstance>   DeserializeItem(const xstring& str) const;

  // Calculate the CollectionInfo structure.
  // This returns context dependant information on the current collection.
  // In fact all items in the set excluding and their subitems are excluded from the calculation.
  // The function uses an internal cache (CollectionInfoCache) to optimize access.
  // This cache is automatically invalidated.
  // If you call this function while the requested flags of InfoValid are not yet set,
  // you will get incomplete information. This is not an error.
  const CollectionInfo&       GetCollectionInfo(InfoFlags what, const PlayableSetBase& excluding = PlayableSetBase::Empty);
 public: // Editor functions
  // Insert a new item before the item "before".
  // If the prameter before is NULL the item is appended.
  // The funtion fails with returning false if the PlayableInstance before is no longer valid
  // or the update flags cannot be locked.
  virtual bool                InsertItem(const PlayableSlice& item, PlayableInstance* before = NULL);
  // Move an item inside the list.
  // If the prameter before is NULL the item is moved to the end.
  // The funtion fails with returning false if one of the PlayableInstances is no longer valid
  // or the update flags cannot be locked.
  virtual bool                MoveItem(PlayableInstance* item, PlayableInstance* before);
  // Remove an item from the playlist.
  // The funtion fails with returning false if the PlayableInstance is no longer valid
  // or the update flags cannot be locked.
  virtual bool                RemoveItem(PlayableInstance* item);
  // Remove all items from the playlist.
  // The funtion returns false if the update flags cannot be locked.
  virtual bool                Clear();
  // Sort all items with a comparer.
  // The funtion returns false if the update flags cannot be locked.
  virtual bool                Sort(ItemComparer comp);
  // Randomize record sequence.
  // The funtion returns false if the update flags cannot be locked.
  virtual bool                Shuffle();

  // Save the current playlist as new file.
  // If the destination name is omitted, the data is saved under the current URL.
  // However this might not succeed, if the URL is read-only.
  // Saving under a different name does not change the name of the curren object.
  // It is like save copy as.
  virtual bool                Save(const url123& URL, save_options opt = SaveDefault);
  bool                        Save(save_options opt = SaveDefault) { return Save(GetURL(), opt); }

 protected:
  // Notify that the data source is likely to have changed.
  // This is a NO-OP in the default implementation.
  virtual void                NotifySourceChange();
 public:
  // This event is raised whenever the collection has changed.
  // The event may be called from any thread.
  // The event is called in synchronized context.
  event<const change_args>    CollectionChange;
};
FLAGSATTRIBUTE(PlayableCollection::serialization_options);
FLAGSATTRIBUTE(PlayableCollection::save_options);


/* Class representing a playlist file.
 */
class Playlist : public PlayableCollection
{private:
  // Helper class to deserialize playlists
  class LSTReader;
  friend class LSTReader;
  class LSTReader
  {private:
    const Playlist&           Parent;
    EntryList&                Dest;
    FORMAT_INFO2              Format;
    TECH_INFO                 Tech;
    PHYS_INFO                 Phys;
    RPL_INFO                  Rpl;
    DECODER_INFO2             Info;
    xstring                   Alias;
    xstring                   Start;
    xstring                   Stop;
    url123                    URL;
   private:
    void                      Reset();
    void                      Create();
   public:
    LSTReader(const Playlist& pl, EntryList& list)  : Parent(pl), Dest(list) { Reset(); }
    ~LSTReader()              { Create(); }
    void                      ParseLine(char* line);
  };

 private:
  // Loads the PM123 native playlist file.
  bool                        LoadLST(EntryList& list, XFILE* x);
  // Loads the WarpAMP playlist file.
  bool                        LoadMPL(EntryList& list, XFILE* x);
  // Loads the WinAMP playlist file.
  bool                        LoadPLS(EntryList& list, XFILE* x);
 protected:
  // really load the playlist
  virtual bool                LoadList(EntryList& list, PHYS_INFO& phys);
 public:
  Playlist(const url123& URL, const DECODER_INFO2* ca = NULL)
   : PlayableCollection(URL, ca) { DEBUGLOG(("Playlist(%p)::Playlist(%s, %p)\n", this, URL.cdata(), ca)); }
  // Get attributes
  virtual Flags               GetFlags() const;

 protected:
  // Notify that the data source is likely to have changed.
  virtual void                NotifySourceChange();
};


/* Class representing a folder with songs.
 * (Something like an implicit playlist.)
 */
class PlayFolder : public PlayableCollection
{private:
  xstring                     Pattern;
  bool                        Recursive;
  signed char                 SortMode;     // 1 = yes, -1 = no, 0 = default
  signed char                 FoldersFirst; // 1 = yes, -1 = no, 0 = default 

 private:
  void                        ParseQueryParams();
  static int                  CompName(const PlayableInstance* l, const PlayableInstance* r);
  static int                  CompNameFoldersFirst(const PlayableInstance* l, const PlayableInstance* r);
 public:
  PlayFolder(const url123& URL, const DECODER_INFO2* ca = NULL);
  virtual bool                LoadList(EntryList& list, PHYS_INFO& phys);
};


#endif

