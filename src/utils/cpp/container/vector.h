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


class xstring;

/** @brief Internal logically abstract base class of vector<T> with all non-template core implementations.
 * @details This class can only store reference type objects (pointers).
 * It does not handle the ownership of the referenced objects.
 * The class methods are not synchronized.
 */
class vector_base
{private:
  void** Data;
  size_t Size;
  size_t Capacity;

 protected:
  /// Create a new vector with a given initial capacity.
  /// @param capacity If \a capacity is \c 0 the vector is initially created empty
  /// and allocated with the default capacity when the first item is inserted.
  vector_base(size_t capacity = 0);
  /// Initialize a vector with size copies of elem.
  vector_base(size_t size, void* elem, size_t spare = 0);
  /// @brief Copy constructor
  /// @details This does \e no deep copy.
  vector_base(const vector_base& r, size_t spare = 0);
  /// @brief Destroy the vector.
  /// @details This will not affect the referenced objects.
  ~vector_base();
  /// @brief Initialize storage for new assignment.
  /// @details Postcondition: Size = size && Capacity >= size.
  void              prepare_assign(size_t size);
  /// @brief Assignment
  /// @details This does \e no deep copy.
  void              operator=(const vector_base& r);
  /// @brief Swap content of two instances.
  /// @details This is O(1).
  void              swap(vector_base& r);

  /// @brief Access an element by number.
  /// @param where Precondition: \a where in [0,size()-1], Performance: O(1)
  /// @details This is in fact like operator[], but since this method is not type safe
  /// it should not be exposed public.
  void*&            at(size_t where) const         { ASSERT(where < Size); return Data[where]; }
  /// @brief Get storage root.
  /// @return This must not be dereferenced beyond size().
  /// It may be NULL if size() is 0.
  void**            begin() const                  { return Data; };
  /// @brief Finds an element is in the list. Uses instance equality.
  /// @param elem Element to find.
  /// @return Iterator to the found element or NULL if not found.
  void**            find(const void* elem) const;

  /// @brief Append a new element to the end of the vector.
  /// @return The function does not take the value of the new element.
  /// Instead it returns a reference to the new location that should be assigned later.
  /// @details The reference is valid until the next non-const member function call.
  /// The initial value is NULL.
  /// The function is not type safe and should not be exposed public.
  void*&            append();
  /// @brief Insert a new element in the vector at location where
  /// @param where Precondition: where in [0,size()], Performance: O(n)
  /// @return The function does not take the value of the new element.
  /// Instead it returns a reference to the new location that should be assigned later.
  /// @details The reference is valid until the next non-const member function call.
  /// The initial value is NULL.
  /// The function is not type safe and should not be exposed public.
  void*&            insert(size_t where);
  /// @brief Removes an element from the vector and return it's value.
  /// @param where The \a where pointer is set to the next item after the removed one.
  /// Precondition: \a where in [begin(),end()), Performance: O(n)
  /// @details The function is not type safe and should not be exposed public.
  void*             erase(void**& where);
  /// @brief Removes an element from the vector and return it's value.
  /// @param where Precondition: \a where in [0,size()-1], Performance: O(n)
  /// @details The function is not type safe and should not be exposed public.
  void*             erase(size_t where)            { void** p = Data + where; return erase(p); }
 public:
  /// @brief Return the number of elements in the container.
  /// @details This is not equal to the storage capacity.
  size_t            size() const                   { return Size; }
  /// @brief Adjust the size to a given value.
  /// @details If the array is increased NULL values are appended.
  void              set_size(size_t size);
  /// Remove all elements
  void              clear()                        { Size = 0; }
  /// Move the element at \a from in the list to the location \a to.
  /// Precondition: from and to in [0,size()-1], Performance: O(n)
  void              move(size_t from, size_t to);
  /// Ensure that the container can store size elements without reallocation.
  /// Reserve will also shrink the storage. It is an error if size is less than the actual size.
  void              reserve(size_t size);

  friend bool       binary_search_base(const vector_base& data, int (*fcmp)(const void* elem, const void* key),
                                       const void* key, size_t& pos);
};

/** @brief Type safe vector of reference type objects (pointers).
 * @details The class does not handle the ownership of the referenced objects.
 * The class methods are not synchronized.
 */
