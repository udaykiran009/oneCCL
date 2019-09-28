ENABLE_DEBUG ?= 0
SHELL = bash
INCLUDES = -I$(CCL_ROOT)/include -I./../include
LDFLAGS  = -L$(CCL_ROOT)/lib/ -lOpenCL -ldl -lpthread -lccl -lsycl -lstdc++ -lrt

CXX         = clang++
tests       =  $(patsubst %.cpp, %.out, $(wildcard *.cpp))
ENABLE_SYCL = 1
CMAKE_CLANG_FLAGS = -Wno-pessimizing-move -Wno-dangling-field -Wno-return-type -Wno-defaulted-function-deleted -Wno-mismatched-tags -Wno-unused-private-field -Wno-unneeded-internal-declaration -Wno-delete-non-abstract-non-virtual-dtor -Wno-reorder -DENABLE_SYCL=1 -D__STRICT_ANSI__ -fsycl
ifeq ($(ENABLE_DEBUG),1)
  CXXFLAGS += -O0 -g
else
  CXXFLAGS += -std=c++11 -O3 -DNDEBUG  -O3   -std=gnu++11 $(CMAKE_CLANG_FLAGS)
endif


CXX_COMPILE = $(CXX)  $(CXXFLAGS) $(INCLUDES)
CXX_LINK = $(CXX) $(CXXFLAGS) $(LDFLAGS)

.SUFFIXES:
.SUFFIXES: .cpp

%.o: %.cpp 
	$(CXX_COMPILE)  $(CXXFLAGS) -o $@ -c $<

%.out: %.o 
	$(CXX_LINK) $(CXXFLAGS) $^ -o $@; \

all: $(tests)

clean:
	-rm -f $(tests).o $(tests).out 


clean_logs: 
	-rm -f  *.log