/*
 * Copyright 2012-2013 M.Mueller
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

#ifndef CPP_CONTAINER_BTREE_H
#define CPP_CONTAINER_BTREE_H

#include <cpp/smartptr.h>
#include <cpp/algorithm.h> // for sort_comparer
#include <cpp/xstring.h>
#include <limits.h>
#include <debuglog.h>


/** @brief Non-generic base implementation of a binary tree.
 * @details The class stores only untyped reference type objects.
 * All mutating functions invalidate all iterators except for the ones
 * passed by reference or returned by the function. The container
 * does not take any ownership of the elements inside.
 * Look at \c \see btree_own&lt;&gt; or \c \see btree_int&lt;&gt; for this purpose.
 * @remarks In general you should prefer the strongly typed version \c \see btree&lt;&gt;.
 * @note The implementation uses a rather small memory footprint.
 * Amortized you have a total size of approximately 48 bits per entry.
 * This consists of 32 bits for the pointer, about 4 bits for the tree structure
 * and an average load factor of 75%.
 */
class btree_base
{protected:
  enum
  { NODE_SIZE = 32  ///< Number of entries in tree nodes. (32767 max!) @remarks Making this a constant saves another set of storage.
  , MAX_DEPTH = 6   ///< Maximum depth of the tree. @remarks The limit comes from the maximum memory size and the \c NODE_SIZE.
  };

 private:
  struct Node;
  struct Leaf
  {public:
    typedef unsigned short pos_t;
   public:
    Node*           Parent;                         ///< Link to the node that contains this one or \c NULL in case of the root.
    pos_t           ParentPos;                      ///< Position in the parent node.
   protected:
    pos_t           Size;                           ///< Number of used entries in this node.
   public:
    void*           Content[NODE_SIZE];             ///< Entries
   private: // non-copyable
                    Leaf(const Leaf&);
    void            operator=(const Leaf&);
   protected:
                    Leaf()                          {}
   public:
                    Leaf(size_t size)               : Size(size | SHRT_MIN) {}
    bool            isLeaf() const                  { return (short)Size < 0; }
    size_t          size() const                    { ASSERT((Size & SHRT_MAX) <= NODE_SIZE); return Size & SHRT_MAX; }

