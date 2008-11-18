/*
 * Copyright 2007-2008 M.Mueller
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

#ifndef  SHOWACCEL_H
#define  SHOWACCEL_H

#define INCL_WIN
#include <cpp/container/sorted_vector.h>
#include <os2.h>

/* Automatically append strings with the accelerator keys to the menu items. */
class MenuShowAccel
{private:
  struct AccEntry : public IComparableTo<ACCEL>, public ACCEL
  { virtual int compareTo(const ACCEL& r) const;
  };
  typedef sorted_vector_own<AccEntry, ACCEL> AccList;

 private:
  AccList       AccData;

 public:
  // Create worker class from accelerator table.
                MenuShowAccel(HACCEL accel);
  // Convert accelerator key to text. The text will not be longer than 20 characters (including terminating 0).
  static void   Accel2Text(char* text, ACCEL as);
  // Append or update accelerater text of a sigle menu entry.
  static void   UpdateAccel(HWND menu, USHORT id, const ACCEL* accel);
  // Append or update accelerater table entries in menu window.
  // Process subwindows also if incl_sub is true.
  void          ApplyTo(HWND menu, bool incl_sub = true);
};

#endif

