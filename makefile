CC = clang

ROOT_DIR = .
TEST_DIR = $(ROOT_DIR)/tests

CFLAGS = -std=c11
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wundef
CFLAGS += -Wno-ignored-qualifiers

TEST_LDFLAGS = -L$(TEST_DIR)
TESTLIB = -lunity

OBJS = vector.o dict.o

.PHONY: clean test

all: $(OBJS)
	$(CC) $(CFLAGS) $< -c -o $@

test: buildtest
	./temp/test_$(m)
	@rm -r ./temp

buildtest: $(OBJS)
	@mkdir ./temp
	$(CC) $(CFLAGS) $(TEST_LDFLAGS) $(TESTLIB) \
		$(TEST_DIR)/test_$(m).c -o temp/test_$(m)
	./temp/test_$(m)

vector.o: vector.c vector.h cadt.h
dict.o: dict.c dict.h cadt.h

clean:
	@rm ./*.o -f
	@rm ./temp -rf

