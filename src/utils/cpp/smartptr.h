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

#ifndef CPP_SMARTPTR_H
#define CPP_SMARTPTR_H

#include <config.h>
#include <stdlib.h>
#include <cpp/cpputil.h>

#include <debuglog.h>

/* Scoped pointer class, non-copyable */
template <class T>
class sco_ptr
{private:
  T* Ptr;
 private:
  sco_ptr(const sco_ptr<T>&); // non-copyable
 public:
  // Store a new object under control or initialize a NULL pointer.
  sco_ptr(T* ptr = NULL) : Ptr(ptr)          { DEBUGLOG2(("sco_ptr(%p)::sco_ptr(%p)\n", this, ptr)); };
  // Destructor, frees the stored object.
  ~sco_ptr();
  // Basic operators
  T* get() const                             { return Ptr; }
  T& operator*()  const                      { ASSERT(Ptr); return *Ptr; }
  T* operator->() const                      { ASSERT(Ptr); return Ptr; }
  sco_ptr<T>& operator=(T* ptr);
  // Swap two pointers
  void swap(sco_ptr<T>& r);
  // Take object with ownership from the sco_ptr
  T* detach()                                { return xchg(Ptr, (T*)NULL); }
  // Comparsion
  friend bool operator==(const sco_ptr<T>& l, const sco_ptr<T>& r) { return l.Ptr == r.Ptr; }
  friend bool operator==(const sco_ptr<T>& sco, const T* ptr) { return sco.Ptr == ptr; }
  friend bool operator==(const T* ptr, const sco_ptr<T>& sco) { return sco.Ptr == ptr; }
  friend bool operator!=(const sco_ptr<T>& l, const sco_ptr<T>& r) { return l.Ptr != r.Ptr; }
  friend bool operator!=(const sco_ptr<T>& sco, const T* ptr) { return sco.Ptr != ptr; }
  friend bool operator!=(const T* ptr, const sco_ptr<T>& sco) { return sco.Ptr != ptr; }
};

// IBM C++ work around:
// These functions must not be declared in the class to avoid type dependency problems.
template <class T>
inline sco_ptr<T>::~sco_ptr()
{ DEBUGLOG2(("sco_ptr(%p)::~sco_ptr(): %p\n", this, Ptr));
  delete Ptr;
}

template <class T>
inline sco_ptr<T>& sco_ptr<T>::operator=(T* ptr)
{ DEBUGLOG2(("sco_ptr(%p)::operator=(%p): %p\n", this, ptr, Ptr));
  delete Ptr;
  Ptr = ptr;
  return *this;
}

template <class T>
inline void sco_ptr<T>::swap(sco_ptr<T>& r)
{ T* tmp = Ptr;
  Ptr = r.Ptr;
  r.Ptr = tmp;
}


/* Interface to make a class reference countable */
class Iref_Count
{ friend class int_ptr_base;
 private:
  volatile unsigned Count;
 protected:
  Iref_Count() : Count(0) {}
  ~Iref_Count() {} // You must not call the non-virtual destructor directly.
 public:
  // Checks whether the object is currently unique. If you currently hold an int_ptr with the object
  // you can safely assume that it is your's, unless you pass the reference explicitely or implicitly
  // to another thread or int_ptr instance.
  bool RefCountIsUnique() const { return Count == 1; } 
  // Checks whether the object is not under control of a int_ptr.
  // This is the case when the object is just constructed and not yet assigned to an int_ptr instance or
  // if the object is about to be deleted. You should be able to distinguish thes two cases
  // from the context of the call. Be very careful in multi-threaded environments.
  bool RefCountIsUnmanaged() const { return Count == 0; } 
};

/* Abstract non-template base class of int_ptr */
class int_ptr_base
{protected:
  Iref_Count* Ptr;
  
 protected:
  // Store a new object under reference count control or initialize a NULL pointer.
  int_ptr_base(const Iref_Count* ptr);
  // Copy constructor
  int_ptr_base(const int_ptr_base& r);
  // Protect destructor because it is not polymorphic
  ~int_ptr_base() {}
  // Core of the assignment operator
  Iref_Count* reassign(const Iref_Count* ptr);
  // Core of the assignment operator
  Iref_Count* reassign_weak(const Iref_Count* ptr);
  // Destructor core
  Iref_Count* unassign();
  // core of toCptr
  Iref_Count* detach();
  // Swap two pointers
  void swap(int_ptr_base& r);
};