template <class T>
class vector : public vector_base
{public:
  /// @brief Create a new vector with a given initial capacity.
  /// @param capacity If \a capacity is 0 the vector is initially created empty
  /// and allocated with the default capacity when the first item is inserted.
  vector(size_t capacity = 0)                      : vector_base(capacity) {}
  /// Initialize a vector with \a size copies of \a elem.
  vector(size_t size, T* elem, size_t spare = 0)   : vector_base(size, elem, spare) {}
  /// @brief Copy constructor
  /// @details This does \e no deep copy.
  vector(const vector<T>& r, size_t spare = 0)     : vector_base(r, spare) {}
  /// @brief Swap content of two instances.
  /// @details This is O(1).
  void              swap(vector<T>& r)             { vector_base::swap(r); }

  /// @brief Access an element by it's index.
  /// @details Precondition: \a where in [0,size()), Performance: O(1)
  /// @return This is the constant version of the operator[]. Since we deal only with references
  /// it returns a value copy rather than a const reference to the element.
  T*                operator[](size_t where) const { return (T*)at(where); }
  /// @brief Access an element by it's index.
  /// @details Precondition: \a where in [0,size()), Performance: O(1)
  T*&               operator[](size_t where)       { return (T*&)at(where); }
  /// @brief Get a constant iterator that points to the first element or past the end if the vector is empty.
  /// @details Precondition: none, Performance: O(1)
  T*const*          begin() const                  { return (T*const*)vector_base::begin(); }
  /// @brief Get a iterator that points to the first element or past the end if the vector is empty.
  /// @details Precondition: none, Performance: O(1)
  T**               begin()                        { return (T**)vector_base::begin(); }
  /// @brief Get a constant iterator that points past the end.
  /// @details Precondition: none, Performance: O(1)
  T*const*          end() const                    { return begin() + size(); }
  /// @brief Get a iterator that points past the end.
  /// @details Precondition: none, Performance: O(1)
  T**               end()                          { return begin() + size(); }
  /// @brief Finds an element is in the list. Uses instance equality.
  /// @param elem Element to find.
  /// @return Iterator to the found element or \c NULL if not found.
  T*const*          find(const T* elem) const      { return (T*const*)vector_base::find(elem); }
  /// @brief Finds an element is in the list. Uses instance equality.
  /// @param elem Element to find.
  /// @return Iterator to the found element or \c NULL if not found.
  T**               find(const T* elem)            { return (T**)vector_base::find(elem); }

  /// @brief Append a new element to the end of the vector.
  /// @return The function does not take the value of the new element.
  /// Instead it returns a reference to the new location that should be assigned later.
  /// @details The reference is valid until the next non-const member function call.
  /// The initial value of the new element is \c NULL.
  T*&               append()                       { return (T*&)vector_base::append(); }
  /// @brief Insert a new element in the vector at location \a where.
  /// @param where Precondition: where in [0,size()], Performance: O(n)
  /// @return The function does not take the value of the new element.
  /// Instead it returns a reference to the new location that should be assigned later.
  /// @details The reference is valid until the next non-const member function call.
  /// The initial value of the new element is \c NULL.
  T*&               insert(size_t where)           { return (T*&)vector_base::insert(where); }
  /// @brief Removes an element from the vector and return it's value.
  /// @param where The \a where pointer is set to the next item after the removed one.
  /// @details Precondition: where in [begin(),end()), Performance: O(n)
  T*                erase(T*const*& where)         { return (T*)vector_base::erase((void**&)where); }
  /// @brief Removes an element from the vector and return it's value.
  /// @details Precondition: \a where in [0,size()), Performance: O(n)
  T*                erase(size_t where)            { return (T*)vector_base::erase(where); }
};

/* Helper functions to implement some strongly typed functionality only once per type T.
 * Intrnal use only.
 */
template <class T>
void vector_own_base_destroy(vector<T>& v)
{ T*const* end = v.end();
  T*const* tpp = v.begin();
  while (tpp != end)
  { DEBUGLOG2(("vector_own_base_destroy &%p, %u\n", &v, v.size()));
    delete *tpp++;
  }
  v.clear();
}
template <class T>
void vector_own_base_destroy(vector<T>& v, size_t nsize)
{ ASSERT(nsize <= v.size());
  T*const* end = v.end();
  T*const* tpp = v.begin() + nsize;
  while (tpp != end)
  { DEBUGLOG2(("vector_own_base_destroy &%p, %u\n", &v, v.size()));
    delete *tpp++;
  }
  v.set_size(nsize);
}

