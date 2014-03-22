/* * Copyright 2006-2007 Marcel Mueller
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

/* Utility factories to compile easy function adaptors in the data segment.
 * They require a platform with a IA-32 compatible instruction set and a flat
 * memory model to operate. Compiler or operating system is not significant.
 *
 * - Virtual .NET like delegates from function pointer.
 * - Replacer to replace objects in of member-function like functions.
 *
 * These implementations are very performant and can be altered at run time.
 * The VDELEGATE pattern cannot be solved in any other, more portable way.
 */

#ifndef CPPVDELEGATE_H
#define CPPVDELEGATE_H

#include <vdelegate.h>


/* Typesafe C++ wrappers */
#define PARSIZE(type) ((sizeof(type)+sizeof(int)-1) / sizeof(int))
template <class R, class P>
inline R DLLENTRYP2(vdelegate(VDELEGATE* dg, R DLLENTRYP(func)(P*), P* ptr))()
{ return (R DLLENTRYP()())mkvdelegate(dg, (V_FUNC)func, 0, ptr);
}
template <class R, class P, class P1>
inline R DLLENTRYP2(vdelegate(VDELEGATE* dg, R DLLENTRYP(func)(P*,P1), P* ptr))(P1)
{ return (R DLLENTRYP()(P1))mkvdelegate(dg, (V_FUNC)func, PARSIZE(P1), ptr);
}
template <class R, class P, class P1, class P2>
inline R DLLENTRYP2(vdelegate(VDELEGATE* dg, R DLLENTRYP(func)(P*,P1,P2), P* ptr))(P1,P2)
{ return (R DLLENTRYP()(P1,P2))mkvdelegate(dg, (V_FUNC)func, PARSIZE(P1)+PARSIZE(P2), ptr);
}
template <class R, class P, class P1, class P2, class P3>
inline R DLLENTRYP2(vdelegate(VDELEGATE* dg, R DLLENTRYP(func)(P*,P1,P2,P3), P* ptr))(P1,P2,P3)
{ return (R DLLENTRYP()(P1,P2,P3))mkvdelegate(dg, (V_FUNC)func, PARSIZE(P1)+PARSIZE(P2)+PARSIZE(P3), ptr);
}
template <class R, class P, class P1, class P2, class P3, class P4>
inline R DLLENTRYP2(vdelegate(VDELEGATE* dg, R DLLENTRYP(func)(P*,P1,P2,P3,P4), P* ptr))(P1,P2,P3,P4)
{ return (R DLLENTRYP()(P1,P2,P3,P4))mkvdelegate(dg, (V_FUNC)func, PARSIZE(P1)+PARSIZE(P2)+PARSIZE(P3)+PARSIZE(P4), ptr);
}
template <class R, class P, class P1, class P2, class P3, class P4, class P5>
inline R DLLENTRYP2(vdelegate(VDELEGATE* dg, R DLLENTRYP(func)(P*,P1,P2,P3,P4,P5), P* ptr))(P1,P2,P3,P4,P5)
{ return (R DLLENTRYP()(P1,P2,P3,P4,P5))mkvdelegate(dg, (V_FUNC)func, PARSIZE(P1)+PARSIZE(P2)+PARSIZE(P3)+PARSIZE(P4)+PARSIZE(P5), ptr);
}
template <class R, class P, class P1, class P2, class P3, class P4, class P5, class P6>
inline R DLLENTRYP2(vdelegate(VDELEGATE* dg, R DLLENTRYP(func)(P*,P1,P2,P3,P4,P5,P6), P* ptr))(P1,P2,P3,P4,P5,P6)
{ return (R DLLENTRYP()(P1,P2,P3,P4,P5,P6))mkvdelegate(dg, (V_FUNC)func, PARSIZE(P1)+PARSIZE(P2)+PARSIZE(P3)+PARSIZE(P4)+PARSIZE(P5)+PARSIZE(P6), ptr);
}

