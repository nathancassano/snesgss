#include <stdlib.h>

#ifndef COMMON_H
#define COMMON_H
  /*
#ifndef SKIP_SAFE_MALLOC
void *safe_malloc(const size_t s)
{
	void *p = malloc(s);
	if(!p)
	{
		fprintf(stderr, "Can't allocate enough memory.\n");
		exit(1);
	}
	return p;
}
#else  */
void *safe_malloc(const size_t s) { return malloc(s); }
/*#endif */

#define PI 3.14159265

#define MAX(a, b) ((a)>(b) ? (a) : (b))

typedef signed short pcm_t;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#endif