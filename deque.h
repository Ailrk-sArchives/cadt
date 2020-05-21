#ifndef _CADT_DEQUE
#define _CADT_DEQUE

#include "cadt.h"

typedef struct Block_ {
  void *value;
  struct Block_ *next;
  struct Block_ *prev;
} Block_ ;

typedef struct CADT_Deque {
  Block_ *head;
  Block_ *tail;
  size_t size;
  size_t maxlen;
  size_t memsz;
} CADT_Deque;

#endif /* ifndef _CADT_DEQUE */
