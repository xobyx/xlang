CC ?= gcc
CFLAGS ?= -O3 -fPIC -Wall -Wextra -I../../include
LDFLAGS ?= -shared -lm

SQLITE_LIB ?= $(shell if [ -f /usr/lib/x86_64-linux-gnu/libsqlite3.so.0 ]; then echo "-l:libsqlite3.so.0"; else echo "-lsqlite3"; fi)

TARGET = fastmath.so
SQLITE_TARGET = sqlite3.so
ZLIB_TARGET = zlib.so
MYLIB_TARGET = libmylib.so

all: $(TARGET) $(SQLITE_TARGET) $(ZLIB_TARGET) $(MYLIB_TARGET)
	ln -sf $(TARGET) lib$(TARGET)
	ln -sf $(SQLITE_TARGET) lib$(SQLITE_TARGET)
	ln -sf $(SQLITE_TARGET) sqlite.so
	ln -sf $(SQLITE_TARGET) libsqlite.so
	ln -sf $(ZLIB_TARGET) lib$(ZLIB_TARGET)
	mkdir -p ../../lib
	cp -f $(TARGET) ../../lib/
	cp -Pf lib$(TARGET) ../../lib/
	cp -f $(SQLITE_TARGET) ../../lib/
	cp -Pf lib$(SQLITE_TARGET) ../../lib/
	cp -Pf sqlite.so ../../lib/
	cp -Pf libsqlite.so ../../lib/
	cp -f $(ZLIB_TARGET) ../../lib/
	cp -Pf lib$(ZLIB_TARGET) ../../lib/
	cp -f $(MYLIB_TARGET) ../../lib/
	$(MAKE) -C raylib

$(TARGET): fastmath.c
	$(CC) $(CFLAGS) fastmath.c $(LDFLAGS) -o $(TARGET)

$(SQLITE_TARGET): xsqlite3.c
	$(CC) $(CFLAGS) xsqlite3.c -shared $(SQLITE_LIB) -o $(SQLITE_TARGET)

$(ZLIB_TARGET): xzlib.c
	$(CC) $(CFLAGS) xzlib.c -shared -lz -o $(ZLIB_TARGET)

$(MYLIB_TARGET): mylib.c
	$(CC) $(CFLAGS) mylib.c -shared -o $(MYLIB_TARGET)

clean:
	rm -f $(TARGET) lib$(TARGET) $(SQLITE_TARGET) lib$(SQLITE_TARGET) sqlite.so libsqlite.so $(ZLIB_TARGET) lib$(ZLIB_TARGET) $(MYLIB_TARGET)
	$(MAKE) -C raylib clean

.PHONY: all clean
