#ifndef _CADT
#define _CADT
#include <stdbool.h>
#include <stddef.h>

typedef struct CADT_Dict CADT_Dict;
typedef struct CADT_Vector CADT_Vector;
typedef struct CADT_List CADT_List;
typedef struct CADT_Set CADT_Set;

/* dict.c */
typedef void CADTDictKey;
typedef void CADTDictVal;
CADT_Dict *CADT_Dict_new(const int keysz, const int valsz);
CADTDictKey *CADT_Dict_put(CADT_Dict *, const CADTDictKey *key,
                           CADTDictVal *val);
CADTDictVal *CADT_Dict_get(CADT_Dict *, const CADTDictKey *key);
size_t *CADT_Dict_update(CADT_Dict *, CADT_Dict *);
size_t *CADT_Dict_remove(CADT_Dict *, const CADTDictKey *const key);

/* vector.c */
CADT_Vector *CADT_Vector_new(const size_t size, const int memsz);
CADT_Vector *CADT_Vector_init(const size_t size, const int memsz, ...);
void CADT_Vector_insert(CADT_Vector *, const size_t idx, void *val,
                        const size_t memsz);
void *CADT_Vector_pop(CADT_Vector *);
CADT_Vector *CADT_Vector_push(CADT_Vector *, void *val, const size_t memsz);
CADT_Vector *CADT_Vector_concat(CADT_Vector *, CADT_Vector *);
bool CADT_Vector_contains(CADT_Vector *, const void *const val);
void CADT_Vector_clear(CADT_Vector *);

/* list.c */
typedef void CADTListVal;

/* set.c */
typedef void CADTSetVal;
CADT_Set *CADT_Set_new(const int valsz);
size_t *CADT_Set_add(CADT_Set *, CADTSetVal *const val);
size_t *CADT_Set_remove(CADT_Set *, CADTSetVal *const val);
CADT_Set *CADT_Set_union(CADT_Set *, CADT_Set *);
CADT_Set *CADT_Set_intersect(CADT_Set *, CADT_Set *);
CADT_Set *CADT_Set_compliment(CADT_Set *sub, CADT_Set *s);
size_t CADT_Set_size(const CADT_Set *const);
size_t CADT_Set_issubset(const CADT_Set *const sub, const CADT_Set *const s);

#endif /* ifndef _CADT */
