/*
 * Copyright 2007-2013 M.Mueller
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


/** @brief Function to compare instances of a type.
 * @details The comparer provides an unspecified but stable order.
 */
template <class T>
int CompareInstance(const T& l, const T& r)
{ return (char*)&l - (char*)&r; // Dirty but working unless the virtual address space is larger than 2 GiB, which is not the case on OS/2.
}

/** Sorted variant of vector using the logical key type \a K.
 * @tparam T Element type. Only pointers to \a T are stored in this container.
 * @tparam K Key type. This might be the same than the element type T if the collection has the semantics of a set.
 * In fact it is only the first argument to the comparer \a C.
 * @tparam C Comparer. Compares a given key reference to an element reference and returns the result.
 */
template <class T, class K, sort_comparer(K,T)>
class sorted_vector : public vector<T>
{public:
  /// Create a new vector with a given initial capacity.
  /// @param capacity Initial capacity.
  /// If capacity is 0 the vector is initially created empty
  /// and allocated with the default capacity when the first item is inserted.
  sorted_vector(size_t capacity = 0) : vector<T>(capacity) {}
  /// Copy constructor, O(n).
  sorted_vector(const sorted_vector<T,K,C>& r, size_t spare = 0) : vector<T>(r, spare) {}
  /// @brief Search for a given key. O(log n)
  /// @return The function returns a flag whether you got an exact match or not.
  /// @param pos [out] The index of the first element >= key is always returned in the output parameter \a pos.
  bool               locate(const K& key, size_t& pos) const
                     { return ::binary_search(&key, pos, *this, (int (*)(const void*, const void*))C); }
  /// @brief Find an element by it's key. O(log n)
  /// @return The function will return \c NULL if no such element is in the container.
  T*                 find(const K& key) const;
  /// @brief Ensure an element with a particular key. O(n)
  /// @return This will either return a reference to a pointer to an existing object which equals \a key
  /// or a reference to a \c NULL pointer which is automatically created at the location in the container
  /// where a new object with \a key should be inserted. So you can store the Pointer to this object after the function returned.
  /// There is no check whether the assigned element matches \a key, but if not then subsequent calls
  /// to \c btree_base functions have undefined behavior. It is also an error not to assign
  /// a element if the function returned a reference to NULL.
  T*&                get(const K& key);
  /// Erase the element which equals key and return the removed pointer. O(n)
  /// @return If no such element exists the function returns NULL.
  T*                 erase(const K& key);
  // IBM VAC++ can't parse using...
  T*                 erase(T*const*& where)         { return vector<T>::erase(where); }
  T*                 erase(size_t where)            { return vector<T>::erase(where); }
};


/* Template implementations */
template <class T, class K, sort_comparer(K,T)>
inline T* sorted_vector<T,K,C>::find(const K& key) const
{ size_t pos;
  return locate(key, pos) ? (*this)[pos] : NULL;
}

template <class T, class K, sort_comparer(K,T)>
inline T*& sorted_vector<T,K,C>::get(const K& key)
{ size_t pos;
  return locate(key, pos) ? (*this)[pos] : (vector<T>::insert(pos) = NULL);
}

template <class T, class K, sort_comparer(K,T)>
inline T* sorted_vector<T,K,C>::erase(const K& key)
{ size_t pos;
  return locate(key, pos) ? vector<T>::erase(pos) : NULL;
}


/** @brief Sorted variant of vector_own using the key type \a K.
 * @tparam T The vectors element type. The vector stores only pointers to T.
 * But the vector owns all its elements.
 * @tparam K The vectors key type.
 * @tparam C Comparer. Compares a given key reference to an element reference and returns the result.
 * @details The class owns the referenced objects exclusively.
 * But only the functions that delete or copy the entire container are different.
 * To erase an element you must use @code delete erase(...) @endcode.
 * The class methods are not synchronized.
 */
