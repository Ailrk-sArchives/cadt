/* This dictionary implementation use open space addressing to handle
 * collisions. Large collision can drastically decrease the performance
 * of such technique, so the collision threshold is as small as 256
 * if there are more collisions the dictionary needs to be resized */
#include "dict.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define CADT_DICT_RESIZE_THRESHOLD 0.8
/* on resize the buffer will growth by 4 times before
 * it reach size of 50000 * 8 bytes (this is the fast growth phase).
 * When the size exceed the threshold the growth slow down to 2 times
 * per resize. This is similar to the behaviour of Python dictionary */
#define CADT_DICT_FAST_GROWTH_SZ_LIMIT 50000 * (sizeof(void *))
#define CADT_DICT_FAST_GROWTH_RATE 4
#define CADT_DICT_SLOW_GROWTH_RATE 2
/* minimum size of dictionary buffer */
#define CADT_DICT_MIN_SZ 64
#define EMPTY_ITEM 0

/* -- hash function -- */
/* Using FNV-1a hash function. othe candidates are Murmur and FNV-1
 * Murmur has better specs, but it use 4 byte hashing. It is hard to use
 * it for a general purpose hashing function. FNV-1 is a little bit slower. */

/* FNV-1a 64 bit hash function */
static inline uint64_t fnv1a_1byte(const uint8_t byte, uint64_t hash) {
  const static uint64_t prime = 0x100000001b3;
  return (byte ^ hash) * prime;
}

static uint64_t hash(const void *const data, size_t nbyte) {
  /* FNV ofset basis */
  uint64_t hash = 0xcbf29ce484222325;
  const uint8_t *ptr = (const uint8_t *)data;
  while (nbyte--) {
    hash = fnv1a_1byte(*ptr, hash);
    ptr++;
  }
  return hash;
}

static size_t dhashaddr(const CADT_Dict *const d, const void *const key) {
  return hash(key, d->keysz) % d->len;
}

/* check if a block is empty */
static size_t dempty(const CADT_Dict *const d, const size_t idx) {
  unsigned char *buf = (unsigned char *)d->entries + idx;
  return buf[0] == 0 && !memcmp(buf, buf + 1, d->len - 1);
}

static size_t dbufitemsz(const CADT_Dict *const d) {
  return d->size + d->valsz;
}

static size_t dbufmemspace(const CADT_Dict *const d) {
  return dbufitemsz(d) * d->len;
}

static void *dkey(const CADT_Dict *const d, const size_t idx) {
  unsigned char *ptr = (unsigned char *)d->entries + idx;
  return (void *)ptr;
}

static void *dval(const CADT_Dict *const d, const size_t idx) {
  unsigned char *ptr = (unsigned char *)d->entries + idx + d->keysz;
  return (void *)ptr;
}

static CADT_Dict *dictmalloc(const size_t size, const size_t keysz,
                             const size_t valsz) {
  const size_t itemsz = keysz + valsz;
  CADT_Dict *d = (CADT_Dict *)malloc(sizeof(CADT_Dict));
  d->size = size;
  d->keysz = keysz;
  d->valsz = valsz;
  if (itemsz * size < CADT_DICT_MIN_SZ) {
    d->len = (size_t)(CADT_DICT_MIN_SZ / itemsz);
  } else {
    d->len = 2 * size;
  }
  d->entries = (Item_)malloc(itemsz * d->len);
  memset(d->entries, EMPTY_ITEM, dbufmemspace(d));
  return d;
}

static int dbufresize(CADT_Dict *d) {
  if (d->collisions > 0 && d->size / d->len < CADT_DICT_RESIZE_THRESHOLD) {
    return -1;
  }
  if (d->size < CADT_DICT_FAST_GROWTH_SZ_LIMIT) {
    d->len = d->len * CADT_DICT_FAST_GROWTH_RATE;
  } else {
    d->len = d->len * CADT_DICT_SLOW_GROWTH_RATE;
  }
  const size_t newlen = sizeof(dbufitemsz(d)) * d->len;
  d->entries = (Item_)realloc(d->entries, newlen);
  return 1;
}

/* open addressing to resolve collision. get new address by
 * double hashing */
static size_t dput(CADT_Dict *const d, Item_ item) {
  size_t idx = dhashaddr(d, item);
  while (!dempty(d, idx)) {
    idx = hash(&idx, sizeof(size_t)) % d->len;
  }
  unsigned char *ptr = (unsigned char *)d->entries + idx;
  memmove(ptr, item, dbufitemsz(d));
  free(item);
  return idx;
}

static size_t dlookup(const CADT_Dict *const d, const void *const key) {
}

CADT_Dict *CADT_Dict_new(const size_t keysz, const size_t valsz) {
  CADT_Dict *dict = dictmalloc(keysz, valsz, 0);
  return dict;
}

void CADT_Dict_put(CADT_Dict *d, const CADTDictKey *key, CADTDictVal *val) {
  const size_t itemsz = dbufitemsz(d);
  unsigned char item[itemsz];
  memcpy(item, key, d->keysz);
  memcpy(&item[d->keysz], val, d->valsz);
  dput(d, item);
}

CADTDictVal *CADT_Dict_get(CADT_Dict *, const CADTDictKey *key);
CADTDictVal *CADT_Dict_has(CADT_Dict *, const CADTDictKey *key);
size_t *CADT_Dict_update(CADT_Dict *, CADT_Dict *);
size_t *CADT_Dict_remove(CADT_Dict *, const CADTDictKey *const key);
