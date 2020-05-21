#ifndef _CADT_SET
#define _CADT_SET
#include "cadt.h"

typedef struct CADT_Set {
  void *entries;
  size_t len;
  size_t size;
  size_t memsz;
} CADT_Set;

#endif /* ifndef _CADT_SET */

