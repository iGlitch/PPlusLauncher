#ifndef _COMMON_H_
#define _COMMON_H_

#include <string.h>

#ifdef WIN32
#define BREAK ;
#elif HW_RVL
#include <debug.h>
//Debugging macro
#define BREAK _break();
#endif

void loadInfoFile();

//_____Path handling_____
const char* GetFileName(const char* path);
const char* GetExtension(const char* filepath);
void GetDirectory(const char* src, char* buf);

//_____General string handling_____
bool Contains(const char* target, const char* str);
void ToLower(const char* str, char* buf);
//Declarate string
#define DECLSTR(ident) char ident[256];memset(ident,0,sizeof(ident));
//Declarate and initialize string
#define INITSTR(ident,src) char ident[256];memset(ident,0,sizeof(ident));strcpy(ident,src);
//Concatenate strings into one string
#define CATSTR(ident,first,second) char ident[256];memset(ident,0,sizeof(ident));strcpy(ident,first);strcat(ident,second);
#endif