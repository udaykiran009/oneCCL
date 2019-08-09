ENABLE_DEBUG ?= 0
SHELL = bash
INCLUDES = -I$(CCL_ROOT)/include
LDFLAGS  = -L$(CCL_ROOT)/lib/ -lccl -lOpenCL  -lsycl

CXX     = clang++
tests    =  $(patsubst %.cpp, %.out, $(wildcard *.cpp))
# exclude_tests = unordered_allreduce.out allgatherv_iov.out sparse_allreduce.out custom_allreduce.out
# filtered_tests    = $(filter-out $(exclude_tests), $(tests))
ENABLE_SYCL = 1

ifeq ($(ENABLE_DEBUG),1)
  CXXFLAGS += -O0 -g
else
  CXXFLAGS += -Wall -Werror  -std=c++11 -fsycl -Wno-defaulted-function-deleted -Wno-mismatched-tags -Wno-unused-private-field -Wno-unneeded-internal-declaration -Wno-delete-non-abstract-non-virtual-dtor -Wno-reorder -DENABLE_SYCL=1 -D__STRICT_ANSI__ -fsycl
endif


CXX_COMPILE = $(CXX)  $(CXXFLAGS) $(INCLUDES)
CXX_LINK = $(CXX) $(CXXFLAGS) $(LDFLAGS)

.SUFFIXES:
.SUFFIXES: .cpp

%.o: %.cpp 
	$(CXX_COMPILE)  $(CXXFLAGS) -o $@ -c $<
	
%.out: %.o 
	$(CXX_LINK) $(CXXFLAGS) $^ -o $@; \

run: all 
	for test in $(tests); do \
		echo "running $$test test ..." ; \
		mpiexec.hydra -n 2 -ppn 1 -l ./$$test &> ./$$test.log; \
		grep -r 'PASSED' ./$$test.log; \
		if [ "$${PIPESTATUS[0]}" -ne 0 ]; then \
			echo "$$test FAILED." ; \
			exit -1 ; \
		else \
			echo "$$test test ... PASSED." ; \
		fi ; \
	done

all: $(tests)

clean:
	-rm -f $(tests).o $(tests).out 


clean_logs: 
	-rm -f  *.log