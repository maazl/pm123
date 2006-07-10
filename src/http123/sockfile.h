typedef struct _SOCKFILE
{
   /* normal file stuff */
   FILE *stream;

   /* http stuff */
   int position;
   int handle;
   int sockmode;

   HTTP_INFO http_info;

   /* read ahead stuff */
   char *buffer;
   int buffersize;
   int available;
   int bufferpos;
   int bufferthread;
   int nobuffermode;
   HEV fillbuffer;
   HEV bufferfilled;
   HEV bufferchanged;
   HMTX accessbuffer;
   HMTX accessfile;
   int justseeked;

} SOCKFILE;

#define HTTP 1 /* sockmode, 0 = usual local file access */
#define FTP  2

size_t PM123_ENTRY xio_fread ( void* buffer, size_t size, size_t count, FILE* stream );
long   PM123_ENTRY xio_ftell ( FILE* stream );
FILE*  PM123_ENTRY xio_fopen ( const char* fname, const char* mode, int sockmode, int buffersize, int bufferwait );
int    PM123_ENTRY xio_fclose( FILE* stream );
int    PM123_ENTRY xio_fseek ( FILE* stream, long offset, int origin );
void   PM123_ENTRY xio_rewind( FILE* stream );
size_t PM123_ENTRY xio_fsize ( FILE* stream );

int PM123_ENTRY sockfile_errno( int sockmode );
int PM123_ENTRY sockfile_bufferstatus( FILE* stream );
int PM123_ENTRY sockfile_nobuffermode( FILE* stream, int setnobuffermode );
int PM123_ENTRY sockfile_abort( FILE* stream );
int PM123_ENTRY sockfile_httpinfo( FILE* stream, HTTP_INFO* http_info );
