ENABLE_DEBUG ?= 0
SHELL = bash

C_SOURCES   = $(wildcard *.c)
CXX_SOURCES = $(wildcard *.cpp)
C_OBJECTS   = $(patsubst %.c, %.o, $(C_SOURCES))
CXX_OBJECTS = $(patsubst %.cpp, %.o, $(CXX_SOURCES))
OBJECTS     = $(C_OBJECTS) $(CXX_OBJECTS)
EXAMPLES    = $(patsubst %.o,%.out,$(OBJECTS))

CC                 ?= gcc
CXX                ?= g++
ENABLE_SYCL        ?= 1
INCLUDES            = -I./../include
LDFLAGS             = -lccl -ldl -lpthread -lrt
CXXFLAGS            = -D__STRICT_ANSI__

ifeq ($(ENABLE_SYCL),1)
    CC         = clang
    CXX        = dpcpp
    INCLUDES  += -I$(CCL_ROOT)/include/cpu_gpu_dpcpp
    LDFLAGS   += -L$(CCL_ROOT)/lib/cpu_gpu_dpcpp -lsycl
    CLANGFLAGS = -fsycl -Wno-pessimizing-move -Wno-dangling-field -Wno-return-type -Wno-defaulted-function-deleted -Wno-mismatched-tags -Wno-unused-private-field -Wno-unneeded-internal-declaration -Wno-delete-non-abstract-non-virtual-dtor -Wno-reorder
    CCFLAGS   += $(CLANGFLAGS)
    CXXFLAGS  += $(CLANGFLAGS)
else
    INCLUDES += -I$(CCL_ROOT)/include/cpu_icc
    LDFLAGS  += -L$(CCL_ROOT)/lib/cpu_icc
endif

ifeq ($(ENABLE_DEBUG),1)
  CXXFLAGS += -O0 -g
else
  CCFLAGS  += -O3 -DNDEBUG -std=gnu99 
  CXXFLAGS += -O3 -DNDEBUG -std=gnu++11
endif

CC_COMPILE  = $(CC) $(CCFLAGS) $(INCLUDES)
CXX_COMPILE = $(CXX) $(CXXFLAGS) $(INCLUDES)

.SUFFIXES:
.SUFFIXES: .cpp

%.o: %.cpp
	$(CXX_COMPILE) -o $@ -c $<

%.o: %.c
	$(CC_COMPILE) -o $@ -c $<

%.out: %.o
	$(CXX) $^ $(LDFLAGS) -o $@

all: $(EXAMPLES)

clean:
	-rm -f  *.o *.out
	-rm -f  *.log