template <class T>
void vector_own_base_copy(vector<T>& d, const T*const* spp)
{ T** epp = d.end();
  T** dpp = d.begin();
  while (dpp != epp)
    *dpp++ = new T(**spp++); // copy contructor of T
}


/** @brief Type safe vector of reference type objects (pointers).
 * @details The class owns the referenced objects.
 * But only the function that delete or copy the entire container are different.
 * To erase an element you must use "delete erase(...)".
 * The class methods are not synchronized.
 */
template <class T>
class vector_own : public vector<T>
{protected:
  /// @brief Copy constructor
  /// @details Note that since vector_own own its object exclusively this copy constructor must do
  /// a deep copy of the vector. This is up to the derived class!
  /// You may use vector_own_base_copy to do the job.
  vector_own(const vector_own<T>& r, size_t spare = 0) : vector<T>(r.size() + spare) {}
  /// @brief Assignment
  /// @details Note that since vector_own own its object exclusively the assignment must do
  /// a deep copy of the vector. This is up to the derived class!
  /// You may use vector_own_base_copy to do the job.
  void              operator=(const vector_own<T>& r) { clear(); prepare_assign(r.size()); }

 public:
  /// @brief Create a new vector, optionally with a given initial capacity.
  /// @param capacity If \a capacity is 0 the vector is initially created empty
  /// and allocated with the default capacity when the first item is inserted.
  vector_own(size_t capacity = 0) : vector<T>(capacity) {}
  /// @brief Adjust the size to a given value.
  /// @details If the array is increased NULL values are appended.
  /// If not NULL items are removed, they are deleted.
  void              set_size(size_t size)          { if (this->size() > size) vector_own_base_destroy(*this, size); vector<T>::set_size(size); }
  /// Delete all elements
  void              clear()                        { vector_own_base_destroy(*this); }
  /// Destructor
  ~vector_own()                                    { clear(); }
};


/** Vector with members with intrusive reference counter. */
template <class T>
class vector_int : public vector<T>
{protected:
  /// Increment reference counters of all elements.
  void              IncRefs();
 public:
  /// Default constructor
  /// @param capacity Initial capacity (not size). If \a capacity is 0
  /// the vector is initially created empty and allocated with the default capacity
  /// when the first item is inserted.
  vector_int(size_t capacity = 0)                  : vector<T>(capacity) {}
  /// Initialize a vector with \a size copies of \a elem.
  /// @param spare Reserve space for another \a spare elements.
  vector_int(size_t size, T* elem, size_t spare = 0) : vector<T>(size, elem, spare) { IncRefs(); }
  /// @brief Copy constructor
  /// @remarks The binary copy of vector<T>::vector(const vector_base& r, size_t spare) is fine.
  /// But we have to increment the reference counters.
  vector_int(const vector_int<T>& r, size_t spare = 0) : vector<T>(r, spare) { IncRefs(); }
  /// Destructor
  ~vector_int()                                    { clear(); }
  /// @brief Adjust the size to a given value.
  /// @details If the array is increased NULL values are appended.
  /// If not NULL items are removed, their reference counter is decremented.
  void              set_size(size_t size);
  /// Remove all elements and decrement their reference counter.
  void              clear();
  /// Assignment
  vector_int<T>&    operator=(const vector_int<T>& r);
  /// Swap the content of two instances.
  void              swap(vector_int<T>& r)         { vector<T>::swap(r); }

