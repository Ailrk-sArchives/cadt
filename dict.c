/* This dictionary implementation use open space addressing to handle
 * collisions. Large collision can drastically decrease the performance
 * of such technique, so the collision threshold is as small as 256
 * if there are more collisions the dictionary needs to be resized */

#include "dict.h"
#include <assert.h>
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
#define CADT_DICT_MIN_MEMSZ 64
#define EMPTY_ITEM 0

/* -- hash function -- */
/* Using FNV-1a hash function. othe candidates are Murmur and FNV-1
 * Murmur has better specs, but it use 4 byte hashing. It is hard to use
 * it for a general purpose hashing function. FNV-1 is a little bit slower. */

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

/* -- helper functions -- */

/* hash address within d->len */
static size_t dhash_idx(const CADT_Dict *const d, const void *const key) {
  return hash(key, d->keysz) % d->len;
}

static size_t ditem_sz(const CADT_Dict *const d) { return d->size + d->valsz; }

/* total memory space occupied by d->entries */
static size_t dbufmemspace(const CADT_Dict *const d) {
  return ditem_sz(d) * d->len;
}

/* return an unsigned char * point to the given idx of entries in d */
static Item_ ditem(const CADT_Dict *const d, const size_t idx) {
  return (unsigned char *)d->entries + idx * ditem_sz(d);
}

static unsigned char *dkey(void *item) { return (unsigned char *)item; }

static unsigned char *dval(void *item, const size_t offset) {
  return (unsigned char *)item + offset;
}

/* check if a block is empty */
static bool dempty(const CADT_Dict *const d, const size_t idx) {
  unsigned char *item = ditem(d, idx);
  return item[0] == EMPTY_ITEM && !memcmp(item, item + 1, ditem_sz(d));
}

/* to check if d[idx] has the same key as item
 * note: if item has empty it return false */
static bool samekey(const Item_ item, const void *const key,
                    const size_t keysz) {
  return (!memcmp(key, item, keysz));
}

/* -- addressing and mem management -- */

/* open addressing overwrite mode.
 * it return the address when block contains the same key. */
static unsigned char *dfind_open_addr(CADT_Dict *const d, size_t idx,
                                      const void *const key) {
  assert(d != NULL);

  while (!dempty(d, idx)) {
    idx = hash(&idx, sizeof(size_t)) % d->len;
    Item_ item = ditem(d, idx);
    if (samekey(item, key, d->keysz)) {
      return item;
    }
    d->collisions += 1;
  }

  return NULL;
}

static CADT_Dict *dictmalloc(const size_t size, const size_t keysz,
                             const size_t valsz) {
  const size_t itemsz = keysz + valsz;
  CADT_Dict *d = (CADT_Dict *)malloc(sizeof(CADT_Dict));

  d->size = size;
  d->keysz = keysz;
  d->valsz = valsz;
  if (itemsz * size < CADT_DICT_MIN_MEMSZ / 2) {
    d->len = (size_t)(CADT_DICT_MIN_MEMSZ / itemsz);
  } else {
    d->len = 2 * size;
  }
  d->entries = (Item_)malloc(dbufmemspace(d));
  memset(d->entries, EMPTY_ITEM, dbufmemspace(d));
  return d;
}

/* resize the buffer either when the size fill up 80% of entries or
 * because it has accumulated more than 256 collisions */
static bool dbufresize(CADT_Dict *d) {
  assert(d != NULL);
  if (d->collisions >= 0 && (d->size / d->len) < CADT_DICT_RESIZE_THRESHOLD) {
    return false;
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
  d->entries = (Item_)realloc(d->entries, ditem_sz(d) * d->len);
  return true;
}

/* open addressing to resolve collision. get new address by
 * double hashing */
static bool dput(CADT_Dict *const d, const Item_ item, CADTDictMode mode) {
  assert(d != NULL);
  unsigned char *key = dkey(item);
  unsigned char *ptr = dfind_open_addr(d, dhash_idx(d, key), key);

  if (ptr == NULL) {
    return false;
  }

  d->size += 1;
  dbufresize(d);

  switch (mode) {
    case IGNORE:
      break;

    case OVERWRITE:
      memcpy(ptr, item, ditem_sz(d));
      break;

    default:
      return false;
  }
  return true;
}

/* lookup element with open addressing.
 * the returned pointer point to */
static Item_ dget(CADT_Dict *const d, const void *const key) {
  assert(d != NULL);
  size_t idx = dhash_idx(d, key);
  unsigned char *val = NULL;
  Item_ item = ditem(d, idx);

  while (memcmp(item, key, d->keysz)) {
    idx = hash(&idx, sizeof(size_t)) % d->len;
    /* if hit empty a empty block means key not found */
    if (dempty(d, idx)) {
      return NULL;
    }
    item = ditem(d, idx);
  }

  val = dval(item, d->keysz);
  return (Item_)val;
}

/* -- interface -- */

CADT_Dict *CADT_Dict_new(const size_t keysz, const size_t valsz) {
  if (keysz < 0 || valsz < 0) {
    return NULL;
  }
  return dictmalloc(keysz, valsz, 0);
}

void CADT_Dict_put(CADT_Dict *d, const void *key, void *val,
                   CADTDictMode mode) {
  if (d == NULL) {
    return;
  }
  unsigned char item[ditem_sz(d)];

  memcpy(item, key, d->keysz);
  memcpy(&item[d->keysz], val, d->valsz);
  dput(d, item, mode);
}

void *CADT_Dict_get(CADT_Dict *d, const void *key) {
  if (d == NULL || key == NULL) {
    return NULL;
  }
  return (void *)dval(dget(d, key), d->keysz);
}

size_t CADT_Dict_update(CADT_Dict *d1, CADT_Dict *d2, CADTDictMode mode) {
  if (d1 == NULL || d2 == NULL) {
    return 0;
  }
  for (size_t i = 0; i < d2->len; i++) {
    if (!dempty(d2, i)) {
      unsigned char *top = ditem(d2, i);
      CADT_Dict_put(d1, dkey(top), dval(top, d2->keysz), mode);
    }
  }
  return d1->size;
}

/* because of open space addressing, keys cannot be deleted */
bool CADT_Dict_remove(CADT_Dict *d, const void *const key) {
  if (d == NULL || key == NULL) {
    return false;
  }
  void *val = CADT_Dict_get(d, key);
  if (val == NULL) {
    return false;
  }
  memset(val, EMPTY_ITEM, d->valsz);
  return true;
}

void CADT_Dict_free(CADT_Dict *d) {
  free(d->entries);
  free(d);
}

#undef CADT_DICT_RESIZE_THRESHOLD
#undef CADT_DICT_FAST_GROWTH_SZ_LIMIT
#undef CADT_DICT_FAST_GROWTH_RATE
#undef CADT_DICT_SLOW_GROWTH_RATE
#undef CADT_DICT_MIN_MEMSZ
#undef EMPTY_ITEM
