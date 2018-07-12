
ENABLE_DEBUG ?= 1
SHELL = bash
# Path to Google Test
GTEST_DIR = ../googletest
CXX      ?= g++
MPIRUN    = mpirun
BASE_DIR  = $(shell pwd)

# MLSL root code
MLSL_SRC_DIR  = $(BASE_DIR)/../..
ICT_INFRA_DIR = $(MLSL_SRC_DIR)/ict-infra

# Flags passed to the preprocessor.
# Set Google Test's header directory as a system directory, such that
# the compiler doesn't generate warnings in Google Test headers.
CPPFLAGS += -isystem $(GTEST_DIR)/include

CXXFLAGS += -I$(MLSL_ROOT)/intel64/include -I$(MLSL_ROOT)/../mpirt/include -L$(MLSL_ROOT)/intel64/lib -L$(MLSL_ROOT)/../mpirt/intel64/lib -lmlsl -lmpi -I$(MLSL_SRC_DIR)/src
CXXFLAGS += $(EXTRA_CFLAGS)

ifeq ($(CODECOV),1)
    CXXFLAGS += -prof-gen=srcpos
endif

# Flags passed to the C++ compiler.
CXXFLAGS += -Wall -Wextra -pthread

# All tests produced by this Makefile.	Remember to add new tests you
# created to the list.
TESTS = $(patsubst %.cpp, %.out, $(wildcard *.cpp))

# All Google Test headers. Usually you shouldn't change this definition.
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h $(GTEST_DIR)/include/gtest/internal/*.h

ifneq ($(CXX), icpc)
    CXXFLAGS += -std=c++0x
else
    CXXFLAGS += -std=c++11
endif

CXXFLAGS += -DENABLE_DEBUG=$(ENABLE_DEBUG)
ifeq ($(ENABLE_DEBUG),1)
    CXXFLAGS += -O0 -g
else
    CXXFLAGS += -O2
endif

# House-keeping build targets.

all: $(TESTS)

run:

	for test in $(TESTS); do \
		$(MPIRUN) -n 4 -ppn 1 -l ./$$test --gtest_output="xml:result/$$test.xml"; \
		if [ "$${PIPESTATUS[0]}" -ne 0 ]; then \
			echo "Run FAILED." ; \
		fi ; \
	done
	MLSL_MSG_PRIORITY=1 MLSL_NUM_SERVERS=2 MLSL_MSG_PRIORITY_THRESHOLD=7 $(MPIRUN) -n 2 -ppn 1 -l ./allreduce.out --gtest_filter=*AllReduce* --gtest_output="xml:result/allreduce_rabenseifner.xml"

dev:
	for num_servers in 0 1 4; do \
		echo -n "##################### MLSL_NUM_SERVERS = $$num_servers #####################"; echo -n; \
		for test in $(TESTS); do \
			MLSL_NUM_SERVERS=$$num_servers $(MPIRUN) -n 4 -ppn 1 -l ./$$test --gtest_output="xml:result/$$test.$$num_servers.xml"; \
		done \
	done

clean:
	rm -f $(TESTS) gtest.a gtest_main.a *.o *.dyn *.spi *.spl *.dpi *.dpi.lock
	rm -rf result


codecov:
	profmerge -prof_dpi pgopti.dpi
	codecov -prj msl_cc -comp $(ICT_INFRA_DIR)/code_coverage/codecov_filter_mlsl.txt -spi ../../pgopti.spi -dpi pgopti.dpi -xmlbcvrgfull codecov.xml

# Builds gtest.a and gtest_main.a.

# Usually you shouldn't tweak such internal variables, indicated by a
# trailing _.
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized.  This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.
gtest-all.o: $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
			$(GTEST_DIR)/src/gtest-all.cc

gtest_main.o: $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
			$(GTEST_DIR)/src/gtest_main.cc

gtest.a: gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

gtest_main.a: gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

# Builds a sample test.	 A test should link with either gtest.a or
# gtest_main.a, depending on whether it defines its own main()
# function.


%.o: %.cc $(GTEST_HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $<

%.out: %.o gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@
