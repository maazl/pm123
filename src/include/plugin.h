#ifndef PM123_PLUGIN_H
#define PM123_PLUGIN_H

#define INCL_WIN
#define INCL_BASE
#include <config.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include <os2.h>

#ifdef PM123_CORE
/* maximum supported and most recent plugin-level */
#define PLUGIN_INTERFACE_LEVEL 2
#endif

typedef enum
{ PLUGIN_OK           = 0
, PLUGIN_UNSUPPORTED  = 1
, PLUGIN_NO_READ      = 100
, PLUGIN_GO_ALREADY   = 101
, PLUGIN_GO_FAILED    = 102
, PLUGIN_NO_PLAY      = 200
, PLUGIN_NO_OP        = 300
, PLUGIN_NO_SAVE      = 400
, PLUGIN_ERROR        = 500
, PLUGIN_FAILED       = ((unsigned long)-1)
, PLUGIN_NO_USABLE    = ((unsigned long)-2)
} PLUGIN_RC;

// Old style window messages for Level 1 Plug-ins only
#define WM_PLAYSTOP         (WM_USER +  69)
#define WM_PLAYERROR        (WM_USER + 100)
#define WM_SEEKSTOP         (WM_USER + 666)
#define WM_METADATA         (WM_USER +  42)
#define WM_CHANGEBR         (WM_USER +  43)
#define WM_OUTPUT_OUTOFDATA (WM_USER + 667)
#define WM_PLUGIN_CONTROL   (WM_USER + 668) /* Plugin notify message */

#define PN_TEXTCHANGED  1 /* Display text changed */

#ifndef __cplusplus
/** Dynamically allocated, reference counted string.
 * It is wrapped by a structure to provide type safety, but
 * read access to the embedded C style string is always safe.
 *
 * Note that you MUST initialize these strings to NULL.
 * Avoid copying xstring objects binary. Use xstring_copy instead.
 * The difference is that with xstring_copy both instances must be freed.
 * With a binary copy only one instance must be freed. And since any assignment
 * implicitly frees the previous content this is error prone.
 *
 * You must not access the embedded cstr member of volatile instances.
 * This would cause undefined behavior with race conditions.
 */
typedef struct
{ const char* cstr;   /* pointer to C style null terminated string */
} xstring;
static const xstring xstring_NULL = { NULL };
#elif defined(PM123_CORE)
/** Hack: when compiling the PM123 core we map xstring to the binary compatible
 * C++ class xstring. You must not use this hack for plug-in development
 * because it may give undefined behavior when the memory is freed in another
 * C runtime instance than where it is allocated. */
#include <cpp/xstring.h>
class xstring; // Keep CDT happy
static const xstring xstring_NULL;
#else
/** 2nd Hack: In C++ we forward the string API calls to the runtime of the
 * PM123 core.
 * Note that you \e must not call any of these functions in a static initializer,
 * because at this point \c plugin_init has not yet been called.
 */
class xstring
{ const char* cstr;
 public:
  xstring() : cstr(NULL) {}
  xstring(xstring& r);
  xstring(const xstring& r);
  xstring(const volatile xstring& r);
  xstring(const char* r);
  xstring(const char* r, size_t len);
  ~xstring();
  size_t      length() const;
  bool        operator!() const volatile    { return !cstr; }
  const char* cdata() const                 { return cstr; }
  operator const char*() const              { return cstr; }
  const char& operator[](size_t i) const    { return cstr[i]; }
  int         compareTo(const xstring& r) const;
  char*       allocate(size_t len);
  bool        cmpassign(const char* str);
  void        reset();
  xstring&    operator=(xstring& r);
  xstring&    operator=(const xstring& r);
  void        operator=(const xstring& r) volatile;
  xstring&    operator=(volatile const xstring& r);
  void        operator=(volatile const xstring& r) volatile;
  xstring&    operator=(const char* str);
  void        operator=(const char* str) volatile;
  void        operator+=(const char* str);
  xstring&    sprintf(const char* fmt, ...);
  xstring&    vsprintf(const char* fmt, va_list va);
};
/* Avoid accidental include of xstring.h. */
#define XSTRGING_H
#endif

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(4)

typedef enum
{ PLUGIN_NULL     = 0x00,
  PLUGIN_VISUAL   = 0x01,
  PLUGIN_FILTER   = 0x02,
  PLUGIN_DECODER  = 0x04,
  PLUGIN_OUTPUT   = 0x08
} PLUGIN_TYPE;

typedef struct _PLUGIN_QUERYPARAM
{
  int   type;         /* PLUGIN_*. values can be ORed */
  char* author;       /* Author of the plug-in        */
  char* desc;         /* Description of the plug-in   */
  int   configurable; /* Is the plug-in configurable  */
  int   interface;    /* Interface revision           */

} PLUGIN_QUERYPARAM;

/* Common services of the PM123 core for plug-ins. */
typedef struct
{ /* error message function */
  void DLLENTRYP(error_display)(const char* msg);
  /* info message function */
  /* this information is always displayed to the user right away */
  void DLLENTRYP(info_display)(const char* msg);

  /* retrieve configuration setting */
  int DLLENTRYP(profile_query)(const char* key, void* data, int maxlength);
  /* store configuration setting */
  int DLLENTRYP(profile_write)(const char* key, const void* data, int length);
  /* execute remote command */
  /* See the documentation of remote commands for a description. */
  const char* DLLENTRYP(exec_command)(const char* cmd);
  /* Invalidate object properties */
  /* what is a bit-vector of INFOTYTE */
  void DLLENTRYP(obj_invalidate)(const char* url, int what);
} PLUGIN_API; 

