USE_SECURITY_FLAGS ?= 0
ENABLE_DEBUG       ?= 0

CC       = icc

BASE_DIR = $(shell pwd)

ARX86    = ar

MLSL_DIR     = $(BASE_DIR)
SRC_DIR      = $(MLSL_DIR)/src
EXAMPLES_DIR = $(MLSL_DIR)/examples

ATL_OFI_DIR    = $(SRC_DIR)/atl/ofi
ATL_OFI_TARGET = libmlsl_atl_ofi

CFLAGS  += -Wall -Werror -pthread -std=gnu99 -I. -fPIC -D_GNU_SOURCE -fvisibility=internal
LDFLAGS += -lpthread -ldl

CFLAGS += -DATL_TRANSPORT_DL_DIR=\"$(ATL_OFI_DIR)\"

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
        -I$(SRC_DIR)/atl \
        -I$(ATL_OFI_DIR) \
        -I$(SRC_DIR)/coll -I$(SRC_DIR)/comp \
        -I$(SRC_DIR)/exec -I$(SRC_DIR)/parallelizer \
        -I$(SRC_DIR)/common -I$(SRC_DIR)/common/comm \
        -I$(SRC_DIR)/common/env -I$(SRC_DIR)/common/global \
        -I$(SRC_DIR)/common/log -I$(SRC_DIR)/common/request \
        -I$(SRC_DIR)/common/utils

SRCS += $(SRC_DIR)/mlsl.c
SRCS += $(SRC_DIR)/atl/atl.c
SRCS += $(SRC_DIR)/coll/coll.c
SRCS += $(SRC_DIR)/coll/allreduce.c
SRCS += $(SRC_DIR)/coll/barrier.c
SRCS += $(SRC_DIR)/coll/bcast.c
SRCS += $(SRC_DIR)/coll/reduce.c
SRCS += $(SRC_DIR)/coll/allgatherv.c
SRCS += $(SRC_DIR)/comp/comp.c
SRCS += $(SRC_DIR)/sched/sched.c
SRCS += $(SRC_DIR)/sched/sched_cache.c
SRCS += $(SRC_DIR)/sched/sched_queue.c
SRCS += $(SRC_DIR)/exec/exec.c
SRCS += $(SRC_DIR)/exec/worker.c
SRCS += $(SRC_DIR)/parallelizer/parallelizer.c
SRCS += $(SRC_DIR)/common/comm/comm.c
SRCS += $(SRC_DIR)/common/env/env.c
SRCS += $(SRC_DIR)/common/global/global.c
SRCS += $(SRC_DIR)/common/log/log.c
SRCS += $(SRC_DIR)/common/request/request.c
SRCS += $(SRC_DIR)/common/utils/buf_pool.c

OBJS := $(SRCS:.c=.o)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c $(CFLAGS) $(INCS) $< -o $@

$(SRC_DIR)/$(TARGET).a: $(OBJS)
	$(ARX86) rcs $(SRC_DIR)/$(TARGET).a $(OBJS)

$(SRC_DIR)/$(TARGET).so: $(OBJS)
	$(CC) $(CFLAGS) $(INCS) $(LIBS) -shared -Wl,-soname,$(TARGET).so -o $(SRC_DIR)/$(TARGET).so $(OBJS) $(LDFLAGS)

$(TARGET): $(SRC_DIR)/$(TARGET).a $(SRC_DIR)/$(TARGET).so  $(ATL_OFI_TARGET)

$(ATL_OFI_TARGET):
	cd $(ATL_OFI_DIR) && $(MAKE) CC="$(CC)" ENABLE_DEBUG=$(ENABLE_DEBUG) USE_SECURITY_FLAGS=$(USE_SECURITY_FLAGS) libmlsl_atl_ofi

clean:
	rm -f $(SRC_DIR)/$(TARGET).a $(SRC_DIR)/$(TARGET).so
	find . -name "*.o" -type f -delete
	make -C $(EXAMPLES_DIR) clean
	make -C $(ATL_OFI_DIR) clean
