typedef struct
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

size_t _System _fread(void *buffer, size_t size, size_t count, FILE *stream);
long _System _ftell(FILE *stream);
FILE *_System _fopen (const char *fname, const char *mode, int sockmode, int buffersize, int bufferwait);
int _System _fclose (FILE *stream);
int _System _fseek(FILE *stream, long offset, int origin);
void _System _rewind (FILE *stream);
size_t _System _fsize(FILE *stream);

int _System sockfile_errno(int sockmode);
int _System sockfile_bufferstatus(FILE *stream);
int _System sockfile_nobuffermode(FILE *stream, int setnobuffermode);
int _System sockfile_abort(FILE *stream);
int _System sockfile_httpinfo(FILE *stream, HTTP_INFO *http_info);
