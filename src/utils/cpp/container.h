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


#ifndef CPP_CONTAINER_H
#define CPP_CONTAINER_H

#include <cpp/mutex.h>
#include <cpp/smartptr.h>


/* Internal logically abstract base class of vector<T> with all non-template core implementations.
 * This class can only store reference type objects (pointers).
 * It does not handle the ownership of the referenced objects.
 * The class methods are not synchronized.
 */
class vector_base
{private:
  void** Data;
  size_t Size;
  size_t Capacity;

 protected:
  // create a new vector with a given initial capacity.
  // If capacity is 0 the vector is initially created empty
  // and allocated with the default capacity when the first item is inserted.
  vector_base(size_t capacity = 0);
  // copy constructor
  vector_base(const vector_base& r, size_t spare = 0);
  // Destroy the vector and delete all stored references.
  // This will not affect the referenced objects.
  ~vector_base();
  // assignment
  void               operator=(const vector_base& r);
  // swap content of two instances
  void               swap(vector_base& r);
  // Append a new element to the end of the vector.
  // The function does not take the value of the new element.
  // Instead it returns a reference to the new location that should be assigned later.
  // The reference is valid until the next non-const member function call.
  // The initial value is undefined.
  // The function is not type safe and should not be exposed public.
  void*&             append();
  // Insert a new element in the vector at location where
  // Precondition: where in [0,size()], Performance: O(n)
  // The function does not take the value of the new element.
  // Instead it returns a reference to the new location that should be assigned later.
  // The reference is valid until the next non-const member function call.
  // The initial value is undefined.
  // The function is not type safe and should not be exposed public.
  void*&             insert(size_t where);
  // Removes an element from the vector and return it's value.
  // The where pointer is set to the next item after the removed one.
  // Precondition: where in [begin(),end()), Performance: O(n)
  // The function is not type safe and should not be exposed public.
  void*              erase(void**& where);
  // Removes an element from the vector and return it's value.
  // Precondition: where in [0,size()-1], Performance: O(n)
  // The function is not type safe and should not be exposed public.
  void*              erase(size_t where)            { void** p = Data + where; return erase(p); }
  // Access an element by number.
  // Precondition: where in [0,size()-1], Performance: O(1)
  // This is in fact like operator[], but since this method is not type safe
  // it should not be exposed public.
  void*&             at(size_t where) const         { return Data[where]; }
  // Initialize storage for new assignment.
  // Postcondition: Size = size && Capacity >= size.
  void               prepare_assign(size_t size);
 public:
  // Return the number of elements in the container.
  // This is not equal to the storage capacity.
  size_t             size() const                   { return Size; }
  // Remove all elements
  void               clear()                        { Size = 0; }
  // Move the i-th element in the list to the location where j is now.
  // Precondition: from and to in [0,size()-1], Performance: O(n)
  void               move(size_t from, size_t to);
  // Ensure that the container can store size elements without reallocation.
  // Reserve will also shrink the storage. It is an error if size is less than the actual size.
  void               reserve(size_t size);

  friend bool        binary_search_base(const vector_base& data, int (*fcmp)(const void* elem, const void* key),
                                        const void* key, size_t& pos);
};

/* Type safe vector of reference type objects (pointers).
 * The class does not handle the ownership of the referenced objects.
 * The class methods are not synchronized.
 */
