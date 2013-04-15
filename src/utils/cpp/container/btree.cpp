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

#undef DEBUG_LOG
#define DEBUG_LOG 2

#include "btree.h"
#include <cpp/cpputil.h>
#include <memory.h>


/* Class btree_base::Leaf and btree_base::Node */

void*& btree_base::Leaf::insert(size_t pos)
{ DEBUGLOG2(("btree_base::Leaf(%p{%p,%u, %u,%u})::insert(%u)\n", this, Parent, ParentPos, isLeaf(), size(), pos));
  ASSERT(pos <= size() && size() < BTREE_NODE_SIZE);
  void** where = Content + pos;
  memmove(where + 1, where, (size() - pos) * sizeof *Content);
  ++Size;
  return *where;
}

void*& btree_base::Node::insert(size_t pos)
{ DEBUGLOG2(("btree_base::Node(%p{%p,%u, %u})::insert(%u)\n", this, Parent, ParentPos, Size, pos));
  void*& ret = Leaf::insert(pos);
  Leaf** where = SubNodes + (pos + 1);
  Leaf** const end = SubNodes + Size;
  memmove(where + 1, where, ((char*)end - (char*)where));
  // Update uplinks
  while (where != end)
    ++(*++where)->ParentPos;
  return ret;
}

void btree_base::Leaf::erase(size_t pos)
{ DEBUGLOG2(("btree_base::Leaf(%p{%p,%u, %u,%u})::erase(%u)\n", this, Parent, ParentPos, isLeaf(), size(), pos));
  ASSERT(pos < size());
  void** where = Content + pos;
  --Size;
  memmove(where, where + 1, (size() - pos) * sizeof *Content);
  #ifdef DEBUG
  Content[size()] = NULL;
  #endif
}

void btree_base::Node::erase(size_t pos)
{ DEBUGLOG2(("btree_base::Node(%p{%p,%u, %u})::erase(%u)\n", this, Parent, ParentPos, Size, pos));
  Leaf::erase(pos);
  Leaf** where = SubNodes + (pos + 1);
  ASSERT((*where)->size() == 0);
  Leaf** const end = SubNodes + (Size + 1);
  memmove(where, where + 1, ((char*)end - (char*)where));
  #ifdef DEBUG
  *end = NULL;
  #endif
  // Update uplinks
  while (where != end)
    --(*where++)->ParentPos;
}

void btree_base::Leaf::rebalanceR2L(Leaf& src, size_t count)
{ DEBUGLOG2(("btree_base::Leaf(%p{%p,%u, %u,%u})::rebalanceR2L(%p{%p,%u, %u,%u}&, %u)\n", this, Parent, ParentPos, isLeaf(), size(), &src, src.Parent, src.ParentPos, src.isLeaf(), src.size(), count));
  ASSERT(Parent == src.Parent && ParentPos +1 == src.ParentPos);
  ASSERT(Size <= src.Size);
  ASSERT(size() + count <= BTREE_NODE_SIZE);
  ASSERT(count && count <= src.size());
  // New delimiting key
  void*& parentkey = Parent->Content[ParentPos];
  void** target = Content + size();
  *target = parentkey;
  parentkey = src.Content[count-1];
  // Move keys from src.Content[0..count-2] to Content[Size+1..].
  memcpy(target + 1, src.Content, (count-1) * sizeof *Content);
  // Move src.Content[count..] to src.Content[0..].
  memmove(src.Content, src.Content + count, (src.size() - count) * sizeof *Content);
  #ifdef DEBUG
  memset(src.Content + (src.size() - count), 0, count * sizeof *Content);
  #endif
  // Update Size
  Size += count;
  src.Size -= count;
  // polymorphic call
  if (!isLeaf())
    ((Node*)this)->rebalanceR2L((Node&)src, count);
}

