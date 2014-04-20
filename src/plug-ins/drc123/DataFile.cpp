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


#if defined(__IBMCPP__) && defined(__DEBUG_ALLOC__)
void* DataRow::operator new(size_t s, const char*, size_t, size_t l)
#else
void* DataRow::operator new(size_t s, size_t l)
#endif
{ DataRow* that = (DataRow*)new char[s + l * sizeof(float)];
  // Dirty early construction
  that->Size = l;
  memset(that + 1, 0xff, l * sizeof(float));
  return that;
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


bool DataFile::TryParam(const char*& line, const char* name)
{ size_t len = strlen(name);
  if (strnicmp(line, name, len) != 0 || line[len] != '=')
    return false;
  line += len + 1;
  return true;
}

bool DataFile::ParseHeaderField(const char* string)
{ size_t lines;
  if ( TryParam(string, "Lines")
    && sscanf(string, "%u", &lines)
    && lines > capacity() )
    reserve(lines);
  return true;
}

bool DataFile::WriteHeaderFields(FILE* f)
{ return fprintf(f, "##Lines=%u\n", size()) > 0;
}

DataFile::DataFile(unsigned cols)
: FileName(xstring::empty)
, Description(xstring::empty)
, Columns(cols)
{}
DataFile::DataFile(const DataFile& r)
: FileName(r.FileName)
, Description(r.Description)
, Columns(r.Columns)
{}

DataFile::~DataFile()
{}

void DataFile::reset(unsigned cols)
{ DEBUGLOG(("DataFile(%p{%s})::reset(%u)\n", this, FileName.cdata(), cols));
  base::clear();
  Description = xstring::empty;
  Columns = cols;
}

void DataFile::swap(DataFile& r)
{ DEBUGLOG(("DataFile(%p{%s})::swap(&%p{%s})\n", this, FileName.cdata(), &r, r.FileName.cdata()));
  base::swap(r);
  FileName.swap(r.FileName);
  Description.swap(r.Description);
  ::swap(Columns, r.Columns);
}

DataRow*& DataFile::append()
{ // optimization: set reasonable start capacity
  // but only on demand
  if (!capacity())
    reserve(65536);
  return sorted_vector_own<DataRow,float,&DataRow::FrequencyComparer>::append();
}

bool DataFile::Load(const char* filename, bool nodata)
{ DEBUGLOG(("DataFile(%p{%s})::Load(%s, %u)\n", this, FileName.cdata(), filename, nodata));
  ASSERT(Columns);
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
  sco_ptr<DataRow> row;
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

        if (!row)
          row = new (Columns)DataRow;
        float* dp = row->begin();
        float* ep = row->end();
        const char* cp = line;
        int n = -1;
        while (sscanf(cp, "%f%n", dp, &n) > 0)
        { cp += n;
          if (++dp == ep)
            break;
        }
        if (cp[strspn(cp, " \t\r\n")])
          goto error;
        // add row
        if (dp != row->begin()) // ignore empty lines
          append() = row.detach();
        break;
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
    Description.cdata(), size(), Columns, filename));

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
{ if (col >= Columns)
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
