USE_SECURITY_FLAGS ?= 0
ENABLE_DEBUG       ?= 0

CC         ?= gcc

BASE_DIR = $(shell pwd)

ARX86    = ar

MLSL_DIR     = $(BASE_DIR)
SRC_DIR      = $(MLSL_DIR)/src
EXAMPLES_DIR = $(MLSL_DIR)/examples

CFLAGS  += -Wall -Werror -pthread -O2 -std=gnu99 -I. -fPIC -D_GNU_SOURCE -fvisibility=internal
LDFLAGS += -L$(BASE_DIR)/../atl -lmlsl_atl -lpthread -ldl

TARGET   = libmlsl2

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

CFLAGS += -DPT_LOCK_SPIN=1

default: $(TARGET)
	make -C $(EXAMPLES_DIR)

INCS += -I$(MLSL_DIR)/include -iquote $(SRC_DIR)/sched \
        -I$(SRC_DIR)/coll -I$(SRC_DIR)/comp \
        -I$(SRC_DIR)/exec -I$(SRC_DIR)/transport \
        -I$(SRC_DIR)/common -I$(SRC_DIR)/common/comm \
        -I$(SRC_DIR)/common/env -I$(SRC_DIR)/common/global \
        -I$(SRC_DIR)/common/log -I$(SRC_DIR)/common/request \
        -I$(SRC_DIR)/common/utils \
        -I$(SRC_DIR)/../../atl

SRCS += $(SRC_DIR)/mlsl.c
SRCS += $(SRC_DIR)/coll/coll.c
SRCS += $(SRC_DIR)/comp/comp.c
SRCS += $(SRC_DIR)/sched/sched.c
SRCS += $(SRC_DIR)/sched/sched_cache.c
SRCS += $(SRC_DIR)/exec/exec.c
SRCS += $(SRC_DIR)/exec/worker.c
SRCS += $(SRC_DIR)/common/comm/comm.c
SRCS += $(SRC_DIR)/common/env/env.c
SRCS += $(SRC_DIR)/common/global/global.c
SRCS += $(SRC_DIR)/common/log/log.c
SRCS += $(SRC_DIR)/common/request/request.c

OBJS := $(SRCS:.c=.o)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c $(CFLAGS) $(INCS) $< -o $@

$(SRC_DIR)/libmlsl2.a: $(OBJS)
	$(ARX86) rcs $(SRC_DIR)/$(TARGET).a $(OBJS)

$(SRC_DIR)/libmlsl2.so: $(OBJS)
	$(CC) $(CFLAGS) $(INCS) $(LIBS) -shared -Wl,-soname,libmlsl2.so -o $(SRC_DIR)/libmlsl2.so $(OBJS) $(LDFLAGS)

$(TARGET): $(SRC_DIR)/libmlsl2.a $(SRC_DIR)/libmlsl2.so

clean:
	rm -f $(SRC_DIR)/libmlsl2.a $(SRC_DIR)/libmlsl2.so
	find . -name "*.o" -type f -delete
	make -C $(EXAMPLES_DIR) clean