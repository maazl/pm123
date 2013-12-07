/*
 * Copyright 2011-2013 M.Mueller
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

#ifndef STRINGSET_H
#define STRINGSET_H

#include <cpp/xstring.h>
#include <cpp/algorithm.h>
#include <cpp/container/vector.h>


class stringset_base : protected vector_base
{private:
  int            (*Comparer)(const void* l, const void* r);
 private:
  void           inc_refs();
  void           dec_refs();
 protected:
  /// Create a new stringset with a given initial capacity.
  /// If capacity is 0 the stringset is initially created empty
  /// and allocated with the default capacity when the first item is inserted.
  stringset_base(int (*comparer)(const void*, const void*), size_t capacity = 0)
                                                : vector_base(capacity), Comparer(comparer) {}
  /// Copy constructor, O(n).
  stringset_base(const stringset_base& r, size_t spare = 0);
  ~stringset_base()                             { dec_refs(); }

  stringset_base& operator=(const stringset_base& r);
 public:
  /// Swap two instances. O(1)
  void           swap(stringset_base& r)        { ::swap(Comparer, r.Comparer); vector_base::swap(r); }

  size_t         size() const                   { return vector_base::size(); }

  /// @brief Access an element by it's index.
  /// @details Performance: O(1)
  /// @return String at index \a where.
  /// @pre \a where &isin; [0,size())
  const xstring& operator[](size_t where) const { return (const xstring&)at(where); }
  /// @brief Get a constant iterator that points to the first element or past the end if the vector is empty.
  /// @details Performance: O(1)
  const xstring* begin() const                  { return (const xstring*)vector_base::begin(); }
  /// @brief Get a iterator that points to the first element or past the end if the vector is empty.
  /// @details Performance: O(1)
  const xstring* end() const                    { return begin() + size(); }

  /// @brief Search for a given key. O(log n)
  /// @return The function returns a flag whether you got an exact match or not.
  /// @param pos [out] The index of the first element >= key is always returned in the output parameter \a pos.
  bool           locate(const xstring& key, size_t& pos) const { return ::binary_search(key.cdata(), pos, *this, Comparer); }
  /// @brief Find an element by it's key. O(log n)
  /// @return The function will return \c NULL if no such element is in the container.
  xstring        find(const xstring& key) const;
  /// @brief Ensure an element with a particular key. O(log n)
  /// @param key Key of the new element.
  /// @return This will either return a reference to an existing string which equals \a key
  /// or a reference to a \c NULL string which is automatically created at the location in the container
  /// where a new object with \a key should be inserted. So you can store the string after the function returned.
  /// There is no check whether the assigned element matches \a key, but if not then subsequent calls
  /// to \c btree_base functions have undefined behavior. It is also an error not to assign
  /// a element if the function returned a reference to NULL.
  xstring&       get(const xstring& item);
  /// @brief Ensure an element with a particular key. O(log n)
  /// @param key Key of the new element.
  /// @return This will either return a reference to an existing string which equals \a key
  /// or a reference to \a item which has been added.
  /// If \a item is \c NULL the collection is unchanged and the function returns \c NULL.
  const xstring& ensure(const xstring& item);
  /// Erase an element with a given key. O(log(n))
  /// @param key Key of the element to erase.
  /// @return Element that just have been removed or \c NULL if no element matches \a key.
  xstring        erase(const xstring& item);
  /// Remove all elements
  void           clear()                        { dec_refs(); vector_base::clear(); }
};

class stringset : public stringset_base
{private:
  static int     Compare(const void* l, const void* r);
 public:
  /// Create a new stringset with a given initial capacity.
  /// If capacity is 0 the stringset is initially created empty
  /// and allocated with the default capacity when the first item is inserted.
  stringset(size_t capacity = 0);
  /// Copy constructor, O(n).
  stringset(const stringset& r, size_t spare = 0) : stringset_base(r, spare) {}

  stringset& operator=(const stringset& r)      { return (stringset&)stringset_base::operator=(r); }
};

class stringsetI : public stringset_base
{private:
  static int     Compare(const void* l, const void* r);
 public:
  /// Create a new stringset with a given initial capacity.
  /// If capacity is 0 the stringset is initially created empty
  /// and allocated with the default capacity when the first item is inserted.
  stringsetI(size_t capacity = 0);
  /// Copy constructor, O(n).
  stringsetI(const stringsetI& r, size_t spare = 0) : stringset_base(r, spare) {}

  stringsetI& operator=(const stringsetI& r)      { return (stringsetI&)stringset_base::operator=(r); }
};


#endif

