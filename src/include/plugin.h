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
#define PLUGIN_INTERFACE_LEVEL 3
#else
#ifndef PLUGIN_INTERFACE_LEVEL
#define PLUGIN_INTERFACE_LEVEL 0 /* default interface level */
#endif
#endif


#ifndef __cplusplus
/** Dynamically allocated, reference counted string.
 * It is wrapped by a structure to provide type safety, but
 * read access to the embedded C style string is always safe.
 *
 * Note that you MUST initialize these strings to NULL.
 * Avoid copying xstring objects binary. Use xstring_copy instead.
 * The difference is that with xstring_copy both instances must be freed.
 * With a binary copy exactly one instance must be freed. And since any assignment
 * implicitly frees the previous content this is error prone.
 *
 * You must not access the embedded cstr member of volatile instances.
 * This would cause undefined behavior with race conditions.
 */
typedef struct xstring
{ const char* cstr;   /* pointer to C style null terminated string */
} xstring;
static const xstring xstring_NULL = { NULL };
#elif defined(PM123_CORE)
struct xstring; // Keep CDT happy
#define xstring_NULL xstring()
#else
struct xstring;
#endif

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(4)

/** Plug-in result codes. */
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

/** Plug-in types */
typedef enum
{ PLUGIN_NULL     = 0x00,
  PLUGIN_VISUAL   = 0x01,
  PLUGIN_FILTER   = 0x02,
  PLUGIN_DECODER  = 0x04,
  PLUGIN_OUTPUT   = 0x08
} PLUGIN_TYPE;

/** Message severity levels. */
typedef enum
{ /** Informational message */
  MSG_INFO,
  /** Warning, something went wrong */
  MSG_WARNING,
  /** Error */
  MSG_ERROR
} MESSAGE_TYPE;

/** file dialog additional flags for \c ulUser field. */
enum FD_UserOpts
{ FDU_NONE       = 0x00 /**< default flags */
, FDU_DIR_ENABLE = 0x01 /**< enable directory selection */
, FDU_FILE_ONLY  = 0x02 /**< hide directory selection */
, FDU_DIR_ONLY   = 0x03 /**< hide file selection */
, FDU_RECURSEBTN = 0x04 /**< 'recurse into sub directories' button */
, FDU_RECURSE_ON = 0x08 /**< 'recurse into sub directories' button selected */
, FDU_RELATIVBTN = 0x10 /**< 'relative path' button */
, FDU_RELATIV_ON = 0x20 /**< 'relative path' button selected */
};

/** return value of plugin_query. */
typedef struct _PLUGIN_QUERYPARAM
{ int         type;     /**< PLUGIN_*. values can be ORed */
  const char* author;   /**< Author of the plug-in        */
  const char* desc;     /**< Description of the plug-in   */
  int         configurable;/**< Is the plug-in configurable*/
  int         interface;/**< Interface revision           */
} PLUGIN_QUERYPARAM;

/** Common services of the PM123 core for plug-ins. */
typedef struct
{ /** message function */
  void DLLENTRYP(message_display)(MESSAGE_TYPE type, const char* msg);

  /** Retrieve configuration setting
   * @param key Name of the configuration parameter.
   * @param data Target buffer where the data should be stored.
   * @param maxlength Maximum number of bytes to store in \a *data.
   * You may pass \c 0 to query the data size.
   * @return length of the data or -1 if the key does not exist.
   * When the data size exceeds \a maxlength then the real data length is returned
   * but only \a maxlength bytes are stored. */
  int DLLENTRYP(profile_query)(const char* key, void* data, int maxlength);
  /** Store configuration setting
   * @param key Name of the configuration parameter.
   * @param data Data to write in the profile. Pass \c NULL to delete a key.
   * @param len Length of the data in \a *data.
   * @return \c 0 on error */
  int DLLENTRYP(profile_write)(const char* key, const void* data, int length);

  /** Execute remote command
   * See the documentation of remote commands for a description. */
  const char* DLLENTRYP(exec_command)(const char* cmd);
  /** Invalidate object properties
   * @param what A bit-vector of INFOTYTE.
   * @return Bits of \a what that caused an invalidation. */
  int DLLENTRYP(obj_invalidate)(const char* url, int what);
  /** Check whether a decoder claims to support this kind of URL and type.
   * @param url URL of the object to check
   * @param type .type extended attribute or mime type respectively.
   * Multiple types may be tab separated.
   * @return true -> yes
   * @remarks The function does not actually cause any I/O.
   * It is not reliable during plug-in initialization. */
  int DLLENTRYP(obj_supported)(const char* url, const char* type);

  /** @brief Invoke PM123's resizable file dialog.
   * @details This function creates and displays the file dialog
   * and returns the user's selection or selections.
   * Important note: \c filedialog->pszIType must point
   * to a \e writable string of at least \c _MAX_PATH bytes.
   * It contains the selected type on return.
   * The ulUser field can be set to any FD_UserOpts values.
   */
  HWND DLLENTRYP(file_dlg)(HWND hparent, HWND howner, struct _FILEDLG* filedialog);
} PLUGIN_API; 

