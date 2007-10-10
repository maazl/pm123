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

 private: // non-copyable
  vector_base(const vector_base& r);
  void operator=(const vector_base& r);
 protected:
  // create a new vector with a given initial capacity.
  // The initial capacity must not be less or equal to 0.
  vector_base(size_t capacity = 16);
  // Destroy the vector and delete all stored references.
  // This will not affect the referenced objects.
  ~vector_base();
  // Insert a new element in the vector at location where
  // Precondition: where in [0,size()], Performance: O(n)
  // The function does not take the value of the new element.
  // Instead it returns a reference to the new location that should be assigned later.
  // The reference is valid until the next non-const member function call.
  // The initial value is undefined.
  // The function is not type safe and should not be exposed public.  
  void*&             insert(size_t where);
  // Removes an element from the vector and return it's value.
  // Precondition: where in [0,size()-1], Performance: O(n)
  // The function is not type safe and should not be exposed public.  
  void*              erase(size_t where);
  // Access an element by number.
  // Precondition: where in [0,size()-1], Performance: O(1)
  // This is in fact like operator[], but since this method is not type safe
  // it should not be exposed public.
  void*&             at(size_t where) const         { return Data[where]; }
 public:
  // Return the number of elements in the container.
  // This is not equal to the storage capacity.
  size_t             size() const                   { return Size; }
};

/* Type safe vector of reference type objects (pointers).
 * The class does not handle the ownership of the referenced objects.
 * The class methods are not synchronized.
 */
