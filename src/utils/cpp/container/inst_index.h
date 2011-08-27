/*
 * Copyright 2007-2010 M.Mueller
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

/** @brief Class to implement a repository of all objects instances of a certain type
 * identified by a key \c K.
 * @details The Instances of type T must implement \c Iref_Count.
 * Classes of type \c T should inherit from \c inst_index<T,K,C> to implement this feature.
 * The repository below does not handle the ownership of to the \c T instances.
 * The lifetime management must be done somewhere else.
 * The class is thread-safe.
 */
template <class T, class K, int (*C)(const T&, const K&)>
class inst_index
{public:
  typedef sorted_vector<T,K,C> IndexType;
  /// Requests an exclusive read-only access to the index repository.
  class IXAccess;
  friend class IXAccess;
  class IXAccess : public Mutex::Lock
  {private:
    IndexType&      IX;
   public:
    IXAccess()      : Mutex::Lock(inst_index<T,K,C>::Mtx), IX(inst_index<T,K,C>::Index) {};
    operator const IndexType*() const   { return &IX; };
    const IndexType& operator*() const  { return IX; };
    const IndexType* operator->() const { return &IX; };
  };

 protected:
  static IndexType  Index;
  static Mutex      Mtx; // protect the index above
 private:
  inst_index();     // No instances of this type

 public:
  /// Get an existing instance of \c T or return \c NULL.
  static int_ptr<T> FindByKey(const K& key);
  /// @brief Get an existing instance of \c T or create a new one.
  static int_ptr<T> GetByKey(K& key, T* (*factory)(K&, void*), void* param);
  static int_ptr<T> GetByKey(K& key, T* (*factory)(K&)) { return GetByKey(key, (T* (*)(K&, void*))factory, NULL); }
  /// Remove the current object from the repository, but only if
  /// it is still registered with this key.
  static T*         RemoveWithKey(const T& elem, K& key);
};

template <class T, class K, int (*C)(const T&, const K&)>
int_ptr<T> inst_index<T,K,C>::FindByKey(const K& key)
{ //DEBUGLOG(("inst_index<>(%p)::FindByKey(&%p)\n", &Index, &key));
  Mutex::Lock lock(Mtx);
  T* p = Index.find(key);
  return p && !p->RefCountIsUnmanaged() ? p : NULL;
}

template <class T, class K, int (*C)(const T&, const K&)>
int_ptr<T> inst_index<T,K,C>::GetByKey(K& key, T* (*factory)(K&, void*), void* param)
{ //DEBUGLOG(("inst_index<>(%p)::GetByKey(&%p,)\n", &Index, &key));
  Mutex::Lock lock(Mtx);
  T*& p = Index.get(key);
  if (p && !p->RefCountIsUnmanaged())
    return p;
  // We must not assign p directly because the factory might have destroyed *p already
  // by deleting the newly created item. Also the factory might never have created an item of
  // type T. In this case we have to destroy the entry.
  try
  { p = factory(key, param);
  } catch (...)
  { // Factory failed => remove the slot immediately if not yet done.
    // There is nothing to delete since we did not yet assign anything.
    T*const* p2 = &p;
    Index.erase(p2);
    throw;
  }
  if (p == NULL)
  { // Factory failed => remove the slot immediately if not yet done.
    // There is nothing to delete since we did not yet assign anything.
    T*const* p2 = &p;
    Index.erase(p2);
    return NULL;
  } else
  { // Succeeded => return the newly created instance.
    return p;
  }
}

template <class T, class K, int (*C)(const T&, const K&)>
T* inst_index<T,K,C>::RemoveWithKey(const T& elem, K& key)
{ DEBUGLOG(("inst_index<%p>::RemoveWithKey(&%p, &%p)\n", &Index, &elem, &key));
  // Deregister from the repository
  // The deregistration is a bit too late, because destructors from the derived
  // class may already be called. But the objects T must be reference counted.
  // And FindByKey/GetByKey checks for the reference counter before it takes
  // T in the repository as a valid object. Furthermore we must check that the
  // instance to remove is really our own one.
  Mutex::Lock lock(Mtx);
  size_t pos;
  if (Index.binary_search(key, pos))
  { //DEBUGLOG(("inst_index::~inst_index: found at %i - %p\n", pos, Index[pos]));
    if (Index[pos] == &elem)
      return Index.erase(pos);
    // else => another instance is already in the index.
  } //else
    //DEBUGLOG(("inst_index::~inst_index: not found at %i\n", pos));
    // else => there is no matching instance
    // This may happen if the reference count on a T instance goes to zero and
    // while the instance is not yet deregistered a new instance is created
    // in the index and it's inst_index destructor is already called.
  return NULL;
}

template <class T, class K, int (*C)(const T&, const K&)>
inst_index<T,K,C>::IndexType inst_index<T,K,C>::Index;
template <class T, class K, int (*C)(const T&, const K&)>
Mutex inst_index<T,K,C>::Mtx;

/*// Due to the nature of the repository comparing instances is equivalent
// to comparing the pointers, because the key of different instances
// MUST be different and there are no two instances with the same key.
template <class T, class K, int (*C)(const K&, const K&)>
inline bool operator==(const inst_index<T,K,C>& l, const inst_index<T,K,C>& r)
{ return &l == &r;
}
template <class T, class K, int (*C)(const K&, const K&)>
inline bool operator!=(const inst_index<T,K,C>& l, const inst_index<T,K,C>& r)
{ return &l != &r;
}*/


/** Same as inst_index but with strongly typed factory */
template <class T, class K, int (*C)(const T&, const K&), class P>
class inst_index2 : public inst_index<T,K,C>
{public:
  static int_ptr<T> GetByKey(K& key, T* (*factory)(K&, P&), P& param);
};

template <class T, class K, int (*C)(const T&, const K&), class P>
int_ptr<T> inst_index2<T,K,C,P>::GetByKey(K& key, T* (*factory)(K&, P&), P& param)
{ return inst_index<T,K,C>::GetByKey(key, (T* (*)(K&, void*))factory, (void*)&param);
}

#endif
