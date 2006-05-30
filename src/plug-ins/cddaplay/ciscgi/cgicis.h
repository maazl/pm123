#if __cplusplus
extern "C" {
#endif

/* see
cgifldld.c
cgiupper.c
cisrcl.h
urldcode.c
urlncode.c
urlvalue.c
*/

void cgifldld(char *src, char *txt, char *dst);
void cgiupper(char *s);
int loadcgi(struct cgi_data *cgd);
int urldcode(char *str);
int urlncode(char *src, char *dst, int size);
int urlvalue(char *src, char *dst, int size);

#if __cplusplus
}
#endif

