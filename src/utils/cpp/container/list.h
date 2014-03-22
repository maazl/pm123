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


#ifndef CPP_CONTAINER_LIST_H
#define CPP_CONTAINER_LIST_H

#include <cpp/smartptr.h>
#include <cpp/cpputil.h>


/// @brief Non-template base of \c list<T>
/// @remarks It is recommended not to use this class directly.
/// Use the strongly typed version \c list<T> instead.
class list_base
{public:
  /// @brief Base class of all list elements.
  /// @details All items stored in a list must derive from this intrusive interface class.
  /// The class does not provided any public interface but it allows the list container
  /// to operate without additional allocations.
  /// Memory footprint: 2 machine size words.
  class entry_base
  { friend class list_base;
   private:
    entry_base* Prev;
    entry_base* Next;
   private: // non-copyable
    entry_base(const entry_base&);
    void operator=(const entry_base&);
   protected:
    entry_base() : Prev(NULL), Next(NULL) {}
  };

 private:
  entry_base* Head;
  entry_base* Tail;

 private: // non-copyable
              list_base(const list_base&);
  void        operator=(const list_base&);
 public:
  /// Create an empty list.
              list_base()                       { list_base::clear(); }
  /// Return \c true if and only if the current list is empty.
  bool        is_empty() const                  { return !Head; }
  /// Swap the content of two instances O(1).
  void        swap(list_base& r)                { ::swap(Head, r.Head); ::swap(Tail, r.Tail); }
  /// Return the element before \a cur. If \a cur is \c NULL the last element is returned.
  entry_base* prev(const entry_base* cur) const { return cur ? cur->Prev : Tail; }
  /// Return the element after \a cur. If \a cur is \c NULL the first element is returned.
  entry_base* next(const entry_base* cur) const { return cur ? cur->Next : Head; }
  /// Prepend an element to the list.
  /// @param elem Element to prepend, must not be NULL.
  void        push_front(entry_base* elem);
  /// Append an element to the list.
  /// @param elem Element to append, must not be NULL.
  void        push_back(entry_base* elem);
  /// Return the first element and remove it from the list.
  /// Return \c NULL if the list is empty.
  entry_base* pop_front();
  /// Return the last element and remove it from the list.
  /// Return \c NULL if the list is empty.
  entry_base* pop_back();
  /// Insert a new element before the element \a before.
  /// If \a before is \c NULL the element is appended. (Same as \c push_back.)
  /// @param elem Element to insert, must not be \c NULL.
  void        insert(entry_base* elem, entry_base* before);
  /// Remove an element from the list.
  /// @param elem Element to remove, must not be \c NULL.
  void        remove(entry_base* elem);
  /// Move an element within the list to the new location before \a before.
  /// If \a before is \c NULL the element is moved to the end.
  /// @return Return \c false if the operation is a no-op.
  /// @param elem Element to move, must not be \c NULL.
  bool        move(entry_base* elem, entry_base* before);
  /// remove all elements from the list.
  void        clear()                           { Head = NULL; Tail = NULL; }
};

/** @brief Strongly typed doubly linked list of pointers to elements.
 * The list does not own its elements.
 * @details The pointers must point to objects that derive from list_base::entry_base.
 * You may store polymorphic elements.
 */
