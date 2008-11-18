/*
 * Copyright 2007-2007 M.Mueller
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


#ifndef CPP_CONTAINER_INST_INDEX_H
#define CPP_CONTAINER_INST_INDEX_H

#include <cpp/mutex.h>
#include <cpp/smartptr.h>
#include <cpp/container/sorted_vector.h>


/* Class to implement ICompareableTo for comparsion with myself and instance equality semantic.
 * The comparer provides an unspecified but stable order.
 */
template <class T>
class InstanceCompareable : public IComparableTo<T>
{public:
  virtual int compareTo(const T& key) const;
};

template <class T>
int InstanceCompareable<T>::compareTo(const T& key) const
{ DEBUGLOG2(("InstanceCompareable<T>(%p)::compareTo(%p)\n", this, &key));
  return (char*)(T*)this - (char*)&key; // Dirty but working unless the virtual address space is larger than 2 GiB, which is not the case on OS/2.
}


/* Class to implement a repository of all objects instances of a certain type
 * identified by a key K.
 * The Instances of type T must implement Iref_Count and ICompareable<K>
 * and both must be /before/ inst_index in the base class list to avoid undefined bahavior.
 * Classes of type T must inherit from inst_index<T, K> to implement this feature.
 * You must redefine the static function GetByKey to provide a suitable factory
 * for new T instances. In the easiest case this is simply a call to new T(key).
 * The repository below does not handle the ownership of to the T instances.
 * The class instances also do not hold the ownership of the keys.
 * The lifetime management must be done somewhere else.
 * The class is thread-safe.
 */
template <class T, class K>
class inst_index
{public:
  // abstract factory interface used for object instantiation
  struct IFactory
  { virtual T* operator()(K& key) = 0;
  };
  // Reques an exclusive read-only access to the index repository.
  class IXAccess;
  friend class IXAccess;
  class IXAccess : public Mutex::Lock
  {private:
    sorted_vector<T,K>& IX;
   public:
    IXAccess() : Mutex::Lock(inst_index<T,K>::Mtx), IX(inst_index<T,K>::Index) {};
    operator const sorted_vector<T,K>*() const { return &IX; };
    const sorted_vector<T,K>& operator*() const { return IX; };
    const sorted_vector<T,K>* operator->() const { return &IX; };
  };

 public:
  const K           Key;        
 private:
  static sorted_vector<T,K> Index;
  static Mutex      Mtx; // protect the index above

 protected: // It does not make sense to create objects of this type directly.
  inst_index(const K& key) : Key(key) {}
  ~inst_index();
 public:
  // Get an existing instance of T or return NULL.
  static int_ptr<T> FindByKey(const K& key);
 protected:
  // Get an existing instance of T or create a new one.
  static int_ptr<T> GetByKey(K& key, IFactory& factory);
};

template <class T, class K>
inst_index<T,K>::~inst_index()
{ DEBUGLOG(("inst_index<%p>(%p)::~inst_index()\n", &Index, this));
  // Deregister from the repository
  // The deregistration is a bit too late, because destructors from the derived
  // class may already be called. But the objects T must be reference counted.
  // And FindByKey/GetByKey checks for the reference counter before it takes
  // T in the repository as a valid object. Furthermore we must check that the
  // instance to remove is really our own one.
  Mutex::Lock lock(Mtx);
  size_t pos;
  if (Index.binary_search(Key, pos))
  { DEBUGLOG(("inst_index::~inst_index: found at %i - %u\n", pos, Index[pos]->compareTo(Key)));
    if (Index[pos]->compareTo(Key) == 0)
      Index.erase(pos);
    // else => another instance is already in the index.
  } else 
    DEBUGLOG(("inst_index::~inst_index: not found at %i\n", pos));
    // else => there is no matching instance
    // This may happen if the reference count on a T instance goes to zero and
    // while the instance is not yet deregistered a new instance is created
    // in the index and it's inst_index destructor is already called. 
}

template <class T, class K>
int_ptr<T> inst_index<T,K>::FindByKey(const K& key)
{ DEBUGLOG(("inst_index<>(%p)::FindByKey(&%p)\n", &Index, &key));
  Mutex::Lock lock(Mtx);
  T* p = Index.find(key);
  CritSect cs;
  return p && !p->RefCountIsUnmanaged() ? p : NULL; 
}

template <class T, class K>
int_ptr<T> inst_index<T,K>::GetByKey(K& key, IFactory& factory)
{ DEBUGLOG(("inst_index<>(%p)::GetByKey(&%p, &%p)\n", &Index, &key, &factory));
  Mutex::Lock lock(Mtx);
  T*& p = Index.get(key);
  { CritSect cs;
    if (p && !p->RefCountIsUnmanaged())
      return p;
  }
  // We must not assign p directly because the factory might have destroyed *p already
  // by deleting the newly created item. Also the factory might never have created an item of
  // type T. In this case we have to destroy the entry.
  T* pf = factory(key);
  if (pf == NULL)
    // Factory failed => remove the slot immediately if not yet done.
    // There is nothing to delete since we did not yet assign anything.
    Index.erase(key);
  else
    // Succseeded => assign the newly created instance.
    p = pf;
  return pf;
}

template <class T, class K>
sorted_vector<T,K> inst_index<T,K>::Index;
template <class T, class K>
Mutex inst_index<T,K>::Mtx;
  

#endif
