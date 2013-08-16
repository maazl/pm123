/*
 * Copyright 2010-2012 M.Mueller
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

#ifndef  PMUTILS_H
#define  PMUTILS_H

#define INCL_WIN
#include <debuglog.h>

#include <cpp/xstring.h>

#include <os2.h>

#ifndef BKS_TABBEDDIALOG
  /* Tabbed dialog. */
  #define BKS_TABBEDDIALOG 0x00000800UL
#endif
#ifndef BKS_BUTTONAREA
  /* Reserve space for buttons. */
  #define BKS_BUTTONAREA   0x00000200UL
#endif

#ifndef DC_PREPAREITEM
  #define DC_PREPAREITEM   0x0040
#endif


/// Get window text as xstring without length limitation.
inline xstring WinQueryWindowXText(HWND hwnd)
{ xstring ret;
  char* dst = ret.allocate(WinQueryWindowTextLength(hwnd));
  PMXASSERT(WinQueryWindowText(hwnd, ret.length()+1U, dst), == ret.length());
  return ret;
}
inline xstring WinQueryDlgItemXText(HWND hwnd, USHORT id)
{ return WinQueryWindowXText(WinWindowFromID(hwnd, id));
}

bool WinIsMyProcess(HWND hwnd);


struct WindowMsg
{ HWND   Hwnd;
  ULONG  Msg;
  MPARAM MP1;
  MPARAM MP2;
  WindowMsg()                               : Hwnd(NULLHANDLE) {}
  WindowMsg(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2) : Hwnd(hwnd), Msg(msg), MP1(mp1), MP2(mp2) {}
  void assign(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2) { Hwnd = hwnd; Msg = msg; MP1 = mp1; MP2 = mp2; }
  void                                      Post() { ASSERT(Hwnd); WinPostMsg(Hwnd, Msg, MP1, MP2); }
};


/// Read string from drag and drop structure as \c xstring.
/// @param hstr String handle to query.
/// @return Value of the string or null if \a hstr is \c NULLHANDLE.
const xstring DrgQueryStrXName(HSTR hstr);

/// Check if the content of a string in a drag and drop structure equals a given value.
/// @param hstr String handle to check.
/// @param str String to compare with.
/// @return \c true := equal
/// @remarks The function can compare against \c NULL. It returns then \c true
/// if and only if \a hstr is \c NULLHANDLE.
bool DrgVerifyStrXValue(HSTR hstr, const char* str);

/// @brief Delete drag and drop string handle and set it to \c NULLHANDLE.
/// @param hstr String handle to delete.
/// @return The function returns \c false if the string handle is invalid (not \c NULLHANDLE).
/// @details The function deletes the string handle \a hstr if not \c NULLHANDLE
/// and resets it's value to \c NULLHANDLE.
/// If the return value is false then \a hstr is not changed.
inline bool DrgDeleteString(HSTR& hstr)
{ if (hstr != NULLHANDLE)
  { if (!DrgDeleteStrHandle(hstr))
      return false;
    hstr = NULLHANDLE;
  }
  return true;
}

inline void DrgReplaceString(HSTR& hstr, const char* str)
{ if (hstr != NULLHANDLE)
    DrgDeleteStrHandle(hstr);
  hstr = DrgAddStrHandle(str);
}

/// Deletes all string handles of a DRAGITEM structure.
void DrgDeleteStrings(DRAGITEM& di);

/** @brief Wrapper class around DRAGITEM.
 * @details Acts like a simple smart pointer with some additional
 * functionality. The class is copyable and does not take any ownership.
 */
class DragItem
{protected:
  DRAGITEM* DI;                             ///< Pointer to the DRAGITEM structure.
 public:
  /// Create \c DragItem wrapper from \c DRAGITEM pointer.
  DragItem(DRAGITEM* di)                    : DI(di) {}
  /// Access the \c DRAGITEM pointer.
  DRAGITEM* get()                           { return DI; }
  /// Dereference \c DRAGITEM pointer.
  DRAGITEM& operator*()                     { return *DI; }
  /// \c DRAGITEM member access.
  DRAGITEM* operator->()                    { return DI; }

  /// Get \c hstrType as \c xstring.
  const xstring Type() const                { return DrgQueryStrXName(DI->hstrType); }
  /// Assign \c hstrType.
  /// @remarks The function allocates the string using \c DrgAddStrHandle.
  /// The previous string handle is released if any.
  void Type(const char* str) const          { DrgReplaceString(DI->hstrType, str); }
  /// Check whether the \c DRAGITEM supports a given type.
  bool VerifyType(const char* type) const   { return (bool)DrgVerifyType(DI, type); }

