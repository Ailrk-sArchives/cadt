#include "vector.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int vecupdate(CADT_Vec *v, const CADT_Vec u) {
  if (v->meta.memsz != u.meta.memsz) {
    return -1;
  }
  if (u.meta.size > u.meta.len) {
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
      .meta =
          {
              .len = len,
              .size = size,
              .memsz = memsz,

          },
      .buf = NULL,
  };

  if (!vecupdate(v, u)) {
    return NULL;
  }

  v->meta.len = size * SZ_LEN_RATIO;
  v->meta.size = size;
  v->meta.memsz = memsz;
  v->buf = malloc(memsz * v->meta.len);
  return v;
}

static size_t vmemspace(CADT_Vec *v) { return v->meta.len * v->meta.memsz; }


static int vbuf_shouldshrink(CADT_Vec *v) {
  return (v->meta.len / v->meta.size) >= SHRINK_THRESHOLD;
}


static int vbuf_shouldbulk(CADT_Vec *v) { return v->meta.len <= v->meta.size; }


static size_t vbuf_shrink(CADT_Vec *v, const size_t delta) {
  /* clean all content if buflen <= delta. */
  if (v->meta.len <= delta) {
    v->meta.len = 0;
    v->meta.size = 0;
    free(v->buf);
    return 0;
  }
  /* if memory usage is small there is no need to resize
   * set 32kB as the threshold. It can hold 4096 doubles and
   * fit in most of the L1 cache. */
  if (vmemspace(v) < 1024 * 32) {
    return v->meta.len;
  }

  v->meta.len -= delta;
  if (v->meta.size <= v->meta.len) {
    v->meta.size = v->meta.len;
  }
  void *const p = realloc(v->buf, vmemspace(v));
  if (p == NULL) {
    return 0;
  }
  v->buf = p;
  return v->meta.len;
}


static size_t vbuf_clear(CADT_Vec *v) {
  v->meta.size = 0;
  return 0;
}


/* increase vector buffer size */
static size_t vbuf_bulk(CADT_Vec *v, const size_t delta) {
  v->meta.len += delta;
  void *const p = realloc(v->buf, vmemspace(v));

  if (p == NULL) {
    return -1;
  }

  v->buf = p;
  return v->meta.len;
}


/* resize buf to size * SZ_LEN_RATIO */
static size_t vbuf_resize(CADT_Vec *v) {
  if (vbuf_shouldshrink(v)) {
    size_t delta = v->meta.len - v->meta.size * SZ_LEN_RATIO;
    vbuf_shrink(v, delta);
  } else if (vbuf_shouldbulk(v)) {
    size_t delta = v->meta.size * SZ_LEN_RATIO - v->meta.len;
    vbuf_bulk(v, delta);
  }
  return v->meta.len;
}


static unsigned char *const vidx(const CADT_Vec *const v, const size_t idx) {
  return (unsigned char *)v->buf + idx * v->meta.memsz;
}

/*-- implement vector interface --*/
CADT_Vec *CADT_Vec_new(const size_t size, const size_t memsz) {
  CADT_Vec *vector = vecalloc(size, memsz);
  return vector;
}

CADT_Vec *CADT_Vec_init(const size_t size, const size_t memsz, ...) {
  CADT_Vec *vector = vecalloc(size, memsz);
  void *top = vector->buf;
  va_list args;

  va_start(args, memsz);
  for (size_t i = 0; i < size; i++) {
    void *const val = va_arg(args, void *);
    memcpy(top, (unsigned char *)val, memsz);
    top = (unsigned char *)top + memsz;
  }
  va_end(args);
  return vector;
}


void CADT_Vec_insert(CADT_Vec *v, const size_t idx, void *val,
                     const size_t memsz) {
  if (memsz != v->meta.memsz) {
    perror("inserting invalid vector element");
    return;
  }
  v->meta.size += 1;
  vbuf_resize(v);
  memmove(vidx(v, idx + 1), vidx(v, idx), memsz * (v->meta.size - idx + 1));
  memcpy(vidx(v, idx), val, memsz);
}


void *const CADT_Vec_get(CADT_Vec *v, const size_t idx, const size_t memsz) {
  if (v->meta.size <= 0 || v->meta.memsz != memsz || idx < v->meta.size) {
    return NULL;
  }
  /* always return a copy rather than a reference. */
  void *const val = malloc(v->meta.memsz);
  memcpy(val, vidx(v, idx), memsz);
  return val;
}


void *const CADT_Vec_pop(CADT_Vec *v, const size_t memsz) {
  void *const val = CADT_Vec_get(v, v->meta.len, memsz);
  v->meta.size -= 1;
  vbuf_resize(v);
  return val;
}


void CADT_Vec_push(CADT_Vec *v, void *val, const size_t memsz) {
  CADT_Vec_insert(v, v->meta.len, val, memsz);
}


CADT_Vec *CADT_Vec_concat(CADT_Vec *v1, CADT_Vec *v2) {
  if (v1->meta.memsz != v2->meta.memsz) {
    return NULL;
  }
  const size_t sz = v1->meta.size + v2->meta.size;
  const size_t memsz = v1->meta.memsz;
  CADT_Vec *vector = vecalloc(sz, memsz);

  assert(vector->meta.len > vector->meta.size);
  assert(vector->meta.size == sz);

  memcpy(vector->buf, v1->buf, vmemspace(v1));
  memcpy((unsigned char *)vector->buf + v1->meta.size, v2->buf, vmemspace(v2));

  return vector;
}


bool CADT_Vec_contains(CADT_Vec *v, const void *const val) {
  size_t memsz = v->meta.memsz;
  unsigned char *p = (unsigned char *)v->buf;
  unsigned char *buffer_end = p + v->meta.size;
  for (; p < buffer_end; p++) {
    if (memcmp(val, p, memsz)) {
      return 1;
    }
  }
  return -1;
}


void CADT_Vec_reserve(CADT_Vec *v, const size_t size) {
  v->meta.size = size;
  vbuf_resize(v);
}


void CADT_Vec_clear(CADT_Vec *v) { vbuf_clear(v); }


// export iterators.
void *const CADT_Vec_begin(CADT_Vec *const v) { return v->buf; }


void *const CADT_Vec_end(CADT_Vec *const v) {
  return vidx(v, v->meta.size - 1);
}


void CADT_Vec_free(CADT_Vec *v) {
  free(v->buf);
  free(v);
}
