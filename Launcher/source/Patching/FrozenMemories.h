#ifndef _FROZENMEMORIES_H_
#define _FROZENMEMORIES_H_

#include <gctypes.h>

//#define BUFSIZE 1024*1024*13
#define RELEASE_PTR(p) {if(p){free(p);p=NULL;}}
extern u8* buffers[3];
#endif