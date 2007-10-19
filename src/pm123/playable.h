/*
 * Copyright 2007-2007 Marcel Mueller
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
#include <string.h>

#include <cpp/event.h>
#include <utilfct.h>
#include <xio.h>
#include <decoder_plug.h>
#include <cpp/mutex.h>
#include <cpp/smartptr.h>
#include <cpp/xstring.h>
#include <cpp/queue.h>
#include <cpp/cpputil.h>
#include <cpp/container.h>
#include <strutils.h>
#include "url.h"


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
class Playable : public Iref_Count, private IComparableTo<char>
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
    IF_Other    = 0x08,// applies e.g. to PlayableCollection::GetEnumerator()
    IF_Status   = 0x10,// applies to GetStatus()
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
  const url           URL;
 protected: // The following vars are protected by the mutex
  DecoderInfo         Info;
  PlayableStatus      Stat;
  char                Decoder[_MAX_FNAME];
  InfoFlags           InfoValid;       // Bitvector of type InfoFlags
  InfoFlags           InfoChangeFlags; // Bitvector with stored events
 public:
  Mutex               Mtx;   // protect this instance
 
 private: // non-copyable
  Playable(const Playable&);
  void operator=(const Playable&);
 protected:
  Playable(const url& url, const FORMAT_INFO2* ca_format = NULL, const TECH_INFO* ca_tech = NULL, const META_INFO* ca_meta = NULL);
  // Check whether a given URL is to be initialized as playlist.
  static bool         IsPlaylist(const url& URL);
  // Update the structure components and return the required InfoChange Flags or 0 if no change has been made.
  void                UpdateInfo(const FORMAT_INFO2* info);
  void                UpdateInfo(const TECH_INFO* info);
  void                UpdateInfo(const META_INFO* info);
  void                UpdateInfo(const DECODER_INFO2* info);
  // Raise the InfoChange event if required.
  // This function must be called in synchronized context.
  void                RaiseInfoChange()   { if (InfoChangeFlags)
                                              InfoChange(change_args(*this, InfoChangeFlags));
                                            InfoChangeFlags = IF_None; } 
 public:
  virtual ~Playable();
  // Get URL
  const url&          GetURL() const      { return URL; }
  // RTTI by the back door.
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
  // When returning true at least the requested info must be valid and the bits in InfoValid must be set.
  // It sould return false only if the status of the current object is bad,
  // i.e. the URL is invalid or unreachable. However, the InfoValid bits must be set anyway.
  // This is a mutable Function when the info-fields are not initialized yet (late initialization).
  // Normally this function is called automatically. However, you may want to force a refresh. 
  virtual void        LoadInfo(InfoFlags what) = 0;
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
  // While this function is implemented it only updates the cache and fires the MetaChange event.
  // So you should overload it if resident changes are intended.
  // Calling this function with meta == NULL deletes the meta information.
  // The function returns false if the update failed.
  virtual bool        SetMetaInfo(const META_INFO* meta);

 public:
  // Event on info change
  // This event will always be called in synchronized context.
  event<const change_args> InfoChange; 

 // asynchronuous request service
 private:
  struct QEntry : public int_ptr<Playable> // derives operator ==
  { InfoFlags         Request;
    QEntry(Playable* obj, InfoFlags req) : int_ptr<Playable>(obj), Request(req) {}
  };
 
 private: 
  static queue<QEntry> WQueue;
  static int          WTid;          // Thread ID of the worker  
  static bool         WTermRq;       // Termination Request to Worker
 
 private:
  // stub to call Populate asynchronuously
  friend static void TFNENTRY PlayableWorker(void*);
 public:
  static void         Init();
  static void         Uninit();

 // Repository
 private:
  static sorted_vector<Playable, char> RPInst;
  static Mutex             RPMutex;
 private:
  virtual int              CompareTo(const char* str) const;
 public:
  // Seek whether an URL is already loaded.
  static int_ptr<Playable> FindByURL(const char* url);
  // FACTORY! Get a new or an existing instance of this URL.
  // The optional parameters ca_* are preloaded informations.
  // This is returned by the apropriate Get* functions without the need to access the underlying data source.
  // This is used to speed up large playlists.
  static int_ptr<Playable> GetByURL(const url& URL, const FORMAT_INFO2* ca_format = NULL, const TECH_INFO* ca_tech = NULL, const META_INFO* ca_meta = NULL);
};
// Flags Attribute for StatusFlags
FLAGSATTRIBUTE(Playable::InfoFlags);


inline Playable::InfoFlags Playable::CheckInfo(InfoFlags what)
{ return what & ~InfoValid;
}


class PlayableCollection;

/* Instance to a Playable object. While the Playable objects are unique per URL
 * the PlayableInstances are not. The PlayableInstance is always owned by a parent PlayableCollection.
 * In contrast to Playable PlayableInstance is not thread-safe.
 * It is usually protected by the collection it belongs to.
 */
