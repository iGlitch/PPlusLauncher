#pragma once
#include <stdint.h>
#include <sys/types.h>

typedef uint8_t u_char;
int bsdiff(u_char* old, off_t oldsize, u_char* _new, off_t newsize, FILE * pf, off_t headeroffset);
