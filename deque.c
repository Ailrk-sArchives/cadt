#include "deque.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static Block_ *newblock_(Block_ other) {
  Block_ *block = (Block_ *)malloc(sizeof(Block_));
  block->next = other.next;
  block->prev = other.prev;
  block->value = other.value;
  return block;
}

static void bfree(Block_ *b) {
  free(b->value);
  free(b);
}

#define newblock(...)                                                          \
  newblock_((Block_){.next = NULL, .prev = NULL, .value = NULL, __VA_ARGS__})

static CADT_Deque *deqalloc(const size_t memsz) {
  CADT_Deque *d = (CADT_Deque *)malloc(sizeof(CADT_Deque));
  d->head = newblock(.next = d->tail);
  d->tail = newblock(.prev = d->head);

  d->size = 0;
  d->memsz = memsz;
  d->maxlen = 0;
  return d;
}

static bool deqmaxlen(CADT_Deque *const d, const size_t maxlen) {
  if (d == NULL) {
    return false;
  }
  d->maxlen = maxlen;
  return true;
}

CADT_Deque *CADT_Deque_new(const size_t memsz) { return deqalloc(memsz); }

CADT_Deque *CADT_Deque_init(const size_t count, const size_t memsz, ...) {
  CADT_Deque *deq = deqalloc(memsz);
  va_list args;
  va_start(args, memsz);
  for (int i = 0; i < count; i++) {
    void *val = va_arg(args, void *);
    Block_ *block =
        newblock(.prev = deq->tail->prev, .next = deq->tail, .value = val);
    deq->tail->prev->next = block;
    deq->tail->prev = block;
  }
  va_end(args);
  deq->size = count;
  return deq;
}

void CADT_Deque_push(CADT_Deque *d, void *const val) {
  if (d->size >= d->maxlen || d == NULL) {
    return;
  }
  Block_ *newhead = newblock(.prev = NULL, .next = d->head, .value = val);
  d->head->prev = newhead;
  d->size++;
}

void CADT_Deque_pushl(CADT_Deque *d, void *val) {
  if (d->size >= d->maxlen || d == NULL) {
    return;
  }
  Block_ *newhead =
      newblock(.prev = d->tail->prev, .next = d->tail, .value = val);
  d->tail->prev = newhead;
  d->size++;
}

void *CADT_Deque_pop(CADT_Deque *d) {
  if (d->size == 0 || d == NULL) {
    return NULL;
  }

  void * value = d->head->value;
  d->head->next->prev = NULL;
  d->head = d->head->next;
  free(d->head);
  return value;
}

void *CADT_Deque_popl(CADT_Deque *d) {
  if (d->size == 0 || d == NULL) {
    return NULL;
  }

  void *value = d->tail->value;
  d->tail->prev->next = NULL;
  d->tail = d->tail->prev;
  free(d->tail);
  return value;
}

bool CADT_Deque_remove(CADT_Deque *d, const void *const val) {
  Block_ *current = d->head;
  while(current != NULL) {
    if (!memcmp(current->value, val, d->memsz)) {
      current->prev->next = current->next;
      current->next->prev = current->prev;
      free(current->value);
      return true;
    }
    current = current->next;
  }
  return false;
}

void CADT_Deque_rotate(CADT_Deque *d, const size_t n) {
  const size_t offset = n % d->size;
  Block_ *current = d->head;
  current = current + offset;
  d->tail->next = d->head;
  d->head->prev = d->tail;

  d->head = current;
  d->tail = current->prev;
  d->head->prev = NULL;
  d->tail->next = NULL;
}

