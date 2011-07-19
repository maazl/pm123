/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
 *
 * Copyright 2006 Dmitry A.Steklenev <glass@ptv.ru>
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

#ifndef ID3V1_TAG_H
#define ID3V1_TAG_H

#include <xio.h>
#include <string.h>


class DSTRING;

enum ID3V1_TAG_COMP
{ ID3V1_TITLE = 1,
  ID3V1_ARTIST,
  ID3V1_ALBUM,
  ID3V1_YEAR,
  ID3V1_COMMENT,
  ID3V1_TRACK,
  ID3V1_GENRE
};

/** ID3V1 tag. Value type.
 * Even if it does not look like that, this structure is a POD type.
 */
struct ID3V1_TAG
{private:
  char id     [ 3];
  char title  [30];
  char artist [30];
  char album  [30];
  char year   [ 4];
  char comment[28];
  unsigned char empty;
  unsigned char track;
  unsigned char genre;

 public:
  static const ID3V1_TAG CleanTag;

  /// Check for semantic equality (does not check validity)
  bool Equals(const ID3V1_TAG& r) const;

  /// Cleanups of a ID3v1 tag structure. Results in an invalid tag.
  void Clean()                  { *this = CleanTag; }
  /// Check whether TAG is clean
  bool IsClean() const          { return Equals(CleanTag); }

  /// Ensure that the tag has a valid signature
  void MakeValid()              { memcpy(id, "TAG", 3); }
  /// Check whether the tag has a valid signature
  bool IsValid() const          { return memcmp(id, "TAG", 3) == 0; }

  /// Places a specified field of the given tag in result.
  /// @param charset Source charset of the tag.
  /// @return Returns true if the tag has a non blank value.
  bool GetField(ID3V1_TAG_COMP type, DSTRING& result, int charset) const;
  /// Sets a specified field of the given tag to the value \a source.
  /// @param charset Destination charset of the tag.
  /// @remarks During assignment the values might be silently truncated
  /// or invalid genres might be discarded.
  void SetField(ID3V1_TAG_COMP type, const char* source, int charset);

  /// Identify the ID3v1 genre number by it's name.
  /// @return Returns -1 on failure.
  static int GetGenre(const char* str);

  /// Reads a ID3v1 tag from the input file and stores them in
  /// the structure.
  /// @return Returns 0 if it successfully reads the tag
  /// or if the input file don't have a ID3v1 tag.
  /// A nonzero return value indicates an I/O error.
  int ReadTag(XFILE* x);
  /// Writes a ID3v1 tag from the given structure to the output file.
  /// @return Returns 0 if it successfully writes the tag.
  /// A nonzero return value indicates an error. */
  int WriteTag(XFILE* x) const;
  /// Remove the tag from the file. Takes care of resizing
  /// the file, if needed.
  /// @return Returns 0 upon success, or -1 if an error occured.
  static int WipeTag(XFILE* x);

  /// Auto detect codepage of an ID3V1 tag.
  /// @param codepage suggested codepage, maybe CH_CP_NONE if unknown
  /// @return identified codepage
  unsigned int DetectCodepage(unsigned int codepage) const;
};

#endif /* ID3V1_TAG_H */
