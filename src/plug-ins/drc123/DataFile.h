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
struct DataRow : public sco_arr<float>
{ /// Construct a row with \a size columns.
                DataRow(size_t size);
  /// Get the col-th column of this row.
  /// @param col Column number counting from 0.
  /// The number might be higher then the length of this row.
  /// @return column value or NAN if no such column exists.
  float         operator[](size_t col) const    { return col >= size() ? NAN : begin()[col]; }
  /// Access the col-th column of this row.
  /// @param col Column number counting from 0.
  /// @return reference to the column value.
  /// @pre col < size()
  float&        operator[](size_t col)          { return sco_arr<float>::operator[](col); }
  /// Check whether this row has at least one non NAN value except for the key
  /// in the first column.
  bool          HasValues() const;
  /// Asymmetric comparer used to compare columns against keys.
  /// The key is assumed to be in the first column.
  static int    FrequencyComparer(const float& f, const DataRow& row);
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
  unsigned      MaxColumns;

 protected:
  /// Check whether a parameter line in the file is a specific parameter.
  /// @param line Parameter line from \c ParseHeaderField.
  /// @param name Parameter name, e.g. \c "Channels". The matching is case-insensitive.
  /// @return Pointer to the parameter value if match, \c NULL if no match.
  /// @example @code const char* value;
  /// if (!!(value = TryParam(line, "Channels"))
  ///   Channels = atoi(value);
  /// else if ((value = TryParam(line, ... @endcode
  static const char* TryParam(const char* line, const char* name);
  virtual bool  ParseHeaderField(const char* string);
  virtual bool  WriteHeaderFields(FILE* f);
 public:
                DataFile(unsigned cols = 0);
  /// Copy everything but the array content.
                DataFile(const DataFile& r);
  virtual       ~DataFile();
  unsigned      columns() const                 { return MaxColumns; }
  virtual void  reset(unsigned cols = 0);
  virtual void  columns(unsigned cols);
  void          swap(DataFile& r);
  virtual bool  Load(const char* filename = NULL, bool nodata = false);
  virtual bool  Save(const char* filename = NULL);
  bool          HasColumn(unsigned col) const;
  void          ClearColumn(unsigned col);
  //void          StoreValue(unsigned col, float key, float value);
  //void          StoreValue(unsigned col, float key, float value1, float value2);
  //float         Interpolate(float f, unsigned col) const;
};


class SharedDataFile : public DataFile, public Iref_count
{public:
  SharedDataFile(unsigned cols) : DataFile(cols) {}
  SharedDataFile(const SharedDataFile& r) : DataFile(r) {}
};


#endif // DATAFILE_H
