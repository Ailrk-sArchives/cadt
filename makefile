CC = clang

CFLAGS = -std=c11
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wundef
CFLAGS += -Wno-ignored-qualifiers

ROOT_DIR = .
TEST_DIR = $(ROOT_DIR)/tests
LIBUNITY = $(ROOT_DIR)/test/libunity.a

OBJS = vector.o

define testmod
	"test_$(1).c"
endef

.PHONY: clean test

all: $(OBJS)
	$(CC) $(CFLAGS) $< -c -o $@

test:
	@mkdir ./temp
	$(CC) $(CFLAGS) -l$(LIBUNITY) $(TEST_DIR)/$(call testmod $(module)) -o temp/test$(module)
	./temp/test$(module)
	@rm -r ./temp

vector.o: vector.c vector.h cadt.h
dict.o: dict.c dict.h cadt.h

clean:
	@rm ./*.o


