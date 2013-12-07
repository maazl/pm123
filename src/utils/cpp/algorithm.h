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

#ifdef __GNUC__
#define sort_comparer(t1,t2) int (*C)(const t1& k, const t2& e)
#else
// Watcom C++ and IBM C++ seem not to support template parameters that depend on previous template args.
// So we fall back to an unchecked pointer in this case.
// But keep in mind that the implementation will sadly fail on anything but the above.
#define sort_comparer(t1,t2) void* C
#endif

/* Algorithmns */

/// Binary search
/// @param key Key to search for.
/// @param pos [out] Location where the key is found or where it should have been inserted.
/// \a pos is in the domain [0, \a size].
/// @param data Pointer to first element of an array to search for.
/// The array must be ordered with respect to the comparer \a fcmp.
/// @param num Number of elements in the array.
/// @param size Size of the elements in the array.
/// @param fcmp Comparer to compare objects against keys.
/// The comparer should return < 0 if the element is less than key, > 0 if it is greater and
/// = 0 if they are equal.
/// The comparison could be asymmetric, i.e. the key type may be distinct from the object type.
/// @return \c true if the element at \a pos exactly matches \a key, \c false otherwise.
/// If \a pos returns \a size then the function always returns \c false.
/// @remarks The function makes no assumptions about the pointers to the elements and the pointer to the key.
/// They are simply passed to \a *fcmp.
/// If the array contains equal elements (with respect to \a *fcmp) it is not defined which element is returned.
bool binary_search( const void* key, size_t& pos,
  const void* data, size_t num, size_t size, int (*fcmp)(const void* key, const void* elem) );
/// Binary search
/// @param key Key to search for.
/// @param pos [out] Location where the key is found or where it should have been inserted.
/// \a pos is in the domain [0, \a num].
/// @param data Pointer to an array of pointers to elements to search for.
/// The array must be ordered with respect to the comparer \a fcmp.
/// @param num Number of elements in the array.
/// @param fcmp Comparer to compare objects against keys.
/// The comparer should return < 0 if the element is less than key, > 0 if it is greater and
/// = 0 if they are equal.
/// The comparison could be asymmetric, i.e. the key type may be distinct from the object type.
/// @return \c true if the element at \a pos exactly matches \a key, \c false otherwise.
/// If \a pos returns \a num then the function always returns \c false.
/// @remarks The function makes no assumptions about the pointers to the elements and the pointer to the key.
/// They are simply passed to \a *fcmp.
/// If the array contains equal elements (with respect to \a *fcmp) it is not defined which element is returned.
bool binary_search( const void* key, size_t& pos,
  const void*const* data, size_t num, int (*fcmp)(const void* key, const void* elem) );
/// Binary search
/// @param key Key to search for.
/// @param pos [out] Location where the key is found or where it should have been inserted.
/// \a pos is in the domain [0, \c data.size()].
/// @param data Array of pointers to elements to search for.
/// The array must be ordered with respect to the comparer \a fcmp.
/// @param fcmp Comparer to compare objects against keys.
/// The comparer should return < 0 if the element is less than key, > 0 if it is greater and
/// = 0 if they are equal.
/// The comparison could be asymmetric, i.e. the key type may be distinct from the object type.
/// @return \c true if the element at \a pos exactly matches \a key, \c false otherwise.
/// If \a pos returns \c data.size() then the function always returns \c false.
/// @remarks The function makes no assumptions about the pointers to the elements and the pointer to the key.
/// They are simply passed to \a *fcmp.
/// If the array contains equal elements (with respect to \a *fcmp) it is not defined which element is returned.
inline bool binary_search( const void* key, size_t& pos,
  const vector_base& data, int (*fcmp)(const void* key, const void* elem) )
{ return binary_search(key, pos, data.begin(), data.size(), fcmp); }
/// Generic binary search (strongly typed)
/// @tparam T element type
/// @tparam K key type
/// @param key Key to search for.
/// @param pos [out] Location where the key is found or where it should have been inserted.
/// \a pos is in the domain [0, \a num].
/// @param data Pointer to first element of an array to search for.
/// The array must be ordered with respect to the comparer \a fcmp.
/// @param num Number of elements in the array.
/// @param fcmp Comparer to compare objects against keys.
/// The comparer should return < 0 if the element is less than key, > 0 if it is greater and
/// = 0 if they are equal.
/// The comparison could be asymmetric, i.e. the key type may be distinct from the object type.
/// @return \c true if the element at \a pos exactly matches \a key, \c false otherwise.
/// If \a pos returns \a size then the function always returns \c false.
/// @remarks The function makes no assumptions about the pointers to the elements and the pointer to the key.
/// They are simply passed to \a *fcmp.
/// If the array contains equal elements (with respect to \a *fcmp) it is not defined which element is returned.
template <class T, class K>
inline bool binary_search(K* key, size_t& pos, T* data, size_t num, int (*fcmp)(K* key, T* elem))
{ return binary_search(key, pos, (const void**)data, num, sizeof(T), (int (*)(const void*, const void*))fcmp);
}
/// Generic binary search (strongly typed)
/// @tparam T element type
/// @tparam K key type
/// @param key Key to search for.
/// @param pos [out] Location where the key is found or where it should have been inserted.
/// \a pos is in the domain [0, \a num].
/// @param data Pointer to an array of pointers to elements to search for.
/// The array must be ordered with respect to the comparer \a fcmp.
/// @param num Number of elements in the array.
/// @param fcmp Comparer to compare objects against keys.
/// The comparer should return < 0 if the element is less than key, > 0 if it is greater and
/// = 0 if they are equal.
/// The comparison could be asymmetric, i.e. the key type may be distinct from the object type.
/// @return \c true if the element at \a pos exactly matches \a key, \c false otherwise.
/// If \a pos returns \a size then the function always returns \c false.
/// @remarks The function makes no assumptions about the pointers to the elements and the pointer to the key.
/// They are simply passed to \a *fcmp.
/// If the array contains equal elements (with respect to \a *fcmp) it is not defined which element is returned.
template <class T, class K>
inline bool binary_search(K* key, size_t& pos, T*const* data, size_t num, int (*fcmp)(K* key, T* elem))
{ return binary_search(key, pos, (const void**)data, num, (int (*)(const void*, const void*))fcmp);
}
/// Generic binary search (strongly typed)
/// @tparam T element type
/// @tparam K key type
/// @param key Key to search for.
/// @param pos [out] Location where the key is found or where it should have been inserted.
/// \a pos is in the domain [0, \c data.size()].
/// @param data Array of pointers to elements to search for.
/// The array must be ordered with respect to the comparer \a fcmp.
/// @param fcmp Comparer to compare objects against keys.
/// The comparer should return < 0 if the element is less than key, > 0 if it is greater and
/// = 0 if they are equal.
/// The comparison could be asymmetric, i.e. the key type may be distinct from the object type.
/// @return \c true if the element at \a pos exactly matches \a key, \c false otherwise.
/// If \a pos returns \c data.size() then the function always returns \c false.
/// @remarks The function makes no assumptions about the pointers to the elements and the pointer to the key.
/// They are simply passed to \a *fcmp.
/// If the array contains equal elements (with respect to \a *fcmp) it is not defined which element is returned.
template <class T, class K>
inline bool binary_search(K* key, size_t& pos, const vector<T>& data, int (*fcmp)(K* key, T* elem))
{ return binary_search(key, pos, data, data.size(), (int (*)(const void*, const void*))fcmp);
}

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
