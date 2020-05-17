#include "cadt.h"
#include <stddef.h>

typedef struct CADT_Dict_Item_ {
  void *key;
  void *value;
} CADT_Dict_Item_;

typedef struct CADT_Dict {
  CADT_Dict_Item_ *entries;
  size_t size;
} CADT_Dict;


