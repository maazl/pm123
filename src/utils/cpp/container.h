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


class vector_base
{private:
  void** Data;
  size_t Size;
  size_t Capacity;

 private: // non-copyable
  vector_base(const vector_base& r);
  void operator=(const vector_base& r);
 protected:
  vector_base(size_t capacity = 16);
  ~vector_base();

  void*&             insert(size_t where);
  void*              erase(size_t where);
  void*&             at(size_t where) const { return Data[where]; }
 public:
  size_t             size() const { return Size; }
};

template <class T>
class vector : public vector_base
{public:
  vector(size_t capacity = 16) : vector_base(capacity) {}
  //~vector_base();
  T*&                insert(size_t where)           { return (T*&)vector_base::insert(where); }
  T*                 erase(size_t where)            { return (T*)vector_base::erase(where); }
  T*                 operator[](size_t where) const { return (T*)at(where); }
  T*&                operator[](size_t where)       { return (T*&)at(where); }
  T*const*           begin() const                  { return &(T*&)at(0); }
  T**                begin()                        { return &(*this)[0]; }
  T*const*           end() const                    { return &(T*&)at(size()); }
  T**                end()                          { return &(*this)[size()]; }
};

/* Interface for compareable objects.
 */
template <class K>
struct IComparableTo
{ virtual int CompareTo(const K* key) const = 0;
};


template <class K>
class sorted_vector_base : public vector<IComparableTo<K> >
{protected:
  sorted_vector_base(size_t capacity) : vector<IComparableTo<K> >(capacity) {}
  bool               binary_search(const K* key, size_t& pos) const;
  IComparableTo<K>*  find(const K* key) const;
  IComparableTo<K>*& get(const K* key);
 public:
  IComparableTo<K>*  erase(const K* key);
};


template <class T, class K>
class sorted_vector : public sorted_vector_base<K>
{public:
  sorted_vector(size_t capacity) : sorted_vector_base<K>(capacity) {}
  T*                 find(const K* key) const       { return (T*)sorted_vector_base<K>::find(key); }
  T*&                get(const K* key)              { return (T*&)sorted_vector_base<K>::get(key); }
};


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
