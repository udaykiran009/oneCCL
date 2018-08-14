
ENABLE_DEBUG ?= 1
SHELL = bash

#INCLUDES = -I$(MLSL_ROOT)/intel64/include
#LDFLAGS  = -L$(MLSL_ROOT)/intel64/lib -lmlsl -lrt

INCLUDES = -I ../include
LDFLAGS  = -L ../src -lmlsl2 -lrt

CC      ?= gcc

c_test    = allreduce

tests     = $(c_test)

ifeq ($(ENABLE_DEBUG),1)
  CFLAGS += -O0 -g
else
  CFLAGS += -O2
endif

CC_COMPILE = $(CC) $(CFLAGS) $(INCLUDES)

CFLAGS  += $(EXTRA_CFLAGS)
LDFLAGS += $(EXTRA_LDFLAGS)


.SUFFIXES:
.SUFFIXES: .c

.c:
	$(CC_COMPILE) -o $* $< $(LDFLAGS)

all: $(tests)

clean_log:
	-rm -f *.log

clean:
	-rm -f $(c_test).o $(c_test) *.log
