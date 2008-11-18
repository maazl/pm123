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


#ifndef CPP_ALGORITHM_H
#define CPP_ALGORITHM_H

#include <cpp/container/vector.h>

#include <stdlib.h>

/* Algorithmns */

// binary search
bool binary_search_base(const vector_base& data, int (*fcmp)(const void* elem, const void* key),
  const void* key, size_t& pos);

// rotate pointer array in place
void rotate_array_base(void** begin, const size_t len, int shift);

template <class T>
inline void rotate_array(T** begin, const size_t len, int shift)
{ rotate_array_base((void**)begin, len, shift);
}

// Sort an array of pointers
void merge_sort_base(void** begin, void** end, int (*comp)(const void* l, const void* r));

template <class T>
inline void merge_sort(T** begin, T** end, int (*comp)(const T* l, const T* r))
{ merge_sort_base((void**)begin, (void**)end, (int (*)(const void* l, const void* r))comp);
}

#endif
