/*
	#libprofile (Tiny code profiler for various game consoles)
 
	types.h
 
	Copyright (c) 2010, Ian Callaghan <ian (at) unrom (dot) com>

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
	documentation files (the "Software"), to deal in the Software without restriction, including without
	limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
	the Software, and to permit persons to whom the Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial
	portions of the Software.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
	LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
	EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
	AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
	OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef __TYPES_H__
#define __TYPES_H__

typedef unsigned char				u8;
typedef unsigned short				u16;
typedef unsigned int				u32;
typedef unsigned long long			u64;

typedef signed char					s8;
typedef signed short				s16;
typedef signed int					s32;
typedef signed long long			s64;

typedef volatile unsigned char		vu8;
typedef volatile unsigned short		vu16;
typedef volatile unsigned long		vu32;
typedef volatile unsigned long long vu64;

typedef volatile signed char		vs8;
typedef volatile signed short		vs16;
typedef volatile signed long		vs32;
typedef volatile signed long long	vs64;

typedef float						f32;
typedef volatile float		        vf32;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define NULL ((void *)0)
#define ALIGNED(n) __attribute__((aligned(n)))

#endif

