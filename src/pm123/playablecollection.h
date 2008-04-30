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
//#include <utilfct.h>
#include <xio.h>
#include <cpp/smartptr.h>
#include <cpp/xstring.h>
#include <cpp/cpputil.h>
#include <cpp/container.h>
#include <cpp/url123.h>

#include <decoder_plug.h>

#include <format.h>


class PlayableCollection;
class SongIterator;
class PlayableInstance;

/* slice of a playable object
 */
/*struct Slice
{ // TODO: cyclic references!!!
  sco_ptr<SongIterator>    Start;
  sco_ptr<SongIterator>    Stop;
  friend bool              operator==(const Slice& l, const Slice& r);
};
inline bool                operator!=(const Slice& l, const Slice& r)
{ return !(l == r);
}*/


/* Playable object together with a start and stop position.
 * Objects of this class may either be reference counted, managed by a int_ptr
 * or used as temporaries. 
 */
class PlayableSlice : public Iref_Count
{private:
  xstring                  Alias;
  // Start and Stop are owned by PlayableSlice. But sind SongIterator is not yet a complete type
  // we have to deal with that manually. Furthermore Start and Stop MUST have *this as root.
  SongIterator*            Start;
  SongIterator*            Stop;
 protected:
  const int_ptr<Playable>  RefTo;

 public:
  PlayableSlice(Playable* pp);
  PlayableSlice(const url123& url, const xstring& alias = xstring());
  // Because of internal dependencies this constructor must be invoked by the new operator only.
  PlayableSlice(const url123& url, const xstring& alias, const char* start, const char* stop);
  virtual                  ~PlayableSlice();
  // Get't the referenced content.
  // This Pointer is valid as long as the PlayableInstance exist and does not change.
  Playable*                GetPlayable() const { return RefTo; }
  // Start and Stop position
  virtual const SongIterator* GetStart() const;
  virtual const SongIterator* GetStop() const;
  // The functions take the ownership of the SongIterator. 
  virtual void             SetStart(SongIterator* iter);
  virtual void             SetStop (SongIterator* iter);
  // Aliasname
  xstring                  GetAlias() const    { return Alias; }
  virtual void             SetAlias(const xstring& alias);
  // Status
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

 protected:
  PlayableCollection*      Parent; // Weak reference to the parent Collection
 private:
  PlayableStatus           Stat;

 protected:
  PlayableInstance(PlayableCollection& parent, Playable* playable);
 public:
  virtual                  ~PlayableInstance() {}
  // Check if this instance (still) belongs to a collection.
  // The return value true is not reliable unless the collection is locked.
  // Calling this method with NULL will check whether the instance does no longer belog to a collection.
  // In this case only the return value true is reliable.  
  bool                     IsParent(const PlayableCollection* parent) const { return Parent == parent; }

  // Play position
  // The functions take the ownership of the SongIterator. 
  virtual void             SetStart(SongIterator* iter);
  virtual void             SetStop (SongIterator* iter);
  // Aliasname
  virtual void             SetAlias(const xstring& alias);
  // Display name
  xstring                  GetDisplayName() const;

  // The Status-Interface of PlayableInstance is identical to that of Playable,
  // but the only valid states for a PlayableInstance are Normal and Used.
  // Status changes of a PlayableInstance are automatically reflected to the underlying Playable object,
  // but /not/ the other way.
  PlayableStatus           GetStatus() const   { return Stat; }
  // Change status of this instance.
  virtual void             SetInUse(bool used);

 public:
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
  { SaveDefault      = 0x00,
    SaveRelativePath = 0x01,
    SaveUseUpdir     = 0x02,
    SaveAsM3U        = 0x10
  };
  struct change_args
  { PlayableCollection&    Collection;
    PlayableInstance&      Item;
    change_type            Type;
    change_args(PlayableCollection& coll, PlayableInstance& item, change_type type)
    : Collection(coll), Item(item), Type(type) {}
  };
  typedef int (*ItemComparer)(const PlayableInstance* l, const PlayableInstance* r);  
  // Information on PlayableCollection.
  struct CollectionInfo
  { T_TIME                 Songlength;
    double                 Filesize;
    int                    Items;
    sorted_vector<Playable, Playable> Excluded;

    void                   Add(T_TIME songlength, double filesize, int items);
    void                   Add(const CollectionInfo& ci);
    void                   Reset();
    CollectionInfo()       { Reset(); }
  };
 protected:
  // internal representation of a PlayableInstance as linked list.
  struct Entry : public PlayableInstance
  { typedef class_delegate<PlayableCollection, const Playable::change_args> TDType;
    typedef class_delegate<PlayableCollection, const PlayableInstance::change_args> IDType;
    int_ptr<Entry> Prev;      // link to the pervious entry or NULL if this is the first
    int_ptr<Entry> Next;      // link to the next entry or NULL if this is the last
    TDType TechDelegate;
    IDType InstDelegate;
    Entry(PlayableCollection& parent, Playable* playable, TDType::func_type tfn, IDType::func_type ifn)
    : PlayableInstance(parent, playable),
      Prev(NULL),
      Next(NULL),
      TechDelegate(playable->InfoChange, parent, tfn),
      InstDelegate(StatusChange, parent, ifn)
    {}
    // Detach a PlayableInstance from the collection.
    // This function must be called only by the parent collection and only while it is locked.
    void Detach()             { Parent = NULL; }
  };
  // CollectionInfo CacheEntry
  struct CollectionInfoEntry
  : public PlayableSet
  { CollectionInfo         Info;
    bool                   Valid;
    CollectionInfoEntry() : Valid(true) {}
  };

