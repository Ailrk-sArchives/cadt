#include "heap.h"
#include <stdlib.h>
#include <string.h>

static void *get(const CADT_Heap *const h, const size_t index) {
  return (unsigned char *)h->data + index * h->meta.memsz;
}

static void set(CADT_Heap *h, const void *val, const size_t index) {
  memcpy((unsigned char *)h->data + index, val, h->meta.memsz);
}

CADT_Heap *CADT_Heap_new(const size_t capacity, const size_t memsz,
                         const CADTHeapType heap_type,
                         int (*cmp)(const void *, const void *)) {

  CADT_Heap *h = (CADT_Heap *)malloc(sizeof(CADT_Heap));
  h->meta.capacity = capacity;
  h->meta.memsz = memsz;
  h->meta.heap_type = heap_type;
  h->meta.size = 0;
  h->meta.cmp = cmp;

  h->data = malloc(sizeof(capacity) * memsz);

  if (h->data == NULL || h->meta.cmp == NULL) {
    return NULL;
  }

  return h;
}

bool CADT_Heap_insert(CADT_Heap *h, const void *val) {
  if (h->meta.size < h->meta.capacity) {
    set(h, val, h->meta.size);
    CADT_Heap_bottomup(h, h->meta.size);
    h->meta.size++;
    return true;
  }
  return false;
}

void CADT_Heap_bottomup(CADT_Heap *h, const size_t index) {
  const size_t parent_index = (index - 1) / 2;
  void *parent = get(h, parent_index);
  void *self = get(h, index);
  if (h->meta.cmp(parent, self) == 1) {
    char temp[h->meta.memsz];
    memcpy(temp, parent, h->meta.memsz);
    memcpy(parent, self, h->meta.memsz);
    memcpy(self, temp, h->meta.memsz);
    CADT_Heap_bottomup(h, parent_index);
  }
}

void CADT_Heap_topdown(CADT_Heap *h, const size_t parent_index) {
  int left = parent_index * 2 + 1;
  int right = left + 1;
  int min;

  if (left >= h->meta.size || left < 0) {
    left = -1;
  }
  if (right >= h->meta.size || right < 0) {
    right = -1;
  }

  if (left != -1 && h->meta.cmp(get(h, left), get(h, parent_index)) == -1) {
    min = left;
  } else {
    min = parent_index;
  }
  if (right != -1 && h->meta.cmp(get(h, right), get(h, min)) == -1) {
    min = right;
  }

  if (min != parent_index) {
    char *temp[h->meta.memsz];
    memcpy(temp, get(h, parent_index), h->meta.memsz);
    memcpy(get(h, min), get(h, parent_index), h->meta.memsz);
    memcpy(get(h, parent_index), temp, h->meta.memsz);

    CADT_Heap_topdown(h, min);
  }
}

void *CADT_Heap_popmin(CADT_Heap *h) {
  if (h->meta.size == 0)
    return NULL;
  void *val = get(h, 0);
  h->meta.size--;
  CADT_Heap_topdown(h, 0);
  return val;
}
