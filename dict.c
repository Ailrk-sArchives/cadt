/* This dictionary implementation use open space addressing to handle
 * collisions. Large collision can drastically decrease the performance
 * of such technique, so the collision threshold is as small as 256
 * if there are more collisions the dictionary needs to be resized */
#include "dict.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

/* -- helper functions -- */

/* hash address within d->len */
static size_t dhash_addr(const CADT_Dict *const d, const void *const key) {
  return hash(key, d->keysz) % d->len;
}

static size_t ditemsz(const CADT_Dict *const d) { return d->size + d->valsz; }
static unsigned char *dkey(unsigned char *const item) { return item; }
static unsigned char *dval(unsigned char *const item, const size_t offset) {
  return item + offset;
}
/* return an unsigned char * point to the given idx of entries in d */
static Item_ ditem(CADT_Dict *const d, const size_t idx) {
  return (unsigned char *)d->entries + idx * ditemsz(d);
}

/* check if a block is empty */
static size_t dempty(const CADT_Dict *const d, const size_t idx) {
  unsigned char *buf = (unsigned char *)d->entries + idx;
  return buf[0] == 0 && !memcmp(buf, buf + 1, d->len - 1);
}

static size_t dbufmemspace(const CADT_Dict *const d) {
  return ditemsz(d) * d->len;
}

/* to check if d[idx] has the same key as item
 * note: if item has empty it return false */
static int hassamekey(const Item_ item, const unsigned char *const key,
                      const size_t keysz) {
  return (!memcmp(key, item, keysz));
}

/* -- addressing and mem management -- */

/* open addressing ignored mode */
static size_t dfind_open_addri(CADT_Dict *const d, size_t idx) {
  assert(d != NULL);
  while (!dempty(d, idx)) {
    idx = hash(&idx, sizeof(size_t)) % d->len;
    d->collisions += 1;
  }
  return idx;
}

/* open addressing overwrite mode.
 * it return the address when block contains the same key. */
static size_t dfind_open_addrow(CADT_Dict *const d, size_t idx,
                               const unsigned char *const key) {
  assert(d != NULL);
  while (!dempty(d, idx)) {
    idx = hash(&idx, sizeof(size_t)) % d->len;
    if (hassamekey(ditem(d, idx), key, d->keysz)) {
      return idx;
    }
    d->collisions += 1;
  }
  return idx;
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

/* resize either because size is greater than  80% of entries length or
 * because it has accumulated more than 256 collisions */
static int dbufresize(CADT_Dict *d) {
  assert(d != NULL);
  if (d->collisions >= 0 && d->size / d->len < CADT_DICT_RESIZE_THRESHOLD) {
    return -1;
  }
  /* reset collision counter */
  if (d->collisions < 0) {
    d->collisions = 0;
  }
  if (d->size < CADT_DICT_FAST_GROWTH_SZ_LIMIT) {
    d->len = d->len * CADT_DICT_FAST_GROWTH_RATE;
  } else {
    d->len = d->len * CADT_DICT_SLOW_GROWTH_RATE;
  }
  const size_t newlen = sizeof(ditemsz(d)) * d->len;
  d->entries = (Item_)realloc(d->entries, newlen);
  return 1;
}

/* open addressing to resolve collision. get new address by
 * double hashing */
static size_t dput(CADT_Dict *const d, const Item_ item, CADTDictMode mode) {
  assert(d != NULL);
  size_t idx;
  unsigned char *key = dkey(item);
  if (mode == IGNORE) {
    idx = dfind_open_addri(d, dhash_addr(d, key));
  }
  else if (mode == OVERWRITE) {
    idx = dfind_open_addrow(d, dhash_addr(d, key), key);
  } else {
    return 0;
  }
  dbufresize(d);
  unsigned char *ptr = ditem(d, idx);
  memcpy(ptr, item, ditemsz(d));
  free(item);
  d->size += 1;
  return idx;
}

/* lookup element with open addressing.
 * the returned pointer point to */
static void *dlookup(CADT_Dict *const d, const void *const key) {
  assert(d != NULL);
  unsigned char *val = NULL;
  size_t idx = dhash_addr(d, key);
  unsigned char *item = ditem(d, idx);
  while (memcmp(item, key, d->keysz)) {
    idx = hash(&idx, sizeof(size_t)) % d->len;
    /* if hit empty a empty block means key not found */
    if (dempty(d, idx)) {
      return NULL;
    }
    item = ditem(d, idx);
  }
  val = dval(item, d->keysz);
  return (void *)val;
}

/* -- interface -- */

CADT_Dict *CADT_Dict_new(const size_t keysz, const size_t valsz) {
  CADT_Dict *dict = dictmalloc(keysz, valsz, 0);
  return dict;
}

void CADT_Dict_put(CADT_Dict *d, const CADTDictKey *key, CADTDictVal *val,
                   CADTDictMode mode) {
  const size_t itemsz = ditemsz(d);
  unsigned char item[itemsz];
  memcpy(item, key, d->keysz);
  memcpy(&item[d->keysz], val, d->valsz);
  dput(d, item, mode);
}

CADTDictVal *CADT_Dict_get(CADT_Dict *d, const CADTDictKey *key) {
  return dlookup(d, key);
}

/* expand d1 with elements in d2 */
size_t CADT_Dict_update(CADT_Dict *d1, CADT_Dict *d2, CADTDictMode mode) {
  if (d1 == NULL || d2 == NULL) {
    return 0;
  }
  const size_t itemsz = ditemsz(d1);
  for (int i = 0; i < d2->len; i++) {
    if (!dempty(d2, i)) {
      unsigned char *top = ditem(d2, i);
      CADT_Dict_put(d1, dkey(top), dval(top, d2->keysz), mode);
    }
  }
  return d1->size;
}

/* because it is open addressing, keys cannot be delete on remove
 * otherwise the collision resolution will no longer be the same as
 * the original dictionary. */
size_t CADT_Dict_remove(CADT_Dict *d, const CADTDictKey *const key) {}
