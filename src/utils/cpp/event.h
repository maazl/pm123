/*
 * Copyright 2007-2007 M.Mueller
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

#ifndef CPP_EVENT_H
#define CPP_EVENT_H

#define INCL_PM
#include <cpp/mutex.h>
#include <config.h>
#include <os2.h>

#include <debuglog.h>


class dummy
{ int dummy;
};

/* Application event.
 * This class is thread safe.
 * This class is not intenden to be used directly.
 */
class event_base
{ friend class delegate_base;
 private:
  delegate_base* Root;
 private:
  // non-copyable
  event_base(const event_base& r);
  void operator=(const event_base& r);
 protected:
  // Create an event.
  event_base() : Root(NULL) { DEBUGLOG(("event_base(%p)::event_base()\n", this)); }
  ~event_base();
  // Add a delegate to the current event
  void operator+=(delegate_base& d);
  // Remove a delegate from the current event and return true if succeeded.
  bool operator-=(delegate_base& d);
  // Fire the event.
  void operator()(dummy& param) const;
 public:
  // remove all registrated delegates
  void reset();
};

/* non-template base class for delegate
 */
class delegate_base
{ friend class event_base;
 protected:
  typedef void (*func_type)(const void* receiver, dummy& param);
 private:
  func_type      Fn;
  const void*    Rcv;
  event_base*    Ev; // attached to this event
  delegate_base* Link;
 private:
  delegate_base(const delegate_base& r); // non-copyable
  void operator=(const delegate_base& r);
 protected:
  // Construct unattached delegate
  delegate_base(func_type fn, const void* rcv) : Fn(fn), Rcv(rcv), Ev(NULL) {}
  // Construct delegate and attach it to an event immediately
  delegate_base(event_base& ev, func_type fn, const void* rcv);
  ~delegate_base()                             { if (Ev) (*Ev) -= *this; }
 public:
  void           detach()                      { if (Ev) { (*Ev) -= *this; Ev = NULL; } }
};                                     

/* Partial typed delegate class.
 * This is the type check level for adding delegate instances to an event.
 * This class is not intended to be used directly.
 *
 * The dirty part of this class is the cast from
 *   void (*)(const void* receiver, P& param)
 * to
 *   void (*)(const void* receiver, const void* param)
 * It relies on the fact that passing a reference is binary compatible to passing a pointer including a pointer to void.
 */
template <class P>
class event;

template <class P>
class delegate_part : public delegate_base
{protected:
  typedef void (*func_type)(const void* receiver, P& param);
 protected:
  delegate_part(func_type fn, const void* rcv)
    : delegate_base((delegate_base::func_type)fn, rcv) {}
  delegate_part(event<P>& ev, func_type fn, const void* rcv)
    : delegate_base(ev, (delegate_base::func_type)fn, rcv) {}
};

/* Application even class.
 * Delegate instances may be registered to the event. They are called when the event is raised.
 * This class is thread safe.
 * It is allowed to destroy instances of this class while delegates are registered.
 * In this case they are deregistered automatically.
 * Example:
 *
 *   event<int> Ev;
 *   ...
 *   Ev += ...; // register delegates
 *   ...
 *   Ev(7);     // raise event
 */
template <class P>
class event : public event_base
{public:
  void operator()(P& param)            { DEBUGLOG2(("event(%p)::operator()(%p)\n", this, &param)); event_base::operator()(*(dummy*)&param); }
  void operator+=(delegate_part<P>& d) { event_base::operator+=(d); }
  void operator-=(delegate_part<P>& d) { event_base::operator-=(d); }
};

/* Fully typed delegate.
 * Use this delegate to call free function or static class member functions.
 *
 * An istance of delegate must not be bound to more than one event.
 * When the delegete is destroyed it is automatically deregistered from the event.
 * When the event is destroyed the delegate gets silently deregistered.
 *
 * Example:
 *
 *   event<int> Ev;
 *
 *   struct my_struct;
 *   void foo(my_struct* context, int i);
 *
 *   ...
 *   my_struct Context; 
 *   delegate<my_struct, int> Deleg(&foo, &Context);
 *   Ev += Deleg;
 *
 * It is essential that the lifetime of Deleg must not exceed the lifetime of Context
 * or Deleg must be deregistered from Ev before Context gets invalidated.
 * To ensure this Context might be a member of my_struct.
 */
