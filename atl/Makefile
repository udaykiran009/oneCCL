USE_SECURITY_FLAGS ?= 0
ENABLE_DEBUG       ?= 0

CC         ?= gcc

BASE_DIR = $(shell pwd)

ATL_OFI_TARGET       = libmlsl_atl_ofi
ATL_OFI_DIR          = $(BASE_DIR)/ofi

CFLAGS  += $(EXTRA_CFLAGS) -Wall -O2 -std=gnu99 -I. -fPIC -D_GNU_SOURCE -I$(ATL_OFI_DIR)
LDFLAGS += $(EXTRA_LDFLAGS)

TARGET = libmlsl_atl

ifeq ($(CC), icc)
    LDFLAGS += -static-intel
endif

ifeq ($(USE_SECURITY_FLAGS), 1)
    SECURITY_CXXFLAGS   = -Wformat -Wformat-security -D_FORTIFY_SOURCE=2 -fstack-protector
    SECURITY_LDFLAGS    = -z noexecstack -z relro -z now
    CFLAGS              += $(SECURITY_CXXFLAGS)
    LDFLAGS             += $(SECURITY_LDFLAGS)
endif

ifeq ($(HAVE_OFI_DL), 0)
    LDFLAGS += -D$(ATL_OFI_DIR) -lmlsl_atl_ofi
endif

default: $(TARGET)

$(TARGET): $(ATL_OFI_TARGET)
	$(CC) $(CFLAGS) -c atl.c
	ar -cvq $(TARGET).a atl.o
	$(CC) $(CFLAGS) -shared -Wl,-soname,$(TARGET).so.1 -o $(TARGET).so.1.0 *.o $(LDFLAGS)

$(ATL_OFI_TARGET):
	cd $(ATL_OFI_DIR) && $(MAKE) CC="$(CC)" EXTRA_CFLAGS="$(EXTRA_CFLAGS)" EXTRA_LDFLAGS="$(EXTRA_LDFLAGS)" \
	                             ENABLE_DEBUG=$(ENABLE_DEBUG) USE_SECURITY_FLAGS=$(USE_SECURITY_FLAGS) \
	                             libmlsl_atl_ofi

clean:
	rm -f *.a *.so *.so.1.0 *.o
	cd $(ATL_OFI_DIR) && $(MAKE) clean