template <class T>
class vector : public vector_base
{public:
  // create a new vector with a given initial capacity.
  // If capacity is 0 the vector is initially created empty
  // and allocated with the default capacity when the first item is inserted.
  vector(size_t capacity = 0) : vector_base(capacity) {}
  // assignment
  //vector<T>&         operator=(const vector<T>& r)  { vector_base::operator=(r); return *this; }
  // swap content of two instances
  void               swap(vector<T>& r)             { vector_base::swap(r); }
  // Append a new element to the end of the vector.
  // The function does not take the value of the new element.
  // Instead it returns a reference to the new location that should be assigned later.
  // The reference is valid until the next non-const member function call.
  // The initial value of the new element is undefined and must be assigned before the next access.
  T*&                append()                       { return (T*&)vector_base::append(); }
  // Insert a new element in the vector at location where
  // Precondition: where in [0,size()], Performance: O(n)
  // The function does not take the value of the new element.
  // Instead it returns a reference to the new location that should be assigned later.
  // The reference is valid until the next non-const member function call.
  // The initial value of the new element is undefined and must be assigned before the next access.
  T*&                insert(size_t where)           { return (T*&)vector_base::insert(where); }
  // Removes an element from the vector and return it's value.
  // The where pointer is set to the next item after the removed one.
  // Precondition: where in [begin(),end()), Performance: O(n)
  T*                 erase(T*const*& where)         { return (T*)vector_base::erase((void**&)where); }
  // Removes an element from the vector and return it's value.
  // Precondition: where in [0,size()-1], Performance: O(n)
  T*                 erase(size_t where)            { return (T*)vector_base::erase(where); }
  // Access an element by it's index.
  // Precondition: where in [0,size()-1], Performance: O(1)
  // This is the constant version of the [] operator. Since we deal only with references
  // it returns a value copy rather than a const reference to the element.
  T*                 operator[](size_t where) const { return (T*)at(where); }
  // Access an element by it's index.
  // Precondition: where in [0,size()-1], Performance: O(1)
  T*&                operator[](size_t where)       { return (T*&)at(where); }
  // Get a constant iterator that points to the firs element or past the end if the vector is empty.
  // Precondition: none, Performance: O(1)
  T*const*           begin() const                  { return &(T*&)at(0); }
  // Get a iterator that points to the firs element or past the end if the vector is empty.
  // Precondition: none, Performance: O(1)
  T**                begin()                        { return &(*this)[0]; }
  // Get a constant iterator that points past the end.
  // Precondition: none, Performance: O(1)
  T*const*           end() const                    { return &(T*&)at(size()); }
  // Get a iterator that points past the end.
  // Precondition: none, Performance: O(1)
  T**                end()                          { return &(*this)[size()]; }
};

/* Helper functions to implement some strongly typed functionality only once per type T.
 * Intrnal use only
 */
template <class T>
void vector_own_base_destroy(vector<T>& v)
{ T** tpp = v.end();
  while (tpp != v.begin())
    delete *--tpp;
}

template <class T>
void vector_own_base_copy(vector<T>& d, const T*const* spp)
{ T** epp = d.end();
  T** dpp = d.begin();
  while (dpp != epp)
    *dpp++ = new T(**spp++); // copy contructor of T
}


/* Type safe vector of reference type objects (pointers).
 * The class owns the referenced objects.
 * But only the function that delete or copy the entire container are different.
 * To erase an elemet you must use "delete erase(...)".
 * The class methods are not synchronized.
 */
template <class T>
class vector_own : public vector<T>
{protected:
  // copy constructor
  // Note that since vector_own own its object exclusively this copy constructor must do
  // a deep copy of the vector. This is up to the derived class!
  // You may use vector_own_base_copy to do the job.
  vector_own(const vector_own<T>& r, size_t spare = 0) : vector<T>(r.size() + spare) {}
  // assignment: same problem as above
  vector_own<T>&     operator=(const vector_own<T>& r);
 public:
  // create a new vector with a given initial capacity.
  // If capacity is 0 the vector is initially created empty
  // and allocated with the default capacity when the first item is inserted.
  vector_own(size_t capacity = 0) : vector<T>(capacity) {}
  // destructor
                     ~vector_own()                  { clear(); }
  // Remove all elements
  void               clear();
};

template <class T>
vector_own<T>& vector_own<T>::operator=(const vector_own<T>& r)
{ clear();
  prepare_assign(r.size());
  //vector_own_base_copy(*this, r.begin());
  return *this;
}

template <class T>
inline void vector_own<T>::clear()
{ vector_own_base_destroy(*this);
  vector<T>::clear();
}


/* Interface for compareable objects.
 * A class that implements this interface with the template parameter K is comparable to const K&.
 * The type K may be the same as the class that implements this interface or not.
 * Note: you may implement IComparableTo<const char*> to be comparable to ordinary C strings.
 */
template <class K>
struct IComparableTo
{ virtual int compareTo(const K& key) const = 0;
 protected:
  // protect destructor to avoid that someone deletes an object of this type through the interface
  // without having a virtual destructor. However, we force the destructor to be virtual to keep gcc happy.
  virtual ~IComparableTo() {}
};


bool binary_search_base(const vector_base& data, int (*fcmp)(const void* elem, const void* key),
  const void* key, size_t& pos);

/* Sorted variant of vector using the key type K.
 * Object in this container must implement IComparableTo<K>
 */