template <class R, class P>
struct vdelegate0 : private VDELEGATE
{ typedef R DLLENTRYP(called_fn)(P*);
  typedef R DLLENTRYP(callee_fn)();
  vdelegate0(called_fn func, P* ptr) { vdelegate(this, func, ptr); }
  void assign(called_fn func, P* ptr) { vdelegate(this, func, ptr); }
  operator callee_fn() { return (callee_fn)this; }
};
template <class R, class P, class P1>
struct vdelegate1 : private VDELEGATE
{ typedef R DLLENTRYP(called_fn)(P*,P1);
  typedef R DLLENTRYP(callee_fn)(P1);
  vdelegate1(called_fn func, P* ptr) { vdelegate(this, func, ptr); }
  void assign(called_fn func, P* ptr) { vdelegate(this, func, ptr); }
  operator callee_fn() { return (callee_fn)this; }
};
template <class R, class P, class P1, class P2>
struct vdelegate2 : private VDELEGATE
{ typedef R DLLENTRYP(called_fn)(P*,P1,P2);
  typedef R DLLENTRYP(callee_fn)(P1,P2);
  vdelegate2(called_fn func, P* ptr) { vdelegate(this, func, ptr); }
  void assign(called_fn func, P* ptr) { vdelegate(this, func, ptr); }
  operator callee_fn() { return (callee_fn)this; }
};
template <class R, class P, class P1, class P2, class P3>
struct vdelegate3 : private VDELEGATE
{ typedef R DLLENTRYP(called_fn)(P*,P1,P2,P3);
  typedef R DLLENTRYP(callee_fn)(P1,P2,P3);
  vdelegate3(called_fn func, P* ptr) { vdelegate(this, func, ptr); }
  void assign(called_fn func, P* ptr) { vdelegate(this, func, ptr); }
  operator callee_fn() { return (callee_fn)this; }
};
template <class R, class P, class P1, class P2, class P3, class P4>
struct vdelegate4 : private VDELEGATE
{ typedef R DLLENTRYP(called_fn)(P*,P1,P2,P3,P4);
  typedef R DLLENTRYP(callee_fn)(P1,P2,P3,P4);
  vdelegate4(called_fn func, P* ptr) { vdelegate(this, func, ptr); }
  void assign(called_fn func, P* ptr) { vdelegate(this, func, ptr); }
  operator callee_fn() { return (callee_fn)this; }
};
template <class R, class P, class P1, class P2, class P3, class P4, class P5>
struct vdelegate5 : private VDELEGATE
{ typedef R DLLENTRYP(called_fn)(P*,P1,P2,P3,P4,P5);
  typedef R DLLENTRYP(callee_fn)(P1,P2,P3,P4,P5);
  vdelegate5(called_fn func, P* ptr) { vdelegate(this, func, ptr); }
  void assign(called_fn func, P* ptr) { vdelegate(this, func, ptr); }
  operator callee_fn() { return (callee_fn)this; }
};
template <class R, class P, class P1, class P2, class P3, class P4, class P5, class P6>
struct vdelegate6 : private VDELEGATE
{ typedef R DLLENTRYP(called_fn)(P*,P1,P2,P3,P4,P5,P6);
  typedef R DLLENTRYP(callee_fn)(P1,P2,P3,P4,P5,P6);
  vdelegate6(called_fn func, P* ptr) { vdelegate(this, func, ptr); }
  void assign(called_fn func, P* ptr) { vdelegate(this, func, ptr); }
  operator callee_fn() { return (callee_fn)this; }
};
#undef PARSIZE

template <class R, class P>
inline R DLLENTRYP2(vreplace1(VREPLACE1* rp, R DLLENTRYP(func)(P*), P* ptr))(P*)
{ return (R DLLENTRYP()(P*))mkvreplace1(rp, (V_FUNC)func, ptr);
}
template <class R, class P, class P2>
inline R DLLENTRYP2(vreplace1(VREPLACE1* rp, R DLLENTRYP(func)(P*,P2), P* ptr))(P*,P2)
{ return (R DLLENTRYP()(P*,P2))mkvreplace1(rp, (V_FUNC)func, ptr);
}
template <class R, class P, class P2, class P3>
inline R DLLENTRYP2(vreplace1(VREPLACE1* rp, R DLLENTRYP(func)(P*,P2,P3), P* ptr))(P*,P2,P3)
{ return (R DLLENTRYP()(P*,P2,P3))mkvreplace1(rp, (V_FUNC)func, ptr);
}
template <class R, class P, class P2, class P3, class P4>
inline R DLLENTRYP2(vreplace1(VREPLACE1* rp, R DLLENTRYP(func)(P*,P2,P3,P4), P* ptr))(P*,P2,P3,P4)
{ return (R DLLENTRYP()(P*,P2,P3,P4))mkvreplace1(rp, (V_FUNC)func, ptr);
}
template <class R, class P, class P2, class P3, class P4, class P5>
inline R DLLENTRYP2(vreplace1(VREPLACE1* rp, R DLLENTRYP(func)(P*,P2,P3,P4,P5), P* ptr))(P*,P2,P3,P4,P5)
{ return (R DLLENTRYP()(P*,P2,P3,P4,P5))mkvreplace1(rp, (V_FUNC)func, ptr);
}
template <class R, class P, class P2, class P3, class P4, class P5, class P6>
inline R DLLENTRYP2(vreplace1(VREPLACE1* rp, R DLLENTRYP(func)(P*,P2,P3,P4,P5,P6), P* ptr))(P*,P2,P3,P4,P5,P6)
{ return (R DLLENTRYP()(P*,P2,P3,P4,P5,P6))mkvreplace1(rp, (V_FUNC)func, ptr);
}

#endif
