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
#include <cpp/container/sorted_vector.h> // for sort_comparer
#include <limits.h>
#include <debuglog.h>


/** @brief Non-generic base implementation of a binary tree.
 * @details The class stores only untyped reference type objects.
 * All mutating functions invalidate all iterators except for the ones
 * passed by reference or returned by the function.
 * @remarks In general you should prefer the strongly typed version \c \see btree<>.
 * @note The implementation uses a rather small memory footprint.
 * Amortized you have a total size of approximately 48 bits per entry.
 * This consists of 32 bits for the pointer, about 4 bits for the tree structure
 * and an average load factor of 75%.
 */
class btree_base
{protected:
  /// Number of entries in tree nodes.
  /// @remarks Making this a constant saves another set of storage.
  enum { BTREE_NODE_SIZE = 32 };

 private:
  /*struct Leaf;
  struct Node;
  /// @brief Polymorphic link to a \c Node or a \c Leaf.
  /// @remarks Instead of making the referred object runtime polymorphic
  /// the class uses an unused bit in the pointer to distinguish the types.
  struct Link
  {private:
    Leaf*           Ptr;
   public:
    bool            operator!() const               { return Ptr == NULL; }
    bool            isLeaf() const                  { return (unsigned)Ptr & 1; }
    bool            isNode() const                  { return Ptr && !((unsigned)Ptr & 1); }
    Leaf*           asLeaf() const                  { ASSERT(Ptr); return (Leaf*)((unsigned)Ptr & ~1); }
    Node*           asNode() const                  { ASSERT(isNode()); return (Node*)Ptr; }
    void            reset()                         { Ptr = NULL; }
    void            operator=(Leaf* leaf)           { Ptr = (Leaf*)((unsigned)leaf|1); }
    void            operator=(Node* node)           { Ptr = node; }
                    Link()                          {}
                    Link(Leaf* leaf)                { *this = leaf; }
                    Link(Node* node)                { *this = node; }
    //size_t          count() const                   { return !Data.Ptr ? 0 : Data.Type.IsLeaf ? Data.Ptr->Size : ((Node*)Data.Ptr)->Count; }
    friend bool     operator==(Link l, Link r)      { return l.Ptr == r.Ptr; }
    friend bool     operator!=(Link l, Link r)      { return l.Ptr != r.Ptr; }

   public: // polymorphic forwarders
    inline void     rebalanceR2L(Link src, size_t count);
    inline void     rebalanceL2R(Link dst, size_t count);
    Link            split(size_t count);
    void            join(Link src);
    inline void     destroy(void (*cleanup)(void*));
    #ifdef DEBUG
    void            check() const;
    #endif
    #ifdef DEBUG_LOG
    void*           raw() const                     { return Ptr; }
    #endif
  };*/
  struct Node;
  struct Leaf
  { Node*           Parent;                         ///< Link to the node that contains this one or \c NULL in case of the root.
    size_t          ParentPos;                      ///< Position in the parent node.
   protected:
    size_t          Size;                           ///< Number of used entries in this node.
   public:
    void*           Content[BTREE_NODE_SIZE];       ///< Entries
   protected:
                    Leaf()                          {}
   public:
                    Leaf(size_t size)               : Size(size | INT_MIN) {}
    bool            isLeaf() const                  { return (int)Size < 0; }
    size_t          size() const                    { return Size & INT_MAX; }

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
    void            destroy(void (*cleanup)(void*));
    #ifdef DEBUG
    void            check() const;
    #endif
  };
  struct Node : public Leaf
  {friend struct Leaf;
    //size_t          Count;                          ///< Number of entries in this node and all sub nodes.
    Leaf*           SubNodes[BTREE_NODE_SIZE+1];    ///< Sub nodes around key elements of base class.
   public:
                    Node(size_t size)               { Size = size; }
    size_t          size() const                    { return Size; }
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
    void            destroy(void (*cleanup)(void*));
    #ifdef DEBUG
    void            check() const;
    #endif
  };
 public:
  class iterator
  { friend class btree_base;
   private:
    Leaf*           Ptr;
    size_t          Pos;
   protected:
                    iterator(Leaf* link, size_t pos) : Ptr(link), Pos(pos) {}
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
  };
  struct findresult
  { iterator        where;
    bool            match;
                    findresult(iterator where)      : where(where), match(false) {}
  };

 private:
  Leaf*             Root;

