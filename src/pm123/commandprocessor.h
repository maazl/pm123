/*
 * Copyright 2008-2011 M.Mueller
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

#ifndef  COMMANDPROCESSOR_H
#define  COMMANDPROCESSOR_H

#include <config.h>
#include <cpp/xstring.h>


/** Class to execute pipe commands with a local context. */
class ACommandProcessor
{protected: // working set
  char*          Request;
  xstringbuilder Reply;

 private: // non-copyable
  ACommandProcessor(const ACommandProcessor&);
  void operator=(const ACommandProcessor&);
 protected:
  ACommandProcessor() {}
  /// Executes the Command \c Request and return a value in \c Reply.
  /// Note that \c Request is mutable. The referenced buffer content will be destroyed.
  virtual void Exec() = 0;
 public:
  virtual ~ACommandProcessor() {}
  /// Executes the Command \a cmd and return a value in \a ret.
  /// Note that \a cmd is mutable. The buffer content will be destroyed.
  const char* Execute(char* cmd) { Request = cmd; Reply.clear(); Exec(); return Reply.cdata(); }
  /// Same as above, but copies the command buffer first.
  const char* Execute(const char* cmd);

  /// Use this factory method to create instances.
  static ACommandProcessor* Create();
};

#endif

