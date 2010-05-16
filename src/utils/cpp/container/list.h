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


/* Doubly linked list of pointers.
 * The pointers must point to objects that derive from list_base::entry_base.
 * You may store polymorphic elements.
 */
class list_base
{public:
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
              list_base()                       { list_base::clear(); }
  // Return true if and only if the current list is empty.
  bool        is_empty() const                  { return !Head; }
  // Return the element before cur. If cur is NULL the last element is returned.
  entry_base* prev(const entry_base* cur) const { return cur ? cur->Prev : Tail; }
  // Return the element after cur. If cur is NULL the first element is returned.
  entry_base* next(const entry_base* cur) const { return cur ? cur->Next : Head; }
  // Prepend an element to the list.
  void        push_front(entry_base* elem);
  // Append an element to the list.
  void        push_back(entry_base* elem);
  // Return the first element and remove it from the list.
  // Return NULL if the list is empty.
  entry_base* pop_front();
  // Return the last element and remove it from the list.
  // Return NULL if the list is empty.
  entry_base* pop_back();
  // Insert a new element before the element 'before'.
  // If before is NULL the element is appended. (Same as push_back.)
  void        insert(entry_base* elem, entry_base* before);
  // Remove an element from the list.
  void        remove(entry_base* elem);
  // Move an element within the list to the new location before 'before'.
  // Return false if the operation is a no-op.
  bool        move(entry_base* elem, entry_base* before);
  // remove all elements from the list.
  inline void clear()                           { Head = NULL; Tail = NULL; }
};

// Strongly typed version of list_base.
template <class T>
class list : public list_base
{public:
  // Return the element before cur. If cur is NULL the last element is returned.
  inline T*   prev(const T* cur) const          { return (T*)list_base::prev(cur); }
  // Return the element after cur. If cur is NULL the first element is returned.
  inline T*   next(const T* cur) const          { return (T*)list_base::next(cur); }
  // Prepend an element to the list.
  inline void push_front(T* elem)               { list_base::push_front(elem); }
  // Append an element to the list.
  inline void push_back(T* elem)                { list_base::push_back(elem); }
  // Return the first element and remove it from the list.
  // Return NULL if the list is empty.
  inline T*   pop_front()                       { return (T*)list_base::pop_front(); }
  // Return the last element and remove it from the list.
  // Return NULL if the list is empty.
  inline T*   pop_back()                        { return (T*)list_base::pop_front(); }
  // Insert a new element before the element 'before'.
  // If before is NULL the element is appended. (Same as push_back.)
  inline void insert(T* elem, T* before)        { list_base::insert(elem, before); }
  // Remove an element from the list.
  // Note that this function will not delete elem!
  inline void remove(T* elem)                   { list_base::remove(elem); }
  // Move an element within the list to the new location before 'before'.
  // Return false if the operation is a no-op.
  inline bool move(T* elem, T* before)          { return list_base::move(elem, before); }
};

/*class list_base_own : public list_base
{public:
              ~list_own()                       { clear(); }
  // Remove all elements from the list.
  // This deletes the elements.
  void        clear();
};*/

template <class T>
class list_own : public list<T>
{public:
              ~list_own()                       { clear(); }
  // Remove all elements from the list.
  // This deletes the elements.
  void        clear();
};

template <class T>
void list_own<T>::clear()
{ T* cur = next(NULL);
  while (cur)
  { T* nxt = next(cur);
    delete cur;
    cur = nxt;
  }
  list<T>::clear();
}


/* Doubly linked list with intrusive reference count.
 * The list items must derive from list_base::entry_base and Iref_count.
 * Although the list is doubly linked, it does not store cyclic references.
 */
template <class T>
class list_int : public list<T>
{public:
              ~list_int()                       { clear(); }
  // Prepend an element to the list.
  void        push_front(T* elem);
  // Append an element to the list.
  void        push_back(T* elem);
  // Return the first element and remove it from the list.
  // Return NULL if the list is empty.
  const int_ptr<T> pop_front();
  // Return the last element and remove it from the list.
  // Return NULL if the list is empty.
  const int_ptr<T> pop_back();
  // Insert a new element before the element 'before'.
  // If before is NULL the element is appended. (Same as push_back.)
  void        insert(T* elem, T* before);
  // Remove an element from the list.
  // Note that this function will not delete elem!
  void        remove(T* elem);
  // Remove all elements from the list.
  // This deletes the elements.
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
  { deleter.fromCptr(next(deleter));
  } while (deleter);
  list<T>::clear();
}

#endif

