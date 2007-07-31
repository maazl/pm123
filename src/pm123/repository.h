/* * Copyright 2007-2007 Marcel Mueller
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


#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <stdlib.h>
#include <cpp/Mutex.h>
#include <utilfct.h>


/* Interface fpr string compareable objects.
 */
struct IStringComparable
{ virtual int CompareTo(const char* str) const = 0;
};


/* Collection class to ensure the uniqueness of objects identified by a string.
 * The string is intrusively stored in the object and accessed by the interface IStringCompareable.
 * The class is not thread safe.
 */
class StringRepository : public Mutex
{private:
  IStringComparable** Data;
  size_t              Size;
  size_t              Count;

 private:
  // get lower_bound. returns true on exact match 
  bool                Search(const char* str, size_t& pos) const;
 public:
  // Create a repository of a given size.
  // The size must not be zero
  StringRepository(size_t initial = 16);
  // Serach for an entry for str.
  // Returns NULL of no entry exists. 
  IStringComparable*  Find(const char* str) const;
  // Get an entry for str.
  // If the entry does not exist a new slot is created at the right position of the collection.
  // In this case the returned pointer reference is NULL. You must store your data pointer there.
  IStringComparable*& Get(const char* str);
  // Remove entry from repository.
  // The function returns the removed element if any.
  // It does not delete anything.
  IStringComparable*  Remove(const char* str);
};


#endif

