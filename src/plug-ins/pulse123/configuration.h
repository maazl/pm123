/*
 * Copyright 2012 Marcel Mueller
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

#ifndef PULSE123_CONFIGURATION_H
#define PULSE123_CONFIGURATION_H

#include "pulse123.h"
#include <cpp/xstring.h>


// Configuration
extern struct Cfg
{ xstring SinkServer;
  bool    SinkKeepAlive;
  xstring Sink;
  xstring SinkPort;
  xstring SourceServer;
  xstring Source;
  xstring SourcePort;
  int     SourceRate;
  int     SourceChannels;

  void    Load();
  void    Save();
} Configuration;


void ini_query(const char* key, xstring& value);
inline void ini_query(const char* key, int& value)
{ (*Ctx.plugin_api->profile_query)(key, &value, sizeof value);
}
inline void ini_query(const char* key, bool& value)
{ (*Ctx.plugin_api->profile_query)(key, &value, 1);
}
#define ini_load(var) ini_query(#var, var)

inline void ini_write(const char* key, const char* value)
{ int len = value ? strlen(value) : 0;
  (*Ctx.plugin_api->profile_write)(key, value, len);
}
inline void ini_write(const char* key, const int value)
{ (*Ctx.plugin_api->profile_write)(key, &value, sizeof(value));
}
inline void ini_write(const char* key, const bool value)
{ (*Ctx.plugin_api->profile_write)(key, &value, 1);
}
#define ini_save(var) ini_write(#var, var)


#endif