typedef struct
{ /** Internal allocator, do not use directly. */
  char*    DLLENTRYP(alloc_core)(size_t s);
  /** Internal deallocator, do not use directly. */
  void     DLLENTRYP(free_core)(char* p);

  /** Initialize a new xstring with a C string */
  struct xstring DLLENTRYP(create) (const char* cstr);
  /** Deallocate dynamic string. This will change the pointer to NULL. */
  void     DLLENTRYP(free)     (volatile xstring* dst);
  /** Return the length of a dynamic string */
  unsigned DLLENTRYP(length)   (const    xstring* src);
  /** Compare two xstrings for (binary!) equality. NULL allowed. */
  char     DLLENTRYP(equal)    (const    xstring* src1, const xstring* src2);
  /** Compare two xstrings instances. NULL allowed. */
  int      DLLENTRYP(compare)  (const    xstring* src1, const xstring* src2);
  /** Copy dynamic string to another one. Any previous content is discarded first.
   * This function will not copy the string itself. It only creates an additional reference to the content. */
  void     DLLENTRYP(copy)     (volatile xstring* dst,  const xstring* src);
  /** Strongly thread safe version of xstring_copy. */
  void     DLLENTRYP(copy_safe)(volatile xstring* dst,  volatile const xstring* src);
  /** Reassign dynamic string from C string. Any previous content is discarded first. */
  void     DLLENTRYP(assign)   (volatile xstring* dst,  const char* cstr);
  /** Reassign dynamic string from C string, but only if the strings differ. */
  char     DLLENTRYP(cmpassign)(         xstring* dst,  const char* cstr);
  /** Append to xstring. The source may also be from a xstring.
   * If dst is NULL a new string is created */ 
  void     DLLENTRYP(append)   (         xstring* dst,  const char* cstr);
  /** Allocate dynamic string. Any previous content is discarded first.
   * The returned memory can be written up to len bytes until the next
   * xstring_* function call on dst. The return value is the same than
   * dst->cstr except for constness. */
  char*    DLLENTRYP(allocate) (         xstring* dst,  unsigned int len);
  /** printf into a xstring. Any previous content is discarded first. */
  void     DLLENTRYP(sprintf)  (volatile xstring* dst,  const char* fmt, ...);
  void     DLLENTRYP(vsprintf) (volatile xstring* dst,  const char* fmt, va_list va);
  /** Deduplicate xstring value im memory. */
  void     DLLENTRYP(deduplicate)(volatile xstring* dst);
} XSTRING_API;

typedef struct
{
  const PLUGIN_API*  plugin_api;
  
  const XSTRING_API* xstring_api;

} PLUGIN_CONTEXT;

extern PLUGIN_CONTEXT Ctx;

/** Query information about plug-in.
 * @return returns 0 -> ok */
int DLLENTRY plugin_query(PLUGIN_QUERYPARAM* param);
/** Initialize plug-in, i.e. load it into the player.
 * @param ctx Some services of the PM123 core.
 * @return 0 -> ok. */
int DLLENTRY plugin_init(const PLUGIN_CONTEXT* ctx);
/** Show configure dialog.
 * @param hwnd parent window.
 * @param module Module handle of the plug-in. Can be used for resource lookups. */
#if PLUGIN_INTERFACE_LEVEL >= 3
/** @return window handle of the configuration dialog if still open. */
HWND DLLENTRY plugin_configure(HWND hwnd, HMODULE module);
#else
void DLLENTRY plugin_configure(HWND hwnd, HMODULE module);
#endif
/** Send remote command to the plug-in.
 * @param command command string.
 * @param result reply.
 * @remarks You might use PLUGIN_CONTEXT->message_display during processing.
 * The error handler is redirected for the current thread if it is a remote command. */
void DLLENTRY plugin_command(const char* command, xstring* result, xstring* messages);
/** Cancel plug-in initialization. Called before the plug-in is unloaded.
 * @param unload ?
 * @return 0 -> ok */
int DLLENTRY plugin_deinit(int unload);


/****************************************************************************
 *
 * Definitions of level 1 interface
 *
 ***************************************************************************/
#if PLUGIN_INTERFACE_LEVEL < 2 || defined(PM123_CORE)

// Old style window messages for Level 1 Plug-ins only
#define WM_PLAYSTOP         (WM_USER +  69)
#define WM_PLAYERROR        (WM_USER + 100)
#define WM_SEEKSTOP         (WM_USER + 666)
#define WM_METADATA         (WM_USER +  42)
#define WM_CHANGEBR         (WM_USER +  43)
#define WM_OUTPUT_OUTOFDATA (WM_USER + 667)
#define WM_PLUGIN_CONTROL   (WM_USER + 668) /* Plugin notify message */

#define PN_TEXTCHANGED  1 /* Display text changed */

#endif /* level 1 interface */


#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* PM123_PLUGIN_H */