    /// Insert a key at \a pos.
    /// @pre \a pos &le; \c size() && size() < BTREE_NODE_SIZE
    void*&          insert(size_t pos);
    /// Erase the entry a pos from the list.
    /// @pre \a pos &lt; \c size()
    void            erase(size_t pos);
    /// Rebalance the current leaf with its right sibling.
    void            rebalanceR2L(Leaf& src, size_t count);
    /// Rebalance the current leaf with its left sibling.
    void            rebalanceL2R(Leaf& dst, size_t count);
    /// Split leaf node into two siblings.
    /// @param dst New leaf to use. \c dst.Size must be initialized before the call to the intended number of slots to move to dst.
    Leaf*           split(size_t count);
    /// Join two siblings.
    void            join(Leaf& src);
    /// Clone this node.
    Leaf*           clone(Node* parent) const;
    /// Destroy this node.
    void            destroy();
    #ifdef DEBUG
    void            check() const;
    #endif
  };
  struct Node : public Leaf
  {friend struct Leaf;
    //size_t          Count;                          ///< Number of entries in this node and all sub nodes.
    Leaf*           SubNodes[NODE_SIZE+1];          ///< Sub nodes around key elements of base class.
   public:
                    Node(size_t size)               { Size = size; }
    pos_t           size() const                    { return Size; }
    /// Insert a key at \a pos.
    /// @pre \a pos &le; \c size() && size() < BTREE_NODE_SIZE
    void*&          insert(size_t pos);
    /// Erase the key \e and the sub node \e after \a pos from the node.
    /// @pre \a pos &lt; \c size()
    void            erase(size_t pos);
   private: // polymorphic functions should only be called through base class
    /// Rebalance the current leaf with its right sibling.
    void            rebalanceR2L(Node& src, size_t count);
    /// Rebalance the current leaf with its left sibling.
    void            rebalanceL2R(Node& dst, size_t count);
    /// Split a node into two siblings.
    /// @param dst New node to use. \c dst.Size must be initialized before the call to the intended number of slots to move to dst.
    void            split(Node& dst);
    /// Join two siblings.
    void            join(Node& src);
    /// Clone this node.
    Node*           clone() const;
    /// Destroy this node.
    void            destroy();
    #ifdef DEBUG
    void            check() const;
    #endif
  };
 public:
  /// Iterator to enumerate the content of the container or to perform partial scans.
  class iterator
  { friend class btree_base;
   private:
    /// Pointer to the leaf or node where the current element resides.
    /// \c NULL if the container is empty.
    Leaf*           Ptr;
    /// Position of the current element within \c *Ptr.
    /// Equals \c Ptr->size() if the iterator is end(). In the latter case \c Ptr->Parent should be \c NULL.
    size_t          Pos;
   protected:
                    iterator()                      {}
                    iterator(Leaf* link, size_t pos) : Ptr(link), Pos(pos) {}
    void*&          get() const                     { return Ptr->Content[Pos]; }
   public:
    /// Advance to the next key.
    /// @pre \c !isend()
    void            next();
    /// Go back to the previous key.
    /// @pre \c !isbegin()
    void            prev();
    /// @brief Fast comparison against \c btree_base.begin()
    /// @return \c true if the iterator points to the start of the container.
    /// @remarks Using this is faster then comparing against btree_base.begin() in a loop
    /// because it avoids the creation of the temporary end iterator.
    bool            isbegin() const;
    /// @brief Fast comparison to \c btree_base.end()
    /// @return \c true if the iterator is at the end and does not point to a valid element.
    /// @remarks Using this is faster then comparing against btree_base.end() in a loop
    /// because it avoids the creation of the temporary end iterator.
    bool            isend() const                   { return !Ptr || Pos == Ptr->size(); }
    /// Advance to the next key.
    /// @pre \c !isend()
    const iterator& operator++()                    { next(); return *this; }
    /// Go back to the previous key.
    /// @pre \c !isbegin()
    const iterator& operator--()                    { prev(); return *this; }
    /// Return the data where the current iterator points to.
    /// @pre \c !isend()
    void*           operator*() const               { ASSERT(!isend()); return Ptr->Content[Pos]; }
    /// Compare two iterators for equality.
    friend bool     operator==(iterator l, iterator r) { return l.Ptr == r.Ptr && l.Pos == r.Pos; }
    /// Compare two iterators for inequality.
    friend bool     operator!=(iterator l, iterator r) { return l.Ptr != r.Ptr || l.Pos != r.Pos; }
    /// Relational compare to another iterator. O(log n)
    /// @param r right comperand.
    /// @return &lt;&nbsp;0 := \c *this &lt; \a r, =&nbsp;0 := \c *this == \a r, &gt;&nbsp;0 := \c *this &gt; \a r, \c INT_MIN := unrelated.
    int             compare(iterator r);
    friend bool     operator<(iterator l, iterator r) { return r.compare(l) > 0; }
    friend bool     operator<=(iterator l, iterator r) { return r.compare(l) >= 0; }
    friend bool     operator>(iterator l, iterator r) { return l.compare(r) > 0; }
    friend bool     operator>=(iterator l, iterator r) { return l.compare(r) >= 0; }
  };

 private:
  Leaf*             Root;
  int               (*const Comparer)(const void* key, const void* elem);

