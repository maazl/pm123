/*
 * Copyright 2008-2008 Marcel Mueller
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

#ifndef PM123_INFODIALOG_H
#define PM123_INFODIALOG_H

#include "windowbase.h"

class Playable;
class PlayableInstance;
class PlayableSetBase;
/****************************************************************************
*
*  Interface to handle playable info and playlist item dialogs
*  All functions of this class are not thread-safe.
*
****************************************************************************/
class AInfoDialog
: public ManagedDialogBase
/*  private OwnedPlayableSet, // aggregation is not sufficient because of the destruction sequence
  public IComparableTo<const PlayableSetBase*>,
  public inst_index<InfoDialog, const PlayableSetBase*const> // depends on base class OwnedPlayableSet*/
{public:
  enum PageNo
  { Page_MetaInfo,
    Page_TechInfo,
    Page_ItemInfo
  };
 protected:
                    AInfoDialog(ULONG rid, HMODULE module) : ManagedDialogBase(rid, module) {}
 public:
  virtual           ~AInfoDialog() {}
  virtual void      ShowPage(PageNo page) = 0;

  // Factory method. Returns always the same instance for the same set of objects.
  static int_ptr<AInfoDialog> GetByKey(const PlayableSetBase& obj);
  // Factory method. Returns always the same instance for the same Playable.
  static int_ptr<AInfoDialog> GetByKey(Playable& obj);
  // Factory method. Returns always the same instance for the same PlayableInstance.
  static int_ptr<AInfoDialog> GetByKey(Playable& list, PlayableInstance& item);
};


#endif
