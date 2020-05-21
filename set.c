#include "set.h"
#include <stdint.h>

/* set use the same resize strategy as dictionary. */
#define CADT_SET_RESIZE_THRESHOLD 0.8
#define CADT_SET_FAST_GROWTH_SZ_LIMIT 50000 * (sizeof(void *))
#define CADT_SET_FAST_GROWTH_RATE 4
#define CADT_SET_SLOW_GROWTH_RATE 2
#define CADT_SET_MIN_SZ 64
#define EMPTY_ITEM 0


/* fnv-1a 64 bit hash function */
static inline uint64_t fnv1a_1byte(const uint8_t byte, uint64_t hash) {
  const static uint64_t prime = 0x100000001b3;
  return (byte ^ hash) * prime;
}

static uint64_t hash(const void *const data, size_t nbyte) {
  /* fnv ofset basis */
  uint64_t hash = 0xcbf29ce484222325;
  const uint8_t *ptr = (const uint8_t *)data;
  while (nbyte--) {
    hash = fnv1a_1byte(*ptr, hash);
    ptr++;
  }
  return hash;
}


CADT_Set *smalloc(const size_t memsz) {
}

CADT_Set *CADT_Set_new(const size_t memsz) {

}
size_t *CADT_Set_add(CADT_Set *s, void *const val) {

}
size_t *CADT_Set_remove(CADT_Set *, void *const val);
CADT_Set *CADT_Set_union(CADT_Set *, CADT_Set *);
CADT_Set *CADT_Set_intersect(CADT_Set *, CADT_Set *);
CADT_Set *CADT_Set_compliment(CADT_Set *sub, CADT_Set *s);
size_t CADT_Set_size(const CADT_Set *const);
size_t CADT_Set_issubset(const CADT_Set *const sub, const CADT_Set *const s);


#undef CADT_SET_RESIZE_THRESHOLD
#undef CADT_SET_FAST_GROWTH_SZ_LIMIT
#undef CADT_SET_FAST_GROWTH_RATE
#undef CADT_SET_SLOW_GROWTH_RATE
#undef CADT_SET_MIN_SZ
#undef EMPTY_ITEM
