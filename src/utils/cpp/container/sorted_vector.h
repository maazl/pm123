/*
 * Copyright 2007-2008 M.Mueller
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


#ifndef CPP_CONTAINER_SORTED_VECTOR_H
#define CPP_CONTAINER_SORTED_VECTOR_H

#include <cpp/smartptr.h>
#include <cpp/algorithm.h>
#include <cpp/container/vector.h>


/* @brief Interface for comparable objects.
 * @details A class that implements this interface with the template parameter \a K is comparable to \c const&nbsp;K&.
 * The type \a K may be the same as the class that implements this interface or not.
 * @remarks Note: you may implement \c IComparableTo<const\ char*> to be comparable to ordinary C strings.
 *
template <class K>
struct IComparableTo
{ virtual int compareTo(const K& key) const = 0;
 protected:
  // protect destructor to avoid that someone deletes an object of this type through the interface
  // without having a virtual destructor.
                     ~IComparableTo() {}
};*/

/** @brief Function to compare instances of a type.
 * @details The comparer provides an unspecified but stable order.
 */
template <class T>
int CompareInstance(const T& l, const T& r)
{ return (char*)&l - (char*)&r; // Dirty but working unless the virtual address space is larger than 2 GiB, which is not the case on OS/2.
}
/** @brief Function to compare pointers by value.
 * @details The comparer provides an unspecified but stable order.
 */
template <class T>
int ComparePtr(T*const& l, T*const& r)
{ return (char*)l - (char*)r; // Dirty but working unless the virtual address space is larger than 2 GiB, which is not the case on OS/2.
}


/*template <class T, class K>
struct sorted_vector_comparer
{ static int cmp(const void* elem, const void* key);
};
template <class T, class K>
int sorted_vector_comparer<T,K>::cmp(const void* elem, const void* key)
{ return ((T*)elem)->compareTo(*(const K*)key);
}*/

/* Sorted variant of vector using the key type K.
 * Object in this container must implement IComparableTo<K>
 */
template <class T, class K, int (*C)(const T& e, const K& k)>
class sorted_vector : public vector<T>
{public:
  // Create a new vector with a given initial capacity.
  // If capacity is 0 the vector is initially created empty
  // and allocated with the default capacity when the first item is inserted.
  sorted_vector(size_t capacity = 0) : vector<T>(capacity) {}
  // Copy constructor.
  sorted_vector(const sorted_vector<T,K,C>& r, size_t spare = 0) : vector<T>(r, spare) {}
  // Search for a given key.
  // The function returns whether you got an exact match or not.
  // The index of the first element >= key is always returned in the output parameter pos.
  // Precondition: none, Performance: O(log(n))
  bool               binary_search(const K& key, size_t& pos) const
                     { return binary_search_base(*this, (int (*)(const void*, const void*))C, &key, pos); }
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
};


/* Template implementations */
template <class T, class K, int (*C)(const T& e, const K& k)>
inline T* sorted_vector<T,K,C>::find(const K& key) const
{ size_t pos;
  return binary_search(key, pos) ? (*this)[pos] : NULL;
}

template <class T, class K, int (*C)(const T& e, const K& k)>
inline T*& sorted_vector<T,K,C>::get(const K& key)
{ size_t pos;
  return binary_search(key, pos) ? (*this)[pos] : (insert(pos) = NULL);
}

template <class T, class K, int (*C)(const T& e, const K& k)>
inline T* sorted_vector<T,K,C>::erase(const K& key)
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
template <class T, class K, int (*C)(const T& e, const K& k)>
class sorted_vector_own : public sorted_vector<T,K,C>
{protected:
  // Copy constructor
  // Note that since sorted_vector_own own its object exclusively this copy constructor must do
  // a deep copy of the vector. This is up to the derived class!
  // You may use vector_own_base_copy to do the job.
  sorted_vector_own(const sorted_vector_own<T,K,C>& r, size_t spare = 0) : sorted_vector<T,K,C>(r.size() + spare) {}
  // assignment: same problem as above
  void               operator=(const sorted_vector_own<T,K,C>& r) { clear(); prepare_assign(r.size()); }
 public:
  // create a new vector with a given initial capacity.
  // If capacity is 0 the vector is initially created empty
  // and allocated with the default capacity when the first item is inserted.
  sorted_vector_own(size_t capacity = 0) : sorted_vector<T,K,C>(capacity) {}
  // copy constructor
  //sorted_vector_own(const sorted_vector_own<T, K>& r, size_t spare = 0);
  // Adjust the size to a given value
  // If the array is increased NULL values are appended.
  void               set_size(size_t size)          { if (this->size() > size) vector_own_base_destroy(*this, size); vector<T>::set_size(size); }
  // Remove all elements
  void               clear()                        { vector_own_base_destroy(*this); }
  // destructor
                     ~sorted_vector_own()           { clear(); }
  // assignment
  //sorted_vector_own<T, K>& operator=(const sorted_vector_own<T, K>& r);
};


/* Sorted vector of objects with members with intrusive reference counter.
 * Objects in this container must implement Iref_count and IComparableTo<K>
 */
template <class T, class K, int (*C)(const T& e, const K& k)>
class sorted_vector_int : public vector_int<T>
{public:
  // Create a new vector with a given initial capacity.
  // If capacity is 0 the vector is initially created empty
  // and allocated with the default capacity when the first item is inserted.
  sorted_vector_int(size_t capacity = 0) : vector_int<T>(capacity) {}

  // Search for a given key.
  // The function returns whether you got an exact match or not.
  // The index of the first element >= key is always returned in the output parameter pos.
  // Precondition: none, Performance: O(log(n))
  bool               binary_search(const K& key, size_t& pos) const
                     { return binary_search_base(*this, (int (*)(const void*, const void*))C, &key, pos); }
  // Find an element by it's key.
  // The function will return NULL if no such element is in the container.
  // Precondition: none, Performance: O(log(n))
  int_ptr<T>         find(const K& key) const;
  // Ensure an element with a particular key.
  // This will either return a reference to a pointer to an existing object which equals to key
  // or a reference to a NULL pointer which is automatically created at the location in the container
  // where a new object with key should be inserted. So you can store the Pointer to this object after the funtion returned.
  // Precondition: none, Performance: O(log(n))
  int_ptr<T>&        get(const K& key);
  // Erase the element which equals key and return the removed pointer.
  // If no such element exists the function returns NULL.
  // Precondition: none, Performance: O(log(n))
  int_ptr<T>         erase(const K& key);
  // IBM VAC++ can't parse using...
  int_ptr<T>         erase(const int_ptr<T>*& where){ return vector_int<T>::erase(where); }
  int_ptr<T>         erase(size_t where)            { return vector_int<T>::erase(where); }
};

/* Template implementations */
template <class T, class K, int (*C)(const T& e, const K& k)>
inline int_ptr<T> sorted_vector_int<T,K,C>::find(const K& key) const
{ size_t pos;
  return binary_search(key, pos) ? (*this)[pos] : NULL;
}

template <class T, class K, int (*C)(const T& e, const K& k)>
inline int_ptr<T>& sorted_vector_int<T,K,C>::get(const K& key)
{ size_t pos;
  return binary_search(key, pos) ? (*this)[pos] : insert(pos);
}

template <class T, class K, int (*C)(const T& e, const K& k)>
int_ptr<T> sorted_vector_int<T,K,C>::erase(const K& key)
{ size_t pos;
  int_ptr<T> ret;
  if (binary_search(key, pos))
    ret = vector_int<T>::erase(pos);
  return ret;
}

#endif
