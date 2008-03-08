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
  // Precondition: where in [begin(),end()), Performance: O(n)
  // The function is not type safe and should not be exposed public.
  void*              erase(void** where);
  // Removes an element from the vector and return it's value.
  // Precondition: where in [0,size()-1], Performance: O(n)
  // The function is not type safe and should not be exposed public.
  void*              erase(size_t where)            { return erase(Data + where); }
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
  // Ensure that the container can store size elements without reallocation.
  // Reserve will also shrink the storage. It is an error if size is less than the actual size.
  void               reserve(size_t size);
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
  // Precondition: where in [begin(),end()), Performance: O(n)
  T*                 erase(T*const* where)          { return (T*)vector_base::erase((void**)where); }
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
{public:
  // create a new vector with a given initial capacity.
  // If capacity is 0 the vector is initially created empty
  // and allocated with the default capacity when the first item is inserted.
  vector_own(size_t capacity = 0) : vector<T>(capacity) {}
  // copy constructor
  vector_own(const vector_own<T>& r, size_t spare = 0);
  // destructor
                     ~vector_own()                  { clear(); }
  // assignment
  vector_own<T>&     operator=(const vector_own<T>& r);
  // Remove all elements
  void               clear();
};

template <class T>
inline vector_own<T>::vector_own(const vector_own<T>& r, size_t spare)
: vector<T>(r.size() + spare)
{ vector_own_base_copy(*this, r.begin());
}

template <class T>
vector_own<T>& vector_own<T>::operator=(const vector_own<T>& r)
{ clear();
  prepare_assign(r.size());
  vector_own_base_copy(*this, r.begin());
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
  bool               binary_search(const K& key, size_t& pos) const;
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
  T*                 erase(size_t where)            { return vector<T>::erase(where); }
};


/* Template implementations */
template <class T, class K>
bool sorted_vector<T,K>::binary_search(const K& key, size_t& pos) const
{ // binary search
  DEBUGLOG(("sorted_vector<T,K>(%p)::binary_search(%p,&%p) - %u\n", this, &key, &pos, size()));
  size_t l = 0;
  size_t r = size();
  while (l < r)
  { size_t m = (l+r) >> 1;
    int cmp = (*this)[m]->compareTo(key);
    DEBUGLOG2(("sorted_vector<T,K>::binary_search %u-%u %u->%p %i\n", l, r, m, (*this)[m], cmp));
    if (cmp == 0)
    { pos = m;
      return true;
    }
    if (cmp < 0)
      l = m+1;
    else
      r = m;
  }
  pos = l;
  return false;
}

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
 * identified by a key K. The Instances of type T must implement ICompareable<K>.
 * Classes of type T mut inherit from inst_repository<T, K> to implement this feature.
 * The class below does not hold strong references to the  
 */
/*template <class T, class K>
class inst_repository
{private:
  static sorted_vector<T, K> RPInst;
  static Mutex      RPMutex; // protect the index above
 protected:
  inst_repository() {}
  ~inst_repository() {}
 public:
  static int_ptr<T> FindByKey(const K& key);
  static int_ptr<T> GetByKey(const K& key);
};*/
 
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
