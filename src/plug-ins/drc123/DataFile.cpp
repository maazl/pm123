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

#include <debuglog.h>


DataRowType::DataRowType(size_t size)
: sco_arr<double>(size)
{ // Set all numbers to quiet NaN.
  memset(begin(), 0xff, size * sizeof *begin());
}


bool DataFile::ParseHeaderField(const char* string)
{ return true;
}

bool DataFile::WriteHeaderFields(FILE* f)
{ return true;
}

DataFile::DataFile(unsigned cols)
: MaxColumns(cols)
{}

DataFile::~DataFile()
{}

void DataFile::reset(unsigned cols)
{ vector_own<DataRowType>::clear();
  Description.reset();
  MaxColumns = cols;
}

void DataFile::columns(unsigned cols)
{ ASSERT(!size());
  MaxColumns = cols;
}

bool DataFile::Load(const char* filename, bool nodata)
{ DEBUGLOG(("DataFile(%p)::Load(%s, %u)\n", this, filename, nodata));
  clear();

  FILE* f = fopen(filename, "r");
  if (!f)
    return false;
  xstringbuilder descr;
  char line[2048];
  bool isheader = true;
  double values[100];
  while (fgets(line, sizeof line, f))
  { switch (*line)
    {case '#':
     case ';':
     case '\'':
      if (line[1] == '#')
      { if (!ParseHeaderField(line+2))
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

        double* dp = values;
        const char* cp = line;
        int n = -1;
        while (sscanf(cp, "%lf%n", dp, &n) > 0)
        { ++dp;
          cp += n;
        }
        if (cp[strspn(cp, " \t\r\n")])
          goto error;
        // add row
        if (dp != values) // ignore empty lines
        { DataRowType* newrow = new DataRowType(dp - values);
          memcpy(newrow->get(), values, newrow->size() * sizeof(double));
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

  FILE* f = fopen(filename, "w");
  if (!f)
    return false;

  bool success = false;
  // write header
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
  foreach (DataRowType,*const*, rp, *this)
  { DataRowType& row = **rp;
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