  /// Get render mechanism and format string as \see xstring.
  const xstring RMF() const                 { return DrgQueryStrXName(DI->hstrRMF); }
  /// Assign \c hstrRMF.
  /// @remarks The function allocates the string using \c DrgAddStrHandle.
  /// The previous string handle is released if any.
  void RMF(const char* str) const           { DrgReplaceString(DI->hstrRMF, str); }
  /// Check whether the \c DRAGITEM matches a given render mechanism and format.
  bool VerifyRMF(const char* mech, const char* format) const { return (bool)DrgVerifyRMF(DI, mech, format); }

  /// Get source container name as \see xstring.
  const xstring ContainerName() const       { return DrgQueryStrXName(DI->hstrContainerName); }
  /// Assign \c hstrContainerName.
  /// @remarks The function allocates the string using \c DrgAddStrHandle.
  /// The previous string handle is released if any.
  void ContainerName(const char* str) const { DrgReplaceString(DI->hstrContainerName, str); }
  /// Check whether \c SourceName matches a given string.
  bool VerifyContainerName(const char* str) const { return DrgVerifyStrXValue(DI->hstrContainerName, str); }

  /// Get source name as \c xstring.
  const xstring SourceName() const          { return DrgQueryStrXName(DI->hstrSourceName); }
  /// Assign \c hstrSourceName.
  /// @remarks The function allocates the string using \c DrgAddStrHandle.
  /// The previous string handle is released if any.
  void SourceName(const char* str) const    { DrgReplaceString(DI->hstrSourceName, str); }
  /// Check whether \c SourceName matches a given string.
  bool VerifySource(const char* str) const  { return DrgVerifyStrXValue(DI->hstrSourceName, str); }
  /// Get \c hstrContainerName and \c hstrSourceName as \see xstring.
  /// @remarks This is the full path of the source.
  /// If either \c hstrContainerName or \c hstrSourceName is \c NULLHANDLE the function returns \c NULL.
  const xstring SourcePath() const;

  /// Get suggested target name as \see xstring.
  const xstring TargetName() const          { return DrgQueryStrXName(DI->hstrTargetName); }
  /// Assign \c hstrTargetName.
  /// @remarks The function allocates the string using \c DrgAddStrHandle.
  /// The previous string handle is released if any.
  void TargetName(const char* str) const    { DrgReplaceString(DI->hstrTargetName, str); }

  /// Send \c DM_ENDCONVERSATION message to the source.
  void EndConversation(bool success)        { DEBUGLOG(("DragItem({%p})::EndConversation(%u)\n", DI, success));
                                              DrgSendTransferMsg(DI->hwndItem, DM_ENDCONVERSATION, MPFROMLONG(DI->ulItemID), MPFROMLONG(success ? DMFL_TARGETSUCCESSFUL : DMFL_TARGETFAIL)); }
};

/** @brief Wrapper class around \c DRAGINFO.
 * @details Acts like a simple smart pointer and a container of
 * \c DragItem. The class is copyable and does not take any ownership.
 */
class DragInfo
{protected:
  DRAGINFO* DI;                             ///< Pointer to the \c DRAGINFO.
 public:
  /// Create wrapper from \c DRAGINFO pointer.
  DragInfo(DRAGINFO* di)                    : DI(di) {}
  /// Access the \c DRAGINFO pointer.
  DRAGINFO* get()                           { return DI; }
  /// Dereference the \c DRAGINFO pointer.
  DRAGINFO& operator*()                     { return *DI; }
  /// Access \c DRAGINFO members.
  DRAGINFO* operator->()                    { return DI; }
  /// Access contained \c DragItem structures.
  DragItem  operator[](size_t i)            { ASSERT(i < DI->cditem); return DragItem(DrgQueryDragitemPtr(DI, i)); }
};

/** @brief Scoped \c DRAGINFO wrapper.
 * @details Takes the ownership and deletes any contained \c DRAGITEM as well as
 * the \c DRAGINFO at the end of the scope. The class is non-copyable.
 */
class ScopedDragInfo : public DragInfo
{protected:
  bool OwnStrings;
 private: // non-copyable
  ScopedDragInfo(const ScopedDragInfo&);
  void operator=(const ScopedDragInfo&);
 public:
  /// Allocate a \c DRAGINFO structure with \c DRAGITEMs.
  /// @param n Number of \c DRAGITEMs.
  ScopedDragInfo(size_t n);
  ScopedDragInfo(DRAGINFO* di);
  /// Destroy the \c DRAGINFO with all it's \c DRAGITEMs and also all non-NULL string handles (if owned).
  ~ScopedDragInfo();

  /// Execute \c DrgDrag.
  HWND Drag(HWND source, DRAGIMAGE* img, ULONG nimg, LONG vkterm = VK_ENDDRAG);
};

