#include "vector.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static void vupdate(CADT_Vec *v, const CADT_Vec u) {
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

/*-- manage vector buffer --*/

static CADT_Vec *valloc(const size_t size, const size_t memsz) {
  CADT_Vec *v = (CADT_Vec *)malloc(sizeof(CADT_Vec));
  vupdate(v, (CADT_Vec){
                 .buflen = (size_t)(size * SZ_LEN_RATIO),
                 .size = size,
                 .memsz = memsz,
                 .buf = NULL,
             });
  v->buflen = size * SZ_LEN_RATIO;
  v->size = size;
  v->memsz = memsz;
  /* buffer will always have 16 elemetents at least. */
  // TODO benchmark it. if it actually save memory push the change to all apis.
  v->buf = malloc(memsz * v->buflen);
  return v;
}

static int vbuf_shouldshrink(CADT_Vec *v) {
  return (v->buflen / v->size) >= SHRINK_THRESHOLD;
}

static int vbuf_shouldbulk(CADT_Vec *v) { return v->buflen <= v->size; }

static size_t vbuf_shrink(CADT_Vec *v, const size_t delta) {
  /* clean all content if buflen <= delta. */
  if (v->buflen <= delta) {
    v->buflen = 0;
    v->size = 0;
    free(v->buf);
    return 0;
  }
  /* if memory usage is small there is no need to resize
   * set 32kB as the threshold. It can hold 4096 doubles and
   * fit in most of the L1 cache. */
  if (v->buflen * v->memsz < 1024 * 32) {
  }
  v->buflen -= delta;
  if (v->size <= v->buflen) {
    v->size = v->buflen;
  }
  v->buf = realloc(v->buf, v->memsz * v->size);
  return v->buflen;
}

static size_t vbuf_clear(CADT_Vec *v) {
  vbuf_shrink(v, v->buflen);
  return 0;
}

/* increase vector buffer size */
static size_t vbuf_bulk(CADT_Vec *v, const size_t delta) {
  v->buflen += delta;
  v->buf = realloc(v->buf, v->buflen * v->memsz);
  return v->buflen;
}

/* resize buf to size * SZ_LEN_RATIO */
static size_t vbuf_resize(CADT_Vec *v) {
  if (vbuf_shouldshrink(v)) {
    size_t delta = v->buflen - v->size * SZ_LEN_RATIO;
    vbuf_shrink(v, delta);
  } else if (vbuf_shouldbulk(v)) {
    size_t delta = v->size * SZ_LEN_RATIO - v->buflen;
    vbuf_bulk(v, delta);
  }
  return v->buflen;
}

/*-- implement vector interface --*/
CADT_Vec *CADT_Vec_new(const size_t size, const int memsz) {
  CADT_Vec *vector = valloc(size, memsz);
  return vector;
}

CADT_Vec *CADT_Vec_init(const size_t size, const int memsz, ...) {
  CADT_Vec *vector = valloc(size, memsz);
  void *buftop = vector->buf;
  va_list args;
  va_start(args, memsz);
  for (int i = 0; i < size; i++) {
    void *val = va_arg(args, void *);
    memcpy(buftop, (char *)val, memsz);
    buftop = (char *)buftop + memsz;
  }
  va_end(args);
  return vector;
}

void CADT_Vec_insert(CADT_Vec *v, const size_t idx, void *val,
                     const size_t memsz) {
  if (memsz != v->memsz) {
    perror("invalid type push into vector");
    return;
  }
  v->size += 1;
  vbuf_resize(v);
  char *needle = (char *)v->buf + idx * memsz;
  memcpy(needle + 1, needle, memsz * v->size - idx + 1);
  memcpy(needle, val, memsz);
}

void *const CADT_Vec_get(CADT_Vec *v, const size_t idx, const size_t memsz) {
  if (v->size <= 0) {
    return NULL;
  }
  void *val = malloc(v->memsz);
  memcpy(val, (char *)v->buf + v->size, v->memsz);
  v->size -= 1;
  vbuf_resize(v);
  return val;
}

void *const CADT_Vec_pop(CADT_Vec *v, const size_t memsz) {
  return CADT_Vec_get(v, v->buflen, memsz);
}

void CADT_Vec_push(CADT_Vec *v, void *val, const size_t memsz) {
  CADT_Vec_insert(v, v->buflen, val, memsz);
}

CADT_Vec *CADT_Vec_concat(CADT_Vec *v1, CADT_Vec *v2) {
  if (v1->memsz != v2->memsz) {
    return NULL;
  }
  const size_t sz1 = v1->size;
  const size_t sz2 = v2->size;
  const size_t memsz = v1->memsz;

  CADT_Vec *vector = valloc(sz1 + sz2, v1->memsz);
  assert(vector->buflen > vector->size);
  assert(vector->size == sz1 + sz2);

  memcpy(vector->buf, (char *)v1->buf, memsz * v1->size);
  memcpy((char *)vector->buf + v1->size, (char *)v2->buf, memsz * v2->size);

  return vector;
}

bool CADT_Vec_contains(CADT_Vec *v, const void *const val) {
  size_t memsz = v->memsz;
  char *ptr = NULL;
  char *buf_end = (char *)v->buf + v->size;
  for (ptr = (char *)v->buf; ptr < buf_end; ptr++) {
    if (memcmp(val, (void *)ptr, memsz)) {
      return 1;
    }
  }
  return -1;
}

void CADT_Vec_reserve(CADT_Vec *v, const size_t size) {
  v->size = size;
  vbuf_resize(v);
}


void CADT_Vec_clear(CADT_Vec *v) { vbuf_clear(v); }

void *const CADT_Vec_begin(CADT_Vec *v) { return v->buf; }

void *const CADT_Vec_end(CADT_Vec *v) { return (char *)v->buf + v->size; }
