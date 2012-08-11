/*
 * Copyright 2007-2010 Marcel Müller
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
#include <cpp/mutex.h>

#include <debuglog.h>

/* Scoped pointer class, non-copyable */
template <class T>
class sco_ptr
{private:
  struct unspecified;
 private:
  T* Ptr;
 private:
  sco_ptr(const sco_ptr<T>&); // non-copyable
 public:
  // Store a new object under control or initialize a NULL pointer.
  sco_ptr(T* ptr = NULL) : Ptr(ptr)     { DEBUGLOG2(("sco_ptr(%p)::sco_ptr(%p)\n", this, ptr)); };
  // Destructor, frees the stored object.
  ~sco_ptr();
  // Basic operators
  T* get() const                        { return Ptr; }
  operator unspecified*() const         { return (unspecified*)Ptr; }
  T& operator*()  const                 { ASSERT(Ptr); return *Ptr; }
  T* operator->() const                 { ASSERT(Ptr); return Ptr; }
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

#define INT_PTR_STOLEN_BITS 2U
#define INT_PTR_ALIGNMENT (1U << INT_PTR_STOLEN_BITS)
#define INT_PTR_POINTER_MASK (~INT_PTR_ALIGNMENT+1U)
#define INT_PTR_COUNTER_MASK (INT_PTR_ALIGNMENT-1U)
#define CLIB_ALIGNMENT 4U // Intrinsic alignment of C runtime

template <class T> class int_ptr;
/* Interface to make a class reference countable */
class Iref_count
{ 
  #ifndef __WATCOMC__
  template <class T>
  #endif
  friend class int_ptr;
  //  template <class T> friend class int_ptr;
 private:
  volatile unsigned Count;
  // This function is the interface to int_ptr<T>
  volatile unsigned& access_counter()   { return Count; }
 private: // non-copyable
  Iref_count(const Iref_count&);
  void operator=(const Iref_count&);
 protected:
  Iref_count() : Count(0) {}
  ~Iref_count() {} // You must not call the non-virtual destructor directly.
 public:
  // Checks whether the object is currently unique. If you currently hold an int_ptr with the object
  // you can safely assume that it is your's, unless you pass the reference explicitly or implicitly
  // to another thread or int_ptr instance.
  bool RefCountIsUnique() const         { return (Count & ~INT_PTR_ALIGNMENT) == 0; }
  // Checks whether the object is not under control of a int_ptr.
  // This is the case when the object is just constructed and not yet assigned to an int_ptr instance or
  // if the object is about to be deleted. You should be able to distinguish thes two cases
  // from the context of the call. Be very careful in multi-threaded environments.
  bool RefCountIsUnmanaged() const      { return Count == 0; }

#if INT_PTR_ALIGNMENT > CLIB_ALIGNMENT
 private:
  typedef unsigned char offset; // must be able to hold ALIGNMENT
 public:
  // alignment
  #if defined(__IBMCPP__) && defined(__DEBUG_ALLOC__)
  static void* operator new(size_t len, const char*, size_t)
  #else
  static void* operator new(size_t len)
  #endif
  { char* p = (char*)::operator new(len + sizeof(offset) + INT_PTR_ALIGNMENT - CLIB_ALIGNMENT);
    offset off = ((-(int)p-sizeof(offset)) & INT_PTR_COUNTER_MASK) + sizeof(offset);
    p += off;
    ((offset*)p)[-1] = off;
    return p;
  }
  static void operator delete( void* ptr )
  { char* p = (char*)ptr;
    offset off = ((offset*)p)[-1];
    ::operator delete(p - off);
  }
#endif
};

/** Exactly the same than Iref_Count but with a public virtual destructor.
 * In contrast to Iref_count you might use this as type argument to int_ptr directly.
 */
class IVref_count : public Iref_count
{protected:
  IVref_count() {}
 public:
  virtual ~IVref_count() {}
};

/* This is a simple and highly efficient reference counted smart pointer
 * implementation for objects of type T.
 * The class is similar to boost::intrusive_ptr but works on very old C++ compilers.
 * The implementation is strongly thread-safe on volatile instances and wait-free.
 * All objects of type T must implement a function called access_counter() that
 * provides access to the reference counter. The easiest way to do so is to derive
 * from Iref_count.
 * Note that all objects of type T MUST be aligned to INT_PTR_ALIGNMENT in memory!
 */
template <class T>
class int_ptr
{private:
  unsigned    Data;
 private:
  // Strongly thread safe read
  unsigned    acquire() volatile const;
  // Destructor core
  static void release(unsigned data);
  // Transfer hold count to the main counter and return the data with hold count 0.
  static unsigned transfer(unsigned data);

