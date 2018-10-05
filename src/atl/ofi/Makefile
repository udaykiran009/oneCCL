USE_SECURITY_FLAGS ?= 0
ENABLE_DEBUG       ?= 0

CC         ?= gcc

BASE_DIR = $(shell pwd)

PMILIB_TARGET       = libpmi
PMILIB_DIR          = $(BASE_DIR)/../util/pm/pmi_rt/pmi

PM_RT_DIR          = $(BASE_DIR)/../util/pm
PMI_RT_DIR          = $(PM_RT_DIR)/pmi_rt

OFILIB_DIR          = $(BASE_DIR)/../../../ofi_rt

CFLAGS  += $(EXTRA_CFLAGS) -Wall -O2 -std=gnu99 -I. -I../ -fPIC -D_GNU_SOURCE
LDFLAGS += $(EXTRA_LDFLAGS)

TARGET = libmlsl_atl_ofi

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

CFLAGS += -I$(PMILIB_DIR)
PMI_LDFLAGS = $(LDFLAGS) -L$(PMILIB_DIR) -lpmi

CFLAGS += -I$(OFILIB_DIR)/include
OFI_LDFLAGS = $(LDFLAGS) -L$(OFILIB_DIR)/lib -lfabric

CFLAGS += -I$(PM_RT_DIR)

default: $(TARGET)

$(TARGET): $(PMILIB_TARGET)
	$(CC) $(CFLAGS) -c atl_ofi.c
	$(CC) $(CFLAGS) -c $(PMI_RT_DIR)/pmi_rt.c
	ar -cvq $(TARGET).a atl_ofi.o pmi_rt.o
	$(CC) $(OFI_CFLAGS) -shared -Wl,-soname,$(TARGET).so.1 -o $(TARGET).so.1.0 *.o \
	$(PMI_LDFLAGS) $(OFI_LDFLAGS)

$(PMILIB_TARGET):
	cd $(PMILIB_DIR) && $(MAKE) CC="$(CC)" EXTRA_CFLAGS="$(EXTRA_CFLAGS)" EXTRA_LDFLAGS="$(EXTRA_LDFLAGS)" \
	                            ENABLE_DEBUG=$(ENABLE_DEBUG) USE_SECURITY_FLAGS=$(USE_SECURITY_FLAGS) \
	                            libpmi

clean:
	rm -f *.a *.so *.so.1.0 *.o