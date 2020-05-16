#include "vector.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static void vupdate_(CADT_Vector *v, const CADT_Vector u) {
  if (v->memsz != u.memsz) {
    perror("changing vector type");
    return;
  }
  if (u.size > u.buflen) {
    perror("vector buffer length should always bigger than size");
    return;
  }
  *v = u;
}

/* optional argumetns, use it like vupdate(.len = 10, .size=10) */
#define vupdate(v, ...)                                                        \
  vupdate_(v,                                                                  \
           (CADT_Vector){                                                      \
               .buflen = 0, .size = 0, .memsz = 0, .buf = NULL, __VA_ARGS__})

/*-- manage vector buffer --*/

static CADT_Vector *valloc(const size_t size, const int memsz) {
  CADT_Vector *v = (CADT_Vector *)malloc(sizeof(CADT_Vector));
  vupdate(v);
  v->buflen = size * SZ_LEN_RATIO;
  v->size = size;
  v->memsz = memsz;

  /* buffer will always have 16 elemetents at least. */
  // TODO benchmark it. if it actually save memory push the change to all apis.
  if (size < 16) {
    v->buf = malloc(memsz * 16);
  }
  v->buf = malloc(memsz * v->buflen);
  return v;
}

static int vbuf_shouldshrink(CADT_Vector *v) {
  return (v->buflen / v->size) >= SHRINK_THRESHOLD;
}

static int vbuf_shouldbulk(CADT_Vector *v) { return v->buflen <= v->size; }

static size_t vbuf_shrink_(CADT_Vector *v, const size_t delta) {
  /* clean all content if buflen - delta is smaller than 0. */
  if (v->buflen - delta <= 0) {
    v->buflen = 0;
    v->size = 0;
    free(v->buf);
    return 0;
  }
  v->buflen -= delta;
  if (v->size <= v->buflen) {
    v->size = v->buflen;
  }
  v->buf = realloc(v->buf, v->memsz * v->size);
  return v->buflen;
}

static size_t vbuf_clear(CADT_Vector *v) {
  vbuf_shrink_(v, v->buflen);
  return 0;
}

/* increase vector buffer size */
static size_t vbuf_bulk_(CADT_Vector *v, const size_t delta) {
  v->buflen += delta;
  v->buf = realloc(v->buf, v->buflen * v->memsz);
  return v->buflen;
}

/* resize buf to size * SZ_LEN_RATIO */
static size_t vbuf_auto_resize(CADT_Vector *v) {
  if (vbuf_shouldshrink(v)) {
    size_t delta =
    vbuf_shrink_(v, delta);
  }
  else if (vbuf_shouldbulk(v)) {
    size_t delta =  v->size * SZ_LEN_RATIO - v->buflen;
    vbuf_bulk_(v, delta);
  }
  return v->buflen;
}

/*-- implement vector interface --*/
CADT_Vector *CADT_Vector_new(const size_t size, const int memsz) {
  CADT_Vector *vector = valloc(size, memsz);
  return vector;
}

CADT_Vector *CADT_Vector_init(const size_t size, const int memsz, ...) {
  CADT_Vector *vector = valloc(size, memsz);
  va_list args;
  va_start(args, size);

  for (int i = 0; i < size; i++) {
    void *val = va_arg(args, void *);
    // TODO: append into vector.
  }
  va_end(args);
  return vector;
}

CADT_Vector *CADT_Vector_insert(CADT_Vector *, const size_t idx, void *val,
                                const size_t memsz);

void *CADT_Vector_pop(CADT_Vector *v) {
  if (v->size <= 0) {
    return NULL;
  }
  void *val = malloc(v->memsz);
  memcpy(val, (char *)v->buf + v->size, v->memsz);
  v->size -= 1;
  /* shrink to size * SZ_LEN_RATIO if necessary. */
  vbuf_auto_resize(v);
  return val;
}

CADT_Vector *CADT_Vector_push(CADT_Vector *v, void *val, const size_t memsz) {
  if (v->memsz != memsz) {
    perror("invalid type push into vector");
    return NULL;
  }
  v->size += 1;
  /* fetch the ptr point to the last element */
  void *size_ptr = (char *)v->buf + v->size;
  memcpy(size_ptr, val, memsz);
  vbuf_auto_resize(v);
  return v;
}

size_t CADT_Vector_concat(CADT_Vector *, CADT_Vector *) {
}

bool CADT_Vector_contains(CADT_Vector *, void *);