void btree_base::Node::rebalanceR2L(Node& src, size_t count)
{ DEBUGLOG2(("btree_base::Node(%p{%p,%u, %u})::rebalanceR2L(%p{%p,%u, %u}&, %u)\n", this, Parent, ParentPos, Size, &src, src.Parent, src.ParentPos, src.Size, count));
  //Leaf::rebalanceR2L(src, count);
  // Move children from src.SubNodes[0..count-1] to SubNodes[Size-count+1..Size].
  size_t offset = Size - count + 1;
  Leaf** where = SubNodes + offset;
  Leaf** end = where + count;
  memcpy(where, src.SubNodes, (char*)end - (char*)where);
  // Update uplinks
  while (where != end)
  { Leaf* leaf = *where++;
    leaf->Parent = this;
    leaf->ParentPos += offset;
  }
  // Move src.SubNodes[count..] to src.SubNodes[0..src.Size].
  where = src.SubNodes;
  end = where + src.Size + 1;
  memmove(where, where + count, (char*)end - (char*)where);
  #ifdef DEBUG
  memset(end, 0, count * sizeof *Content);
  #endif
  // Update uplinks
  while (where != end)
    (*where++)->ParentPos -= count;
}

void btree_base::Leaf::rebalanceL2R(Leaf& dst, size_t count)
{ DEBUGLOG2(("btree_base::Leaf(%p{%p,%u, %u,%u})::rebalanceL2R(%p{%p,%u, %u,%u}&, %u)\n", this, Parent, ParentPos, isLeaf(), size(), &dst, dst.Parent, dst.ParentPos, dst.isLeaf(), dst.size(), count));
  ASSERT(Parent == dst.Parent && ParentPos +1 == dst.ParentPos);
  ASSERT(Size >= dst.Size);
  ASSERT(dst.size() + count <= BTREE_NODE_SIZE);
  ASSERT(count && count <= size());
  // Move dst.Content[0..dst.Size-1] to dst.Content[count..].
  memmove(dst.Content + count, dst.Content, dst.size() * sizeof *Content);
  // New delimiting key
  void*& parentkey = Parent->Content[ParentPos];
  dst.Content[count-1] = parentkey;
  void** target = Content + (size() - count);
  parentkey = *target;
  // Move Content[Size-count+1..Size-1] to dst.Content[0..count-2].
  memcpy(dst.Content, target + 1, (count - 1) * sizeof *Content);
  #ifdef DEBUG
  memset(target + 1, 0, (count - 1) * sizeof *Content);
  #endif
  // Update size
  Size -= count;
  dst.Size += count;
  // polymorphic call
  if (!isLeaf())
    ((Node*)this)->rebalanceL2R((Node&)dst, count);
}

void btree_base::Node::rebalanceL2R(Node& dst, size_t count)
{ DEBUGLOG2(("btree_base::Node(%p{%p,%u, %u})::rebalanceL2R(%p{%p,%u, %u}&, %u)\n", this, Parent, ParentPos, Size, &dst, dst.Parent, dst.ParentPos, dst.Size, count));
  //Leaf::rebalanceL2R(dst, count);
  // Move children from dst.SubNodes[0..dst.Size] to dst.SubNodes[count..].
  Leaf** where = dst.SubNodes + count;
  Leaf** end = dst.SubNodes + dst.Size + 1;
  memmove(where, dst.SubNodes, (char*)end - (char*)where);
  // Update uplinks
  while (where != end)
    (*where++)->ParentPos += count;
  // Move SubNodes[Size + 1..] to dst.SubNodes[0..count-1].
  size_t offset = Size + 1;
  where = dst.SubNodes;
  end = where + count;
  memcpy(where, SubNodes + offset, (char*)end - (char*)where);
  #ifdef DEBUG
  memset(SubNodes + offset, 0, (char*)end - (char*)where);
  #endif
  // Update uplinks.
  while (where != end)
  { Leaf* leaf(*where++);
    leaf->Parent = &dst;
    leaf->ParentPos -= offset;
  }
}

btree_base::Leaf* btree_base::Leaf::split(size_t count)
{ DEBUGLOG2(("btree_base::Leaf(%p{%p,%u, %u,%u})::split(%u)\n", this, Parent, ParentPos, isLeaf(), size(), count));
  ASSERT(size() > count);
  Leaf* dst(!isLeaf() ? new Node(count) : new Leaf(count));
  Size -= count;
  // Move Content[Size..Size+dst.Size-1] to dst.Content[0..dst.Size-1]
  memcpy(dst->Content, Content + size(), count * sizeof *Content);
  #ifdef DEBUG
  memset(Content + size(), 0, count * sizeof *Content);
  #endif
  // Split key := Content[Size-1]
  --Size;
  dst->Parent = Parent;
  dst->ParentPos = ParentPos + 1;
  Parent->insert(ParentPos) = Content[size()];
  Parent->SubNodes[dst->ParentPos] = dst;
  // polymorphic call
  if (!isLeaf())
    ((Node*)this)->split(*(Node*)dst);
  return dst;
}