 private:
  void              rebalanceOrSplit(iterator& where);
  bool              joinOrRebalance(iterator& where);
  void              autoshrink();
 public:
  /// Create empty btree.
                    btree_base(int (*cmp)(const void* key, const void* elem)) : Root(NULL), Comparer(cmp) {}
  /// Copy constructor. O(n)
                    btree_base(const btree_base& r) : Root(r.Root ? r.Root->clone(NULL) : NULL), Comparer(r.Comparer) {}
  /// Swap two instances. O(1)
  void              swap(btree_base& r)             { ::swap(Root, r.Root); }
  /// Assignment. O(n)
  btree_base&       operator=(const btree_base& r);
  /// Iterator to the start of the collection. O(log n)
  iterator          begin() const;
  /// Iterator to the end of the collection. O(1)
  iterator          end() const                     { return iterator(Root, !!Root ? Root->size() : 0); }
  /// Check whether the collection has no elements. O(1)
  bool              empty() const                   { return !Root; }
  /// @brief Locate an element in the container. O(log n)
  /// @param key Key of the element to locate.
  /// @param where [out] The location of the first element >= key is always returned in the output parameter \a where.
  /// @param cmp Comparer to be used to compare the key against elements.
  /// The comparer could be asymmetric, e.g. if the elements have an intrinsic key.
  /// The function could be used as upper/lower bound also if the comparer never returns equal.
  /// @return The function returns a flag whether you got an exact match or not.
  bool              locate(const void* key, iterator& where) const;
  /// @brief Find an element by it's key. O(log n)
  /// @return The function will return \c NULL if no such element is in the container.
  void*             find(const void* key) const;
  /// @brief Insert a new element in the tree.
  /// @param where where to insert the new slot.
  /// @return The function does not take the value of the new element.
  /// Instead it returns a reference to the new location that should be assigned later.
  /// @details The reference is valid until the next non-const member function call.
  /// The initial value is NULL. Performance: O(log(n))
  /// @remarks The function does not ensure the sort order. So be careful.
  void*&            insert(iterator& where);
  /// @brief Ensure an element with a particular key. O(log n)
  /// @param key Key of the new element.
  /// @param cmp Comparer to be used to compare the key against elements.
  /// The comparer could be asymmetric, e.g. if the elements have an intrinsic key.
  /// The container can store multiple elements with the same key, if the comparer never returns equal.
  /// @return This will either return a reference to a pointer to an existing object which equals \a key
  /// or a reference to a \c NULL pointer which is automatically created at the location in the container
  /// where a new object with \a key should be inserted. So you can store the Pointer to this object after the function returned.
  /// There is no check whether the assigned element matches \a key, but if not then subsequent calls
  /// to \c btree_base functions have undefined behavior. It is also an error not to assign
  /// a element if the function returned a reference to NULL.
  void*&            get(const void* key);
  /// @brief Removes an element from the tree. O(log n)
  /// @param where The \a where pointer is set to the next item after the removed one.
  /// @pre \a where &isin; [begin(),end())
  void*             erase(iterator& where);
  /// Erase an element with a given key. O(log n)
  /// @param key Key of the element to erase.
  /// @param cmp Comparer to be used to compare the key against elements.
  /// The comparer could be asymmetric, e.g. if the elements have an intrinsic key.
  /// @return Element that just have been removed or \c NULL if no element matches \a key.
  void*             erase(const void* key);
  /// Remove all elements.
  void              clear();
  /// Destroy container
  /// @remarks This will will free the tree structure but not the elements referenced by the content.
                    ~btree_base()                   { clear(); }
  /// Check container for integrity. (Debug builds only.)
  #ifdef DEBUG
  void              check() const                   { if (Root) { ASSERT(Root->Parent == NULL); Root->check(); } }
  #else
  void              check() const                   {}
  #endif
};

inline btree_base& btree_base::operator=(const btree_base& r)
{ clear();
  Root = r.Root ? r.Root->clone(NULL) : NULL;
  return *this;
}

inline void* btree_base::find(const void* key) const
{ iterator where;
  if (!locate(key, where))
    return NULL;
  else
    return *where;
}

inline void*& btree_base::get(const void* key)
{ iterator where;
  if (!locate(key, where))
    return insert(where);
  else
    return where.Ptr->Content[where.Pos];
}

inline void* btree_base::erase(const void* key)
{ iterator where;
  if (!locate(key, where))
    return NULL;
  else
    return erase(where);
}


/** @brief Strongly typed binary tree.
 * @details The class stores only typed reference objects of type \a T.
 * All mutating functions invalidate all iterators except for the ones
 * passed by reference or returned by the function. The container
 * does not take any ownership of the elements inside.
 * Look at \c \see btree_own&lt;&gt; or \c \see btree_int&lt;&gt; for this purpose.
 * @tparam T Element type in the container.
 * @tparam K Key type of the elements. This is the same than \a T if the container is used as a set,
 * and a distinct type if the container stores elements with an intrinsic key.
 * @tparam C Comparer used to compare keys against elements.
 * @remarks The class does not know how to extract a key from an element.
 * It is up to the (asymmetric) comparer to handle this.
 */
