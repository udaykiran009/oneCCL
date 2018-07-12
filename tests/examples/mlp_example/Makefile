
ENABLE_DEBUG ?= 1
INCLUDES = -I$(MLSL_ROOT)/intel64/include -I$(MKLROOT)/include
LDFLAGS  = -L$(MLSL_ROOT)/intel64/lib -L$(MKLROOT)/lib/intel64 -lmlsl -lmkl_rt -lrt
CXX      = icpc
TARGET   = mlp

#ifneq ($(CXX), icpc)
#    $(error Setup Intel compiler to build that test (current compiler is $(CXX)))
#endif

ifeq ($(ENABLE_DEBUG),1)
  CFLAGS += -O0 -g
else
  CFLAGS += -O2
endif

USE_MLSL = 1
ifeq ($(USE_MLSL),1)
  CFLAGS += -DUSE_MLSL=1
else
  CFLAGS += -DUSE_MLSL=0
endif

CXX_COMPILE = $(CXX) $(CFLAGS) $(INCLUDES)

$(TARGET): mlp.cpp mlsl_wrapper.cpp
	echo "running mlsl_wrappers_test ..." ; \
	$(CXX_COMPILE) -c mlp.cpp
	$(CXX_COMPILE) -c mlsl_wrapper.cpp
	$(CXX_COMPILE) mlp.o mlsl_wrapper.o -o $(TARGET) $(LDFLAGS)
	if [ "$$?" -ne 0 ]; then \
		echo "Run FAILED." ; \
		exit 1; \
	fi ; \
	



all: $(TARGET)

run:

clean:
	-rm -f *.o $(TARGET) *.log