 private:
  void              rebalanceOrSplit(iterator& where);
  bool              joinOrRebalance(iterator& where);
  void              autoshrink();
 protected:
  /// @brief Insert a new element in the tree.
  /// @param where where to insert the
  /// @return The function does not take the value of the new element.
  /// Instead it returns a reference to the new location that should be assigned later.
  /// @details The reference is valid until the next non-const member function call.
  /// The initial value is NULL. Performance: O(log(n))
  /// @remarks The function is neither type safe nor ensures the sort order
  /// and should not be exposed public.
  void*&            insert(iterator& where);
 public:
                    btree_base()                    : Root(NULL) {}
  // Copy constructor.
  //btree(const btree<T,K,C>& r);
  void              swap(btree_base& r)             { ::swap(Root, r.Root); }
  //iterator          at(size_t where) const;
  //void*&            operator[](size_t where) const { return at(where); }
  iterator          begin() const;
  iterator          end() const                     { return iterator(Root, !!Root ? Root->size() : 0); }
  bool              empty() const                   { return !Root; }
  /*// @brief Return the number of elements in the container.
  /// @details This is not equal to the storage capacity.
  size_t            size() const                    { return Root.count(); }*/
  findresult        find(void* key, int (*cmp)(void* key, void* elem)) const;
  findresult        insert(void* key, int (*cmp)(void* key, void* elem));
  /// Remove all elements
  void              clear()                         { Root->destroy(NULL); }
  /// @brief Removes an element from the tree.
  /// @param where The \a where pointer is set to the next item after the removed one.
  /// @pre \a where &isin; [begin(),end())
  /// @details Performance: O(log(n))
  void*             erase(iterator& where);
  void*             erase(void* key, int (*cmp)(void* key, void* elem));
                    ~btree_base()                   { clear(); }
  #ifdef DEBUG
  void              check() const                   { if (Root) { ASSERT(Root->Parent == NULL); Root->check(); } }
  #else
  void              check() const                   {}
  #endif
};

template <class T, class K, sort_comparer>
class btree : btree_base
{public:
  class iterator : public btree_base::iterator
  {public:
    iterator&       operator++()                    { next(); return *this; }
    iterator&       operator--()                    { prev(); return *this; }
    friend bool     operator==(iterator l, iterator r) { return l.Ptr == r.Ptr && l.Pos == r.Pos; }
    friend bool     operator!=(iterator l, iterator r) { return l.Ptr != r.Ptr || l.Pos != r.Pos; }
  };
 public:
  // Copy constructor.
  btree(const btree<T,K,C>& r);
  void              swap(vector<T>& r)             { vector_base::swap(r); }
  //T*                at(size_t where) const         { ASSERT(where < Size); return Data[where]; }
  //T*                operator[](size_t where) const { return (T*)at(where); }
  //iterator          begin() const                  { return (T*const*)vector_base::begin(); }
  //iterator          end() const                    { return !Root iterator(); }
  /// Find an element by it's key.
  /// The function will return NULL if no such element is in the container.
  /// Performance: O(log(n))
  T*                find(const K& key) const;
  /// Ensure an element with a particular key.
  /// This will either return a reference to a pointer to an existing object which equals key
  /// or a reference to a NULL pointer which is automatically created at the location in the container
  /// where a new object with key should be inserted. So you can store the Pointer to this object after the function returned.
  /// Performance: O(log(n))
  T*&               get(const K& key);
  /// Erase the element which equals key and return the removed pointer.
  /// If no such element exists the function returns NULL.
  /// Performance: O(log(n))
  T*                erase(const K& key);
};


/*inline void btree_base::Link::rebalanceR2L(Link src, size_t count)
{ if (isLeaf())
  { ASSERT(src.isLeaf());
    asLeaf()->rebalanceR2L(*src.asLeaf(), count);
  } else
  { ASSERT(src.isNode());
    asNode()->rebalanceR2L(*src.asNode(), count);
  }
}

inline void btree_base::Link::rebalanceL2R(Link dst, size_t count)
{ if (isLeaf())
  { ASSERT(dst.isLeaf());
    asLeaf()->rebalanceL2R(*dst.asLeaf(), count);
  } else
  { ASSERT(dst.isNode());
    asNode()->rebalanceL2R(*dst.asNode(), count);
  }
}

inline void btree_base::Link::destroy(void (*cleanup)(void*))
{ if (isLeaf())
    asLeaf()->destroy(cleanup);
  else if (isNode())
    asNode()->destroy(cleanup);
}*/


inline btree_base::findresult btree_base::insert(void* key, int (*cmp)(void* key, void* elem))
{ findresult result(find(key, cmp));
  if (!result.match)
    insert(result.where) = key;
  else
    result.where.Ptr->Content[result.where.Pos] = key;
  return result;
}

inline void* btree_base::erase(void* key, int (*cmp)(void* key, void* elem))
{ findresult result(find(key, cmp));
  if (result.match)
    return erase(result.where);
  else
    return NULL;
}

/*#ifdef DEBUG
inline void btree_base::Link::check() const
{ if (isLeaf())
    asLeaf()->check();
  else if (isNode())
    asNode()->check();
}

#endif*/

#endif
