#ifndef PM123_PLUGIN_H
#define PM123_PLUGIN_H

#define INCL_WIN
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
 * Avoid copying DSTRING objects binary. Use dstring_copy instead.
 * The difference is that with dstring_copy both instances must be freed.
 * With a binary copy only one instance must be freed. And since any assignment
 * implicitly frees the previous content this is error prone.
 *
 * You must not access the embedded cstr member of volatile instances.
 * This would cause undefined behavior with race conditions.
 */
typedef struct
{ const char* cstr;   /* pointer to C style null terminated string */
} DSTRING;
static const DSTRING dstring_NULL = { NULL };
#elif defined(PM123_CORE)
#include <cpp/xstring.h>
/** Hack: when compiling the PM123 core we map DSTRING to the binary compatible
 * C++ class xstring. You should not use this hack for plug-in development
 * because it may give undefined behavior when the memory is freed in another
 * C runtime instance than where it is allocated. */
typedef xstring DSTRING;
static const DSTRING dstring_NULL;
#else
/** 2nd Hack: In C++ we forward the string API calls to the runtime of the
 * PM123 core.
 * Note that you \e must not call any of these functions in a static initializer,
 * because at this point \c plugin_init has not yet been called.
 */
class DSTRING
{ const char* cstr;
 public:
  DSTRING() : cstr(NULL) {}
  DSTRING(DSTRING& r);
  DSTRING(const DSTRING& r);
  DSTRING(const volatile DSTRING& r);
  DSTRING(const char* r);
  DSTRING(const char* r, size_t len);
  ~DSTRING();
  size_t      length() const;
  bool        operator!() const volatile    { return !cstr; }
  const char* cdata() const                 { return cstr; }
  operator const char*() const              { return cstr; }
  const char& operator[](size_t i) const    { return cstr[i]; }
  int         compareTo(const DSTRING& r) const;
  char*       allocate(size_t len);
  bool        cmpassign(const char* str);
  void        reset();
  DSTRING&    operator=(DSTRING& r);
  DSTRING&    operator=(const DSTRING& r);
  void        operator=(const DSTRING& r) volatile;
  DSTRING&    operator=(volatile const DSTRING& r);
  void        operator=(volatile const DSTRING& r) volatile;
  DSTRING&    operator=(const char* str);
  void        operator=(const char* str) volatile;
  void        operator+=(const char* str);
  DSTRING&    sprintf(const char* fmt, ...);
  DSTRING&    vsprintf(const char* fmt, va_list va);
};
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

} PLUGIN_QUERYPARAM, *PPLUGIN_QUERYPARAM;

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
{ /* Initialize a new DSTRING with a C string */
  DSTRING  DLLENTRYP(create)   (const char* cstr);
  /* Deallocate dynamic string. This will change the pointer to NULL. */
  void     DLLENTRYP(free)     (volatile DSTRING* dst);
  /* Return the length of a dynamic string */
  unsigned DLLENTRYP(length)   (const    DSTRING* src);
  /* Compare two DSTRINGs for (binary!) equality. NULL allowed. */
  char     DLLENTRYP(equal)    (const    DSTRING* src1, const DSTRING* src2);
  /* Compare two DSTRINGs instances. NULL allowed. */
  int      DLLENTRYP(compare)  (const    DSTRING* src1, const DSTRING* src2);
  /* Copy dynamic string to another one. Any previous content is discarded first.
   * This function will not copy the string itself. It only creates an additional reference to the content. */
  void     DLLENTRYP(copy)     (volatile DSTRING* dst,  const DSTRING* src);
  /* Strongly thread safe version of dstring_copy. */
  void     DLLENTRYP(copy_safe)(volatile DSTRING* dst,  volatile const DSTRING* src);
  /* Reassign dynamic string from C string. Any previous content is discarded first. */
  void     DLLENTRYP(assign)   (volatile DSTRING* dst,  const char* cstr);
  /* Reassign dynamic string from C string, but only if the strings differ. */
  char     DLLENTRYP(cmpassign)(         DSTRING* dst,  const char* cstr);
  /* Append to DSTRING. The source may also be from a DSTRING.
   * If dst is NULL a new string is created */ 
  void     DLLENTRYP(append)   (         DSTRING* dst,  const char* cstr);
  /* Allocate dynamic string. Any previous content is discarded first.
   * The returned memory can be written up to len bytes until the next
   * dstring_* function call on dst. The return value is the same than
   * dst->cstr except for constness. */
  char*    DLLENTRYP(allocate) (         DSTRING* dst,  unsigned int len);
  /* printf into a DSTRING. Any previous content is discarded first. */
  void     DLLENTRYP(sprintf)  (volatile DSTRING* dst,  const char* fmt, ...);
  void     DLLENTRYP(vsprintf) (volatile DSTRING* dst,  const char* fmt, va_list va);
} DSTRING_API;