template <class T, class K>
class sorted_vector : public vector<T>
{public:
  // Create a new vector with a given initial capacity.
  // If capacity is 0 the vector is initially created empty
  // and allocated with the default capacity when the first item is inserted.
  sorted_vector(size_t capacity = 0) : vector<T>(capacity) {}
  // Search for a given key.
  // The function returns whether you got an exact match or not.
  // The index of the first element >= key is always returned in the output parameter pos.
  // Precondition: none, Performance: O(log(n))
  bool               binary_search(const K& key, size_t& pos) const
                     { return binary_search_base(*this, &sorted_vector<T,K>::Comparer, &key, pos); }
  // Find an element by it's key.
  // The function will return NULL if no such element is in the container.
  // Precondition: none, Performance: O(log(n))
  T*                 find(const K& key) const;
  // Ensure an element with a particular key.
  // This will either return a reference to a pointer to an existing object which equals to key
  // or a reference to a NULL pointer which is automatically created at the location in the container
  // where a new object with key should be inserted. So you can store the Pointer to this object after the funtion returned.
  // Precondition: none, Performance: O(log(n))
  T*&                get(const K& key);
  // Erase the element which equals key and return the removed pointer.
  // If no such element exists the function returns NULL.
  // Precondition: none, Performance: O(log(n))
  T*                 erase(const K& key);
  // IBM VAC++ can't parse using...
  T*                 erase(T*const*& where)         { return vector<T>::erase(where); }
  T*                 erase(size_t where)            { return vector<T>::erase(where); }
 private:
  static int         Comparer(const void* elem, const void* key);
};


/* Template implementations */
template <class T, class K>
T* sorted_vector<T,K>::find(const K& key) const
{ size_t pos;
  return binary_search(key, pos) ? (*this)[pos] : NULL;
}

template <class T, class K>
T*& sorted_vector<T,K>::get(const K& key)
{ size_t pos;
  return binary_search(key, pos) ? (*this)[pos] : (insert(pos) = NULL);
}

template <class T, class K>
T* sorted_vector<T,K>::erase(const K& key)
{ size_t pos;
  return binary_search(key, pos) ? vector<T>::erase(pos) : NULL;
}

template <class T, class K>
int sorted_vector<T,K>::Comparer(const void* elem, const void* key)
{ return ((T*)elem)->compareTo(*(const K*)key);
}


/* Sorted variant of vector_own using the key type K.
 * Object in this container must implement IComparableTo<K>
 * The class owns the referenced objects.
 * But only the function that delete or copy the entire container are different.
 * To erase an elemet you must use "delete erase(...)".
 * The class methods are not synchronized.
 */
template <class T, class K>
class sorted_vector_own : public sorted_vector<T, K>
{public:
  // create a new vector with a given initial capacity.
  // If capacity is 0 the vector is initially created empty
  // and allocated with the default capacity when the first item is inserted.
  sorted_vector_own(size_t capacity = 0) : sorted_vector<T, K>(capacity) {}
  // copy constructor
  sorted_vector_own(const sorted_vector_own<T, K>& r, size_t spare = 0);
  // destructor
                     ~sorted_vector_own()           { clear(); }
  // assignment
  sorted_vector_own<T, K>& operator=(const sorted_vector_own<T, K>& r);
  // Remove all elements
  void               clear();
};

template <class T, class K>
inline sorted_vector_own<T, K>::sorted_vector_own(const sorted_vector_own<T, K>& r, size_t spare)
: sorted_vector<T, K>(r.size() + spare)
{ vector_own_base_copy(*(vector<T>*)this, r.begin());
}

template <class T, class K>
sorted_vector_own<T, K>& sorted_vector_own<T, K>::operator=(const sorted_vector_own<T, K>& r)
{ clear();
  prepare_assign(r.size());
  vector_own_base_copy(*this, r.begin());
  return *this;
}

template <class T, class K>
inline void sorted_vector_own<T, K>::clear()
{ vector_own_base_destroy(*this);
  vector<T>::clear();
}


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
 * The Instances of type T must implement Iref_Count and ICompareable<K*>
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
  { DEBUGLOG(("inst_index::~inst_index: found at %i - %u\n", pos, Index[pos] == this));
    if (Index[pos] == this)
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
  p = factory(key);
  return p;
}

template <class T, class K>
sorted_vector<T,K> inst_index<T,K>::Index;
template <class T, class K>
Mutex inst_index<T,K>::Mtx;
  

/* Algorithmns */

// rotate pointer array in place
void rotate_array_base(void** begin, const size_t len, int shift);

template <class T>
inline void rotate_array(T** begin, const size_t len, int shift)
{ rotate_array_base((void**)begin, len, shift);
}

// Sort an array of pointers
void merge_sort_base(void** begin, void** end, int (*comp)(const void* l, const void* r));

template <class T>
inline void merge_sort(T** begin, T** end, int (*comp)(const T* l, const T* r))
{ merge_sort_base((void**)begin, (void**)end, (int (*)(const void* l, const void* r))comp);
}

//template <class T>
//T& binary_search(

#endif