void btree_base::Node::split(Node& dst)
{ DEBUGLOG2(("btree_base::Node(%p{%p,%u, %u})::split(%p{%p,%u, %u}&)\n", this, Parent, ParentPos, Size, &dst, dst.Parent, dst.ParentPos, dst.Size));
  //Leaf::split(dst);
  // Move SubNodes[Size+1..] to dst.SubNodes[0..dst.Size]
  size_t offset = Size + 1;
  Leaf** where = dst.SubNodes;
  Leaf** end = where + (dst.Size + 1);
  memcpy(where, SubNodes + offset, (char*)end - (char*)where);
  #ifdef DEBUG
  memset(SubNodes + offset, 0, (char*)end - (char*)where);
  #endif
  // Update uplinks
  while (where != end)
  { Leaf* leaf(*where++);
    leaf->Parent = &dst;
    leaf->ParentPos -= offset;
  }
}

void btree_base::Leaf::join(Leaf& src)
{ DEBUGLOG2(("btree_base::Leaf(%p{%p,%u, %u,%u})::join(%p{%p,%u, %u,%u}&)\n", this, Parent, ParentPos, isLeaf(), size(), &src, src.Parent, src.ParentPos, src.isLeaf(), src.size()));
  ASSERT(Parent == src.Parent && ParentPos + 1 == src.ParentPos);
  ASSERT(size() + src.size() < BTREE_NODE_SIZE); // +1 for the delimiter
  // Move delimiter to Content[Size]
  void** where = Content + size();
  *where = Parent->Content[ParentPos];
  // Move src.Content[0..src.Size-1] to Content[Size+1..]
  memcpy(where + 1, src.Content, src.size() * sizeof *Content);
  #ifdef DEBUG
  memset(src.Content, 0, src.size() * sizeof *Content);
  #endif
  // polymorphic call
  if (!isLeaf())
    ((Node*)this)->join((Node&)src);
  // New size
  Size += src.size() + 1;
  src.Size &= INT_MIN;
  // remove parent slot
  Parent->erase(ParentPos);
  if (!src.isLeaf())
    delete &src;
  else
    delete (Node*)&src;
}

void btree_base::Node::join(Node& src)
{ DEBUGLOG2(("btree_base::Node(%p{%p,%u, %u})::join(%p{%p,%u, %u}&)\n", this, Parent, ParentPos, Size, &src, src.Parent, src.ParentPos, src.Size));
  size_t offset = Size + 1;
  Leaf** where = SubNodes + offset;
  Leaf** end = where + (src.Size + 1);
  //Leaf::join(src);
  // Move src.SubLinks[0..count] to SubLinks[where..]
  memcpy(where, src.SubNodes, (char*)end - (char*)where);
  #ifdef DEBUG
  memset(src.SubNodes, 0, (char*)end - (char*)where);
  #endif
  // update uplinks
  while (where != end)
  { Leaf* leaf(*where++);
    leaf->Parent = this;
    leaf->ParentPos += offset;
  }
}

void btree_base::Leaf::destroy(void (*cleanup)(void*))
{ DEBUGLOG2(("btree_base::Leaf(%p{%p,%u, %u,%u})::destroy(%p)\n", this, Parent, ParentPos, isLeaf(), size(), cleanup));
  if (cleanup)
  { void** pp = Content;
    void** ppe = pp + size();
    while (pp != ppe)
      cleanup(*pp++);
  }
  // polymorphic call
  if (!isLeaf())
  { ((Node*)this)->destroy(cleanup);
    delete (Node*)this;
  } else
    delete this;
}

void btree_base::Node::destroy(void (*cleanup)(void*))
{ DEBUGLOG2(("btree_base::Node(%p{%p,%u, %u})::destroy(%p)\n", this, Parent, ParentPos, Size, cleanup));
  // destroy children
  Leaf** lp = SubNodes;
  Leaf** lpe = lp + Size;
  do
    (*lp)->destroy(cleanup);
  while (lp++ != lpe);
  //Leaf::destroy(cleanup);
}

