/*
 * Copyright 2008-2011 M.Mueller
 * Copyright 2006 Dmitry A.Steklenev
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

#ifndef XIO_PROTOCOL_H
#define XIO_PROTOCOL_H

#include <config.h>

#include <cpp/cpputil.h>
#include <cpp/mutex.h>

struct _XSTAT;

enum XOFLAGS
{ XO_NONE          = 0x0000,
  XO_WRITE         = 0x0001,
  XO_READ          = 0x0002,
  XO_CREATE        = 0x0004,
  XO_APPEND        = 0x0008,
  XO_TRUNCATE      = 0x0010,
  XO_NOBUFFER      = 0x0100, // no buffering
  XO_ASYNCBUFFER   = 0x1000, // Do asynchronuous buffering
  XO_NOMUTEX       = 0x2000  // Do not synchronize access
};
FLAGSATTRIBUTE(XOFLAGS);

enum XSFLAGS
{ XS_NONE          = 0x0000,
  XS_CAN_READ      = 0x0001,
  XS_CAN_WRITE     = 0x0002,
  XS_CAN_READWRITE = 0x0004,
  XS_CAN_CREATE    = 0x0008,
  XS_CAN_SEEK      = 0x0010,
  XS_CAN_SEEK_FAST = 0x0020
  //XS_NOT_BUFFERIZE = 0x0080
};
FLAGSATTRIBUTE(XSFLAGS);


/* Base interface of all protocol implementations.
   The buffer classes also derive from this protocol and act as a proxy
   to the underlying protocol class instance. 

   Class tree:

   XPROTOCOL (abstract)
    +- XIOfile
    +- XIOreadonly (abstract)
        +- XIOhttp
        +- XIOftp
        +- XIOcddb
        +- XIObuffer (abstract)
            +- XIOsyncbuffer
            +- XIOasyncbuffer
*/
class XPROTOCOL {
 public:
  // Interface for observing meta data changes
  struct Iobserver
  { virtual ~Iobserver() {}
    virtual void metacallback(int type, const char* metabuff, long pos, long pos64) = 0;
  };
 
 public: 
  bool eof;       // End of input stream flag.
  int  error;     // Last error that appies to the stream state.
  int  blocksize; // Recommended Blocking factor of the protocol. Filled by Protocol implementation.

 protected:
  Iobserver*      observer;
 
 public:
                  XPROTOCOL();
  virtual         ~XPROTOCOL() {}

  /* Note: All methods of the protocol are serialized by library
     except for close, tell, getsize, get_metainfo and supports. */

  /* Opens the file specified by filename. Returns 0 if it
     successfully opens the file. A return value of -1 shows an error. */
  virtual int     open( const char* filename, XOFLAGS oflags ) = 0;

  /* Reads count bytes from the file into buffer. Returns the number
     of bytes placed in result. The return value 0 indicates an attempt
     to read at end-of-file. A return value -1 indicates an error.
     Precondition: count > 0 && !error && !eof && XO_READ */
  virtual int     read( void* result, unsigned int count ) = 0;

  /* Reads up to n-1 characters from the stream or stop at the first
     new line. CR characters (\r) are discarded.
     Precondition: n > 1 && !error && !eof && XO_READ */
  virtual char*   gets( char* string, unsigned int n );

  /* Writes count bytes from source into the file. Returns the number
     of bytes moved from the source to the file. The return value may
     be positive but less than count. A return value of -1 indicates an
     error.
     Precondition: count > 0 && !error && XO_WRITE */
  virtual int     write( const void* source, unsigned int count ) = 0;

  /* Copies string to the output file at the current position.
     It does not copy the null character (\0) at the end of the string.
     Returns -1 if an error occurs; otherwise, it returns a non-negative
     value.
     Precondition: !error && XO_WRITE */
  virtual int     puts( const char* string );

  /* Closes the file. Returns 0 if it successfully closes the file. A
     return value of -1 shows an error. */
  virtual int     close() = 0;

  /* Returns the current position of the file pointer. The position is
     the number of bytes from the beginning of the file. On devices
     incapable of seeking, the return value is -1L. */
  virtual long    tell( long* offset64 = NULL ) = 0;

  /* Moves any file pointer to a new location that is offset bytes from
     the origin. Returns the offset, in bytes, of the new position from
     the beginning of the file. A return value of -1L indicates an
     error. */
  virtual long    seek( long offset, int origin, long* offset64 = NULL ) = 0;

  /* Returns the size of the file. A return value of -1L indicates an
     error or an unknown size. */
  virtual long    getsize( long* offset64 = NULL ) = 0;

  /* Lengthens or cuts off the file to the length specified by size.
     You must open the file in a mode that permits writing. Adds null
     characters when it lengthens the file. When cuts off the file, it
     erases all data from the end of the shortened file to the end
     of the original file. Returns the value 0 if it successfully
     changes the file size. A return value of -1 shows an error.
     Precondition: XO_WRITE */
  virtual int     chsize( long size, long offset64 = 0 ) = 0;

  /* Retrieve stat information of the current file */
  virtual int     getstat( _XSTAT* st ) = 0;

  /* Returns a specified meta information if it is provided by associated stream.
     The default implementation always return "".
     Precondition: size > 0 */
  virtual char*   get_metainfo( int type, char* result, int size );

  /* Set an observer that is called whenever a source updates the metadata.
     The function returns the previously set observer.
     You cannot set more than one observer.
     The callback will only be executed while calling read(). */ 
  virtual Iobserver* set_observer( Iobserver* observer );

  /* Return the supported properties of the current protocol */
  virtual XSFLAGS supports() const = 0;
};

/* Specialization of XPROTOCOL for ready only access */
class XIOreadonly : public XPROTOCOL
{
  virtual int write( const void* source, unsigned int count );
  virtual int chsize( long size, long offset64 = 0 );
};

/* public C-style class interface */
typedef struct _XFILE {
  //int scheme;  // is write only
  XOFLAGS       oflags;

  XPROTOCOL*    protocol;   /* Backend                                       */

  int           serial;     /* Magic number to avoid accidential use bad handle. */
  
  Mutex*        mtx;        /* Serializes most i/o operations.               */
  bool          in_use;     /* Flags instance as used in unsynchronized mode */

  /* Initializes the file structure.
   * This does not open anything or assign a protocol. */
  _XFILE();
  /* Cleanups the file structure. */
  ~_XFILE();

  bool          Request();  /* Lock the current instance                     */
  void          Release();  /* Release the current instance                  */

} XFILE;

#endif /* XIO_FILE_H */

