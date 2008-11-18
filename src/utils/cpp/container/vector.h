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


#ifndef CPP_CONTAINER_VECTOR_H
#define CPP_CONTAINER_VECTOR_H

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
  // Create a new vector with a given initial capacity.
  // If capacity is 0 the vector is initially created empty
  // and allocated with the default capacity when the first item is inserted.
                     vector_base(size_t capacity = 0);
  // Initialize a vector with size copies of elem.
                     vector_base(size_t size, void* elem, size_t spare = 0);
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
  // The initial value is NULL.
  // The function is not type safe and should not be exposed public.
  void*&             append();
  // Insert a new element in the vector at location where
  // Precondition: where in [0,size()], Performance: O(n)
  // The function does not take the value of the new element.
  // Instead it returns a reference to the new location that should be assigned later.
  // The reference is valid until the next non-const member function call.
  // The initial value is NULL.
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
  // Create a new vector with a given initial capacity.
  // If capacity is 0 the vector is initially created empty
  // and allocated with the default capacity when the first item is inserted.
                     vector(size_t capacity = 0)    : vector_base(capacity) {}
  // Initialize a vector with size copies of elem.
                     vector(size_t size, T* elem, size_t spare = 0) : vector_base(size, elem, spare) {}
  // swap content of two instances
  void               swap(vector<T>& r)             { vector_base::swap(r); }
  // Append a new element to the end of the vector.
  // The function does not take the value of the new element.
  // Instead it returns a reference to the new location that should be assigned later.
  // The reference is valid until the next non-const member function call.
  // The initial value of the new element is NULL.
  T*&                append()                       { return (T*&)vector_base::append(); }
  // Insert a new element in the vector at location where
  // Precondition: where in [0,size()], Performance: O(n)
  // The function does not take the value of the new element.
  // Instead it returns a reference to the new location that should be assigned later.
  // The reference is valid until the next non-const member function call.
  // The initial value of the new element is NULL.
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
  T*const*           begin() const                  { return &(T*const&)at(0); }
  // Get a iterator that points to the firs element or past the end if the vector is empty.
  // Precondition: none, Performance: O(1)
  T**                begin()                        { return &(*this)[0]; }
  // Get a constant iterator that points past the end.
  // Precondition: none, Performance: O(1)
  T*const*           end() const                    { return &(T*const&)at(size()); }
  // Get a iterator that points past the end.
  // Precondition: none, Performance: O(1)
  T**                end()                          { return &(*this)[size()]; }
};

/* Helper functions to implement some strongly typed functionality only once per type T.
 * Intrnal use only
 */
template <class T>
void vector_own_base_destroy(vector<T>& v)
{ T*const* tpp = v.end();
  while (tpp != v.begin())
  { DEBUGLOG2(("vector_own_base_destroy &%p, %u\n", &v, v.size()));
    delete *--tpp;
  }
  v.clear();
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
  // Copy constructor
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
  // Remove all elements
  void               clear()                        { vector_own_base_destroy(*this); }
  // destructor
                     ~vector_own()                  { clear(); }
};

template <class T>
vector_own<T>& vector_own<T>::operator=(const vector_own<T>& r)
{ clear();
  prepare_assign(r.size());
  //vector_own_base_copy(*this, r.begin());
  return *this;
}


