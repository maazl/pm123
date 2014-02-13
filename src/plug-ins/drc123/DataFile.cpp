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


#include "DataFile.h"

#include <stdio.h>
#include <errno.h>
#include <cpp/algorithm.h>

#include <debuglog.h>


DataRow::DataRow(size_t size)
: sco_arr<float>(size)
{ // Set all numbers to quiet NaN.
  memset(begin(), 0xff, size * sizeof *begin());
}

int DataRow::FrequencyComparer(const float& f, const DataRow& row)
{ float f2 = row[0];
  if (f < f2)
    return -1;
  if (f > f2)
    return 1;
  return 0;
}

bool DataRow::HasValues() const
{ if (size() <= 1)
    return false;
  const float* sp = begin();
  const float* spe = end();
  while (++sp != spe)
    if (!isnan(*sp))
      return true;
  return false;
}

const char* DataFile::TryParam(const char* line, const char* name)
{ size_t len = strlen(name);
  if (strnicmp(line, name, len) == 0 && line[len] == '=')
    return line + len + 1;
  return NULL;
}

bool DataFile::ParseHeaderField(const char* string)
{ return true;
}

bool DataFile::WriteHeaderFields(FILE* f)
{ return true;
}

DataFile::DataFile(unsigned cols)
: FileName(xstring::empty)
, Description(xstring::empty)
, MaxColumns(cols)
{}
DataFile::DataFile(const DataFile& r)
: FileName(r.FileName)
, Description(r.Description)
, MaxColumns(r.MaxColumns)
{}

DataFile::~DataFile()
{}

void DataFile::reset(unsigned cols)
{ base::clear();
  Description = xstring::empty;
  MaxColumns = cols;
}

void DataFile::columns(unsigned cols)
{ ASSERT(!size());
  MaxColumns = cols;
}

void DataFile::swap(DataFile& r)
{ base::swap(r);
  FileName.swap(r.FileName);
  Description.swap(r.Description);
  ::swap(MaxColumns, r.MaxColumns);
}

bool DataFile::Load(const char* filename, bool nodata)
{ DEBUGLOG(("DataFile(%p{%s})::Load(%s, %u)\n", this, FileName.cdata(), filename, nodata));
  clear();
  if (filename == NULL)
  { if (!FileName.length())
    { errno = EINVAL;
      return false;
    }
    filename = FileName;
  }
  FILE* f = fopen(filename, "r");
  if (!f)
    return false;
  xstringbuilder descr;
  char line[2048];
  bool isheader = true;
  float values[100];
  while (fgets(line, sizeof line, f))
  { switch (*line)
    {case '#':
     case ';':
     case '\'':
      if (line[1] == '#')
      { { // strip trailling \n
          size_t len = strlen(line + 2) + 1;
          if (line[len] == '\n')
            line[len] = 0;
        }
        if (!ParseHeaderField(line+2))
        {error:
          errno = ERANGE;
          fclose(f);
          return false;
        }
      } else if (isheader)
        descr.append(line + 1 + strspn(line+1, " \t\r\n"));
      break;
     default:
      { isheader = false;
        if (nodata)
          goto nodata;

        float* dp = values;
        const char* cp = line;
        int n = -1;
        while (sscanf(cp, "%f%n", dp, &n) > 0)
        { ++dp;
          cp += n;
        }
        if (cp[strspn(cp, " \t\r\n")])
          goto error;
        // add row
        if (dp != values) // ignore empty lines
        { DataRow* newrow = new DataRow(dp - values);
          memcpy(newrow->get(), values, newrow->size() * sizeof(*values));
          append() = newrow;
          // adjust MaxColumns
          if (MaxColumns < newrow->size())
            MaxColumns = newrow->size();
        }
      }
     case '\r':
     case '\n':
     case 0:
      break; // ignore empty lines
    }
  }
 nodata:
  FileName = filename;
  Description = descr.get();
  fclose(f);
  return true;
}

bool DataFile::Save(const char* filename)
{ DEBUGLOG(("DataFile(%p{%s, %u,%u})::Save(%s)\n", this,
    Description.cdata(), size(), MaxColumns, filename));

  if (filename == NULL)
    filename = FileName;

  FILE* f = fopen(filename, "w");
  if (!f)
    return false;

  bool success = false;
  // write header
  fputs("## Generated by DRC123 V1.0\n", f);
  const char* cp = Description;
  if (!WriteHeaderFields(f))
    goto end;
  for(;;)
  { const char* cp2 = strchr(cp, '\n');
    if (!cp2)
      break;
    if ( fputs("# ", f) == EOF
      || fwrite(cp, cp2-cp+1, 1, f) != 1 )
      goto end;
    cp = cp2+1;
  }
  if ( fputs("# ", f) == EOF
    || fputs(cp,   f) == EOF
    || fputc('\n', f) == EOF )
    goto end;

  // write data
  foreach (DataRow,*const*, rp, *this)
  { DataRow& row = **rp;
    for (unsigned i = 0; i < row.size(); ++i)
    { if ( fprintf(f, "%f", row[i]) < 0
        || fputc(i == row.size()-1 ? '\n' : '\t', f) == EOF )
        goto end;
    }
  }

  FileName = filename;
  success = true;
 end:
  fclose(f);
  return success;
}