/** @brief Wrapper around \c DRAGTRANSFER structure.
 * @remarks The class is cheaply copyable. It has the memory footprint identical to \c PDRAGTRANSFER but provides some additional member functions.
 */
class DragTransfer
{protected:
  DRAGTRANSFER* DT;                         ///< Pointer to the \c DRAGTRANSFER structure.
 public:
  /// initialize from a \c DRAGTRANSFER pointer.
  /// @param dt Pointer to the \c DRAGTRANSFER structure to wrap. \c NULL is allowed.
  DragTransfer(DRAGTRANSFER* dt)            : DT(dt) {}
  ~DragTransfer()                           {}
  /// Access the contained \c DRAGTRANSFER pointer.
  DRAGTRANSFER* get()                       { return DT; }
  /// Dereference the \c DRAGTRANSFER pointer.
  DRAGTRANSFER& operator*()                 { return *DT; }
  /// \c DRAGTRANSFER member access.
  DRAGTRANSFER* operator->()                { return DT; }
  /// Access the referenced \c DRAGITEM as \c DragItem.
  DragItem      Item()                      { return DragItem(DT->pditem); }

  /// Read \c hstrSelectedRMF as \c xstring.
  const xstring SelectedRMF() const         { return DrgQueryStrXName(DT->hstrSelectedRMF); }
  /// Assign \c hstrSelectedRMF.
  /// @remarks The function allocates the string using \c DrgAddStrHandle.
  /// The previous string handle is released if any.
  void SelectedRMF(const char* str) const   { DrgReplaceString(DT->hstrSelectedRMF, str); }
  /// Check whether \c SelectedRMF matches a given string.
  bool VerifySelectedRMF(const char* str) const { return DrgVerifyStrXValue(DT->hstrSelectedRMF, str); }

  /// Read \c hstrRenderToName as \c xstring.
  const xstring RenderToName() const        { return DrgQueryStrXName(DT->hstrRenderToName); }
  /// Assign \c hstrRenderToName.
  /// @remarks The function allocates the string using \c DrgAddStrHandle.
  /// The previous string handle is released if any.
  void RenderToName(const char* str) const  { DrgReplaceString(DT->hstrRenderToName, str); }

  /// Send \c DM_RENDERPREPARE message.
  /// @return true: \c DM_RENDERPREPARE succeeded.
  virtual bool RenderPrepare()              { return (bool)DrgSendTransferMsg(DT->pditem->hwndItem, DM_RENDERPREPARE, MPFROMP(DT), 0); }
  /// Set \c RenderToName and send optionally a \c DM_RENDERPREPARE message.
  /// @return true: \c DM_RENDERPREPARE is not required or succeeded.
  bool RenderToPrepare(const char* str);
  /// Send \c DM_RENDER message.
  /// @return true: \c DM_RENDER succeeded.
  virtual bool Render()                     { return (bool)DrgSendTransferMsg(DT->pditem->hwndItem, DM_RENDER, MPFROMP(DT), 0); }

  /// Send \c DM_RENDERCOMPLETE message.
  /// @return true: \c DM_RENDER succeeded.
  bool RenderComplete(USHORT flags)         { return (bool)DrgSendTransferMsg(DT->hwndClient, DM_RENDERCOMPLETE, MPFROMP(DT), MPFROMSHORT(flags)); }
};

class HandleDragTransfers;
/** @brief Access helper for \c DRAGTRANSFER.
 * @details This is a refinement of \see DragTransfer to handle resource management
 * together with \see HandleDragTransfers. It keeps the \c DRAGTRANSFER array alive
 * if a \c DM_RENDER message is sent.
 * @remarks The implementation relies on \c ulTargetInfo to be a pointer to the \c HandleDragTransfers instance.
 * The MSB of the pointer is abused as busy indicator.
 */
class UseDragTransfer : public DragTransfer
{private: // non-copyable
  UseDragTransfer(const UseDragTransfer&);
  void operator=(const UseDragTransfer&);
 protected:
  void DetachWorker();
 public:
  /// @brief Create wrapper from \c DRAGTRANSFER pointer.
  /// @remarks This Constructor is intended to be used at the \c DM_RENDERCOMPLETE processing.
  /// It must not be used on DRAGTRANSFER structures not allocated by \see ScopedDragTransfers.
  UseDragTransfer(DRAGTRANSFER* dt, bool isrendercomplete = false);
  /// Discards the \c DRAGTRANSFER access. If no \c DM_RENDER has been sent (or \c Hold called)
  /// a \c DM_ENDCONVERSATION with \a success = false is sent to the source.
  virtual ~UseDragTransfer();