template <class R, class P>
class delegate : public delegate_part<P>
{public:
  typedef void (*func_type)(R* receiver, P& param);
 public:
  delegate(func_type fn, R* rcv = NULL)
  : delegate_part<P>((delegate_part<P>::func_type)fn, rcv)
  { DEBUGLOG2(("delegate(%p)::delegate(%p, %p)\n", this, fn, rcv)); }
  delegate(event<P>& ev, func_type fn, R* rcv = NULL)
  : delegate_part<P>(ev, (delegate_part<P>::func_type)fn, rcv)
  { DEBUGLOG2(("delegate(%p)::delegate(%p, %p, %p)\n", this, &ev, fn, rcv)); }
};

/* Use this delegate to call a non-static member function of your class.
 * Example:
 *
 *   event<int> Ev;
 *
 *   class my_class
 *   {private:
 *     class_delegate<my_class, int> Deleg;
 *     void ReceiveEvent(int i);
 *    public:
 *     my_class() : Deleg(this, &ReceiveEvent) { Ev += Deleg; }
 *     ...
 *   };
 */
template <class C, class P>
class class_delegate : public delegate<class_delegate<C, P>, P>
{public:
  typedef void (C::*func_type)(P& param);
 private:
  C&              Inst;
  func_type const Func;
 private:
  static void CallFunc(class_delegate<C, P>* rcv, P& param);
 public:
  class_delegate(C& inst, func_type fn)
  : delegate<class_delegate<C, P> ,P>(&CallFunc, this)
  , Inst(inst)
  , Func(fn)
  {}
  class_delegate(event<P>& ev, C& inst, func_type fn)
  : delegate<class_delegate<C, P> ,P>(ev, &CallFunc, this)
  , Inst(inst)
  , Func(fn)
  {}
};

template <class C, class P>
void class_delegate<C, P>::CallFunc(class_delegate<C, P>* rcv, P& param)
{ (rcv->Inst.*rcv->Func)(param);
} 

template <class C, class P, class P2>
class class_delegate2 : public delegate<class_delegate2<C, P, P2>, P>
{public:
  typedef void (C::*func_type)(P&, P2);
 private:
  C&              Inst;
  func_type const Func;
  P2              Param;
 private:
  static void CallFunc(class_delegate2<C, P, P2>* rcv, P& param);
 public:
  class_delegate2(C& inst, func_type fn, P2 param2)
  : delegate<class_delegate2<C, P, P2> ,P>(&CallFunc, this)
  , Inst(inst)
  , Func(fn)
  , Param(param2)
  { DEBUGLOG2(("class_delegate2(%p)::class_delegate2(%p, %p, %p)\n", this, &inst, fn, param2)); }
  class_delegate2(event<P>& ev, C& inst, func_type fn, P2 param2)
  : delegate<class_delegate2<C, P, P2> ,P>(ev, &CallFunc, this)
  , Inst(inst)
  , Func(fn)
  , Param(param2)
  { DEBUGLOG2(("class_delegate2(%p)::class_delegate2(%p, %p, %p)\n", this, &inst, fn, param2)); }
}; 

template <class C, class P, class P2>
void class_delegate2<C, P, P2>::CallFunc(class_delegate2<C, P, P2>* rcv, P& param)
{ DEBUGLOG2(("class_delegate2::CallFunc(%p{%p, xx, %p}, %p)\n", rcv, &rcv->Inst, rcv->Param, &param));
  (rcv->Inst.*rcv->Func)(param, rcv->Param);
} 

/* Non-template base to PostMsgDelegate */
class PostMsgDelegateBase
{private:
  const HWND        Window;
  const ULONG       Msg;
  const void* const Param2;
 protected:
  static void callback(PostMsgDelegateBase* receiver, const void* param);
 protected:
  PostMsgDelegateBase(HWND window, ULONG msg, const void* param2 = NULL) : Window(window), Msg(msg), Param2(param2) {}
};

/* This class posts a window message when the event is raised.
 * The message has the following properties:
 * hwnd, msg - taken from constructor parameters
 * mp1       - event parameter
 * mp2       - param2 from constructor
 */
template <class P>
class PostMsgDelegate : private PostMsgDelegateBase, public delegate_part<P>, 
{public:
  PostMsgDelegate(HWND window, ULONG msg, const void* param2)
  : delegate_part<P>((func_type)&callback, (PostMsgDelegateBase*)this), PostMsgDelegateBase(window, msg, param2) {}
};

#endif