/* Vector with members with intrusive reference counter. */ 
template <class T>
class vector_int : public vector_base
{protected:
  // Increment reference counters of all elements.
  void               IncRefs();
 public:
  // Default constructor
                     vector_int(size_t capacity = 0): vector_base(capacity) {}
  // Initialize a vector with size copies of elem.
                     vector_int(size_t size, T* elem) : vector_base(size, elem) { IncRefs(); }
  // Copy constructor
  // The binary copy of vector_base::vector_base(const vector_base& r, size_t spare) is fine.
  // But we have to increment the reference counters.
                     vector_int(const vector_int<T>& r, size_t spare = 0)
                                                    : vector_base(r, spare) { IncRefs(); }
  // destructor
                     ~vector_int()                  { clear(); }
  // Remove all elements
  void               clear();
  // assignment
  vector_int<T>&     operator=(const vector_int<T>& r);
  // swap content of two instances
  void               swap(vector_int<T>& r)         { vector_base::swap(r); }
  // Append a new element to the end of the vector.
  // The function does not take the value of the new element.
  // Instead it returns a reference to the new location that should be assigned later.
  // The reference is valid until the next non-const member function call.
  // The initial value of the new element is NULL.
  int_ptr<T>&        append()                       { return (int_ptr<T>&)vector_base::append(); }
  // Insert a new element in the vector at location where
  // Precondition: where in [0,size()], Performance: O(n)
  // The function does not take the value of the new element.
  // Instead it returns a reference to the new location that should be assigned later.
  // The reference is valid until the next non-const member function call.
  // The initial value of the new element is NULL.
  int_ptr<T>&        insert(size_t where)           { return (int_ptr<T>&)vector_base::insert(where); }
  // Removes an element from the vector and return it's value.
  // The where pointer is set to the next item after the removed one.
  // Precondition: where in [begin(),end()), Performance: O(n)
  int_ptr<T>         erase(const int_ptr<T>*& where){ return int_ptr<T>((const int_ptr<T>&)vector_base::erase((void**&)where)); }
  // Removes an element from the vector and return it's value.
  // Precondition: where in [0,size()-1], Performance: O(n)
  int_ptr<T>         erase(size_t where)            { return int_ptr<T>((const int_ptr<T>&)vector_base::erase(where)); }
  // Access an element by it's index.
  // Precondition: where in [0,size()-1], Performance: O(1)
  // This is the constant version of the [] operator. Since we deal only with references
  // it returns a value copy rather than a const reference to the element.
  T*                 operator[](size_t where) const { return &*(const int_ptr<T>&)at(where); }
  // Access an element by it's index.
  // Precondition: where in [0,size()-1], Performance: O(1)
  int_ptr<T>&        operator[](size_t where)       { return (int_ptr<T>&)at(where); }
  // Get a constant iterator that points to the firs element or past the end if the vector is empty.
  // Precondition: none, Performance: O(1)
  const int_ptr<T>*  begin() const                  { return &(const int_ptr<T>&)at(0); }
  // Get a iterator that points to the firs element or past the end if the vector is empty.
  // Precondition: none, Performance: O(1)
  int_ptr<T>*        begin()                        { return &(*this)[0]; }
  // Get a constant iterator that points past the end.
  // Precondition: none, Performance: O(1)
  const int_ptr<T>*  end() const                    { return &(const int_ptr<T>&)at(size()); }
  // Get a iterator that points past the end.
  // Precondition: none, Performance: O(1)
  int_ptr<T>*        end()                          { return &(*this)[size()]; }
};

template <class T>
void vector_int<T>::IncRefs()
{ int_ptr<T>* tpp = end();
  int_ptr<T> worker;
  while (tpp != begin())
  { DEBUGLOG2(("vector_int(%p)::IncRefs - %p\n", this, tpp->get()));
    worker.fromCptr(*--tpp);
  }
}

template <class T>
void vector_int<T>::clear()
{ const int_ptr<T>* tpp = end();
  while (tpp != begin())
  { DEBUGLOG2(("vector_int(%p)::clear - %p\n", this, tpp->get()));
    (--tpp)->~int_ptr();
  }
  vector_base::clear();
}

template <class T>
vector_int<T>& vector_int<T>::operator=(const vector_int<T>& r)
{ clear();
  // The binary copy of vector_base::operator=(const vector_base& r) is fine.
  // But we have to increment the reference counters.
  vector_base::operator=(r);
  IncRefs();
  return *this; 
}

#endif
