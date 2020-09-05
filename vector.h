#ifndef _CADT_VECTOR
#define _CADT_VECTOR

#include "cadt.h"
#include <stddef.h>
#include <stdlib.h>
#define SZ_LEN_RATIO 1.65
#define SHRINK_THRESHOLD 2

typedef struct CADT_Vec {
  struct {
    size_t len;   /* length of the buffer */
    size_t size;  /* amount of element currently stored. */
    size_t memsz; /* size of the type stored */
  } meta;
  void *buf; /* buffer for storage */
} CADT_Vec;

#endif /* ifndef SYMBOL */
