/*
 * Copyright 2010-2010 M.Müller
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

#ifndef ITERATOR_H_
#define ITERATOR_H_

#include "cpp/container/vector.h"

class iterator_base
{public:
  virtual ~iterator_base() {}
  virtual bool next() = 0;
  virtual void remove() = 0;
};
/** @brief Interface for container iteration
 *
 * Iterators in this model are \e not implemented by the container classes.
 * Instead a facade is used to adopt them.
 */
template <class T>
class iterator : virtual iterator_base
{public:
  virtual T* get() const = 0;
};

class vector_iterator_base : virtual iterator_base
{private:
  vector_base& Source;
  int          At;
 public:
               vector_iterator_base(vector_base& source);
  virtual bool next();
  virtual void remove();
};
template <class T>
class vector_iterator : vector_iterator_base, virtual iterator<T>
{public:
               vector_iterator();
  virtual T*   get() const;
};


#endif /* ITERATOR_H_ */
