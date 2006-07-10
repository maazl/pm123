typedef struct
{
   int length;
   int icy_metaint;
   char icy_genre[128];
   char icy_name[128];

} HTTP_INFO;

char* PM123_ENTRY http_strerror(void); /* thread specific */
int   PM123_ENTRY writebuffer (char *buffer, int size, int handle);
int   PM123_ENTRY readbuffer (char *buffer, int size, int handle);
int   PM123_ENTRY readline (char *line, int maxlen, int handle);
int   PM123_ENTRY http_open (const char *url, HTTP_INFO *http_info);
int   PM123_ENTRY http_close (int socket);
void  PM123_ENTRY set_proxyurl( const char* url  );
void  PM123_ENTRY set_httpauth( const char* auth );