#ifdef DEBUG
void btree_base::Leaf::check() const
{ // do not exceed the size of the array
  ASSERT(size() <= BTREE_NODE_SIZE);
  // no empty nodes
  ASSERT(size());
  // All nodes except for the root should be at least half filled
  //ASSERT(!Parent || Size >= (BTREE_NODE_SIZE >> 1));
  // polymorphic call
  if (!isLeaf())
    ((Node*)this)->check();
}

void btree_base::Node::check() const
{ //Leaf::check();
  bool hassubleafs;
  for (size_t n = 0; n <= Size; ++n)
  { Leaf* sublink = SubNodes[n];
    // nodes must not contain empty slots
    ASSERT(!!sublink);
    // Links must be valid
    PASSERT(sublink);
    // check parent link
    ASSERT(sublink->Parent == this);
    // check parent position
    ASSERT(sublink->ParentPos == n);
    // check that all children are of the same type
    if (!n)
      hassubleafs = sublink->isLeaf();
    else
      ASSERT(hassubleafs == sublink->isLeaf());
    // check the child itself
    sublink->check();
  }
}
#endif

/*btree_base::Link btree_base::Link::split(size_t count)
{ ASSERT(!!*this);
  Leaf* leaf(asLeaf());
  if (isLeaf())
  { Leaf* newleaf = new Leaf();
    newleaf->Size = count;
    leaf->split(*newleaf);
    leaf->Parent->SubNodes[newleaf->ParentPos] = newleaf;
    return newleaf;
  } else
  { Node* newnode = new Node();
    newnode->Size = count;
    asNode()->split(*newnode);
    leaf->Parent->SubNodes[newnode->ParentPos] = newnode;
    return newnode;
  }
}

void btree_base::Link::join(Link src)
{ if (isLeaf())
  { ASSERT(src.isLeaf());
    Leaf* leaf = src.asLeaf();
    asLeaf()->join(*leaf);
    delete leaf;
  } else
  { ASSERT(src.isNode());
    Node* node = src.asNode();
    asNode()->join(*node);
    delete node;
  }
}*/

/* Class btree_base::iterator */

bool btree_base::iterator::isbegin() const
{ if (!Ptr)
    return true;
  if (Pos || !Ptr->isLeaf())
    return false;
  const Leaf* node = Ptr;
  while (true)
  { if (!node->Parent)
      return true;
    if (node->ParentPos)
      return false;
    node = node->Parent;
  }
}

void btree_base::iterator::next()
{ DEBUGLOG2(("btree_base::iterator(%p{%p,%u})::next()\n", this, Ptr, Pos));
  ASSERT(!isend());
  ++Pos;
  while (!Ptr->isLeaf())
  { // if we are at a node, we need to enumerate the sub entries between #Pos and #Pos+1.
    Ptr = ((Node*)Ptr)->SubNodes[Pos];
    Pos = 0;
  }
  while (true)
  { const Leaf* leaf(Ptr);
    if (Pos != leaf->size())
      return;
    // end of this node
    if (!leaf->Parent)
      // end of list
      return;
    // leave
    Ptr = leaf->Parent;
    Pos = leaf->ParentPos;
  }
}

void btree_base::iterator::prev()
{ DEBUGLOG2(("btree_base::iterator(%p{%p,%u})::prev()\n", this, Ptr, Pos));
  ASSERT(!!Ptr);
  while (!Ptr->isLeaf())
  { // If we are at a node then the next item before Pos is in the sublist at Pos.
    Ptr = ((Node*)Ptr)->SubNodes[Pos];
    Pos = Ptr->size();
  }
  while (Pos == 0)
  { const Leaf* leaf(Ptr);
    ASSERT(leaf->Parent); // Must not decrement begin()
    Ptr = leaf->Parent;
    Pos = leaf->ParentPos;
  }
  // previous pos
  --Pos;
}


