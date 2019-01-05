/*
	#libprofile (Tiny code profiler for various game consoles)
 
	timer.c
 
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

#include "types.h"
#include "profiler.h"

// This function is independant on target platform
// Currently only Wii and Gamecube is supported, both which are 32bit.

//#ifdef GAMECUBE
//#ifdef WII
u64 timer_gettime()
{
	u32 tb_upper;
	u32 tb_upper1;
	u32 tb_lower;
	u64 time;
	
	do
    {
      __asm__ __volatile__ ("mftbu %0" : "=r"(tb_upper));
      __asm__ __volatile__ ("mftb %0" : "=r"(tb_lower));
      __asm__ __volatile__ ("mftbu %0" : "=r"(tb_upper1));
    }
	while (tb_upper != tb_upper1);
	
	time = (((u64)tb_upper) << 32) | tb_lower;
	return time;
}
//#endif