  // Raw initialization
  explicit    int_ptr(unsigned data)    : Data(data) {}
 public:
  // Initialize a NULL pointer.
              int_ptr()                 : Data(0) {}
  // Store a new object under reference count control.
              int_ptr(T* ptr);
  // Helper to disambiguate calls.
              int_ptr(int_ptr<T>& r);
  // Copy constructor
              int_ptr(const int_ptr<T>& r);
  // Copy constructor, strongly thread-safe.
              int_ptr(volatile const int_ptr<T>& r);
  // Destructor, frees the stored object if this is the last reference.
              ~int_ptr();
  // swap instances (not thread safe)
  void        swap(int_ptr<T>& r)       { unsigned temp = r.Data; r.Data = Data; Data = temp; }
  // Strongly thread safe swap
  void        swap(volatile int_ptr<T>& r);
  // Strongly thread safe swap
  void        swap(int_ptr<T>& r) volatile;
  // reset the current instance to NULL
  void        reset();
  void        reset() volatile;
  // Basic operators
  T*          get()         const       { return (T*)Data; }
              operator T*() const       { return (T*)Data; }
  bool        operator!()   const volatile { return !Data; }
  T&          operator*()   const       { ASSERT(Data); return *(T*)Data; }
  T*          operator->()  const       { ASSERT(Data); return (T*)Data; }
  // assignment
  int_ptr<T>& operator=(T* ptr);
  int_ptr<T>& operator=(int_ptr<T>& r);      // Helper to disambiguate calls.
  int_ptr<T>& operator=(const int_ptr<T>& r);
  int_ptr<T>& operator=(volatile const int_ptr<T>& r);
  void        operator=(T* ptr) volatile;
  void        operator=(int_ptr<T>& r) volatile; // Helper to disambiguate calls.
  void        operator=(const int_ptr<T>& r) volatile;
  void        operator=(volatile const int_ptr<T>& r) volatile;
  /*// equality
  friend bool operator==(const int_ptr<T>& l, const int_ptr& r);
  friend bool operator!=(const int_ptr<T>& l, const int_ptr& r);*/
  // manual resource management for adaption of C libraries.
  T*          toCptr()                  { T* ret = (T*)Data; Data = 0; return ret; }
  T*          toCptr() volatile;
  int_ptr<T>& fromCptr(T* ptr);
  void        fromCptr(T* ptr) volatile;
  //T*          swapCptr(T* ptr);
  #ifdef DEBUG_LOG
  volatile T* debug() volatile const    { return (T*)(Data & INT_PTR_POINTER_MASK); }
  #endif
};

#ifdef DEBUG
static volatile unsigned max_outer_count = 0;
#endif

template <class T>
unsigned int_ptr<T>::acquire() volatile const
{ if (!Data)
    return 0; // fast path
  const unsigned old_outer = InterlockedXad(&(unsigned&)Data, 1) + 1;
  const unsigned outer_count = old_outer & INT_PTR_COUNTER_MASK;
  ASSERT((old_outer & INT_PTR_COUNTER_MASK) != 0); // overflow condition
  const unsigned new_outer = old_outer & INT_PTR_POINTER_MASK;
  if (new_outer)
    // Transfer counter to obj->count.
    InterlockedAdd(&((T*)new_outer)->access_counter(), INT_PTR_ALIGNMENT - outer_count + 1);
  // And reset it in *this.
  if (InterlockedCxc(&(unsigned&)Data, old_outer, new_outer) != old_outer && new_outer)
    // Someone else does the job already => undo.
    InterlockedAdd(&((T*)new_outer)->access_counter(), outer_count);
    // The global count cannot return to zero here, because we have an active reference.
  #ifdef DEBUG
  // Diagnostics
  unsigned max_outer = max_outer_count;
  if (max_outer < outer_count)
  { do max_outer = InterlockedCxc(&max_outer_count, max_outer, outer_count);
    while (max_outer < outer_count);
    DEBUGLOG(("int_ptr<T>::acquire() : max_outer_count now at %lu\n", max_outer));
  } 
  #endif
  return new_outer;
}

template <class T>
void int_ptr<T>::release(unsigned data)
{ T* obj = (T*)(data & INT_PTR_POINTER_MASK);
  if (obj)
  { unsigned adjust = -((data & INT_PTR_COUNTER_MASK) + INT_PTR_ALIGNMENT);
    // If you get an error about access_counter undeclared,
    // this is most likely because T is an incomplete type.
    adjust += InterlockedXad(&obj->access_counter(), adjust);
    if (adjust == 0)
      delete obj;
  }
}

template <class T>
unsigned int_ptr<T>::transfer(unsigned data)
{ const unsigned outer = data & INT_PTR_COUNTER_MASK;
  if (outer)
  { data &= INT_PTR_POINTER_MASK;
    if (data)
      InterlockedSub(&((T*)data)->access_counter(), outer);
  }
  return data;
}

// IBM C++ work around:
// These functions must not be declared in the class to avoid type dependency problems.
template <class T>
inline int_ptr<T>::int_ptr(T* ptr)
: Data((unsigned)ptr)
{ if (Data)
    InterlockedAdd(&((T*)Data)->access_counter(), INT_PTR_ALIGNMENT);
}
template <class T>
inline int_ptr<T>::int_ptr(int_ptr<T>& r)
: Data(r.Data)
{ if (Data)
    InterlockedAdd(&((T*)Data)->access_counter(), INT_PTR_ALIGNMENT);
}
template <class T>
inline int_ptr<T>::int_ptr(const int_ptr<T>& r)
: Data(r.Data)
{ if (Data)
    InterlockedAdd(&((T*)Data)->access_counter(), INT_PTR_ALIGNMENT);
}
template <class T>
inline int_ptr<T>::int_ptr(volatile const int_ptr<T>& r)
: Data(r.acquire())
{}

template <class T>
inline int_ptr<T>::~int_ptr()
{ release(Data);
}

template <class T>
inline void int_ptr<T>::swap(volatile int_ptr<T>& r)
{ Data = transfer(InterlockedXch(&r.Data, Data));
}
template <class T>
inline void int_ptr<T>::swap(int_ptr<T>& r) volatile
{ r.swap(*this);
}

template <class T>
inline void int_ptr<T>::reset()
{ unsigned d = Data;
  Data = 0;
  release(d);
}
template <class T>
inline void int_ptr<T>::reset() volatile
{ release(InterlockedXch(&Data, 0));
}

template <class T>
inline int_ptr<T>& int_ptr<T>::operator=(T* ptr)
{ int_ptr<T>(ptr).swap(*this);
  return *this;
}
template <class T>
inline int_ptr<T>& int_ptr<T>::operator=(int_ptr<T>& r)
{ int_ptr<T>(r).swap(*this);
  return *this;
}
template <class T>
inline int_ptr<T>& int_ptr<T>::operator=(const int_ptr<T>& r)
{ int_ptr<T>(r).swap(*this);
  return *this;
}
template <class T>
inline int_ptr<T>& int_ptr<T>::operator=(volatile const int_ptr<T>& r)
{ int_ptr<T>(r).swap(*this);
  return *this;
}
template <class T>
inline void int_ptr<T>::operator=(T* ptr) volatile
{ int_ptr<T>(ptr).swap(*this);
}
template <class T>
inline void int_ptr<T>::operator=(int_ptr<T>& r) volatile
{ int_ptr<T>(r).swap(*this);
}
template <class T>
inline void int_ptr<T>::operator=(const int_ptr<T>& r) volatile
{ int_ptr<T>(r).swap(*this);
}
template <class T>
inline void int_ptr<T>::operator=(volatile const int_ptr<T>& r) volatile
{ int_ptr<T>(r).swap(*this);
}

template <class T>
inline T* int_ptr<T>::toCptr() volatile
{ return (T*)transfer(InterlockedXch(&Data, 0));
}

template <class T>
inline int_ptr<T>& int_ptr<T>::fromCptr(T* ptr)
{ int_ptr<T>((unsigned)ptr).swap(*this);
  return *this;
}
template <class T>
inline void int_ptr<T>::fromCptr(T* ptr) volatile
{ int_ptr<T>((unsigned)ptr).swap(*this);
}


/// Scoped array class, non-copyable
template <class T>
class sco_arr
{private:
  T*     Ptr;
  size_t Size;
 private: // non-copyable
  sco_arr(const sco_arr<T>&);
  void operator=(const sco_arr<T>&);
 public:
  /// Initialize a empty array.
  sco_arr()                             : Ptr(NULL), Size(0) {};
  /// Allocates an array of size.
  sco_arr(size_t size)                  : Ptr(new T[size]), Size(size) {}
  /// Store a new object under control.
  sco_arr(T* ptr, size_t size)          : Ptr(ptr), Size(size) {};
  /// Destructor, frees the stored object if any.
  ~sco_arr()                            { delete[] Ptr; }
  /// Gets a pointer to the first array element.
  T*     get() const                    { return Ptr; }
  /// Gets the number of array elements.
  size_t size() const                   { return Size; }
  /// Access the i-th array element.
  T&     operator[](size_t idx) const   { ASSERT(idx < Size); return Ptr[idx]; }
  /// Return start iterator
  T*     begin()                        { ASSERT(Ptr); return Ptr; }
  /// Return start iterator
  const T* begin() const                { ASSERT(Ptr); return Ptr; }
  /// Return beyond the end iterator
  T*     end()                          { ASSERT(Ptr); return Ptr + Size; }
  /// Return beyond the end iterator
  const T* end() const                  { ASSERT(Ptr); return Ptr + Size; }

  /// Frees the stored array (if any) and sets size to 0,
  void   reset()                        { delete[] Ptr; Ptr = NULL; Size = 0; }
  /// Frees the stored array (if any) and allocates a new array of size.
  void   reset(size_t size)             { delete[] Ptr; Ptr = new T[size]; Size = size; }
  /// Frees the stored array (if any) and assigns a new array.
  void   assign(T* ptr, size_t size)    { delete[] Ptr; Ptr = ptr; Size = size; }
};

#endif
