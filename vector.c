#include "vector.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int vecupdate(CADT_Vec *v, const CADT_Vec u) {
  if (v->memsz != u.memsz) {
    return -1;
  }
  if (u.size > u.len) {
    return -1;
  }
  *v = u;
  return 1;
}

/*-- manage vector buffer --*/

static CADT_Vec *vecalloc(const size_t size, const size_t memsz) {
  CADT_Vec *v = (CADT_Vec *)malloc(sizeof(CADT_Vec));
  if (v == NULL) {
    return NULL;
  }
  const size_t len = size * SZ_LEN_RATIO;
  CADT_Vec u = {
      .len = len,
      .size = size,
      .memsz = memsz,
      .buf = NULL,
  };
  if (!vecupdate(v, u)) {
    return NULL;
  }
  v->len = size * SZ_LEN_RATIO;
  v->size = size;
  v->memsz = memsz;
  v->buf = malloc(memsz * v->len);
  return v;
}

static size_t vmemspace(CADT_Vec *v) { return v->len * v->memsz; }

static int vbuf_shouldshrink(CADT_Vec *v) {
  return (v->len / v->size) >= SHRINK_THRESHOLD;
}

static int vbuf_shouldbulk(CADT_Vec *v) { return v->len <= v->size; }

static size_t vbuf_shrink(CADT_Vec *v, const size_t delta) {
  /* clean all content if buflen <= delta. */
  if (v->len <= delta) {
    v->len = 0;
    v->size = 0;
    free(v->buf);
    return 0;
  }
  /* if memory usage is small there is no need to resize
   * set 32kB as the threshold. It can hold 4096 doubles and
   * fit in most of the L1 cache. */
  if (vmemspace(v) < 1024 * 32) {
    return v->len;
  }

  v->len -= delta;
  if (v->size <= v->len) {
    v->size = v->len;
  }
  void *const p = realloc(v->buf, vmemspace(v));
  if (p == NULL) {
    return 0;
  }
  v->buf = p;
  return v->len;
}

static size_t vbuf_clear(CADT_Vec *v) {
  v->size = 0;
  return 0;
}

/* increase vector buffer size */
static size_t vbuf_bulk(CADT_Vec *v, const size_t delta) {
  v->len += delta;
  void *const p = realloc(v->buf, vmemspace(v));
  if (p == NULL) {
    return -1;
  }
  v->buf = p;
  return v->len;
}

/* resize buf to size * SZ_LEN_RATIO */
static size_t vbuf_resize(CADT_Vec *v) {
  if (vbuf_shouldshrink(v)) {
    size_t delta = v->len - v->size * SZ_LEN_RATIO;
    vbuf_shrink(v, delta);
  } else if (vbuf_shouldbulk(v)) {
    size_t delta = v->size * SZ_LEN_RATIO - v->len;
    vbuf_bulk(v, delta);
  }
  return v->len;
}

/*-- implement vector interface --*/
CADT_Vec *CADT_Vec_new(const size_t size, const int memsz) {
  CADT_Vec *vector = vecalloc(size, memsz);
  return vector;
}

CADT_Vec *CADT_Vec_init(const size_t size, const int memsz, ...) {
  CADT_Vec *vector = vecalloc(size, memsz);
  void *top = vector->buf;
  va_list args;

  va_start(args, memsz);
  for (size_t i = 0; i < size; i++) {
    const void *const val = va_arg(args, void *);
    memcpy(top, (char *)val, memsz);
    top = (char *)top + memsz;
  }
  va_end(args);
  return vector;
}

void CADT_Vec_insert(CADT_Vec *v, const size_t idx, void *val,
                     const size_t memsz) {
  if (memsz != v->memsz) {
    perror("inserting invalid vector element");
    return;
  }
  v->size += 1;
  vbuf_resize(v);
  char *needle = (char *)v->buf + idx * memsz;
  memcpy(needle + 1, needle, memsz * v->size - idx + 1);
  memcpy(needle, val, memsz);
}

void *const CADT_Vec_get(CADT_Vec *v, const size_t idx, const size_t memsz) {
  if (v->size <= 0 || v->memsz != memsz || idx < v->size) {
    return NULL;
  }
  /* always return a copy rather than a reference. */
  void *const val = malloc(v->memsz);
  char *needle = (char *)v->buf + idx;
  memcpy(val, needle, memsz);
  return val;
}

void *const CADT_Vec_pop(CADT_Vec *v, const size_t memsz) {
  void *const val = CADT_Vec_get(v, v->len, memsz);
  v->size -= 1;
  vbuf_resize(v);
  return val;
}

void CADT_Vec_push(CADT_Vec *v, void *val, const size_t memsz) {
  CADT_Vec_insert(v, v->len, val, memsz);
}

CADT_Vec *CADT_Vec_concat(CADT_Vec *v1, CADT_Vec *v2) {
  if (v1->memsz != v2->memsz) {
    return NULL;
  }
  const size_t sz = v1->size + v2->size;
  const size_t memsz = v1->memsz;
  CADT_Vec *vector = vecalloc(sz, memsz);

  assert(vector->len > vector->size);
  assert(vector->size == sz);

  memcpy(vector->buf, (char *)v1->buf, vmemspace(v1));
  char *buf_end = (char *)vector->buf + v1->size;
  memcpy(buf_end, v2->buf, vmemspace(v2));

  return vector;
}

bool CADT_Vec_contains(CADT_Vec *v, const void *const val) {
  size_t memsz = v->memsz;
  char *p = NULL;
  char *buffer_end = (char *)v->buf + v->size;

  for (p = (char *)v->buf; p < buffer_end; p++) {
    if (memcmp(val, (void *)p, memsz)) {
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
