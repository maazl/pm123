/*
 * Copyright 2007-2010 M.Mueller
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


#ifndef URL_H
#define URL_H

#include <cpp/xstring.h>
#include <cpp/container/stringmap.h>

/** class to handle PM123 URLs.
 *
 * The supported schemes are:
 *
 * Files and directories:
 *   file:///drive:path/file                 local file
 *   file://server/path/file                 remote file
 *   file:///drive:path/                     all known file types in a local folder
 *   file:///drive:path/?recursive           all known file types in a local folder including subdirectories
 *   file:///drive:path/?pattern=abc*.mp3    all matching files in a local folder
 * The options remote, folder, pattern matching and recursion may be combined in any way.
 *
 * Any other URL style accepted by at least one of the plug-ins, like:
 *   cdda:///drive:/Track number             CD track
 *   cdda:///drive:/Frame from-to            CD slice
 *   http://server[:port]/path/item?params   usual http URL
 *   record:///sounddevice?param=value&...   recording plug-in
 * URLs starting with cd: (old PM123 versions) are converted to cdda:.
 * There are no other restriction about fully qualified URLs in this class.
 *
 * The function normalizeURL also accepts the following short forms that are converted to file URLs:
 *   drive:path\file                         filename only
 *   \\server\path\file                      UNC filename
 *   drive:path\                             foldername only
 *   \\server\path\                          UNC folder
 *   drive:path\?...                         foldername only with parameters
 *   \\server\path\?...                      UNC folder with parameters
 */
class url123 : public xstring
{
 public: // Work around to avoid temporaries in conditional expressions.
  static const url123 EmptyURL;
 protected:
  static size_t decode(char* dst, const char* src, size_t len);
 public:
  static bool   isPathDelimiter(char c) { return c == '/' || c == '\\'; }
  static bool   hasScheme(const char* str);
  static bool   isAbsolute(const char* str);
  static void   parseParameter(stringmap& dest, const char* params);
  static const xstring makeParameter(const stringmap& params); 
  static bool*  parseBoolean(const char* val);
  /// Normalize the given URL.
  /// @param str URL to normalize. This may be either a file name or any URI scheme.
  /// @return Normalized URL or \c NULL in case of an error.
  /// @details This functions takes care of backslashes in file names and converts them to forward slash.
  /// Furthermore /xxx/../ is reduced. (Not Unix compliant.)
  static const url123 normalizeURL(const char* str);

                url123() {}
                url123(const xstring& r) : xstring(r) {}
  explicit      url123(const char* r)    : xstring(r) {}

  /// Returns the path component of the url including protocol and a trailing slash.
  const xstring getBasePath() const;
  /// Returns only the object name with extension
  /// This is the part after the basepath and before any query parameters (if any)
  const xstring getObjectName() const;
  /// This returns that part of the object name that is likely to be a file extension.
  /// If no extension is found it returns an empty string.
  const xstring getExtension() const;
  /// Return query parameter if any or an empty string otherwise.
  const xstring getParameter() const;

  /// Returns a simplified version of the url containing only the important part
  /// E.g. filenames are reported without file:///.
  const xstring getDisplayName() const;
  /// Returns only the object name without extension
  /// This is the part after the basepath and before any query parameters (if any)
  const xstring getShortName() const;

  /// test whether the url belongs to a given scheme
  bool          isScheme(const char* scheme) const { return startsWithI(scheme); }
  /// Make the given URL absolute (if required) using the current object as starting point.
  /// If the URL is already absolute it will simply create a normalized URL.
  const url123  makeAbsolute(const char* rel) const;
  /// Make URL relative (if possible) using root as starting point.
  /// This function makes use of ../ if required and \a useupdir is true.
  /// If this is not possible, the current URL is returned.
  const xstring makeRelative(const char* root, bool useupdir = true) const;
};


#endif
