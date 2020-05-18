#include "cadt.h"
#include <stddef.h>

typedef unsigned char *Item_;

/* use a consecutive array to store both key and data. each element is
 * a key value tuple. Use a offset and type conversion to get value. */
typedef struct CADT_Dict {
  Item_ entries; /* each block is a (key, val) tuple. */
  size_t len;     /* entries length */
  size_t size;    /* number of element stored */
  size_t keysz;
  size_t valsz;
  int collisions : 8; /* resize when there is more than 256 collisions */
} CADT_Dict;
