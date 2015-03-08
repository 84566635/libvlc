CFLAGS=-g -O2 -Wall -Wextra -Isrc -rdynamic $(OPTFLAGS)
CC=gcc
LIBS=-pthread -lm

SOURCES=$(wildcard src/**/*.c src/*.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

TEST_SRC=$(wildcard tests/*_tests.c)
TESTS=$(patsubst %.c,%,$(TEST_SRC))

INSTALL_SRC=$(wildcard bin/src/**/*.c bin/src/*.c)
INSTALL=$(patsubst %.c,%.o,$(INSTALL_SRC))

TARGET=build/cvlc.a
SO_TARGET=$(patsubst %.a,%.so,$(TARGET))
INSTALL_TARGET=bin/cvlc

all: $(TARGET) $(SO_TARGET)

$(TARGET): CFLAGS += -fPIC
$(TARGET): build $(OBJECTS)
#	$(CC) $(CFLAGS) $(LIBS) -o $(TARGET) $(OBJECTS)
	ar rcs $@ $(OBJECTS)
	ranlib $@

$(SO_TARGET): $(TARGET) $(OBJECTS)
	$(CC) -shared -o $@ $(OBJECTS)

build:
	@mkdir -p build
	@mkdir -p bin

# The Unit Tests
.PHONY: tests
tests: CFLAGS += $(OBJECTS)
tests: 
	$(CC) $(CFLAGS) $(LIBS) $(TEST_SRC) -o $(TESTS)
	sh ./tests/runtests.sh

# The Cleaner
clean:
	rm -rf build $(OBJECTS) $(TESTS)
	rm -f tests/tests.log
	find . -name "*.gc*" -exec rm {} \;
	rm -rf `find . -name "*.dSYM" -print`
	rm -f $(INSTALL_TARGET)

# The Install
install: CFLAGS += $(OBJECTS)
install:
	$(CC) $(CFLAGS) $(LIBS) $(INSTALL_SRC) -o $(INSTALL_TARGET)
