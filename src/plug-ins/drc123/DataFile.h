/*
 * Copyright 2012-2013 Marcel Mueller
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

#ifndef DATAFILE_H
#define DATAFILE_H

#include <cpp/xstring.h>
#include <cpp/mutex.h>
#include <cpp/smartptr.h>
#include <cpp/container/sorted_vector.h>
#include <math.h>
#include <stdio.h>


/// Row of measurement data
/// @remarks The single precision is sufficient for what we are going to do here
/// and conserves memory.
class DataRow
{ size_t        Size; // Logically const, but since we initialize it before constructor it has to be non-const to avoid the error.
 public:
  /// Construct a row with \a size columns.
  size_t        size() const                    { return Size; }
  float*        begin()                         { return (float*)(this+1); }
  const float*  begin() const                   { return (float*)(this+1); }
  float*        end()                           { return (float*)(this+1) + Size; }
  const float*  end() const                     { return (float*)(this+1) + Size; }
  /// Get the col-th column of this row.
  /// @param col Column number counting from 0.
  /// The number might be higher then the length of this row.
  /// @return column value or NAN if no such column exists.
  float         operator[](size_t col) const    { return col >= Size ? NAN : begin()[col]; }
  /// Access the col-th column of this row.
  /// @param col Column number counting from 0.
  /// @return reference to the column value.
  /// @pre col < size()
  float&        operator[](size_t col)          { ASSERT(col < Size); return begin()[col]; }
  /// Check whether this row has at least one non NAN value except for the key
  /// in the first column.
  bool          HasValues() const;
  /// Asymmetric comparer used to compare columns against keys.
  /// The key is assumed to be in the first column.
  static int    FrequencyComparer(const float& f, const DataRow& row);

 public: // allocators, they are replaced in plug-in builds.
  #if defined(__IBMCPP__) && defined(DEBUG_ALLOC)
  static void*  operator new(size_t s, const char*, size_t, size_t l);
  static void   operator delete(void* p, const char*, size_t) { delete[] (char*)p; }
  #else
  static void*  operator new(size_t s, size_t l);
  static void   operator delete(void* p)        { delete[] (char*)p; }
  #endif
 private: // Avoid array creation and ordinary new operator!
  #if defined(__IBMCPP__) && defined(__DEBUG_ALLOC__)
  static void*  operator new(size_t, const char*, size_t);
  static void*  operator new[](size_t, const char*, size_t);
  static void   operator delete[](void*, const char*, size_t);
  #else
  static void*  operator new(size_t);
  static void*  operator new[](size_t);
  static void   operator delete[](void*);
  #endif
};

class DataFile
: public sorted_vector_own<DataRow,float,&DataRow::FrequencyComparer>
, public ASyncAccess
{protected:
  typedef sorted_vector_own<DataRow,float,&DataRow::FrequencyComparer> base;
 public:
  /// File name of the data, optional, never \c NULL.
  /// Updated by \c Load and \c Save.
  xstring       FileName;
  /// Description of the file, optional, never \c NULL.
  xstring       Description;
 private:
  unsigned      Columns;

 protected:
  /// Check whether a parameter line in the file is a specific parameter.
  /// @param line [in/out] Parameter line from \c ParseHeaderField.
  /// Adjusted behind the = sign in case of a match, unchanged otherwise.
  /// @param name Parameter name, e.g. \c "Channels". The matching is case-insensitive.
  /// @return true: match, false: no match
  /// @example @code if (TryParam(line, "Channels"))
  ///   Channels = atoi(line);
  /// else if (TryParam(line, ... @endcode
  static bool   TryParam(const char*& line, const char* name);
  /// @brief Parse parameter in the header of the DataFile.
  /// @param string line of the file without the trailing ##.
  /// @return false := error.
  /// @details Header lines are lines starting with ##.
  /// They should continue with a unique prefix in the form "name=".
  /// The value after = is arbitrary.
  /// @p Derived classes might override this method to parse additional
  /// parameters in the header. If and only if the prefix is not their own
  /// they should pass the call to the base class.
  virtual bool  ParseHeaderField(const char* string);
  /// @brief Write header fields to a file.
  /// @param f Stream where to write the parameters to.
  /// @return false := error.
  /// @details Header lines written by this function should have the format
  /// "##name=value\\n". The name should only use letter or digits. The value after = is arbitrary.
  /// @p Derived classes might override this method to add additional
  /// parameters to the header. They must always call WriteHeaderFields of their base class.
  /// A class should not write header fields that cannot be parsed by ParseHeaderField of the class.
  virtual bool  WriteHeaderFields(FILE* f);
 public:
                DataFile(unsigned cols);
  /// Copy everything but the array content.
                DataFile(const DataFile& r);
  virtual       ~DataFile();
  unsigned      columns() const                 { return Columns; }
  virtual void  reset(unsigned cols);
  void          swap(DataFile& r);
  DataRow*&     append();
  virtual bool  Load(const char* filename = NULL, bool nodata = false);
  virtual bool  Save(const char* filename = NULL);
  bool          HasColumn(unsigned col) const;
  void          ClearColumn(unsigned col);
};


class SharedDataFile : public DataFile, public Iref_count
{public:
  SharedDataFile(unsigned cols) : DataFile(cols) {}
  SharedDataFile(const SharedDataFile& r) : DataFile(r) {}
};


#endif // DATAFILE_H
