CC ?= gcc
CFLAGS ?= -O3 -fPIC -Wall -Wextra -I../../../include -I../../../vendor/raylib/include
LDFLAGS ?= -shared -L../../../vendor/raylib/lib -l:libraylib.a -lGL -lm -lpthread -ldl -lrt -lX11

ifeq ($(shell uname -s),Linux)
    LDFLAGS += -Wl,-soname,raylib.so
endif

TARGET = raylib.so

all: $(TARGET)
	ln -sf $(TARGET) lib$(TARGET)
	mkdir -p ../../../lib
	cp -f $(TARGET) ../../../lib/
	cp -Pf lib$(TARGET) ../../../lib/

$(TARGET): xraylib.c
	$(CC) $(CFLAGS) xraylib.c $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET) lib$(TARGET)

.PHONY: all clean