template <class T>
class list : public list_base
{public:
  /// Swap the content of two instances O(1).
  inline void swap(list<T>& r)                  { list_base::swap(r); }
  /// Return the element before \a cur. If \a cur is \c NULL the last element is returned.
  inline T*   prev(const T* cur) const          { return (T*)list_base::prev(cur); }
  /// Return the element after \a cur. If \a cur is \c NULL the first element is returned.
  inline T*   next(const T* cur) const          { return (T*)list_base::next(cur); }
  /// Prepend an element to the list.
  /// @param elem Element to prepend, must not be NULL.
  inline void push_front(T* elem)               { list_base::push_front(elem); }
  /// Append an element to the list.
  /// @param elem Element to append, must not be NULL.
  inline void push_back(T* elem)                { list_base::push_back(elem); }
  /// Return the first element and remove it from the list.
  /// Return \c NULL if the list is empty.
  inline T*   pop_front()                       { return (T*)list_base::pop_front(); }
  /// Return the last element and remove it from the list.
  /// Return NULL if the list is empty.
  inline T*   pop_back()                        { return (T*)list_base::pop_front(); }
  /// Insert a new element before the element \a before.
  /// If \a before is \c NULL the element is appended. (Same as \c push_back.)
  /// @param elem Element to insert, must not be \c NULL.
  inline void insert(T* elem, T* before)        { list_base::insert(elem, before); }
  /// Remove an element from the list.
  /// Note that this does not delete the element.
  /// @param elem Element to remove, must not be \c NULL.
  inline void remove(T* elem)                   { list_base::remove(elem); }
  /// Move an element within the list to the new location before \a before.
  /// If \a before is \c NULL the element is moved to the end.
  /// @return Return \c false if the operation is a no-op.
  /// @param elem Element to move, must not be \c NULL.
  inline bool move(T* elem, T* before)          { return list_base::move(elem, before); }
};


/** @brief Strongly typed doubly linked list of pointers to elements.
 * The list owns its elements.
 * @details The pointers must point to objects that derive from list_base::entry_base.
 * You may store polymorphic elements.
 */
template <class T>
class list_own : public list<T>
{public:
              ~list_own()                       { clear(); }
  /// Remove all elements from the list. This deletes the elements.
  void        clear();
};

template <class T>
void list_own<T>::clear()
{ T* cur = list<T>::next(NULL);
  while (cur)
  { T* nxt = list<T>::next(cur);
    delete cur;
    cur = nxt;
  }
  list<T>::clear();
}


/** @brief Strongly typed doubly linked list of intrusive reference counted elements.
 * @details The list items must derive from \c list_base::entry_base and \c Iref_count.
 * @remarks Although the list is doubly linked, it does not store cyclic references.
 */
template <class T>
class list_int : public list<T>
{public:
              ~list_int()                       { clear(); }
  /// Prepend an element to the list.
  /// @param elem Element to prepend, must not be NULL.
  void        push_front(T* elem);
  /// Append an element to the list.
  /// @param elem Element to append, must not be NULL.
  void        push_back(T* elem);
  /// Return the first element and remove it from the list.
  /// Return \c NULL if the list is empty.
  const int_ptr<T> pop_front();
  /// Return the last element and remove it from the list.
  /// Return NULL if the list is empty.
  const int_ptr<T> pop_back();
  /// Insert a new element before the element \a before.
  /// If \a before is \c NULL the element is appended. (Same as \c push_back.)
  /// @param elem Element to insert, must not be \c NULL.
  void        insert(T* elem, T* before);
  /// Remove an element from the list.
  /// @param elem Element to remove, must not be \c NULL.
  void        remove(T* elem);
  /// Remove all elements from the list.
  void        clear();
};

template <class T>
void list_int<T>::push_front(T* elem)
{ list<T>::push_front(int_ptr<T>(elem).toCptr());
}

template <class T>
void list_int<T>::push_back(T* elem)
{ list<T>::push_back(int_ptr<T>(elem).toCptr());
}

template <class T>
const int_ptr<T> list_int<T>::pop_front()
{ return int_ptr<T>().fromCptr((T*)list_base::pop_front());
}

template <class T>
const int_ptr<T> list_int<T>::pop_back()
{ return int_ptr<T>().fromCptr((T*)list_base::pop_back());
}

template <class T>
void list_int<T>::insert(T* elem, T* before)
{ list<T>::insert(int_ptr<T>(elem).toCptr(), before);
}

template <class T>
void list_int<T>::remove(T* elem)
{ list<T>::remove(elem);
  int_ptr<T>().fromCptr(elem);
}

template <class T>
void list_int<T>::clear()
{ int_ptr<T> deleter;
  do
  { deleter.fromCptr(this->next(deleter));
  } while (deleter);
  list<T>::clear();
}

#endif