template <class T, class K, sort_comparer>
class btree : btree_base
{public:
  /// Iterator to enumerate the content of the container or to perform partial scans.
  class iterator : public btree_base::iterator
  { friend class btree;
   private:
    iterator(const btree_base::iterator& r)         : btree_base::iterator(r) {}
   public:
    iterator()                                      {}
    /// Advance to the next key.
    /// @pre \c !isend()
    const iterator& operator++()                    { next(); return *this; }
    /// Go back to the previous key.
    /// @pre \c !isbegin()
    const iterator& operator--()                    { prev(); return *this; }
    /// Return the data where the current iterator points to.
    /// @pre \c !isend()
    T*              operator*() const               { return (T*)btree_base::iterator::operator*(); }
  };
 public:
                    btree()                         : btree_base((int (*)(const void*, const void*))C) {}
  /// Swap two instances. O(1)
  void              swap(btree<T,K,C>& r)           { btree_base::swap(r); }
  /// Iterator to the start of the collection. O(log n)
  iterator          begin() const                   { return (iterator)btree_base::begin(); }
  /// Iterator to the end of the collection. O(1)
  iterator          end() const                     { return (iterator)btree_base::end(); }
  /// @brief Locate an element in the container. O(log n)
  /// @param key Key of the element to locate.
  /// @param where [out] The index of the first element >= key is always returned in the output parameter \a where.
  /// @param cmp Comparer to be used to compare the key against elements.
  /// The comparer could be asymmetric, e.g. if the elements have an intrinsic key.
  /// The function could be used as upper/lower bound also if the comparer never returns equal.
  /// @return The function returns a flag whether you got an exact match or not.
  bool              locate(const K& key, iterator& where) const { return btree_base::locate(&key, (btree_base::iterator&)where); }
  /// @brief Find an element by it's key. O(log n)
  /// @return The function will return \c NULL if no such element is in the container.
  T*                find(const K& key) const        { return (T*)btree_base::find(&key); }
  /// @brief Insert a new element in the tree.
  /// @param where where to insert the new slot.
  /// @return The function does not take the value of the new element.
  /// Instead it returns a reference to the new location that should be assigned later.
  /// @details The reference is valid until the next non-const member function call.
  /// The initial value is NULL. Performance: O(log(n))
  /// @remarks The function does not ensure the sort order. So be careful.
  T*&               insert(iterator& where)         { return (T*&)btree_base::insert((btree_base::iterator&)where); }
  /// @brief Ensure an element with a particular key. O(log n)
  /// @param key Key of the new element.
  /// @param cmp Comparer to be used to compare the key against elements.
  /// The comparer could be asymmetric, e.g. if the elements have an intrinsic key.
  /// The container can store multiple elements with the same key, if the comparer never returns equal.
  /// @return This will either return a reference to a pointer to an existing object which equals \a key
  /// or a reference to a \c NULL pointer which is automatically created at the location in the container
  /// where a new object with \a key should be inserted. So you can store the Pointer to this object after the function returned.
  /// There is no check whether the assigned element matches \a key, but if not then subsequent calls
  /// to \c btree_base functions have undefined behavior. It is also an error not to assign
  /// a element if the function returned a reference to NULL.
  T*&               get(const K& key)               { return (T*&)btree_base::get(&key); }
  /// @brief Removes an element from the tree. O(log(n))
  /// @param where The \a where pointer is set to the next item after the removed one.
  /// @pre \a where &isin; [begin(),end())
  T*                erase(iterator& where)          { return (T*)btree_base::erase((btree_base::iterator&)where); }
  /// Erase an element with a given key. O(log(n))
  /// @param key Key of the element to erase.
  /// @return Element that just have been removed or \c NULL if no element matches \a key.
  T*                erase(const K& key)             { return (T*)btree_base::erase(&key); }
};


template <class T, class K, sort_comparer>
class btree_own : public btree<T,K,C>
{private: // non-copyable
  btree_own(const btree_own<T,K,C>&);
  void operator=(const btree_own<T,K,C>&);
 public:
                    btree_own()                     {}
  /// Remove and delete all elements.
  void              clear();
                    ~btree_own()                    { clear(); }
};

template <class T, class K, sort_comparer>
void btree_own<T,K,C>::clear()
{ typename btree_own<T,K,C>::iterator where(begin());
  while (!where.isend())
  { delete *where;
    ++where;
  }
  btree<T,K,C>::clear();
}


