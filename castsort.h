/*
	Copyright (C) 2014-2022 Igor van den Hoven ivdhoven@gmail.com
*/

/*
	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the
	"Software"), to deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to
	permit persons to whom the Software is furnished to do so, subject to
	the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
	fluxsort 1.2.1.1
*/

#ifndef FLUXSORT_H
#define FLUXSORT_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <float.h>

typedef int CMPFUNC (const void *a, const void *b);

typedef double VALFUNC(const void *v);

//#define cmp(a,b) (*(a) > *(b))

#ifndef QUADSORT_H
  //#include "quadsort.h"
#endif

// When sorting an array of 32/64 bit pointers, like a string array, QUAD_CACHE
// needs to be adjusted in quadsort.h and here for proper performance when
// sorting large arrays.

#ifdef cmp
  #define QUAD_CACHE 4294967295
#else
//#define QUAD_CACHE 131072
  #define QUAD_CACHE 262144
//#define QUAD_CACHE 524288
//#define QUAD_CACHE 4294967295
#endif

typedef struct mem_st {
    size_t start;
    size_t nb;
    size_t act;
    double max;
    double min;
} mem_t;

typedef struct {
    double origin_val;
    double transformed_val;
    size_t final_index;
} value_t;

//////////////////////////////////////////////////////////////////////////
//┌────────────────────────────────────────────────────────────────────┐//
//│ ██████┐ █████┐ ███████┐████████┐███████┐ ██████┐ ██████┐ ████████┐ │//
//│██┌────┘██┌──██┐██┌────┘└──██┌──┘██┌────┘██┌───██┐██┌──██┐└──██┌──┘ │//
//│██│     ███████│███████┐   ██│   ███████┐██│   ██│██████┌┘   ██│    │//
//│██│     ██┌──██│└────██│   ██│   └────██│██│   ██│██┌──██┐   ██│    │//
//│└██████┐██│  ██│███████│   ██│   ███████│└██████┌┘██│  ██│   ██│    │//
//│ └─────┘└─┘  └─┘└──────┘   └─┘   └──────┘ └─────┘ └─┘  └─┘   └─┘    │//
//└────────────────────────────────────────────────────────────────────┘//
//////////////////////////////////////////////////////////////////////////
void castsort(void *toSort, void *sorted, mem_t *work_mem, size_t nmemb, size_t size, VALFUNC *cast);
int lower(const void *a, const void *b);


#undef QUAD_CACHE

#endif