  /// @brief Access an element by it's index.
  /// @details Precondition: \a where in [0,size()), Performance: O(1)
  /// This is the constant version of the operator[]. Since we deal only with references
  /// it returns a value copy rather than a const reference to the element.
  T*                operator[](size_t where) const { return vector<T>::operator[](where); } // = return ((const int_ptr<T>&)at(where)).get();
  /// @brief Access an element by it's index.
  /// @details Precondition: \a where in [0,size()), Performance: O(1)
  int_ptr<T>&       operator[](size_t where)       { return (int_ptr<T>&)at(where); }
  /// @brief Get a constant iterator that points to the first element or past the end if the vector is empty.
  /// @details Precondition: none, Performance: O(1)
  const int_ptr<T>* begin() const                  { return (const int_ptr<T>*)vector_base::begin(); }
  /// @brief Get a iterator that points to the first element or past the end if the vector is empty.
  /// @details Precondition: none, Performance: O(1)
  int_ptr<T>*       begin()                        { return (int_ptr<T>*)vector_base::begin(); }
  /// @brief Get a constant iterator that points past the end.
  /// @details Precondition: none, Performance: O(1)
  const int_ptr<T>* end() const                    { return begin() + size(); }
  /// @brief Get a iterator that points past the end.
  /// @details Precondition: none, Performance: O(1)
  int_ptr<T>*       end()                          { return begin() + size(); }
  /// @brief Finds an element is in the list. Uses instance equality.
  /// @param elem Element to find.
  /// @return Iterator to the found element or \c NULL if not found.
  const int_ptr<T>* find(const T* elem) const      { return (const int_ptr<T>*)vector_base::find(elem); }
  /// @brief Finds an element is in the list. Uses instance equality.
  /// @param elem Element to find.
  /// @return Iterator to the found element or \c NULL if not found.
  int_ptr<T>*       find(const T* elem)            { return (int_ptr<T>*)vector_base::find(elem); }

  /// @brief Append a new element to the end of the vector.
  /// @details The function does not take the value of the new element.
  /// Instead it returns a reference to the new location that should be assigned later.
  /// The reference is valid until the next non-const member function call.
  /// The initial value of the new element is \c NULL.
  int_ptr<T>&       append()                       { return (int_ptr<T>&)vector<T>::append(); }
  /// @brief Insert a new element in the vector at location where
  /// @details Precondition: \a where in [0,size()], Performance: O(n)
  /// The function does not take the value of the new element.
  /// Instead it returns a reference to the new location that should be assigned later.
  /// The reference is valid until the next non-const member function call.
  /// The initial value of the new element is \c NULL.
  int_ptr<T>&       insert(size_t where)           { return (int_ptr<T>&)vector<T>::insert(where); }
  /// @brief Removes an element from the vector and return it's value.
  /// @details The \a where pointer is set to the next item after the removed one.
  /// Precondition: \a where in [begin(),end()), Performance: O(n)
  const int_ptr<T>  erase(const int_ptr<T>*& where){ return int_ptr<T>((const int_ptr<T>&)vector<T>::erase((T*const*&)where)); }
  /// @brief Removes an element from the vector and return it's value.
  /// @details Precondition: \a where in [0,size()), Performance: O(n)
  const int_ptr<T>  erase(size_t where)            { return int_ptr<T>((const int_ptr<T>&)vector<T>::erase(where)); }
};

template <class T>
void vector_int<T>::IncRefs()
{ const int_ptr<T>* epp = end();
  const int_ptr<T>* tpp = begin();
  int_ptr<T> worker;
  while (tpp != epp)
  { DEBUGLOG2(("vector_int(%p)::IncRefs - %p\n", this, tpp->get()));
    worker = *tpp++;
    worker.toCptr();
  }
}

template <class T>
void vector_int<T>::set_size(size_t size)
{ if (size < this->size())
  { const int_ptr<T>* epp = end();
    const int_ptr<T>* tpp = begin() + size;
    while (tpp != epp)
    { DEBUGLOG2(("vector_int(%p)::clear - %p\n", this, tpp->get()));
      tpp++->~int_ptr();
    }
  }
  vector<T>::set_size(size);
}

template <class T>
void vector_int<T>::clear()
{ const int_ptr<T>* epp = end();
  const int_ptr<T>* tpp = begin();
  while (tpp != epp)
  { DEBUGLOG2(("vector_int(%p)::clear - %p\n", this, tpp->get()));
    tpp++->~int_ptr();
  }
  vector<T>::clear();
}

template <class T>
vector_int<T>& vector_int<T>::operator=(const vector_int<T>& r)
{ clear();
  // The binary copy of vector_base::operator=(const vector_base& r) is fine.
  // But we have to increment the reference counters.
  vector<T>::operator=(r);
  IncRefs();
  return *this;
}

#endif
