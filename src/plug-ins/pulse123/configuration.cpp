/*
 * Copyright 2010-2012 Marcel Mueller
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


#include "configuration.h"
#include <inimacro.h>

#include <debuglog.h>


void ini_query(const char* key, xstring& value)
{ int len = (*Ctx.plugin_api->profile_query)(key, NULL, 0);
  if (len >= 0)
  { char* cp = value.allocate(len);
    (*Ctx.plugin_api->profile_query)(key, cp, len);
  }
}


Cfg Configuration;


void Cfg::Load()
{ // Defaults
  SinkKeepAlive = false;
  SourceRate = 48000;
  SourceChannels = 2;

  ini_load(SinkServer);
  ini_load(SinkKeepAlive);
  ini_load(Sink);
  ini_load(SinkPort);
  ini_load(SourceServer);
  ini_load(Source);
  ini_load(SourcePort);
  ini_load(SourceRate);
  ini_load(SourceChannels);
}

void Cfg::Save()
{ ini_save(SinkServer);
  ini_save(SinkKeepAlive);
  ini_save(Sink);
  ini_save(SinkPort);
  ini_save(SourceServer);
  ini_save(Source);
  ini_save(SourcePort);
  ini_save(SourceRate);
  ini_save(SourceChannels);
}