typedef struct
{
  const PLUGIN_API*  plugin_api;
  
  const DSTRING_API* dstring_api;

} PLUGIN_CONTEXT;

/* returns 0 -> ok */
int DLLENTRY plugin_query(PLUGIN_QUERYPARAM* param);
int DLLENTRY plugin_init(const PLUGIN_CONTEXT* ctx); // Optional
int DLLENTRY plugin_configure(HWND hwnd, HMODULE module);
int DLLENTRY plugin_deinit(int unload);

#pragma pack()

#ifdef __cplusplus
}

#ifndef PM123_CORE
/* DSTRING proxy implementation
 *
 * For this to work a global symbol Ctx with the parameter ctx of plugin_init must be supplied.
 */
extern PLUGIN_CONTEXT Ctx;

inline DSTRING::DSTRING(DSTRING& r) : cstr(NULL) { Ctx.dstring_api->copy(this, &r); }
inline DSTRING::DSTRING(const DSTRING& r) : cstr(NULL) { Ctx.dstring_api->copy(this, &r); }
inline DSTRING::DSTRING(const volatile DSTRING& r) : cstr(NULL) { Ctx.dstring_api->copy_safe(this, &r); }
inline DSTRING::DSTRING(const char* r) : cstr(((const char* DLLENTRYP()(const char*))Ctx.dstring_api->create)(r)) {}
inline DSTRING::DSTRING(const char* r, size_t len) : cstr(NULL) { memcpy(Ctx.dstring_api->allocate(this, len), &r, len); }
inline DSTRING::~DSTRING() { Ctx.dstring_api->free(this); }
inline size_t   DSTRING::length() const { return Ctx.dstring_api->length(this); }
inline int      DSTRING::compareTo(const DSTRING& r) const { return Ctx.dstring_api->compare(this, &r); }
inline char*    DSTRING::allocate(size_t len) { return Ctx.dstring_api->allocate(this, len); }
inline bool     DSTRING::cmpassign(const char* str) { return Ctx.dstring_api->cmpassign(this, str); }
inline void     DSTRING::reset() { Ctx.dstring_api->free(this); }
inline DSTRING& DSTRING::operator=(DSTRING& r) { Ctx.dstring_api->copy(this, &r); return *this; }
inline DSTRING& DSTRING::operator=(const DSTRING& r) { Ctx.dstring_api->copy(this, &r); return *this; }
inline void     DSTRING::operator=(const DSTRING& r) volatile { Ctx.dstring_api->copy(this, &r); }
inline DSTRING& DSTRING::operator=(volatile const DSTRING& r) { Ctx.dstring_api->copy_safe(this, &r); return *this; }
inline void     DSTRING::operator=(volatile const DSTRING& r) volatile { Ctx.dstring_api->copy_safe(this, &r); }
inline DSTRING& DSTRING::operator=(const char* str) { Ctx.dstring_api->assign(this, str); return *this; }
inline void     DSTRING::operator=(const char* str) volatile { Ctx.dstring_api->assign(this, str); }
inline void     DSTRING::operator+=(const char* str) { Ctx.dstring_api->append(this, str); }
inline DSTRING& DSTRING::sprintf(const char* fmt, ...) { va_list va; va_start(va, fmt); vsprintf(fmt, va); return *this; }
inline DSTRING& DSTRING::vsprintf(const char* fmt, va_list va) { Ctx.dstring_api->vsprintf(this, fmt, va); return *this; }
#endif
#endif

#endif /* PM123_PLUGIN_H */
