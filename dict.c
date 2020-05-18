/* This dictionary implementation use open space addressing to handle
 * collisions. Large collision can drastically decrease the performance
 * of such technique, so the collision threshold is as small as 256
 * if there are more collisions the dictionary needs to be resized */
#include "dict.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

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

static size_t dbufitemsz (const CADT_Dict *const d) {
  return d->size + d->valsz;
}

static size_t dbufmemspace(const CADT_Dict *const d) {
  return dbufitemsz(d) * d->len;
}

static CADT_Dict *dictmalloc(const size_t size, const size_t keysz,
                             const size_t valsz) {
  const size_t itemsz = keysz + valsz;
  CADT_Dict *d = (CADT_Dict *)malloc(sizeof(CADT_Dict));
  if (itemsz * size < CADT_DICT_MIN_SZ) {

  }
  d->entries = (CADT_Dict_Item_ *)malloc(CADT_DICT_ITMESZ * size);
  d->size = size;
  d->keysz = keysz;
  d->valsz = valsz;
  return d;
}

static int dbufresize(CADT_Dict *d) {
  if (d->collisions > 0 && d->size / d->len < CADT_DICT_RESIZE_THRESHOLD) {
    return -1;
  }
  d->len = d->len * 2;
  const size_t newlen = sizeof(dbufitemsz(d)) * d->len;
  d->entries = (CADT_Dict_Item_ *)realloc(d->entries, newlen);
  return 1;
}


CADT_Dict *CADT_Dict_new(const int keysz, const int memsz);
CADTDictKey *CADT_Dict_put(CADT_Dict *, const CADTDictKey *key,
                           CADTDictVal *val);
CADTDictVal *CADT_Dict_get(CADT_Dict *, const CADTDictKey *key);
CADTDictVal *CADT_Dict_has(CADT_Dict *, const CADTDictKey *key);
size_t *CADT_Dict_update(CADT_Dict *, CADT_Dict *);
size_t *CADT_Dict_remove(CADT_Dict *, const CADTDictKey *const key);
