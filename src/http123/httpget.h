typedef struct
{
   int length;
   int icy_metaint;
   char icy_genre[128];
   char icy_name[128];
} HTTP_INFO;

char *_System http_strerror(void); /* thread specific */

int _System writebuffer (char *buffer, int size, int handle);
int _System readbuffer (char *buffer, int size, int handle);
int _System readline (char *line, int maxlen, int handle);
int _System http_open (const char *url, HTTP_INFO *http_info);
int _System http_close (int socket);

extern char *proxyurl;
extern unsigned long proxyip;
extern char *httpauth;