class PlayableInstance
{public:
  // Parameters for StatusChange Event
  enum StatusFlags
  { SF_None    = 0,
    SF_InUse   = 0x01,
    SF_Alias   = 0x02,
    SF_PlayPos = 0x04,
    SF_All     = SF_InUse|SF_Alias|SF_PlayPos,
    SF_Destroy = 0x80 // This is the last event of a playable instance when it goes out of scope.
  };
  struct change_args
  { PlayableInstance& Instance;
    StatusFlags       Flags; // Bitvector of type StatusFlags
    change_args(PlayableInstance& inst, StatusFlags flags) : Instance(inst), Flags(flags) {}
  };

 private:
  PlayableCollection& Parent;
  const int_ptr<Playable> RefTo;
  PlayableStatus      Stat;
  xstring             Alias;
  double              Pos;

 protected:
  PlayableInstance(PlayableCollection& parent, Playable* playable);
  ~PlayableInstance();

 public:
  Playable&           GetPlayable() const { return *RefTo; }
  // Get the parent Collection containing this Playable object.
  PlayableCollection& GetParent() const   { return Parent; }
  
  // The Status-Interface of PlayableInstance is identical to that of Playable,
  // but the only valid states for a PlayableInstance are Normal and Used.
  // Status changes of a PlayableInstance are automatically reflected to the underlying Playable object,
  // but /not/ the other way.
  PlayableStatus      GetStatus() const   { return Stat; }
  // Change status of this instance.
  void                SetInUse(bool used);
  // Aliasname
  xstring             GetAlias() const    { return Alias; }
  void                SetAlias(const xstring& alias);
  // Play position
  double              GetPlayPos() const  { return Pos; }
  void                SetPlayPos(double pos);
  // Display name
  xstring             GetDisplayName() const;
 public:
  // event on status change
  event<const change_args> StatusChange;
};
// Flags Attribute for StatusFlags
FLAGSATTRIBUTE(PlayableInstance::StatusFlags);


/* Class representing exactly one song.
 */
class Song : public Playable
{public:
  Song(const url& URL, const FORMAT_INFO2* ca_format = NULL, const TECH_INFO* ca_tech = NULL, const META_INFO* ca_meta = NULL)
   : Playable(URL, ca_format, ca_tech, ca_meta) { DEBUGLOG(("Song(%p)::Song(%s, %p, %p, %p)\n", this, URL.cdata(), ca_format, ca_tech, ca_meta)); }

  virtual void        LoadInfo(InfoFlags what);
};


/* Interface to enumerate the content of a Playlist or something like that.
 * The Enumerator is a bidirectional enumerator. Instances of this class are not
 * thread-safe. 
 * Initially the Enumerator is before the start of the list, i.e. IsValid() returns false.
 * You must call Next() or Prev() to move to the first or last element respectively.
 * Remember that calling Prev() and then Next() is not neccessarily a no-op, when the underlying
 * collection is multi-threaded. 
 */
class PlayableEnumerator
{protected:
  // For performance reasons we do this jobs in the base class.
  PlayableInstance*   Current;
 protected:
  PlayableEnumerator::PlayableEnumerator() : Current(NULL) {}
 public:
  virtual ~PlayableEnumerator() {}
  // Check wether dereferencing the enumerator is currently allowed.
  // The return value of this function is identical to that of the last call to Next() or Prev().
  // It is always false if neither Next() nor Prev() have been called so far.
  bool IsValid() const { return Current != NULL; }
  // Dereference this instance. Calling this function is undefined when IsValid() returns false. 
  PlayableInstance& operator*() const { return *Current; }
  // Shortcut for structured objects.
  PlayableInstance* operator->() const { return Current; }
  operator PlayableInstance*() const { return Current; }
  // Reset the current enumerator to it's initial state before the first element.
  virtual void Reset();
  // Move to the pervious element. If the enumerator is initial Prev() moves to the last element.
  // The function returns false if there are no more elements.
  // The function is non-blocking.
  virtual bool Prev() = 0;
  // Move to the next element. If the enumerator is initial Prev() moves to the first element.
  // The function returns false if there are no more elements.
  // The function is non-blocking.
  virtual bool Next() = 0;
  // clone the current enumerator instance
  virtual PlayableEnumerator* Clone() const = 0;
};

/* Abstract class representing a collection of PlayableInstances.
 */
class PlayableCollection : public Playable
{public:
  enum sort_order
  { Native, // no user defined sort order
    Name    // sort by URL
  };
  enum change_type
  { Insert, // item just inserted
    Move,   // item just moved
    Delete  // item about to be deleted
  };
  struct change_args
  { PlayableCollection& Collection;
    PlayableInstance&   Item;
    change_type         Type;
    change_args(PlayableCollection& coll, PlayableInstance& item, change_type type)
    : Collection(coll), Item(item), Type(type) {}
  };
  class Enumerator : public PlayableEnumerator
  {private:
    int_ptr<PlayableCollection> Parent;
    