template <class T, class K, sort_comparer>
class btree_int : public btree_base
{public:
  /// Iterator to enumerate the content of the container or to perform partial scans.
  class iterator : public btree_base::iterator
  { friend class btree_int;
   private:
    iterator(const btree_base::iterator& r)         : btree_base::iterator(r) {}
   public:
    iterator()                                      {}
    /// Advance to the next key.
    /// @pre \c !isend()
    const iterator& operator++()                    { next(); return *this; }
    /// Go back to the previous key.
    /// @pre \c !isbegin()
    const iterator& operator--()                    { prev(); return *this; }
    /// Return the data where the current iterator points to.
    /// @pre \c !isend()
    T*              operator*() const               { return (T*)btree_base::iterator::operator*(); }
  };
 private:
  void              inc_refs();
  void              dec_refs();
 public:
                    btree_int()                     : btree_base((int (*)(const void*, const void*))C) {}
                    btree_int(const btree_int<T,K,C>& r) : btree_base(r) { inc_refs(); }
  btree_int<T,K,C>& operator=(const btree_int<T,K,C>& r);
  /// Swap two instances. O(1)
  void              swap(btree_int<T,K,C>& r)       { btree_base::swap(r); }
  /// Iterator to the start of the collection. O(log n)
  iterator          begin() const                   { return (iterator)btree_base::begin(); }
  /// Iterator to the end of the collection. O(1)
  iterator          end() const                     { return (iterator)btree_base::end(); }
  /// @brief Locate an element in the container. O(log n)
  /// @param key Key of the element to locate.
  /// @param where [out] The index of the first element >= key is always returned in the output parameter \a where.
  /// @param cmp Comparer to be used to compare the key against elements.
  /// The comparer could be asymmetric, e.g. if the elements have an intrinsic key.
  /// The function could be used as upper/lower bound also if the comparer never returns equal.
  /// @return The function returns a flag whether you got an exact match or not.
  bool              locate(const K& key, iterator& where) const { return btree_base::locate(&key, (btree_base::iterator&)where); }
  /// @brief Find an element by it's key. O(log n)
  /// @return The function will return \c NULL if no such element is in the container.
  T*                find(const K& key) const        { return (T*)btree_base::find(&key); }
  /// @brief Insert a new element in the tree.
  /// @param where where to insert the new slot.
  /// @return The function does not take the value of the new element.
  /// Instead it returns a reference to the new location that should be assigned later.
  /// @details The reference is valid until the next non-const member function call.
  /// The initial value is NULL. Performance: O(log(n))
  /// @remarks The function does not ensure the sort order. So be careful.
  int_ptr<T>&       insert(iterator& where)         { return (int_ptr<T>&)btree_base::insert((btree_base::iterator&)where); }
  /// @brief Ensure an element with a particular key. O(log n)
  /// @param key Key of the new element.
  /// @param cmp Comparer to be used to compare the key against elements.
  /// The comparer could be asymmetric, e.g. if the elements have an intrinsic key.
  /// The container can store multiple elements with the same key, if the comparer never returns equal.
  /// @return This will either return a reference to a pointer to an existing object which equals \a key
  /// or a reference to a \c NULL pointer which is automatically created at the location in the container
  /// where a new object with \a key should be inserted. So you can store the Pointer to this object after the function returned.
  /// There is no check whether the assigned element matches \a key, but if not then subsequent calls
  /// to \c btree_base functions have undefined behavior. It is also an error not to assign
  /// a element if the function returned a reference to NULL.
  int_ptr<T>&       get(const K& key)               { return (int_ptr<T>&)btree_base::get(&key); }
  /// @brief Removes an element from the tree. O(log(n))
  /// @param where The \a where pointer is set to the next item after the removed one.
  /// @pre \a where &isin; [begin(),end())
  int_ptr<T>        erase(iterator& where)          { return int_ptr<T>().fromCptr((T*)btree_base::erase((btree_base::iterator&)where)); }
  /// Erase an element with a given key. O(log(n))
  /// @param key Key of the element to erase.
  /// @return Element that just have been removed or \c NULL if no element matches \a key.
  int_ptr<T>        erase(const K& key)             { return int_ptr<T>().fromCptr((T*)btree_base::erase(&key)); }
  /// Remove and release all elements.
  void              clear()                         { dec_refs(); btree_base::clear(); }
                    ~btree_int()                    { dec_refs(); }
};

template <class T, class K, sort_comparer>
void btree_int<T,K,C>::inc_refs()
{ typename btree_int<T,K,C>::iterator where(begin());
  int_ptr<T> ptr;
  while (!where.isend())
  { ptr = *where;
    ptr.toCptr();
    ++where;
  }
}

template <class T, class K, sort_comparer>
void btree_int<T,K,C>::dec_refs()
{ typename btree_int<T,K,C>::iterator where(begin());
  int_ptr<T> ptr;
  while (!where.isend())
  { ptr.fromCptr(*where);
    ++where;
  }
}

template <class T, class K, sort_comparer>
btree_int<T,K,C>& btree_int<T,K,C>::operator=(const btree_int<T,K,C>& r)
{ dec_refs();
  btree_base::operator=(r);
  inc_refs();
  return *this;
}

