
ENABLE_DEBUG ?= 1
SHELL = bash
INCLUDES = -I$(MLSL_ROOT)/intel64/include
LDFLAGS  = -L$(MLSL_ROOT)/intel64/lib -lmlsl -lrt
CXX     ?= g++
tests    = mlsl_test_replicate

ifeq ($(ENABLE_DEBUG),1)
  CFLAGS += -O0 -g
else
  CFLAGS += -O2
endif

CXX_COMPILE = $(CXX) $(CFLAGS) $(INCLUDES)

.SUFFIXES:
.SUFFIXES: .cpp

.cpp:
	$(CXX_COMPILE) -o $* $< $(LDFLAGS)

all: $(tests)

run: all
	echo "running mlsl_test_replicate ..." ; \
	echo "mpiexec.hydra -n 6 -ppn 3 ./$(tests) 1 0 0 2" > output.log 2>&1 ; \
	mpiexec.hydra  -n 6 -ppn 3 ./$(tests) 1 0 0 2 >> output.log 2>&1 ; \
	if [ "$${PIPESTATUS[0]}" -ne 0 ]; then \
		echo "Run FAILED." ; \
	fi ; \
	echo "result in output.log"

clean:
	-rm -f $(tests).o $(tests) *.log