void btree_base::rebalanceOrSplit(iterator& where)
{ Leaf* leaf = where.Ptr;
  DEBUGLOG2(("btree_base(%p{%p})::rebalanceOrSplit({%p{%p,%u, %u,%u},%u}&)\n", this, Root, leaf, leaf->Parent, leaf->ParentPos, leaf->isLeaf(), leaf->size(), where.Pos));
  ASSERT(leaf->size() == BTREE_NODE_SIZE);

  // Try rebalance
  Node* parent = leaf->Parent;
  if (parent != NULL)
  { size_t parentpos = leaf->ParentPos;
    // not the root
    if (parentpos)
    { // Try rebalance with left sibling.
      Leaf* left(parent->SubNodes[parentpos - 1]);
      size_t count = (BTREE_NODE_SIZE - left->size()) >> (where.Pos < BTREE_NODE_SIZE);
      if (!count)
        count = 1;
      if (left->size() + count < BTREE_NODE_SIZE)
      { left->rebalanceR2L(*where.Ptr, count);
        ASSERT(BTREE_NODE_SIZE - leaf->size() == count);
        if (where.Pos < count)
        { // Mover insert position to left sibling.
          where.Ptr = left;
          where.Pos += left->size() + 1;
        }
        where.Pos -= count;
        ASSERT(where.Ptr->size() < BTREE_NODE_SIZE);
        return;
      }
    }

    if (parentpos < parent->size())
    { // Try rebalance with right sibling.
      Leaf* right(parent->SubNodes[parentpos + 1]);
      if (right->size() < BTREE_NODE_SIZE)
      { size_t count = (BTREE_NODE_SIZE - right->size()) >> (where.Pos > 0);
        if (!count)
          count = 1;
        if (right->size() + count < BTREE_NODE_SIZE)
        { where.Ptr->rebalanceL2R(*right, count);
          if (where.Pos > leaf->size())
          { // Move insert position to right sibling.
            where.Pos -= leaf->size() + 1;
            where.Ptr = right;
          }
          ASSERT(where.Ptr->size() < BTREE_NODE_SIZE);
          return;
        }
      }
    }

    // rebalance failed => ensure space in the parent node
    if (parent->size() == BTREE_NODE_SIZE)
    { iterator parentiter(parent, parentpos);
      rebalanceOrSplit(parentiter);
    }
  } else
  { // at the root => create a new level
    Node* newroot(new Node(0));
    newroot->Parent = NULL;
    newroot->ParentPos = 0;
    //newroot->Count = 0;
    newroot->SubNodes[0] = where.Ptr;
    leaf->Parent = newroot;
    leaf->ParentPos = 0;
    Root = newroot;
    // now we can split
  }

  // Split
  size_t splitcount;
  switch (where.Pos)
  {case 0:
    splitcount = leaf->size() - 1;
    break;
   case BTREE_NODE_SIZE:
    splitcount = 0;
    break;
   default:
    splitcount = leaf->size() >> 1;
  }

  Leaf* newlink(where.Ptr->split(splitcount));
  if (where.Pos > leaf->size())
  { where.Pos -= leaf->size() + 1;
    where.Ptr = newlink;
  }
}

bool btree_base::joinOrRebalance(iterator& where)
{ DEBUGLOG2(("btree_base(%p{%p})::joinOrRebalance({%p,%u}&)\n", this, Root, where.Ptr, where.Pos));
  Leaf* leaf(where.Ptr);
  Node* parent(leaf->Parent);
  if (leaf->ParentPos)
  { // try join with left sibling
    Leaf* left(parent->SubNodes[leaf->ParentPos - 1]);
    size_t size(left->size() + 1);
    if (size + leaf->size() <= BTREE_NODE_SIZE)
    { where.Pos += size;
      left->join(*where.Ptr);
      where.Ptr = left;
      return true;
    }
  }
  if (leaf->ParentPos < parent->size())
  { Leaf* right(parent->SubNodes[leaf->ParentPos + 1]);
    size_t size(right->size());
    if (leaf->size() + size <= BTREE_NODE_SIZE - 1)
    { where.Ptr->join(*right);
      return true;
    }
    // rebalance right?
    if (size > BTREE_NODE_SIZE >> 1 && (leaf->size() == 0 || where.Pos))
    { size_t count((size - leaf->size()) >> 1);
      if (count >= size)
        count = size - 1;
      leaf->rebalanceR2L(*right, count);
      goto done;
    }
  }
  if (leaf->ParentPos)
  { Leaf* left(parent->SubNodes[leaf->ParentPos - 1]);
    size_t size(left->size());
    // rebalance left?
    if (size > BTREE_NODE_SIZE >> 1 && (leaf->size() == 0 || where.Pos < leaf->size()))
    { size_t count((size - leaf->size()) >> 1);
      if (count >= leaf->size())
        count = leaf->size() - 1;
      left->rebalanceL2R(*leaf, count);
      where.Pos += count;
    }
  }
 done:
  return false;
}