template <class T>
class vector : public vector_base
{public:
  // create a new vector with a given initial capacity.
  // The initial capacity must not be less or equal to 0.
  vector(size_t capacity = 16) : vector_base(capacity) {}
  // Insert a new element in the vector at location where
  // Precondition: where in [0,size()], Performance: O(n)
  // The function does not take the value of the new element.
  // Instead it returns a reference to the new location that should be assigned later.
  // The reference is valid until the next non-const member function call.
  // The initial value of the new element is undefined and must be assigned before the next access.
  T*&                insert(size_t where)           { return (T*&)vector_base::insert(where); }
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

/* Interface for compareable objects.
 * A class that implements this interface with the template parameter K is comparable to const K*.
 * The type K may be the same as the class that implements this interface or not.
 * Note: you may implement IComparableTo<char> to be comparable to ordinary C strings.
 */
template <class K>
struct IComparableTo
{ virtual int CompareTo(const K* key) const = 0;
};


/* Base class for a sorted vector.
 * This is a unique associative container.
 * It is intrusive to the stored objects by the way that they must implement IComparable<K>.
 * The referenced objects must not change their keys while they are in the container.
 */
template <class K>
class sorted_vector_base : public vector<IComparableTo<K> >
{public:
  // Create a new vector with a given initial capacity.
  // The initial capacity must not be less or equal to 0.
  sorted_vector_base(size_t capacity) : vector<IComparableTo<K> >(capacity) {}
  // Search for a given key.
  // The function returns whether you got an exact match or not.
  // The index of the first element >= key is always returned in the output parameter pos.
  // Precondition: none, Performance: O(log(n))
  bool               binary_search(const K* key, size_t& pos) const;
  // Find an element by it's key.
  // The function will return NULL if no such element is in the container.
  // Precondition: none, Performance: O(log(n))
  IComparableTo<K>*  find(const K* key) const;
  // Ensure an element with a particular key.
  // This will either return a reference to a pointer to an existing object which equals to key
  // or a reference to a NULL pointer which is automatically createt at the location in the container
  // where a new object with key should be inserted. So you can store the Pointer to this object after the funtion returned.
  // Precondition: none, Performance: O(log(n))
  IComparableTo<K>*& get(const K* key);
  // Erase the element which equals key and return the removed pointer.
  // If no such element exists the function returns NULL.
  // Precondition: none, Performance: O(log(n))
  IComparableTo<K>*  erase(const K* key);
  // IBM VAC++ can't parse using...
  IComparableTo<K>*  erase(size_t where)            { return vector<IComparableTo<K> >::erase(where); }
};

/* Type-safe implementation of sorted_vector_base.
 * This class shares all properties of sorted_vector_base,
 * except that it is strongly typed to objects of type T which must implement IComparableTo<K>.
 * It is recommended to prefer this type-safe implementation over sorted_vector_base. 
 */
template <class T, class K>
class sorted_vector : public sorted_vector_base<K>
{public:
  // Create a new vector with a given initial capacity.
  // The initial capacity must not be less or equal to 0.
  sorted_vector(size_t capacity) : sorted_vector_base<K>(capacity) {}
  // Find an element by it's key.
  // The function will return NULL if no such element is in the container.
  // Precondition: none, Performance: O(log(n))
  T*                 find(const K* key) const       { return (T*)sorted_vector_base<K>::find(key); }
  // Ensure an element with a particular key.
  // This will either return a reference to a pointer to an existing object which equals to key
  // or a reference to a NULL pointer which is automatically created at the location in the container
  // where a new object with key should be inserted. So you can store the Pointer to this object after the funtion returned.
  // Precondition: none, Performance: O(log(n))
  T*&                get(const K* key)              { return (T*&)sorted_vector_base<K>::get(key); }
  // Erase the element which equals key and return the removed pointer.
  // If no such element exists the function returns NULL.
  // Precondition: none, Performance: O(log(n))
  T*                 erase(const K* key)            { return (T*)sorted_vector_base<K>::erase(key); }
  // Removes an element from the vector and return it's value.
  // Precondition: where in [0,size()-1], Performance: O(n)
  // The function is not type safe and should not be exposed public.  
  T*                 erase(size_t where)            { return (T*)sorted_vector_base<K>::erase(where); }
  // Access an element by it's index.
  // Precondition: where in [0,size()-1], Performance: O(1)
  // This is the constant version of the [] operator. Since we deal only with references
  // it returns a value copy rather than a const reference to the element.
  T*                 operator[](size_t where) const { return (T*)sorted_vector_base<K>::operator[](where); }
  // Access an element by it's index.
  // Precondition: where in [0,size()-1], Performance: O(1)
  T*&                operator[](size_t where)       { return (T*&)sorted_vector_base<K>::operator[](where); }
  // Get a constant iterator that points to the firs element or past the end if the vector is empty.
  // Precondition: none, Performance: O(1)
  T*const*           begin() const                  { return (T*const*)sorted_vector_base<K>::begin(); }
  // Get a iterator that points to the firs element or past the end if the vector is empty.
  // Precondition: none, Performance: O(1)
  T**                begin()                        { return (T**)sorted_vector_base<K>::begin(); }
  // Get a constant iterator that points past the end.
  // Precondition: none, Performance: O(1)
  T*const*           end() const                    { return (T*const*)sorted_vector_base<K>::end(); }
  // Get a iterator that points past the end.
  // Precondition: none, Performance: O(1)
  T**                end()                          { return (T**)sorted_vector_base<K>::end(); }
};


/* Template implementations */
template <class K>
bool sorted_vector_base<K>::binary_search(const K* key, size_t& pos) const
{ // binary search                                  
  DEBUGLOG(("sorted_vector_base<K>::binary_search(%p,&%p)\n", key, &pos));
  size_t l = 0;
  size_t r = size();
  while (l < r)
  { size_t m = (l+r) >> 1;
    int cmp = (*this)[m]->CompareTo(key);
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

template <class K>
IComparableTo<K>* sorted_vector_base<K>::find(const K* key) const
{ size_t pos;
  return binary_search(key, pos) ? (*this)[pos] : NULL;
}

template <class K>
IComparableTo<K>*& sorted_vector_base<K>::get(const K* key)
{ size_t pos;
  return binary_search(key, pos) ? (*this)[pos] : (insert(pos) = NULL);
}

template <class K>
IComparableTo<K>* sorted_vector_base<K>::erase(const K* key)
{ size_t pos;
  return binary_search(key, pos) ? vector<IComparableTo<K> >::erase(pos) : NULL;
}

#endif