template <class T, class K, sort_comparer(K,T)>
class sorted_vector_own : public sorted_vector<T,K,C>
{protected:
  /// @brief Copy constructor
  /// @remarks Since sorted_vector_own own its object exclusively this copy constructor must do
  /// a deep copy of the vector. This is up to the derived class!
  /// You may use vector_own_base_copy to do the job.
  sorted_vector_own(const sorted_vector_own<T,K,C>& r, size_t spare = 0) : sorted_vector<T,K,C>(r.size() + spare) {}
  /// @brief Assignment
  /// @remarks Since sorted_vector_own own its object exclusively this assignment must do
  /// a deep copy of the vector. This is up to the derived class!
  /// You may use vector_own_base_copy to do the job.
  void               operator=(const sorted_vector_own<T,K,C>& r) { clear(); prepare_assign(r.size()); }
 public:
  /// Create a new vector with a given initial capacity.
  /// @param capacity Initial capacity.
  /// If capacity is 0 the vector is initially created empty
  /// and allocated with the default capacity when the first item is inserted.
  sorted_vector_own(size_t capacity = 0) : sorted_vector<T,K,C>(capacity) {}
  /// @brief Adjust the size to a given value
  /// @details If the array is increased NULL values are appended.
  void               set_size(size_t size)          { if (this->size() > size) vector_own_base_destroy(*this, size); vector<T>::set_size(size); }
  /// Remove and destroy all elements in the vector.
  void               clear()                        { vector_own_base_destroy(*this); }
  /// Destroy the vector and all elements in it.
                     ~sorted_vector_own()           { clear(); }
};


/** Sorted vector of objects with members with intrusive reference counter.
 */
template <class T, class K, sort_comparer(K,T)>
class sorted_vector_int : public vector_int<T>
{public:
  /// Create a new vector with a given initial capacity.
  /// @param capacity Initial capacity. If capacity is 0
  /// the vector is initially created empty and allocated
  /// with the default capacity when the first item is inserted.
  sorted_vector_int(size_t capacity = 0) : vector_int<T>(capacity) {}

  /// Search for a given key.
  /// @return The function returns whether you got an exact match or not.
  /// @param pos [out] The index of the first element >= key is always returned in pos.
  /// @remarks Performance: O(log(size()))
  bool               locate(const K& key, size_t& pos) const
                     { return ::binary_search(&key, pos, *this, (int (*)(const void*, const void*))C); }
  /// Find an element by it's key.
  /// @return The function will return \c NULL if no such element is in the container.
  /// @remarks Performance: O(log(size()))
  int_ptr<T>         find(const K& key) const;
  /// Ensure an element with a particular key.
  /// @return This will either return a reference to a pointer to an existing object which equals to key
  /// or a reference to a \c NULL pointer which is automatically created at the location in the container
  /// where a new object with key should be inserted. So you can store the Pointer to this object after the function returned.
  /// @remarks Performance: O(log(size()))
  int_ptr<T>&        get(const K& key);
  /// Erase the element which equals key and return the removed pointer.
  /// If no such element exists the function returns NULL.
  /// @remarks Performance: O(log(size()))
  int_ptr<T>         erase(const K& key);
  // IBM VAC++ can't parse using...
  int_ptr<T>         erase(const int_ptr<T>*& where){ return vector_int<T>::erase(where); }
  int_ptr<T>         erase(size_t where)            { return vector_int<T>::erase(where); }
};

/* Template implementations */
template <class T, class K, sort_comparer(K,T)>
inline int_ptr<T> sorted_vector_int<T,K,C>::find(const K& key) const
{ size_t pos;
  return locate(key, pos) ? (*this)[pos] : NULL;
}

template <class T, class K, sort_comparer(K,T)>
inline int_ptr<T>& sorted_vector_int<T,K,C>::get(const K& key)
{ size_t pos;
  return locate(key, pos) ? (*this)[pos] : vector_int<T>::insert(pos);
}

template <class T, class K, sort_comparer(K,T)>
int_ptr<T> sorted_vector_int<T,K,C>::erase(const K& key)
{ size_t pos;
  int_ptr<T> ret;
  if (locate(key, pos))
    ret = vector_int<T>::erase(pos);
  return ret;
}

#endif
