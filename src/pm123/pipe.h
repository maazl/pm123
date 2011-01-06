/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
 *
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2008-2010 M.Mueller
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

#ifndef  PIPE_H
#define  PIPE_H

#include <config.h>
#include <stdlib.h>
#include <cpp/xstring.h>


/** Create main pipe with only one instance possible since these pipe
 * is almost all the time free, it wouldn't make sense having multiple
 * intances. */
bool amp_pipe_create();
/** Shutdown the player pipe. */
void amp_pipe_destroy();

/** Opens specified pipe and writes data to it. */
bool amp_pipe_open_and_write(const char* pipename, const char* data, size_t size);


/** Class to execute pipe commands with a local context. */
class ACommandProcessor
{protected:
          ACommandProcessor() {}
 public:
  virtual ~ACommandProcessor() {}
  /// Executes the Command \a cmd and return a value in \a ret.
  /// Note that \a cmd is mutable. The buffer content will be destroyed.
  virtual void Execute(xstring& ret, char* cmd) = 0;
  /// Same as above, but copies the command buffer first.
          void Execute(xstring& ret, const char* cmd);

  /// Use this factory method to create instances.
  static  ACommandProcessor* Create();
};

#endif

