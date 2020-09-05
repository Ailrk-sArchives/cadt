#ifndef _CADT_HEAP
#include "cadt.h"

typedef struct CADT_Heap {
  void *data;
  struct {
    size_t size;
    size_t capacity;
    size_t memsz;
    CADTHeapType heap_type; // 0 min, 1 max
    int (*cmp)(const void *, const void *);  // 0 eq, 1 greater -1 smaller
  } meta;
} CADT_Heap;

#define _CADT_HEAP
#endif /* ifndef _CADT_HEAP */