void btree_base::autoshrink()
{ DEBUGLOG2(("btree_base(%p{%p})::autoshrink()\n", this, Root));
  if (Root->size())
    return;
  if (Root->isLeaf())
  { delete Root;
    Root = NULL;
  } else
  { Leaf* child(((Node*)Root)->SubNodes[0]);
    delete ((Node*)Root);
    child->Parent = NULL;
    Root = child;
  }
}

btree_base::iterator btree_base::begin() const
{ Leaf* leftmost = Root;
  if (leftmost)
  { while (!leftmost->isLeaf())
      leftmost = ((Node*)leftmost)->SubNodes[0];
  }
  return iterator(leftmost, 0);
}

void*& btree_base::insert(iterator& where)
{ DEBUGLOG2(("btree_base(%p{%p})::insert(&{%p,%u})\n", this, Root, where.Ptr, where.Pos));
  if (!where.Ptr)
  { // empty container => create root node
    Leaf* leaf = new Leaf(0);
    leaf->Parent = NULL;
    leaf->ParentPos = 0;
    Root = leaf;
    where.Ptr = leaf;
    where.Pos = 0;
  } else
  { if (!where.Ptr->isLeaf() && where.Pos < BTREE_NODE_SIZE)
    { where.prev();
      ++where.Pos;
      // where.Pos might point beyond the end of the current leaf here.
      ASSERT(where.Ptr->isLeaf());
    }
    if (where.Ptr->size() == BTREE_NODE_SIZE)
    { // leaf is full
      rebalanceOrSplit(where);
    }
  }
  ASSERT(where.Ptr->isLeaf());
  return where.Ptr->insert(where.Pos);
}

btree_base::findresult btree_base::find(void* key, int (*cmp)(void* key, void* elem)) const
{ DEBUGLOG2(("btree_base(%p{%p})::find(%p,)\n", this, Root, key));
  btree_base::findresult result(iterator(Root, 0));
  if (!!result.where.Ptr)
    while (true)
    { // locate key within *result.where.Ptr
      result.match = binary_search(key, result.where.Pos, result.where.Ptr->Content, result.where.Ptr->size(), cmp);
      if (result.match || result.where.Ptr->isLeaf())
        break;
      result.where.Ptr = ((Node*)result.where.Ptr)->SubNodes[result.where.Pos];
    }
  return result;
}

void* btree_base::erase(iterator& where)
{ DEBUGLOG2(("btree_base(%p{%p})::erase(&{%p,%u})\n", this, Root, where.Ptr, where.Pos));
  ASSERT(!where.isend());
  bool wasnode = false;
  void* delvalue = *where;
  if (!where.Ptr->isLeaf())
  { // Delete key in internal node => move the key before this one into the location
    // and delete the original location instead.
    wasnode = true;
    iterator oldpos(where);
    where.prev();
    ASSERT(where.Ptr->isLeaf());
    oldpos.Ptr->Content[oldpos.Pos] = *where;
  }
  // Delete key in leaf.
  where.Ptr->erase(where.Pos);
  // rebalance
  iterator iter(where);
  while (true)
  { if (iter.Ptr->Parent == NULL)
    { // Root
      autoshrink();
      if (!Root)
      { where.Ptr = NULL;
        where.Pos = 0;
        goto done;
      }
      break;
    }
    // Node has enough entries
    if (iter.Ptr->size() >= BTREE_NODE_SIZE / 2)
      break;
    bool joined = joinOrRebalance(iter);
    //iter.Ptr->check();
    if (iter.Ptr->isLeaf())
      where = iter;
    if (!joined)
      break;
    iter.Ptr = iter.Ptr->Parent;
  }
  // return value
  if (where.Pos == where.Ptr->size())
  { --where.Pos;
    where.next();
  }
  if (wasnode)
    where.next();
 done:
  return delvalue;
}
