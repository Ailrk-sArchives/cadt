#include "cadt.h"
#include <stddef.h>

#define CADT_DICT_ITMESZ = (sizeof(void *) * 2)
#define CADT_DICT_RESIZE_THRESHOLD 0.8
/* on resize the buffer size will growth by 4 times before
 * it reach size of 50000 * 8 bytes. This is the fast growth phase.
 * When the size exceed the threshold the growth slow down to 2 times
 * per resize. This is similar to the behaviour of Python dictionary */
#define CADT_DICT_FAST_GROWTH_SZ_LIMIT 50000 * (sizeof(void *))
#define CADT_DICT_FAST_GROWTH_RATE 4
#define CADT_DICT_SLOW_GROWTH_RATE 2
/* minimum size of dictionary buffer */
#define CADT_DICT_MIN_SZ 64

typedef struct CADT_Dict_Item_ {
  void *key;
  void *value;
} CADT_Dict_Item_;

typedef struct CADT_Dict {
  CADT_Dict_Item_ *entries;
  size_t len;         /* entries length */
  size_t size;        /* number of element stored */
  size_t keysz;
  size_t valsz;
  int collisions : 8; /* resize when there is more than 256 collisions */
} CADT_Dict;
