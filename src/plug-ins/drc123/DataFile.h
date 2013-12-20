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


struct DataRowType : public sco_arr<double>
{               DataRowType(size_t size);
  double        operator[](size_t col) const    { return col >= size() ? NAN : begin()[col]; }
  double&       operator[](size_t col)          { return sco_arr<double>::operator[](col); }
  static int    FrequencyComparer(const double& f, const DataRowType& row);
};

class DataFile
: public sorted_vector_own<DataRowType,double,&DataRowType::FrequencyComparer>
, public ASyncAccess
{protected:
  typedef sorted_vector_own<DataRowType,double,&DataRowType::FrequencyComparer> base;
 public:
  class Enumerator
  { DataRowType*const* Current;
    DataRowType*const* const End;
   public:
    Enumerator(const DataFile& data) : Current(data.begin()), End(data.end()) {}
    DataRowType* operator*() const              { ASSERT(Current != End); return *Current; }
    DataRowType* operator->() const             { ASSERT(Current != End); return *Current; }
    bool        Next()                          { return Current != End && ++Current != End; }
    bool        isEnd() const                   { return Current == End; }
    int         comparef(double f) const;
  };
 public:
  xstring       FileName;
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
  double        Interpolate(double f, unsigned col);
};

#endif // DATAFILE_H
