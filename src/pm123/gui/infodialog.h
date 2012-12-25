/*
 * Copyright 2008-2012 Marcel Mueller
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

#include "../core/playable.h"
#include <cpp/windowbase.h>
#include <cpp/container/sorted_vector.h>


class PlayableSetBase;
/****************************************************************************
*
*  Interface to handle playable info and playlist item dialogs
*  All functions of this class are not thread-safe.
*
****************************************************************************/
class AInfoDialog
: public ManagedDialog<NotebookDialogBase>
{public:
  enum PageNo
  { Page_MetaInfo,
    Page_TechInfo,
    Page_ItemInfo
  };
  struct KeyType : public sorted_vector_int<APlayable, APlayable, &CompareInstance<APlayable> >
  { KeyType() {}
    KeyType(size_t initial_capacity) : sorted_vector_int<APlayable, APlayable, &CompareInstance<APlayable> >(initial_capacity) {}
    int_ptr<Playable> Parent;
    static int      compare(const KeyType& l, const KeyType& r);
  };
 protected:
                    AInfoDialog(ULONG rid, HMODULE module) : ManagedDialog<NotebookDialogBase>(rid, module, DF_AutoResize) {}
 public:
  virtual           ~AInfoDialog() {}
  virtual bool      IsVisible(PageNo) const = 0;
  virtual void      ShowPage(PageNo page) = 0;

  /// Generic Factory method for Info dialogs.
  /// @param set List of objects to edit. This can be either:
  /// - a single Playable object,
  /// - a set of Playable objects to be edited simultaneously,
  /// - a PlayableInstance and the parent playlist to edit the item information too or
  /// - a set of PlayableInstances of the same playlist and their parent Playable object.
  /// @return Returns always the same instance for the same set of objects.
  static int_ptr<AInfoDialog> GetByKey(const KeyType& set);
  /// Factory method for single Playable objects.
  /// @param obj Object to edit.
  /// @return Returns always the same instance for the same object.
  static int_ptr<AInfoDialog> GetByKey(APlayable& obj);
  /// Lookup for existing editor.
  /// @param obj Object to edit.
  /// @return Returns always the same instance for the same object or \c NULL if no instance exists.
  static int_ptr<AInfoDialog> FindByKey(APlayable& obj);
};


#endif
