
ENABLE_DEBUG ?= 1
SHELL = bash
INCLUDES = -I$(MLSL_ROOT)/intel64/include
LDFLAGS  = -L$(MLSL_ROOT)/intel64/lib -lmlsl -lrt
CXX     ?= g++
tests    = mlsl_multiple_comms_test

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
	echo "running mlsl_block_nonblock_test ..." ; \
	echo "mpiexec.hydra -n 4 -ppn 1 -l ./$(tests)" > output.log 2>&1 ; \
	mpiexec.hydra -n 4 -ppn 1 ./$(tests) >> output.log 2>&1 ; \
    if [ "$${PIPESTATUS[0]}" -ne 0 ]; then \
		echo "Run FAILED." ; \
	fi ; \
	echo "result in output.log"

clean:
	-rm -f $(tests).o $(tests) *.log
