/*
 * Copyright 2013-2014 Marcel Mueller
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

#ifndef ITERATORS_H_
#define ITERATORS_H_

#include "DataFile.h"
#include <cpp/cpputil.h>
#include <stdlib.h>
#include <math.h>


/// @brief Abstract input iterator.
/// @details The class forms a function <i>f</i>(<i>x</i>) that can be retrieved only in forward order,
/// i.e. any subsequent <i>x</i> must be greater than or equal to the last one.
class ReadIterator
{protected:
  struct Point
  { double             X;
    double             Y;
    operator unspecified_bool_type()                { return (unspecified_bool_type)!isnan(Y); }
  };
 protected:
  /// Left bound = first non NAN point less than or equal to the last key returned
  /// or NAN if before the start.
  Point                Left;
  /// Next non NAN point after \c Left. Might be \c NAN if after the end.
  Point                Right;
  /// Current value, initially \c NAN, filled by \c ReadNext.
  double               Value;
 protected:
  /// Create initial iterator.
                       ReadIterator();
  /// Start iteration.
  /// @return true if the column contains at least one non NAN value.
  void                 Reset()                      { Left.X = Left.Y = Right.X = Right.Y = Value = NAN; }
  /// Move Right to Left and fetch the next point into Left.
  virtual void         FetchNext() = 0;
  virtual double       ScaleResult(double value) const;
 public:
  virtual              ~ReadIterator() {}
  /// Fetch the next linear interpolation value at \a key.
  /// @param key Retrieve the interpolating function value at \a key.
  /// If key is outside the domain of the data source constant extrapolation is used,
  /// i.e. the first or the last value respectively is returned.
  /// @pre key must be greater or equal to any value previously passed to \c Next
  /// after the last call to \c Start.
  /// @post this->Value contains the result.
  virtual void         ReadNext(double x) = 0;
  /// Retrieve current value, initially \c NAN
  double               GetValue() const             { return ScaleResult(Value); }
};

/// @brief Abstract input iterator over a column of a \c DataFile instance.
/// @details The class forms a function <i>f</i>(<i>x</i>) that can be retrieved only in forward order,
/// i.e. any subsequent <i>x</i> must be greater than or equal to the last one.
/// @remarks The iterator uses the forward only restriction to turn the operation
/// from O(n log(n)) into O(n).
class FileIterator : public ReadIterator
{protected:
  /// Column to interpolate.
  const unsigned       Column;
  /// End of data.
  const DataRow*const* End;
  /// Current data pointer.
  const DataRow*const* Current;
 protected:
  /// Setup iteration for a column.
  /// @param col Column in the data source.
  FileIterator(unsigned col)                        : Column(col), End(NULL), Current(NULL) {}
  /// Move Right to Left and fetch the next point into Left.
  virtual void         FetchNext();
 public:
  /// Start iteration over a DataFile.
  /// @param data Data source.
  /// @return true if the column contains at least one non NAN value.
  /// @pre The rows in the data source must be strictly ordered by the key in the first column.
  virtual bool         Reset(const DataFile& data);
};

/// @brief Input iterator over a linear interpolating function of a column of a \c DataFile instance.
/// @remarks The iterator uses the forward only restriction to turn the interpolation operation
/// from O(n log(n)) into O(n).
class InterpolationIterator : public FileIterator
{public:
  /// Create interpolating function over a column of data.
  /// @param col Column in the data source.
  InterpolationIterator(unsigned col) : FileIterator(col) {}
  /// Fetch the next linear interpolation value at \a key.
  /// @param key Retrieve the interpolating function value at \a key.
  /// If key is outside the domain of the data source constant extrapolation is used,
  /// i.e. the first or the last value respectively is returned.
  /// @pre key must be greater or equal to any value previously passed to \c Next
  /// after the last call to \c Start.
  /// @post this->Value contains the result.
  virtual void         ReadNext(double x);
};

/// @brief Abstract input iterator over a column of a \c DataFile instance
/// that performs some aggregation.
class AggregateIterator : public FileIterator
{protected:
  double               MinValue;
  double               MaxValue;
  /// Key passed to \c ReadNext the last time.
  double               LastX;
 protected:
  /// @brief Aggregate result - to be implemented by subclass.
  /// @param from Key from where to aggregate
  /// @param to Key up to where to aggregate
  /// @pre @code (!Left || ***Left <= \a from) && \a from <= \a to && (!Right || \a to <= ***Right) @endcode
  /// @post The function must assign \c this->Value.
  /// @details If Left == NULL the range [from,to] is before the start,
  /// if Right == NULL it is after the end, if both are NULL there is no data in the iteration
  /// and otherwise [from, to is in the range [***Left,***Right].
  virtual void         Aggregate(double from, double to) = 0;
                       AggregateIterator(unsigned col) : FileIterator(col), MinValue(NAN), MaxValue(NAN), LastX(0.) {}
 public:
  /// Start iteration over a DataFile.
  /// @param data Data source.
  /// @return true if the column contains at least one non NAN value.
  /// @pre The rows in the data source must be strictly ordered by the key in the first column.
  virtual bool         Reset(const DataFile& data);
  /// Fetch the next linear value at \a key.
  /// @param key Retrieve the average function value at \a key.
  /// If key is outside the domain of the data source constant extrapolation is used,
  /// i.e. the first or the last value respectively is returned.
  /// @pre key must be greater or equal to any value previously passed to \c Next
  /// after the last call to \c Start.
  /// @post this->Value contains the result.
  /// @remarks Although \c AggregateIterator implements this function
  /// it is usually necessary to override this method to initialize \c Value
  /// before the next aggregation.
  virtual void         ReadNext(double x);
  /// Get minimum value in the aggregation interval.
  double               GetMinValue() const          { return ScaleResult(MinValue); }
  /// Get maximum value in the aggregation interval.
  double               GetMaxValue() const          { return ScaleResult(MaxValue); }
};


class AverageIterator : public AggregateIterator
{protected:
  virtual void         Aggregate(double from, double to);
 public:
                       AverageIterator(unsigned col) : AggregateIterator(col) {}
  virtual void         ReadNext(double x);
};

class DelayIterator : public AverageIterator
{protected:
  virtual void         Aggregate(double from, double to);
 public:
                       DelayIterator(unsigned col) : AverageIterator(col) {}
};

class DelayAverageIterator : public AverageIterator
{ double               LastFX;
 public:
                       DelayAverageIterator(unsigned col) : AverageIterator(col), LastFX(0.) {}
  virtual bool         Reset(const DataFile& data);
  virtual void         FetchNext();
};


/// @brief Output iterator to update column values in sorted order.
/// @remarks The iterator uses the forward only restriction to turn the interpolation operation
/// from O(n log(n)) into O(n).
class StoreIterator
{ DataFile&            Target;
  const unsigned       Column;
  const unsigned       Discard;
  const float          Accuracy;
  /// Points to the line in Target that has \e not yet been used to store a value so far.
  unsigned             Current;
 protected:
  /// Discard target columns of the current row.
  /// @pre Discard > 0
  /// @post Current points to the next row.
  void                 DiscardColumns();
  /// @brief Ensure next row at \a key.
  /// @details This either moves Current forward to match key or inserts
  /// a new row if no matching row exists.
  /// @return Existing or new row.
  /// @post Current points to the returned row + 1.
  DataRow&             RowAtKey(float key);
 public:
  /// @brief Create StoreIterator to update a column.
  /// @param data DataFile to update. The rows in \a data must not be modified
  /// by anything else but this class during iteration.
  /// @param col First column to update in the DataFile.
  /// @param discard If this in non-zero all values of columns [col,col+discard]
  /// are overwritten not only those with keys passed to Store().
  /// This is faster than calling DataFile::ClearColumn first.
  /// @param accuracy If this parameter is non-zero (default) rows with keys that match
  /// in the interval [key/(1+accuracy),key*(1+accuracy)] are reused rather than
  /// inserting new rows.
  StoreIterator(DataFile& data, unsigned col, unsigned discard = 0, float accuracy = 2E-6);
  ~StoreIterator();
  /// @brief Store a new value.
  /// @param Key of the row.
  /// @param value Value to store in the target column.
  /// @pre \a key must be larger than any key passed to this iterator instance before.
  /// @remarks The function overwrites existing values in the key interval
  /// [key/(1+Accuracy),key*(1+Accuracy)] or creates a new row if there is no matching one.
  void                 Store(float key, float value)   { RowAtKey(key)[Column] = value; }
  /// @brief Store two new values.
  /// @param Key of the row.
  /// @param value1 First value to store in the target column.
  /// @param value2 Second value to store in the target column + 1.
  /// @pre \a key must be larger than any key passed to this iterator instance before.
  /// @remarks The function overwrites existing values in the key interval
  /// [key/(1+Accuracy),key*(1+Accuracy)] or creates a new row if there is no matching one.
  void                 Store(float key, float value1, float value2);
};


#endif // ITERATORS_H_