 protected:
  static const FORMAT_INFO2 no_format;
  // The object list is implemented as a doubly linked list to keep the iterators valid on modifications.
  int_ptr<Entry> Head;
  int_ptr<Entry> Tail;
  // Cache mit subenumeration infos
  // This object is protected by a critical section.
  sorted_vector<CollectionInfoEntry, PlayableSet> CollectionInfoCache;

 private:
  // This is called by the InfoChange events of the children.
  void                        ChildInfoChange(const Playable::change_args& args);
  // This is called by the StatusChange events of the children.
  void                        ChildInstChange(const PlayableInstance::change_args& args);
 protected:
  // Fill the THEC_INFO structure.
  void                        CalcTechInfo(TECH_INFO& dst);
  // Create new entry and make the path absolute if required.
  virtual Entry*              CreateEntry(const char* url, const FORMAT_INFO2* ca_format = NULL, const TECH_INFO* ca_tech = NULL, const META_INFO* ca_meta = NULL);
  // Append a new entry at the end of the list.
  // The list must be locked before calling this Function.
  void                        AppendEntry(Entry* entry);
  // Insert a new entry into the list.
  // The list must be locked before calling this Function.
  void                        InsertEntry(Entry* entry, Entry* before);
  // Move a entry inside the list.
  // The list must be locked before calling this Function.
  // The function returns false if the move is a no-op.
  bool                        MoveEntry(Entry* entry, Entry* before);
  // Remove an entry from the list.
  // The list must be locked before calling this Function.
  void                        RemoveEntry(Entry* entry);
  // Subfunction to LoadInfo which really populates the linked list.
  // The return value indicates whether the object is valid.
  virtual bool                LoadList() = 0;
  // Save to stream as PM123 playlist format
  bool                        SaveLST(XFILE* of, bool relative);
  // Save to stream as WinAmp playlist format
  bool                        SaveM3U(XFILE* of, bool relative);
  // Constructor with defaults if available.
  PlayableCollection(const url123& URL, const TECH_INFO* ca_tech = NULL, const META_INFO* ca_meta = NULL);
 public:
  virtual                     ~PlayableCollection();
  // RTTI by the back door.
  virtual Flags               GetFlags() const;
  // Check whether the current Collection is a modified shadow of a unmodified backend.
  // Calling Save() will turn the state into unmodified.
  // The default implementation of this class will always return false.
  // This is the behaviour of a read-only or a synchronized collection.
  virtual bool                IsModified() const;

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
  // The collection must be locked when this function ist called;
  const CollectionInfo&       GetCollectionInfo(const PlayableSet& excluding = PlayableSet::Empty);
  // Load Information from URL
  // This implementation is only the framework. It reqires LoadInfoCore() for it's work.
  virtual InfoFlags           LoadInfo(InfoFlags what);
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
    Playlist&                 List;
    FORMAT_INFO2              Format;
    TECH_INFO                 Tech;
    bool                      has_format;
    bool                      has_tech;
    bool                      has_techinfo;
    xstring                   Alias;
    xstring                   Start;
    xstring                   Stop;
    url123                    URL;
   private:
    void                      Reset();
    void                      Create();
   public:
    LSTReader(Playlist& lst)  : List(lst) { Reset(); }
    ~LSTReader()              { Create(); }
    void                      ParseLine(char* line);
  };

 private:
  bool                        Modified;

 private:
  // Loads the PM123 native playlist file.
  bool                        LoadLST(XFILE* x);
  // Loads the WarpAMP playlist file.
  bool                        LoadMPL(XFILE* x);
  // Loads the WinAMP playlist file.
  bool                        LoadPLS(XFILE* x);
 protected:
  // really load the playlist
  virtual bool                LoadList();
 public:
  Playlist(const url123& URL, const TECH_INFO* ca_tech = NULL, const META_INFO* ca_meta = NULL)
   : PlayableCollection(URL, ca_tech, ca_meta), Modified(false) { DEBUGLOG(("Playlist(%p)::Playlist(%s, %p, %p)\n", this, URL.cdata(), ca_tech, ca_meta)); }
  // Get attributes
  virtual Flags               GetFlags() const;
  // Check whether the current Collection is a modified shadow of a unmodified backend.
  // Calling Save() will turn the state into unmodified.
  virtual bool                IsModified() const;

  // Insert a new item before the item "before".
  // If the prameter before is NULL the item is appended.
  // The funtion fails with returning false if and only if the PlayableInstance before is no longer valid.
  virtual bool                InsertItem(const PlayableSlice& item, PlayableInstance* before = NULL);
  // Move an item inside the list.
  // If the prameter before is NULL the item is moved to the end.
  // The funtion fails with returning false if and only if one of the PlayableInstances is no longer valid.
  virtual bool                MoveItem(PlayableInstance* item, PlayableInstance* before);
  // Remove an item from the playlist.
  // Attension: passing NULL as argument will remove all items.
  // The funtion fails with returning false if and only if the PlayableInstance is no longer valid.
  virtual bool                RemoveItem(PlayableInstance* item);
  // Remove all items from the playlist.
  void                        Clear() { RemoveItem(NULL); }
  // Sort all items with a comparer.
  virtual void                Sort(ItemComparer comp);
  // Randomize record sequence.
  virtual void                Shuffle();
  // Save the current playlist as new file.
  virtual bool                Save(const url123& URL, save_options opt = SaveDefault);

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

 private:
  void                        ParseQueryParams();
 public:
  PlayFolder(const url123& URL, const TECH_INFO* ca_tech = NULL, const META_INFO* ca_meta = NULL);
  virtual bool                LoadList();
};


#endif