typedef struct
{ /* Initialize a new xstring with a C string */
  xstring  DLLENTRYP(create)   (const char* cstr);
  /* Deallocate dynamic string. This will change the pointer to NULL. */
  void     DLLENTRYP(free)     (volatile xstring* dst);
  /* Return the length of a dynamic string */
  unsigned DLLENTRYP(length)   (const    xstring* src);
  /* Compare two xstrings for (binary!) equality. NULL allowed. */
  char     DLLENTRYP(equal)    (const    xstring* src1, const xstring* src2);
  /* Compare two xstrings instances. NULL allowed. */
  int      DLLENTRYP(compare)  (const    xstring* src1, const xstring* src2);
  /* Copy dynamic string to another one. Any previous content is discarded first.
   * This function will not copy the string itself. It only creates an additional reference to the content. */
  void     DLLENTRYP(copy)     (volatile xstring* dst,  const xstring* src);
  /* Strongly thread safe version of xstring_copy. */
  void     DLLENTRYP(copy_safe)(volatile xstring* dst,  volatile const xstring* src);
  /* Reassign dynamic string from C string. Any previous content is discarded first. */
  void     DLLENTRYP(assign)   (volatile xstring* dst,  const char* cstr);
  /* Reassign dynamic string from C string, but only if the strings differ. */
  char     DLLENTRYP(cmpassign)(         xstring* dst,  const char* cstr);
  /* Append to xstring. The source may also be from a xstring.
   * If dst is NULL a new string is created */ 
  void     DLLENTRYP(append)   (         xstring* dst,  const char* cstr);
  /* Allocate dynamic string. Any previous content is discarded first.
   * The returned memory can be written up to len bytes until the next
   * xstring_* function call on dst. The return value is the same than
   * dst->cstr except for constness. */
  char*    DLLENTRYP(allocate) (         xstring* dst,  unsigned int len);
  /* printf into a xstring. Any previous content is discarded first. */
  void     DLLENTRYP(sprintf)  (volatile xstring* dst,  const char* fmt, ...);
  void     DLLENTRYP(vsprintf) (volatile xstring* dst,  const char* fmt, va_list va);
} XSTRING_API;

typedef struct
{
  const PLUGIN_API*  plugin_api;
  
  const XSTRING_API* xstring_api;

} PLUGIN_CONTEXT;

/* returns 0 -> ok */
int DLLENTRY plugin_query(PLUGIN_QUERYPARAM* param);
int DLLENTRY plugin_init(const PLUGIN_CONTEXT* ctx); // Optional
void DLLENTRY plugin_configure(HWND hwnd, HMODULE module);
void DLLENTRY plugin_command(const char* command, xstring* result);
int DLLENTRY plugin_deinit(int unload);

#pragma pack()

#ifdef __cplusplus
}

#ifndef PM123_CORE
/* xstring proxy implementation
 *
 * For this to work a global symbol Ctx with the parameter ctx of plugin_init must be supplied.
 */
extern PLUGIN_CONTEXT Ctx;

inline xstring::xstring(xstring& r) : cstr(NULL) { Ctx.xstring_api->copy(this, &r); }
inline xstring::xstring(const xstring& r) : cstr(NULL) { Ctx.xstring_api->copy(this, &r); }
inline xstring::xstring(const volatile xstring& r) : cstr(NULL) { Ctx.xstring_api->copy_safe(this, &r); }
inline xstring::xstring(const char* r) : cstr(((const char* DLLENTRYP()(const char*))Ctx.xstring_api->create)(r)) {}
inline xstring::xstring(const char* r, size_t len) : cstr(NULL) { memcpy(Ctx.xstring_api->allocate(this, len), &r, len); }
inline xstring::~xstring() { Ctx.xstring_api->free(this); }
inline size_t   xstring::length() const { return Ctx.xstring_api->length(this); }
inline int      xstring::compareTo(const xstring& r) const { return Ctx.xstring_api->compare(this, &r); }
inline char*    xstring::allocate(size_t len) { return Ctx.xstring_api->allocate(this, len); }
inline bool     xstring::cmpassign(const char* str) { return Ctx.xstring_api->cmpassign(this, str); }
inline void     xstring::reset() { Ctx.xstring_api->free(this); }
inline xstring& xstring::operator=(xstring& r) { Ctx.xstring_api->copy(this, &r); return *this; }
inline xstring& xstring::operator=(const xstring& r) { Ctx.xstring_api->copy(this, &r); return *this; }
inline void     xstring::operator=(const xstring& r) volatile { Ctx.xstring_api->copy(this, &r); }
inline xstring& xstring::operator=(volatile const xstring& r) { Ctx.xstring_api->copy_safe(this, &r); return *this; }
inline void     xstring::operator=(volatile const xstring& r) volatile { Ctx.xstring_api->copy_safe(this, &r); }
inline xstring& xstring::operator=(const char* str) { Ctx.xstring_api->assign(this, str); return *this; }
inline void     xstring::operator=(const char* str) volatile { Ctx.xstring_api->assign(this, str); }
inline void     xstring::operator+=(const char* str) { Ctx.xstring_api->append(this, str); }
inline xstring& xstring::sprintf(const char* fmt, ...) { va_list va; va_start(va, fmt); vsprintf(fmt, va); return *this; }
inline xstring& xstring::vsprintf(const char* fmt, va_list va) { Ctx.xstring_api->vsprintf(this, fmt, va); return *this; }
#endif
#endif

#endif /* PM123_PLUGIN_H */
