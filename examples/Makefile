ENABLE_DEBUG ?= 0
SHELL = bash

tests               = $(patsubst %.cpp, %.out, $(wildcard *.cpp))
CXX                ?= g++
ENABLE_SYCL        ?= 1
INCLUDES            = -I./../include
LDFLAGS             = -lccl -ldl -lpthread -lrt
CXXFLAGS            = -D__STRICT_ANSI__

ifeq ($(ENABLE_SYCL),1)
    CXX      = clang++
    INCLUDES += -I$(CCL_ROOT)/include/cpu_gpu_dpcpp
    LDFLAGS  += -L$(CCL_ROOT)/lib/cpu_gpu_dpcpp -lsycl
    CXXFLAGS += -fsycl -Wno-pessimizing-move -Wno-dangling-field -Wno-return-type -Wno-defaulted-function-deleted -Wno-mismatched-tags -Wno-unused-private-field -Wno-unneeded-internal-declaration -Wno-delete-non-abstract-non-virtual-dtor -Wno-reorder
else
    INCLUDES += -I$(CCL_ROOT)/include/cpu_icc
    LDFLAGS  += -L$(CCL_ROOT)/lib/cpu_icc
endif

ifeq ($(ENABLE_DEBUG),1)
  CXXFLAGS += -O0 -g
else
  CXXFLAGS += -std=c++11 -O3 -DNDEBUG  -O3 -std=gnu++11
endif

CXX_COMPILE = $(CXX) $(CXXFLAGS) $(INCLUDES)
CXX_LINK = $(CXX) $(CXXFLAGS)

.SUFFIXES:
.SUFFIXES: .cpp

%.o: %.cpp 
	$(CXX_COMPILE) -o $@ -c $<

%.out: %.o 
	$(CXX_LINK) $^ $(LDFLAGS) -o $@; \

all: $(tests)

clean:
	-rm -f $(tests).o $(tests).out
	-rm -f  *.log
