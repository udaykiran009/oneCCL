
ENABLE_DEBUG ?= 1
SHELL = bash
INCLUDES = -I$(ICCL_ROOT)/intel64/include
LDFLAGS  = -L$(ICCL_ROOT)/intel64/lib -liccl -lrt
CXX     ?= g++
tests    = iccl_configure_test

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
	-rm -f *.log
	echo "running iccl_configure_test ..." ; \
	for server_count in 1 2 ; do \
		for group_count in 1 2 ; do \
			for group_size in 1 2 3 ; do \
				process_count=$$((server_count+group_count*group_size)) ; \
				echo "process_count = $$process_count, server_count = $$server_count, group_count = $$group_count group_size = $$group_size" >> output.log 2>&1 ; \
				mpiexec.hydra  -n $$process_count -ppn 1 -l ./$(tests) $$server_count $$group_count >> output.log 2>&1 ; \
				if [ "$${PIPESTATUS[0]}" -ne 0 ]; then \
					echo "Run FAILED." ; \
				fi ; \
			done ; \
		done ; \
	done ;

clean:
	-rm -f $(tests).o $(tests) *.log