   private:
    Enumerator(const PlayableEnumerator& r);
   public:
    Enumerator(PlayableCollection* list) : Parent(list) {}
    // Move to the pervious element. If the enumerator is initial Prev() moves to the last element.
    // The function returns false if there are no more elements.
    virtual bool Prev();
    // Move to the next element. If the enumerator is initial Prev() moves to the first element.
    // The function returns false if there are no more elements.
    virtual bool Next();
    // clone the current enumerator instance
    virtual PlayableEnumerator* Clone() const;
  };
  friend class Enumerator;

 protected:
  struct Entry : public PlayableInstance
  { typedef class_delegate<PlayableCollection, const Playable::change_args> TDType;
    Entry* Prev;                   // link to the pervious entry or NULL if this is the first
    Entry* Next;                   // link to the next entry or NULL if this is the last
    TDType TechDelegate;
    Entry(PlayableCollection& parent, Playable* playable, TDType::func_type fn)
    : PlayableInstance(parent, playable),
      Prev(NULL),
      Next(NULL),
      TechDelegate(playable->InfoChange, parent, fn)
    {}
  };


 protected:
  static const FORMAT_INFO2 no_format;
  // The object list is implemented as a doubly linked list to keep the iterators valid on modifications.
  Entry*      Head;
  Entry*      Tail;
  sort_order  Sort;

 private:
  // Internal Subfunction to void CalcTechInfo(Playable& play);
  static void                 AddTechInfo(TECH_INFO& dst, const TECH_INFO& tech);
  // This is called by the TechChange events of the children. 
  void                        ChildInfoChange(const Playable::change_args& child); 
 protected:
  // (Re)calculate the TECH_INFO structure.
  virtual void                CalcTechInfo(TECH_INFO& dst);
  // Create new entry and make the path absolute if required.
  virtual Entry*              CreateEntry(const char* url);
  // Append a new entry at the end of the list.
  // The list must be locked before calling this Function.
  void                        AppendEntry(Entry* entry);
  // Insert a new entry into the list.
  // The list must be locked before calling this Function.
  void                        InsertEntry(Entry* entry, Entry* before);
  // Remove an entry from the list.
  // The list must be locked before calling this Function.
  void                        RemoveEntry(Entry* entry);
  // Subfunction to LoadInfo which really populates the linked list.
  // The return value indicates whether the object is valid.
  virtual bool                LoadInfoCore() = 0;
  // Constructor with defaults if available.
  PlayableCollection(const url& URL, const TECH_INFO* ca_tech = NULL, const META_INFO* ca_meta = NULL);
 public:
  virtual                     ~PlayableCollection();
  // RTTI by the back door.
  virtual Flags               GetFlags() const;

  // Get an enumerator for this instance.
  // Generally you must delete the returned instance once you don't need it anymore.
  // It is recommended to use sco_ptr for this purpose.
  // Since the enumeration process is intrinsically not thread-safe you have to lock this collection
  // during enumeration. Alternatively you have to catch the remove events of the current item.
  // and do the Prev/Next operations together with the assignment of the remove events while
  // you locked the collection. 
  virtual PlayableEnumerator* GetEnumerator();
  // Load Information from URL
  // This implementation is only the framework. It reqires LoadInfoCore() for it's work.
  virtual void                LoadInfo(InfoFlags what);

 public:
  // This event is raised whenever the collection has changed.
  // The event may be called from any thread.
  // The event is called in synchronized context.
  event<const change_args>    CollectionChange;
};

/* Class representing a playlist file.
 */
class Playlist : public PlayableCollection
{private:
  // Loads the PM123 native playlist file.
  bool                        LoadLST(XFILE* x);
  // Loads the WarpAMP playlist file.
  bool                        LoadMPL(XFILE* x);
  // Loads the WinAMP playlist file.
  bool                        LoadPLS(XFILE* x);
 protected:
  // really load the playlist
  virtual bool                LoadInfoCore();
 public:
  Playlist(const url& URL, const TECH_INFO* ca_tech = NULL, const META_INFO* ca_meta = NULL)
   : PlayableCollection(URL, ca_tech, ca_meta) { DEBUGLOG(("Playlist(%p)::Playlist(%s, %p, %p)\n", this, URL.cdata(), ca_tech, ca_meta)); }
  // Get attributes 
  virtual Flags               GetFlags() const;
  // Insert a new item before the item "before".
  // If the prameter before is NULL the the item is appended. 
  virtual void                InsertItem(const char* url, const xstring& alias, double pos, PlayableInstance* before = NULL);
  // Remove an item from the playlist.
  // Attension: passing NULL as argument will remove all items.
  virtual void                RemoveItem(PlayableInstance* item);
  // Remove all items from the playlist.
  void                        Clear() { RemoveItem(NULL); }
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
  PlayFolder(const url& URL, const TECH_INFO* ca_tech = NULL, const META_INFO* ca_meta = NULL);
  virtual bool                LoadInfoCore();
};

#endif