inline void int_ptr_base::swap(int_ptr_base& r)
{ Iref_Count* tmp = Ptr;
  Ptr = r.Ptr;
  r.Ptr = tmp;
}

/* This is a very simple and highly efficient reference counted smart pointer
 * implementation for objects of type T.
 * This class adds the type specific part to int_ptr_base.
 * The class is similar to boost::intrusive_ptr but works on very old C++ compilers. 
 * It relies on the fact that all managed objects must derive from Iref_Count.
 * The implementation is thread-safe and lock-free.
 */
template <class T>
class int_ptr : protected int_ptr_base
{public:
  // Store a new object under reference count control or initialize a NULL pointer.
  int_ptr(T* ptr = NULL);
  // Generated copy constructor should be sufficient.
  // Destructor, frees the stored object if this is the last reference.
  ~int_ptr();
  // Basic operators
  T* get()        const                      { return (T*)Ptr; } 
  operator T*()   const                      { return (T*)Ptr; }
  T& operator*()  const                      { ASSERT(Ptr); return *(T*)Ptr; }
  T* operator->() const                      { ASSERT(Ptr); return (T*)Ptr; }
  int_ptr<T>& operator=(T* ptr);
  int_ptr<T>& operator=(const int_ptr<T>& r);
  // Special function to assing only valid objects.
  // This function rejects the assignment if the assigned object is not owned by another instance
  // of int_ptr_base. This is particulary useful to avoid the assignment of objects that are about
  // to die. ptr is like treated as weak pointer to the object. But in contrast to real weak pointers
  // the destructor of the referenced object must ensure that ptr is invalidated before the object dies completely.
  // Of course, this has to been done synchronized with the call to assign_weak.
  // The time window between the reference count reaching zero and the invalidation of ptr by the destructor
  // is handled by this function the way that the current instance will be set to NULL in this case.
  // This function must not be used to assing a newly constructed object because this will never be deleted.
  int_ptr<T>& assign_weak(T* ptr);
  void        swap(int_ptr<T>& r)            { int_ptr_base::swap(r); }
  // manual resource management
  T*          toCptr()                       { return (T*)detach(); }
  int_ptr<T>& fromCptr(T* ptr);
};

// IBM C++ work around:
// These functions must not be declared in the class to avoid type dependency problems.
template <class T>
inline int_ptr<T>::int_ptr(T* ptr)
: int_ptr_base(ptr)
{}

template <class T>
inline int_ptr<T>::~int_ptr()
{ delete (T*)unassign();
}

template <class T>
inline int_ptr<T>& int_ptr<T>::operator=(T* ptr)
{ delete (T*)reassign(ptr);
  return *this;
}

template <class T>
inline int_ptr<T>& int_ptr<T>::operator=(const int_ptr<T>& r)
{ delete (T*)reassign(r.Ptr);
  return *this;
}

template <class T>
inline int_ptr<T>& int_ptr<T>::assign_weak(T* ptr)
{ delete (T*)reassign_weak(ptr);
  return *this;
}

template <class T>
inline int_ptr<T>& int_ptr<T>::fromCptr(T* ptr)
{ delete (T*)unassign();
  Ptr = ptr;
  return *this;
};

/* Scoped array class, non-copyable */
template <class T>
class sco_arr
{private:
  T* Ptr;
 private:
  sco_arr(const sco_arr<T>&); // non-copyable
 public:
  // Store a new object under control or initialize a NULL pointer.
  sco_arr(T* ptr = NULL) : Ptr(ptr) {};
  // Destructor, frees the stored object.
  ~sco_arr()                                 { delete[] Ptr; }
  // Basic operators
  T* get() const                             { return Ptr; }
  T& operator[](size_t idx) const            { return Ptr[idx]; }
  void assign(T* ptr)                        { delete[] Ptr; Ptr = ptr; }
  sco_arr<T>& operator=(T* ptr)              { assign(ptr); return *this; }
};

#endif
