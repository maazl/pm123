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

#include "Iterators.h"
#include "DataFile.h"



ReadIterator::ReadIterator()
{ Reset();
}


bool FileIterator::Reset(const DataFile& data)
{ End = data.end();
  Current = data.begin();
  ReadIterator::Reset();
  FetchNext();
  return !isnan(Right.Y);
}


void FileIterator::FetchNext()
{ Left = Right;
  while (Current < End)
  { Right.X = (**Current)[0];
    Right.Y = (**Current)[Column];
    ++Current;
    if (!isnan(Right.Y))
      return;
  }
  Right.X = Right.Y = NAN;
}

double ReadIterator::ScaleResult(double value) const
{ return value;
}


void InterpolationIterator::ReadNext(double key)
{ // seek for next row with row[0] > key
  while (Right)
  { if (key < Right.X)
    { // !extrapolate before the start?
      if (!Left)
        Value = Right.Y;
      else // interpolate
        Value = (Left.Y * (Right.X-key) + Right.Y * (key-Left.X)) / (Right.X-Left.X);
      return;
    }
    FetchNext();
  }
  // !extrapolate behind the end
  if (Left)
    Value = Left.Y;
}

bool AggregateIterator::Reset(const DataFile& data)
{ MinValue = MaxValue = NAN;
  LastX = -1E-6; // Residual value to avoid 0/0 at ReadNext(0)"
  return FileIterator::Reset(data);
}

void AggregateIterator::ReadNext(double x)
{ while (Right && x > Right.X)
  { Aggregate(LastX, Right.X);
    LastX = Right.X;
    // next value
    FetchNext();
  }
  Aggregate(LastX, x);
  LastX = x;
}


void AverageIterator::Aggregate(double from, double to)
{ if (Left.Y < MinValue)
    MinValue = Left.Y;
  if (Right.Y < MinValue)
    MinValue = Right.Y;
  if (Left.Y > MaxValue)
    MaxValue = Left.Y;
  if (Right.Y > MaxValue)
    MaxValue = Right.Y;
  double avg;
  if (!Right)
  { // beyond the end
    if (!Left)
    { Value = NAN;
      return;
    }
    avg = Left.Y;
  } else if (!Left)
  { // before start
    avg = Right.Y;
  } else
  { // intermediate
    avg = (from + to) / 2;
    avg = ((avg - Left.X) * Right.Y + (Right.X - avg) * Left.Y)
        / (Right.X - Left.X);
  }
  Value += avg * (to - from);
}

void AverageIterator::ReadNext(double key)
{ Value = 0;
  MinValue = +INFINITY;
  MaxValue = -INFINITY;
  double lx = LastX;
  AggregateIterator::ReadNext(key);
  Value /= (key - lx);
}


bool DelayAverageIterator::Reset(const DataFile& data)
{ LastFX = 0;
  return AverageIterator::Reset(data);
}

void DelayAverageIterator::FetchNext()
{ FileIterator::FetchNext();
  double x = Right.X;
  Right.X = (LastFX + Right.X) / 2;
  LastFX = x;
}


StoreIterator::StoreIterator(DataFile& data, unsigned col, unsigned discard, float accuracy)
: Target(data)
, Column(col)
, Discard(discard)
, Accuracy(1. + accuracy)
, Current(0)
{}

StoreIterator::~StoreIterator()
{
  if (Discard)
    while (Current < Target.size())
      DiscardColumns();
}

void StoreIterator::DiscardColumns()
{ DataRow& row = *Target[Current];
  unsigned col = Column + Discard;
  do
    row[--col] = NAN;
  while (col != Column);
  if (row.HasValues())
    ++Current;
  else
    Target.erase(Current);
}

DataRow& StoreIterator::RowAtKey(float key)
{
  while (Current < Target.size())
  { DataRow& row = *Target[Current];
    if (key <= row[0] * Accuracy)
    { if (key * Accuracy < row[0])
        // Value before Current => insert
        break;
      else
      { // hit
        ++Current;
        return row;
      }
    }
    // key larger than Current, try at later location
    if (Discard)
      DiscardColumns();
    else
      ++Current;
  }
  // insert new row at Current
  DataRow& row = *(Target.insert(Current++) = new DataRow(Target.columns()));
  row[0] = key;
  return row;
}

void StoreIterator::Store(float key, float value1, float value2)
{ DataRow& row = RowAtKey(key);
  row[Column] = value1;
  row[Column+1] = value2;
}
