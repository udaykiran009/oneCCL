USE_SECURITY_FLAGS ?= 0
ENABLE_DEBUG       ?= 0

CC         ?= gcc

HAVE_CFLAGS = -DHAVE_UNISTD_H -DHAVE_STDLIB_H -DHAVE_STRING_H -DHAVE_STRINGS_H

CFLAGS  += $(EXTRA_CFLAGS) -Wall -O2 -std=gnu99 -I. -fPIC -D_GNU_SOURCE $(HAVE_CFLAGS)
LDFLAGS += $(EXTRA_LDFLAGS)

TARGET = libpmi

ifeq ($(CC), icc)
  LDFLAGS += -static-intel
endif

ifeq ($(ENABLE_DEBUG), 1)
    USE_SECURITY_FLAGS = 0
    CFLAGS += -O0 -g -DENABLE_DEBUG=1
else
    CFLAGS += -O2
endif

ifeq ($(USE_SECURITY_FLAGS), 1)
    SECURITY_CXXFLAGS   = -Wformat -Wformat-security -D_FORTIFY_SOURCE=2 -fstack-protector
    SECURITY_LDFLAGS    = -z noexecstack -z relro -z now
    CFLAGS              += $(SECURITY_CXXFLAGS)
    LDFLAGS             += $(SECURITY_LDFLAGS)
endif

default: $(TARGET)

$(TARGET): simple_pmiutil.c simple_pmi.c
	$(CC) $(CFLAGS) -c simple_pmiutil.c
	$(CC) $(CFLAGS) -c simple_pmi.c
	ar -cvq $(TARGET).a simple_pmiutil.o simple_pmi.o
	$(CC) $(CFLAGS) -shared -Wl,-soname,$(TARGET).so.1 -o $(TARGET).so.1.0 *.o $(LDFLAGS)

test:
	cd tests; make clean; make

clean:
	rm -f *.a *.so *.so.1.0 *.o