class btree_string : public btree_base
{public:
  /// Iterator to enumerate the content of the container or to perform partial scans.
  class iterator : public btree_base::iterator
  { friend class btree_string;
   private:
    iterator(const btree_base::iterator& r)         : btree_base::iterator(r) {}
   public:
    iterator()                                      {}
    /// Advance to the next key.
    /// @pre \c !isend()
    const iterator& operator++()                    { next(); return *this; }
    /// Go back to the previous key.
    /// @pre \c !isbegin()
    const iterator& operator--()                    { prev(); return *this; }
    /// Return the data where the current iterator points to.
    /// @pre \c !isend()
    const xstring&  operator*() const               { return (xstring&)btree_base::iterator::get(); }
    /// Access the data where the current iterator points to.
    /// @pre \c !isend()
    const xstring*  operator->() const              { return &(xstring&)btree_base::iterator::get(); }
  };

 private:
  static int        Compare(const void* key, const void* elem);
  void              inc_refs();
  void              dec_refs();
 public:
                    btree_string()                  : btree_base(&btree_string::Compare) {}
                    btree_string(const btree_string& r) : btree_base(r) { inc_refs(); }
  btree_string&     operator=(const btree_string& r);
  /// Swap two instances. O(1)
  void              swap(btree_string& r)           { btree_base::swap(r); }
  /// Iterator to the start of the collection. O(log n)
  iterator          begin() const                   { return (iterator)btree_base::begin(); }
  /// Iterator to the end of the collection. O(1)
  iterator          end() const                     { return (iterator)btree_base::end(); }
  /// @brief Locate an element in the container. O(log n)
  /// @param key Key of the element to locate.
  /// @param where [out] The index of the first element >= key is always returned in the output parameter \a where.
  /// @param cmp Comparer to be used to compare the key against elements.
  /// The comparer could be asymmetric, e.g. if the elements have an intrinsic key.
  /// The function could be used as upper/lower bound also if the comparer never returns equal.
  /// @return The function returns a flag whether you got an exact match or not.
  bool              locate(const xstring& key, iterator& where) const { return btree_base::locate((const void*)key, (btree_base::iterator&)where); }
  /// @brief Find an element by it's key. O(log n)
  /// @return The function will return \c NULL if no such element is in the container.
  xstring           find(const xstring& key) const;
  /// @brief Insert a new element in the tree.
  /// @param where where to insert the new slot.
  /// @return The function does not take the value of the new element.
  /// Instead it returns a reference to the new location that should be assigned later.
  /// @details The reference is valid until the next non-const member function call.
  /// The initial value is NULL. Performance: O(log(n))
  /// @remarks The function does not ensure the sort order. So be careful.
  xstring&          insert(iterator& where)         { return (xstring&)btree_base::insert((btree_base::iterator&)where); }
  /// @brief Ensure an element with a particular key. O(log n)
  /// @param key Key of the new element.
  /// @param cmp Comparer to be used to compare the key against elements.
  /// The comparer could be asymmetric, e.g. if the elements have an intrinsic key.
  /// The container can store multiple elements with the same key, if the comparer never returns equal.
  /// @return This will either return a reference to a pointer to an existing object which equals \a key
  /// or a reference to a \c NULL pointer which is automatically created at the location in the container
  /// where a new object with \a key should be inserted. So you can store the Pointer to this object after the function returned.
  /// There is no check whether the assigned element matches \a key, but if not then subsequent calls
  /// to \c btree_base functions have undefined behavior. It is also an error not to assign
  /// a element if the function returned a reference to NULL.
  xstring&          get(const xstring& key)         { return (xstring&)btree_base::get((const void*)key); }
  /// @brief Removes an element from the tree. O(log(n))
  /// @param where The \a where pointer is set to the next item after the removed one.
  /// @pre \a where &isin; [begin(),end())
  xstring           erase(iterator& where)          { return xstring().fromCstr((const char*)btree_base::erase((btree_base::iterator&)where)); }
  /// Erase an element with a given key. O(log(n))
  /// @param key Key of the element to erase.
  /// @return Element that just have been removed or \c NULL if no element matches \a key.
  xstring           erase(const xstring& key)       { return xstring().fromCstr((const char*)btree_base::erase((void*)key.cdata())); }
  /// Remove and release all elements.
  void              clear()                         { dec_refs(); btree_base::clear(); }
                    ~btree_string()                 { dec_refs(); }
};

inline xstring btree_string::find(const xstring& key) const
{ iterator where;
  if (!locate(key, where))
    return xstring();
  else
    return *where;
}


#endif