  /// Access the related worker instance.
  HandleDragTransfers* Worker()             { return (HandleDragTransfers*)(DT->ulTargetInfo & 0x7fffffff); }
  /// Check whether DM_RENDER is on the way.
  bool IsHold()                             { return (long)DT->ulTargetInfo < 0; }
  /// Mark the instance as still in use.
  /// @remarks This could be used if you are at \c DM_RENDERCOMPLETE and you
  /// cannot immediately do the required processing. Later on you should
  /// create another \c UseDragTransfer instance with \a isrendercomplete true.
  void Hold()                               { ASSERT((long)DT->ulTargetInfo > 0); DT->ulTargetInfo |= 0x80000000; }

  /// @brief Send \c DM_RENDERPREPARE message.
  /// @return true: \c DM_RENDERPREPARE succeeded or it was not required.
  /// @remarks If \c RenderPrepare fails the worker is detached from this instance
  /// and \c ulTargetInfo is set to \c NULL.
  virtual bool RenderPrepare();
  /// @brief Send \c DM_RENDER message.
  /// @return true: \c DM_RENDER succeeded.
  /// @remarks If \c Render succeeds the busy indicator (MSB of \c ulTargetInfo) is set.
  virtual bool Render();
  /// @brief Send \c DM_RENDER message using \a renderto name.
  /// @return true: \c DM_RENDER succeeded.
  /// @remarks This method combines \c RenderToPrepare and Render.
  bool RenderTo(const char* renderto)       { return RenderToPrepare(renderto) && Render(); }
  /// Send \c DM_ENDCONVERSATION message to the source.
  /// @param success \c true: send \c DMFL_TARGETSUCCESSFUL, otherwise DMFL_TARGETFAIL is sent.
  /// @remarks This will implicitly mark the instance as no longer used regardless of \a success.
  /// The worker will be detached from this instance and \c ulTargetInfo will be set to \c NULL.
  /// You must not access the structure after this function call anymore.
  void EndConversation(bool success = true);
};

/** @brief Scoped array of \c DRAGTRANSFER structures.
 * @details This manages the ownership of an array of \c DRAGTRANSFER structures.
 * The class uses \c ulTargetInfo for internal purposes. So this field must not be used otherwise.
 * It should be used in conjunction with class \c UseDragTransfer.
 * The class is non-copyable.
 * @remarks The \c ulTargetInfo filed is used in the following way:
 * - MSB: in use flag
 * - remaining bits: Pointer to this.
 * The in use flag is controlled by the \see UseDragTransfer class.
 * The \c DRAGTRANSFER array is freed exactly if all items are detached
 * from this worker and the is no strong reference to this anymore.
 * You should use \c int_ptr<HandleDragTransfers> to keep references to this class.
 */
class HandleDragTransfers : public ScopedDragInfo, public Iref_count
{protected:
  DRAGTRANSFER* DT;                         ///< Pointer to the \c DRAGTRANSFER structures.
 private: // non-copyable
  HandleDragTransfers(const HandleDragTransfers&);
  void operator=(const HandleDragTransfers&);
 public:
  /// @brief Create array of \c DRAGTRANSFER structures for all items in \c DRAGINFO structure.
  /// @param di The \c DRAGINFO structure.
  /// @param hwnd Window handle of the target which should receive the \c DM_RENDERCOMPLETE messages.
  /// @details This constructor fills \c cb, \c hwndClient, \c pditem, \c usOperation and \c ulTargetInfo
  /// of each \c DRAGTRANSFER structure in the array.
  HandleDragTransfers(DRAGINFO* di, HWND hwnd);
  /// @brief Destroy \c ScopedDragTransfers instance.
  /// @details The destructor will free the contained array.
  virtual ~HandleDragTransfers();

 public:
  /// Access the i-th element in the array.
  /// @pre \a i < \c size().
  DRAGTRANSFER* operator[](size_t i)        { ASSERT(i < DI->cditem); return DT + i; }
};


/** RAII for presentation space. */
class PresentationSpace
{private:
  const HPS     Hps;

 public:
                PresentationSpace(HWND hwnd): Hps(WinGetPS(hwnd)) { PMASSERT(Hps != NULLHANDLE); }
                ~PresentationSpace()        { PMRASSERT(WinReleasePS(Hps)); }
                operator HPS() const        { return Hps; }
};

class PaintPresentationSpace
{private:
  const HPS     Hps;

 public:
                PaintPresentationSpace(HWND hwnd): Hps(WinBeginPaint(hwnd, NULLHANDLE, NULL)) { PMASSERT(Hps != NULLHANDLE); }
                ~PaintPresentationSpace()   { PMRASSERT(WinReleasePS(Hps)); }
                operator HPS() const        { return Hps; }
};

#endif

