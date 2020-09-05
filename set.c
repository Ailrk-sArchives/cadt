#include "set.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


/* set use the same resize strategy as dictionary. */
#define CADT_SET_RESIZE_THRESHOLD 0.8
#define CADT_SET_FAST_GROWTH_SZ_LIMIT 50000 * (sizeof(void *))
#define CADT_SET_FAST_GROWTH_RATE 4
#define CADT_SET_SLOW_GROWTH_RATE 2
#define CADT_SET_MIN_MEMSZ 64
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


static unsigned char *svalue(const CADT_Set *const d, const size_t idx) {
  return (unsigned char *)d->entries + idx * d->meta.memsz;
}


static CADT_Set *salloc(const size_t memsz) {
  CADT_Set *set = (CADT_Set *)malloc(sizeof(CADT_Set));
  set->meta.len = (size_t)(CADT_SET_MIN_MEMSZ / memsz);
  set->meta.size = 0;
  set->meta.memsz = memsz;
  set->entries = malloc(memsz * set->meta.len);
  return set;
}


static bool sempty(const CADT_Set *const s, const size_t idx) {
  unsigned char *val = svalue(s, idx);
  return val[0] == EMPTY_ITEM && !memcmp(val, val + 1, s->meta.memsz);
}


CADT_Set *CADT_Set_new(const size_t memsz) { return salloc(memsz); }


/* find available address for key */
static unsigned char *sfind_openaddr(CADT_Set *s, size_t idx,
                                     const void *const key) {
  assert(s != NULL);
  while (!sempty(s, idx)) {
    idx = hash(&idx, sizeof(size_t)) % s->meta.len;
    unsigned char *value = svalue(s, idx);
    if (!memcmp(value, key, s->meta.memsz)) {
      return value;
    }
    s->collisions += 1;
  }
  return NULL;
}


static unsigned char *sget(CADT_Set *s, const void *const key) {
  assert(s != NULL);
}


bool CADT_Set_add(CADT_Set *s, void *const val) {
  assert(s != NULL);
  unsigned char *ptr = sfind_openaddr(s, hash(val, s->meta.memsz) % s->meta.len, val);
  if (ptr == NULL) {
    return false;
  }
  s->meta.size += 1;
  // TODO resize

  memcpy(ptr, val, s->meta.memsz);
  return true;
}


size_t *CADT_Set_remove(CADT_Set *, void *const key){

};


CADT_Set *CADT_Set_union(CADT_Set *, CADT_Set *){

};


CADT_Set *CADT_Set_intersect(CADT_Set *a, CADT_Set *b) {
  if (a->meta.memsz != b->meta.memsz) {
    return NULL;
  }
  CADT_Set *set = salloc(a->meta.memsz);
  const size_t len = a->meta.len < b->meta.len ? a->meta.len : b->meta.len;
  for (size_t i = 0; i < len; i++) {
  }
};


CADT_Set *CADT_Set_compliment(CADT_Set *sub, CADT_Set *s){

};


size_t CADT_Set_size(const CADT_Set *const){

};


size_t CADT_Set_issubset(const CADT_Set *const sub, const CADT_Set *const s){

};

#undef CADT_SET_RESIZE_THRESHOLD
#undef CADT_SET_FAST_GROWTH_SZ_LIMIT
#undef CADT_SET_FAST_GROWTH_RATE
#undef CADT_SET_SLOW_GROWTH_RATE
#undef CADT_SET_MIN_MEMSZ
#undef EMPTY_ITEM
