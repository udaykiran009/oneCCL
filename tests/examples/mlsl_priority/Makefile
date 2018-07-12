
ENABLE_DEBUG ?= 1
SHELL = bash
INCLUDES = -I$(MLSL_ROOT)/intel64/include
LDFLAGS  = -L$(MLSL_ROOT)/intel64/lib -lmlsl -lrt
CXX     ?= g++
tests    = mlsl_priority

ifeq ($(ENABLE_DEBUG),1)
  CFLAGS += -O0 -g
else
  CFLAGS += -O2
endif

CFLAGS  += $(EXTRA_CFLAGS)
LDFLAGS += $(EXTRA_LDFLAGS)

CXX_COMPILE = $(CXX) $(CFLAGS) $(INCLUDES)

.SUFFIXES:
.SUFFIXES: .cpp

.cpp:
	$(CXX_COMPILE) -o $* $< $(LDFLAGS)

all: $(tests)

run: all
	echo "running $(tests) ..." ; \
	cmd="MLSL_MSG_PRIORITY=1 mpiexec.hydra -n 2 -ppn 1 -l ./$(tests)" ; \
	echo $$cmd > output.log 2>&1 ; \
	eval $$cmd >> output.log 2>&1 ; \
	if [ "$${PIPESTATUS[0]}" -ne 0 ]; then \
		echo "Run FAILED." ; \
	fi ; \
	echo "result in output.log"

clean:
	-rm -f $(tests).o $(tests) *.log