bool DataFile::HasColumn(unsigned col) const
{ if (col >= MaxColumns)
    return false;
  foreach (DataRow,*const*, rp, *this)
    if (!isnan((**rp)[col]))
      return true;
  return false;
}

void DataFile::ClearColumn(unsigned col)
{
  base newdata(size());
  foreach (DataRow,**, sp, *this)
  { DataRow* rp = *sp;
    (*rp)[col] = NAN;
    // keep only data lines with values
    if (rp->HasValues())
    { newdata.append() = rp;
      *sp = NULL;
    }
  }
  newdata.swap(*this);
}

static int dlcmp(const float& k, const DataRow& row)
{ float value = row[0];
  if (k > value * 1.0001)
    return 1;
  if (k < value / 1.0001)
    return -1;
  return 0;
}

/*void DataFile::StoreValue(unsigned col, float f, float value)
{ if (isnan(value))
    return;
  size_t pos;
  DataRow* rp;
  if (!binary_search<DataRow,const float>(f, pos, *this, &dlcmp))
  { rp = insert(pos) = new DataRow(MaxColumns);
    (*rp)[0] = f;
  } else
    rp = (*this)[pos];
  (*rp)[col] = value;
}
void DataFile::StoreValue(unsigned col, float f, float mag, float delay)
{ if (isnan(mag) && isnan(delay))
    return;
  size_t pos;
  DataRow* rp;
  if (!binary_search<DataRow,const float>(f, pos, *this, &dlcmp))
  { rp = insert(pos) = new DataRow(MaxColumns);
    (*rp)[0] = f;
  } else
    rp = (*this)[pos];
  (*rp)[col] = mag;
  (*rp)[col + 1] = delay;
}

float DataFile::Interpolate(float f, unsigned col) const
{
  // search for frequency f
  size_t pos;
  if (locate(f, pos))
  { // exact match
    register float value = (*(*this)[pos])[col];
    // but if nan then treat as inexact match
    if (!isnan(value))
      return value;
  }
  // left bound
  DataRow*const* pe = begin();
  DataRow*const* lp = pe + pos;
  float lv = NAN;
  do
  { if (lp == pe) // at the start
      break;
    lv = (**--lp)[col];
  } while (isnan(lv));
  // right bound
  DataRow*const* rp = pe + pos;
  float rv = NAN;
  pe = end();
  for (;;)
  { if (rp == pe) // behind the end
      break;
    rv = (**rp)[col];
    if (!isnan(rv))
      break;
    ++rp;
  }

  // interpolate
  if (isnan(rv))
    return lv; // use extrapolation
  else if(isnan(lv))
    return rv; // use extrapolation
  else
  { register float lf = (**lp)[0];
    register float rf = (**rp)[0];
    return (lv * (rf-f) + rv * (f-lf)) / (rf - lf);
  }
}*/

const DataRow*const* DataFile::InterpolationIterator::SkipNAN(const DataRow*const* ptr)
{ for(; ptr < End; ++ptr)
  { if (!isnan((**ptr)[Column]))
      return ptr;
  }
  return NULL;
}

DataFile::InterpolationIterator::InterpolationIterator(const DataFile& data, unsigned col)
: End(data.end())
, Column(col)
  // place the first two non-NAN rows in Left, Right
, Left(SkipNAN(data.begin()))
, Right(SkipNAN(Left+1))
{ ASSERT(Left);
}

float DataFile::InterpolationIterator::FetchNext(float key)
{ float lk = (**Left)[0];
  // !extrapolate before the start?
  if (key > lk)
  { // seek for next row with row[0] > key
    // !extrapolate behind the end?
    while (Right)
    { float rk = (**Right)[0];
      if (key < rk)
      { // interpolate
        register float lv = (**Left)[Column];
        register float rv = (**Right)[Column];
        return (lv * (rk-key) + rv * (key-lk)) / (rk-lk);
      }
      Right = SkipNAN((Left = Right) + 1);
      lk = rk;
    }
  }
  return (**Left)[Column];
}


DataFile::StoreIterator::StoreIterator(DataFile& data, unsigned col, unsigned discard, float accuracy)
: Target(data)
, Column(col)
, Discard(discard)
, Accuracy(1. + accuracy)
, Current(0)
{}

DataFile::StoreIterator::~StoreIterator()
{
  if (Discard)
    while (Current < Target.size())
      DiscardColumns();
}

void DataFile::StoreIterator::DiscardColumns()
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

DataRow& DataFile::StoreIterator::RowAtKey(float key)
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
  DataRow& row = *(Target.insert(Current++) = new DataRow(Target.MaxColumns));
  row[0] = key;
  return row;
}

void DataFile::StoreIterator::Store(float key, float value1, float value2)
{ DataRow& row = RowAtKey(key);
  row[Column] = value1;
  row[Column+1] = value2;
}
