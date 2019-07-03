
ENABLE_DEBUG ?= 1
INCLUDES = -I$(ICCL_ROOT)/intel64/include -I$(MKLROOT)/include
LDFLAGS  = -L$(ICCL_ROOT)/intel64/lib -L$(MKLROOT)/lib/intel64 -liccl -lmkl_rt -lrt
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

USE_ICCL = 1
ifeq ($(USE_ICCL),1)
  CFLAGS += -DUSE_ICCL=1
else
  CFLAGS += -DUSE_ICCL=0
endif

CXX_COMPILE = $(CXX) $(CFLAGS) $(INCLUDES)

$(TARGET): mlp.cpp iccl_wrapper.cpp
	echo "running iccl_wrappers_test ..." ; \
	$(CXX_COMPILE) -c mlp.cpp
	$(CXX_COMPILE) -c iccl_wrapper.cpp
	$(CXX_COMPILE) mlp.o iccl_wrapper.o -o $(TARGET) $(LDFLAGS)
	if [ "$$?" -ne 0 ]; then \
		echo "Run FAILED." ; \
		exit 1; \
	fi ; \
	



all: $(TARGET)

run:

clean:
	-rm -f *.o $(TARGET) *.log
