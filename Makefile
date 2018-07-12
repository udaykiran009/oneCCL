BASE_DIR = $(shell pwd)

SHELL=bash
include $(BASE_DIR)/Makefile.custom
SHELL = bash
COMPILER ?= gnu
#COMPILER ?= intel

# possible values: intel, openmpi, cray
MPIRT     ?= intel
MPIRT_DIR ?= $(BASE_DIR)/mpirt

USE_SECURITY_FLAGS ?= 1
ENABLE_DEBUG       ?= 0
ENABLE_CHKP        ?= 0
ENABLE_CHKP_INT    ?= 0
ENABLE_MT_MEMCPY   ?= 0

# will be propagated to other modules
EXTRA_CFLAGS  = 
EXTRA_LDFLAGS = 

ifeq ($(ENABLE_CHKP), 1)
    CHKP_FLAGS = -check-pointers=rw -check-pointers-dangling=all -check-pointers-undimensioned -check-pointers-narrowing -rdynamic
    EXTRA_CFLAGS  += $(CHKP_FLAGS) -DENABLE_CHKP=1
    EXTRA_LDFLAGS += $(CHKP_FLAGS)
endif

EXTRA_CFLAGS += -Wall -Werror

CXXFLAGS += $(EXTRA_CFLAGS)
LDFLAGS  += $(EXTRA_LDFLAGS)

ARX86       = ar
CXXFLAGS    += -fPIC
INCS        += -I$(INCLUDE_DIR) -I$(SRC_DIR)

ifeq ($(COMPILER), intel)
    CC        = icc
    CXX       = icpc
    CXXFLAGS += -std=c++11
    LDFLAGS  += -static-intel
else ifeq ($(COMPILER), gnu)
    CC        = gcc
    CXX       = g++
    CXXFLAGS += -std=c++0x
else
    $(error Unsupported compiler $(COMPILER))
endif

ifeq ($(ENABLE_MT_MEMCPY), 1)
    ifeq ($(COMPILER), intel)
        CXXFLAGS += -qopenmp
        LDFLAGS  += -qopenmp -liomp5
    else
        CXXFLAGS += -fopenmp
        LDFLAGS  += -fopenmp -lgomp
    endif
endif

ifeq ($(ENABLE_DEBUG), 1)
    USE_SECURITY_FLAGS = 0
    CXXFLAGS += -O0 -g
else
    CXXFLAGS += -O2
endif

ifeq ($(CODECOV),1)
    CXXFLAGS += -prof-gen=srcpos -prof-src-root-cwd
endif

EPLIB_TARGET        = libep
ATLLIB_TARGET       = libmlsl_atl
TARGET              = libmlsl
TESTS_TARGET        = tests
SRC_DIR             = $(BASE_DIR)/src
INCLUDE_DIR         = $(BASE_DIR)/include
SCRIPTS_DIR         = $(BASE_DIR)/scripts
DOC_DIR             = $(BASE_DIR)/doc
TESTS_DIR           = $(BASE_DIR)/tests
EXAMPLES_DIR        = $(TESTS_DIR)/examples
UTESTS_DIR          = $(TESTS_DIR)/unit
FTESTS_DIR          = $(TESTS_DIR)/functional
EXAMPLES_DIR        = $(TESTS_DIR)/examples
EPLIB_DIR           = $(BASE_DIR)/eplib
ATLLIB_DIR           = $(BASE_DIR)/atl
QUANT_DIR           = $(BASE_DIR)/quant
DOXYGEN_DIR         = $(BASE_DIR)/doc/doxygen
ICT_INFRA_DIR       = $(BASE_DIR)/ict-infra
PREFIX              ?= $(BASE_DIR)/_install
TMP_COVERAGE_DIR    ?= $(BASE_DIR)
CODECOV_SRCROOT     ?= $(BASE_DIR)
INTEL64_PREFIX      = $(PREFIX)/intel64
STAGING             = $(BASE_DIR)/_staging
INTEL64_STAGING     = $(STAGING)/intel64
TMP_DIR             = $(BASE_DIR)/_tmp
TMP_ARCHIVE_DIR     = $(TMP_DIR)/$(MLSL_ARCHIVE_PREFIX)$(MLSL_ARCHIVE_SUFFIX)
CXXFLAGS            += -I$(MPIRT_DIR)/include

ifeq ($(MPIRT),$(filter $(MPIRT), intel openmpi))
    LDFLAGS             += -L$(MPIRT_DIR)/lib -lmpi -ldl -lrt -lpthread
else ifeq ($(MPIRT), cray)
    LDFLAGS             += -L$(MPIRT_DIR)/lib -lmpich -ldl -lrt -lpthread
endif

ifeq ($(USE_SECURITY_FLAGS),1)
    SECURITY_CXXFLAGS   = -Wformat -Wformat-security -D_FORTIFY_SOURCE=2 -fstack-protector
    SECURITY_LDFLAGS    = -z noexecstack -z relro -z now
    CXXFLAGS            += $(SECURITY_CXXFLAGS)
    LDFLAGS             += $(SECURITY_LDFLAGS)
endif

CXXFLAGS += -DMLSL_YEAR=$(MLSL_YEAR) -DMLSL_FULL_VERSION=$(MLSL_FULL_VERSION) -DMLSL_OFFICIAL_VERSION="$(MLSL_OFFICIAL_VERSION)" -DMLSL_GIT_VERSION="STR($(MLSL_GIT_VERSION))"

SRCS += src/mlsl.cpp
SRCS += src/mlsl_impl.cpp
SRCS += src/mlsl_impl_stats.cpp
SRCS += src/log.cpp
SRCS += src/env.cpp
SRCS += src/c_bind.cpp
SRCS += src/sysinfo.cpp

ifeq ($(ENABLE_CHKP_INT), 1)
    SRCS += src/pointer_checker.cpp
endif

INCS        += -I$(EPLIB_DIR) -I$(QUANT_DIR)
EPLIB_OBJS  := $(EPLIB_DIR)/*.o
LIBS        += $(EPLIB_OBJS)
SRCS        += src/comm_ep.cpp
ifeq ($(BASEFAST),1)
    ENABLE_BASEMPI_COMM_FAST=1
endif

OBJS := $(SRCS:.cpp=.o)

yesnolist += ENABLE_DEBUG
yesnolist += ENABLE_CHKP_INT

DEFS += $(strip $(foreach var, $(yesnolist), $(if $(filter 1, $($(var))), -D$(var))))
DEFS += $(strip $(foreach var, $(deflist), $(if $($(var)), -D$(var)=$($(var)))))

ifneq ($(strip $(DEFS)),)
    $(info Using Options:$(DEFS))
endif

all: $(EPLIB_TARGET) $(TARGET)

dev:
	$(MAKE) clean && $(MAKE) all && $(MAKE) install

dev_testing:
#	$(MAKE) -C $(UTESTS_DIR)   CC="$(CC)" CXX="$(CXX)" EXTRA_CFLAGS="$(EXTRA_CFLAGS)" EXTRA_LDFLAGS="$(EXTRA_LDFLAGS)" ENABLE_DEBUG=$(ENABLE_DEBUG) && $(MAKE) -C $(UTESTS_DIR)   dev
	$(MAKE) -C $(FTESTS_DIR)   CC="$(CC)" CXX="$(CXX)" EXTRA_CFLAGS="$(EXTRA_CFLAGS)" EXTRA_LDFLAGS="$(EXTRA_LDFLAGS)" ENABLE_DEBUG=$(ENABLE_DEBUG) && $(MAKE) -C $(FTESTS_DIR)   dev
#	$(MAKE) -C $(EXAMPLES_DIR) CC="$(CC)" CXX="$(CXX)" EXTRA_CFLAGS="$(EXTRA_CFLAGS)" EXTRA_LDFLAGS="$(EXTRA_LDFLAGS)" ENABLE_DEBUG=$(ENABLE_DEBUG) && $(MAKE) -C $(EXAMPLES_DIR) dev

testing:
	$(MAKE) -s -C $(UTESTS_DIR)   CC="$(CC)" CXX="$(CXX)" EXTRA_CFLAGS="$(EXTRA_CFLAGS)" EXTRA_LDFLAGS="$(EXTRA_LDFLAGS)" ENABLE_DEBUG=$(ENABLE_DEBUG) && $(MAKE) -s -C $(UTESTS_DIR)   run
	$(MAKE) -s -C $(FTESTS_DIR)   CC="$(CC)" CXX="$(CXX)" EXTRA_CFLAGS="$(EXTRA_CFLAGS)" EXTRA_LDFLAGS="$(EXTRA_LDFLAGS)" ENABLE_DEBUG=$(ENABLE_DEBUG) && $(MAKE) -s -C $(FTESTS_DIR)   run
	$(MAKE) -s -C $(EXAMPLES_DIR) CC="$(CC)" CXX="$(CXX)" EXTRA_CFLAGS="$(EXTRA_CFLAGS)" EXTRA_LDFLAGS="$(EXTRA_LDFLAGS)" ENABLE_DEBUG=$(ENABLE_DEBUG) && $(MAKE) -s -C $(EXAMPLES_DIR) run

examples:
	$(MAKE) -C $(EXAMPLES_DIR) CC="$(CC)" CXX="$(CXX)" MLSL_ROOT=$(PREFIX) EXTRA_CFLAGS="$(EXTRA_CFLAGS)" EXTRA_LDFLAGS="$(EXTRA_LDFLAGS)" ENABLE_DEBUG=$(ENABLE_DEBUG)

codecov:
	cd $(UTESTS_DIR) && profmerge -prof_dpi pgopti_unit.dpi && cp pgopti_unit.dpi $(BASE_DIR)/pgopti_unit.dpi
	cd $(FTESTS_DIR) && profmerge -prof_dpi pgopti_func.dpi && cp pgopti_func.dpi $(BASE_DIR)/pgopti_func.dpi
#	cd $(EXAMPLES_DIR) && profmerge -prof_dpi pgopti_examples.dpi && cp pgopti_examples.dpi $(BASE_DIR)/pgopti_examples.dpi
	profmerge -prof_dpi pgopti.dpi -a pgopti_unit.dpi pgopti_func.dpi
	codecov -prj mlsl -comp $(ICT_INFRA_DIR)/code_coverage/codecov_filter_mlsl.txt -spi $(TMP_COVERAGE_DIR)/pgopti.spi -dpi pgopti.dpi -xmlbcvrgfull codecov.xml -srcroot $(CODECOV_SRCROOT)
	python $(ICT_INFRA_DIR)/code_coverage/codecov_to_cobertura.py codecov.xml coverage.xml

$(CUSTOM_MAKE_FILE): checkoptions

.lastoptions: checkoptions

checkoptions: ;
	@echo $(DEFS) > .curoptions
	@if ! test -f .lastoptions || ! diff .curoptions .lastoptions > /dev/null ; then mv -f .curoptions .lastoptions ; fi
	@rm -f .curoptions

$(TARGET): src/libmlsl.a src/$(LIBMLSL_SO_FILENAME)

# TODO: need to link libmpi.a
src/libmlsl.a: $(OBJS)
	$(ARX86) rcs $(SRC_DIR)/$(TARGET).a $(OBJS) $(EPLIB_OBJS)

src/$(LIBMLSL_SO_FILENAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCS) $(LIBS) -shared -Wl,-soname,$(LIBMLSL_SONAME) -o $(SRC_DIR)/$(LIBMLSL_SO_FILENAME) $(OBJS) $(LDFLAGS)

src/%.o: $(SRC_DIR)/%.cpp .lastoptions
	$(CXX) -c $(CXXFLAGS) $(DEFS) $(INCS) $< -o $@

clean:
	rm -f $(SRC_DIR)/*.o $(SRC_DIR)/*.d $(SRC_DIR)/$(LIBMLSL_SO_FILENAME) $(SRC_DIR)/*.a .lastoptions *.tgz *.log
	rm -f CODE_COVERAGE.HTML codecov.xml coverage.xml *.dpi *.dpi.lock *.dyn *.spi *.spl
	rm -rf $(STAGING) $(TMP_DIR) CodeCoverage coverage
	cd $(EPLIB_DIR) && $(MAKE) clean
	cd $(ATLLIB_DIR) && $(MAKE) clean
	cd $(SCRIPTS_DIR)/rpm_build && $(MAKE) clean
	cd $(UTESTS_DIR) && $(MAKE) clean
	cd $(FTESTS_DIR) && $(MAKE) clean
	cd $(EXAMPLES_DIR) && $(MAKE) clean

cleanall: clean

-include $(SRCS:.cpp=.d)

$(EPLIB_TARGET): $(ATLLIB_TARGET) eplib/ep_server eplib/libep.a

eplib/ep_server:
	cd $(EPLIB_DIR) && $(MAKE) CC="$(CC)" EXTRA_CFLAGS="$(EXTRA_CFLAGS)" EXTRA_LDFLAGS="$(EXTRA_LDFLAGS)" \
	                           ENABLE_DEBUG=$(ENABLE_DEBUG) USE_SECURITY_FLAGS=$(USE_SECURITY_FLAGS) \
	                           ENABLE_CHKP=$(ENABLE_CHKP) MPIRT="$(MPIRT)" MPIRT_DIR="$(MPIRT_DIR)" \
	                           ep_server

eplib/libep.a:
	cd $(EPLIB_DIR) && $(MAKE) CC="$(CC)" EXTRA_CFLAGS="$(EXTRA_CFLAGS)" EXTRA_LDFLAGS="$(EXTRA_LDFLAGS)" \
	                           ENABLE_DEBUG=$(ENABLE_DEBUG) USE_SECURITY_FLAGS=$(USE_SECURITY_FLAGS) \
	                           ENABLE_CHKP=$(ENABLE_CHKP) MPIRT="$(MPIRT)" MPIRT_DIR="$(MPIRT_DIR)" \
	                           libep

$(ATLLIB_TARGET):
	cd $(ATLLIB_DIR) && $(MAKE) CC="$(CC)" EXTRA_CFLAGS="$(EXTRA_CFLAGS)" EXTRA_LDFLAGS="$(EXTRA_LDFLAGS)" \
	                            ENABLE_DEBUG=$(ENABLE_DEBUG) USE_SECURITY_FLAGS=$(USE_SECURITY_FLAGS) \
	                            libmlsl_atl


rpm: _staging
	make rpm -C $(SCRIPTS_DIR)/rpm_build

deb: _staging
	make deb -C $(SCRIPTS_DIR)/rpm_build

yum_repo: rpm
	make yum_repo -C $(SCRIPTS_DIR)/rpm_build

apt_repo: deb
	make apt_repo -C $(SCRIPTS_DIR)/rpm_build

packages: _staging
	make all -C $(SCRIPTS_DIR)/rpm_build

archive: _staging
	rm -f $(BASE_DIR)/$(MLSL_ARCHIVE_NAME)
	rm -rf $(TMP_DIR)
	mkdir -p $(TMP_DIR)
	mkdir -p $(TMP_ARCHIVE_DIR)
	cd $(STAGING) && tar cfz $(TMP_ARCHIVE_DIR)/files.tar.gz ./*
	cp $(SCRIPTS_DIR)/install.sh $(TMP_ARCHIVE_DIR)/
	echo "Adding Copyright notice ..."
	mkdir -p $(STAGING)/_tmp
	echo "1a" > $(STAGING)/_tmp/copyright.sh
	echo "#" >> $(STAGING)/_tmp/copyright.sh
	sed -e "s|^|# |" -e "s|MLSL_SUBSTITUTE_COPYRIGHT_YEAR|$(MLSL_COPYRIGHT_YEAR)|g" $(BASE_DIR)/doc/copyright >> $(STAGING)/_tmp/copyright.sh
	echo "#" >> $(STAGING)/_tmp/copyright.sh
	echo "." >> $(STAGING)/_tmp/copyright.sh
	echo "w" >> $(STAGING)/_tmp/copyright.sh
	ed $(TMP_ARCHIVE_DIR)/install.sh < $(STAGING)/_tmp/copyright.sh > /dev/null 2>&1
	rm -rf $(STAGING)/_tmp
	sed -i -e "s|MLSL_SUBSTITUTE_FULL_VERSION|$(MLSL_FULL_VERSION)|g" $(TMP_ARCHIVE_DIR)/install.sh
	sed -i -e "s|MLSL_SUBSTITUTE_OFFICIAL_VERSION|$(MLSL_OFFICIAL_VERSION)|g" $(TMP_ARCHIVE_DIR)/install.sh
	cd $(TMP_DIR) && tar cfz $(BASE_DIR)/$(MLSL_ARCHIVE_NAME) $(MLSL_ARCHIVE_PREFIX)$(MLSL_ARCHIVE_SUFFIX)
	rm -rf $(TMP_DIR)

install: _staging
	rm -rf $(PREFIX)
	cp -pr $(STAGING) $(PREFIX)
	sed -i -e "s|MLSL_SUBSTITUTE_INSTALLDIR|$(PREFIX)|g" $(INTEL64_PREFIX)/bin/mlslvars.sh
	if [ "$(MPIRT)" == "intel" ]; then \
		sed -i -e "s|I_MPI_SUBSTITUTE_INSTALLDIR|$(PREFIX)|g" $(INTEL64_PREFIX)/etc/mpiexec.conf; \
	fi

doxygen:
	cd $(DOXYGEN_DIR) && doxygen $(DOXYGEN_DIR)/doxygen.config

_staging:
	rm -rf $(STAGING)
	mkdir -p $(INTEL64_STAGING)/bin
	mkdir -p $(INTEL64_STAGING)/lib
	mkdir -p $(INTEL64_STAGING)/include
	mkdir -p $(INTEL64_STAGING)/include/mlsl
	mkdir -p $(INTEL64_STAGING)/etc
	mkdir -p $(STAGING)/test
	mkdir -p $(STAGING)/example
	mkdir -p $(STAGING)/licensing/mpi
	mkdir -p $(STAGING)/licensing/mlsl
	mkdir -p $(STAGING)/doc
	mkdir -p $(STAGING)/doc/api
	cp $(EPLIB_DIR)/ep_server $(INTEL64_STAGING)/bin
	cp $(SRC_DIR)/$(TARGET).so.1.0 $(INTEL64_STAGING)/lib
	cd $(INTEL64_STAGING)/lib && ln -s $(TARGET).so.1.0 $(TARGET).so.1
	cd $(INTEL64_STAGING)/lib && ln -s $(TARGET).so.1 $(TARGET).so
	cp $(INCLUDE_DIR)/mlsl.hpp $(INTEL64_STAGING)/include
	cp $(INCLUDE_DIR)/mlsl.h $(INTEL64_STAGING)/include
	cp $(INCLUDE_DIR)/mlsl/mlsl.py $(INTEL64_STAGING)/include/mlsl
	cp $(INCLUDE_DIR)/mlsl/__init__.py $(INTEL64_STAGING)/include/mlsl
	cp $(SCRIPTS_DIR)/mlslvars.sh $(INTEL64_STAGING)/bin
	if [ "$(MPIRT)" == "intel" ]; then \
		cp $(MPIRT_DIR)/bin/mpiexec.hydra $(INTEL64_STAGING)/bin; \
		cp $(MPIRT_DIR)/bin/pmi_proxy $(INTEL64_STAGING)/bin; \
		cp $(MPIRT_DIR)/bin/mpirun $(INTEL64_STAGING)/bin; \
		cp $(MPIRT_DIR)/bin/hydra_persist $(INTEL64_STAGING)/bin; \
		cd $(INTEL64_STAGING)/bin && ln -s mpiexec.hydra mpiexec; \
		cp $(MPIRT_DIR)/lib/libmpi.so.12.0 $(INTEL64_STAGING)/lib; \
		cd $(INTEL64_STAGING)/lib && ln -s libmpi.so.12.0 libmpi.so.12; \
		cd $(INTEL64_STAGING)/lib && ln -s libmpi.so.12.0 libmpi.so; \
		cp $(MPIRT_DIR)/lib/libtmi.so.1.2 $(INTEL64_STAGING)/lib; \
		cd $(INTEL64_STAGING)/lib && ln -s libtmi.so.1.2 libtmi.so; \
		cd $(INTEL64_STAGING)/lib && ln -s libtmi.so.1.2 libtmi.so.1.0; \
		cd $(INTEL64_STAGING)/lib && ln -s libtmi.so.1.2 libtmi.so.1.1; \
		cp $(MPIRT_DIR)/lib/libtmip_psm.so.1.2 $(INTEL64_STAGING)/lib; \
		cd $(INTEL64_STAGING)/lib && ln -s libtmip_psm.so.1.2 libtmip_psm.so; \
		cd $(INTEL64_STAGING)/lib && ln -s libtmip_psm.so.1.2 libtmip_psm.so.1.0; \
		cd $(INTEL64_STAGING)/lib && ln -s libtmip_psm.so.1.2 libtmip_psm.so.1.1; \
		cp $(MPIRT_DIR)/lib/libtmip_psm2.so.1.0 $(INTEL64_STAGING)/lib; \
		cd $(INTEL64_STAGING)/lib && ln -s libtmip_psm2.so.1.0 libtmip_psm2.so; \
		cp $(MPIRT_DIR)/etc/mpiexec.conf $(INTEL64_STAGING)/etc; \
		cp $(MPIRT_DIR)/etc/tmi.conf $(INTEL64_STAGING)/etc; \
		cp $(MPIRT_DIR)/licensing/license.txt $(STAGING)/licensing/mpi/; \
		cp $(MPIRT_DIR)/licensing/third-party-programs.txt $(STAGING)/licensing/mpi/; \
	fi
	cp $(EXAMPLES_DIR)/mlsl_test/mlsl_test.cpp $(STAGING)/test/; \
	cp $(EXAMPLES_DIR)/mlsl_test/mlsl_test.py $(STAGING)/test/; \
	cp $(EXAMPLES_DIR)/mlsl_test/Makefile $(STAGING)/test/; \
	cp $(EXAMPLES_DIR)/mlsl_example/mlsl_example.cpp $(STAGING)/example/; \
	cp $(EXAMPLES_DIR)/mlsl_example/Makefile $(STAGING)/example/; \
	cp $(DOC_DIR)/README.txt $(STAGING)/doc/
	cp $(DOC_DIR)/API_Reference.htm $(STAGING)/doc/
	cp -r $(DOC_DIR)/doxygen/html/* $(STAGING)/doc/api
	cp $(DOC_DIR)/Developer_Guide.pdf $(STAGING)/doc/
	cp $(DOC_DIR)/Release_Notes.txt $(STAGING)/doc/
	sed -i -e "s|MLSL_SUBSTITUTE_OFFICIAL_VERSION|$(MLSL_OFFICIAL_VERSION)|g" $(STAGING)/doc/Release_Notes.txt
	echo "Adding Copyright notice ..."
	mkdir -p $(STAGING)/_tmp
	echo "1a" > $(STAGING)/_tmp/copyright.sh
	echo "#" >> $(STAGING)/_tmp/copyright.sh
	sed -e "s|^|# |" -e "s|MLSL_SUBSTITUTE_COPYRIGHT_YEAR|$(MLSL_COPYRIGHT_YEAR)|g" $(BASE_DIR)/doc/copyright >> $(STAGING)/_tmp/copyright.sh
	echo "#" >> $(STAGING)/_tmp/copyright.sh
	echo "." >> $(STAGING)/_tmp/copyright.sh
	echo "w" >> $(STAGING)/_tmp/copyright.sh
	echo "1i" > $(STAGING)/_tmp/copyright.c
	echo "/*" >> $(STAGING)/_tmp/copyright.c
	sed -e "s|^| |" -e "s|MLSL_SUBSTITUTE_COPYRIGHT_YEAR|$(MLSL_COPYRIGHT_YEAR)|g" $(BASE_DIR)/doc/copyright >> $(STAGING)/_tmp/copyright.c
	echo "*/" >> $(STAGING)/_tmp/copyright.c
	echo "." >> $(STAGING)/_tmp/copyright.c
	echo "w" >> $(STAGING)/_tmp/copyright.c
	ed $(STAGING)/intel64/bin/mlslvars.sh < $(STAGING)/_tmp/copyright.sh > /dev/null 2>&1
	ed $(STAGING)/intel64/include/mlsl.hpp < $(STAGING)/_tmp/copyright.c > /dev/null 2>&1
	ed $(STAGING)/intel64/include/mlsl.h < $(STAGING)/_tmp/copyright.c > /dev/null 2>&1
	ed $(STAGING)/intel64/include/mlsl/mlsl.py < $(STAGING)/_tmp/copyright.sh > /dev/null 2>&1
	ed $(STAGING)/intel64/include/mlsl/__init__.py < $(STAGING)/_tmp/copyright.sh > /dev/null 2>&1
	ed $(STAGING)/test/mlsl_test.cpp < $(STAGING)/_tmp/copyright.c > /dev/null 2>&1
	ed $(STAGING)/test/mlsl_test.py < $(STAGING)/_tmp/copyright.sh > /dev/null 2>&1
	ed $(STAGING)/test/Makefile < $(STAGING)/_tmp/copyright.sh > /dev/null 2>&1
	ed $(STAGING)/example/mlsl_example.cpp < $(STAGING)/_tmp/copyright.c > /dev/null 2>&1
	ed $(STAGING)/example/Makefile < $(STAGING)/_tmp/copyright.sh > /dev/null 2>&1
	rm -rf $(STAGING)/_tmp
	echo "done."
	chmod 755 $(STAGING)/doc
	chmod 755 $(STAGING)/doc/api
	chmod 644 $(STAGING)/doc/API_Reference.htm
	find $(STAGING)/doc/api -type d -exec chmod 755 {} \;
	find $(STAGING)/doc/api -type f -exec chmod 644 {} \;
	chmod 644 $(STAGING)/doc/Developer_Guide.pdf
	chmod 644 $(STAGING)/doc/README.txt
	chmod 644 $(STAGING)/doc/Release_Notes.txt
	chmod 755 $(INTEL64_STAGING)
	chmod 755 $(INTEL64_STAGING)/bin
	chmod 755 $(INTEL64_STAGING)/bin/ep_server
	chmod 755 $(INTEL64_STAGING)/bin/mlslvars.sh
	chmod 755 $(INTEL64_STAGING)/include
	chmod 644 $(INTEL64_STAGING)/include/mlsl.hpp
	chmod 644 $(INTEL64_STAGING)/include/mlsl.h
	chmod 755 $(INTEL64_STAGING)/include/mlsl
	chmod 644 $(INTEL64_STAGING)/include/mlsl/mlsl.py
	chmod 644 $(INTEL64_STAGING)/include/mlsl/__init__.py
	chmod 755 $(INTEL64_STAGING)/lib
	chmod 755 $(INTEL64_STAGING)/lib/libmlsl.so
	chmod 755 $(INTEL64_STAGING)/lib/libmlsl.so.1
	chmod 755 $(INTEL64_STAGING)/lib/libmlsl.so.1.0
	chmod 755 $(STAGING)/test
	chmod 644 $(STAGING)/test/mlsl_test.cpp
	chmod 755 $(STAGING)/test/mlsl_test.py
	chmod 644 $(STAGING)/test/Makefile
	chmod 755 $(STAGING)/licensing
	if [ "$(MPIRT)" == "intel" ]; then \
		chmod 755 $(INTEL64_STAGING)/bin/mpiexec; \
		chmod 755 $(INTEL64_STAGING)/bin/mpiexec.hydra; \
		chmod 755 $(INTEL64_STAGING)/bin/mpirun; \
		chmod 755 $(INTEL64_STAGING)/bin/pmi_proxy; \
		chmod 755 $(INTEL64_STAGING)/etc; \
		chmod 644 $(INTEL64_STAGING)/etc/mpiexec.conf; \
		chmod 644 $(INTEL64_STAGING)/etc/tmi.conf; \
		chmod 755 $(INTEL64_STAGING)/lib/libmpi.so; \
		chmod 755 $(INTEL64_STAGING)/lib/libmpi.so.12; \
		chmod 755 $(INTEL64_STAGING)/lib/libmpi.so.12.0; \
		chmod 755 $(INTEL64_STAGING)/lib/libtmip_psm2.so; \
		chmod 755 $(INTEL64_STAGING)/lib/libtmip_psm2.so.1.0; \
		chmod 755 $(INTEL64_STAGING)/lib/libtmip_psm.so; \
		chmod 755 $(INTEL64_STAGING)/lib/libtmip_psm.so.1.0; \
		chmod 755 $(INTEL64_STAGING)/lib/libtmip_psm.so.1.1; \
		chmod 755 $(INTEL64_STAGING)/lib/libtmip_psm.so.1.2; \
		chmod 755 $(INTEL64_STAGING)/lib/libtmi.so; \
		chmod 755 $(INTEL64_STAGING)/lib/libtmi.so.1.0; \
		chmod 755 $(INTEL64_STAGING)/lib/libtmi.so.1.1; \
		chmod 755 $(INTEL64_STAGING)/lib/libtmi.so.1.2; \
		chmod 755 $(STAGING)/licensing/mpi; \
		chmod 644 $(STAGING)/licensing/mpi/license.txt; \
		chmod 644 $(STAGING)/licensing/mpi/third-party-programs.txt; \
	fi
	chmod 755 $(STAGING)/licensing/mlsl
	cp $(BASE_DIR)/LICENSE $(STAGING)/licensing/mlsl/
	cp $(BASE_DIR)/third-party-programs.txt $(STAGING)/licensing/mlsl/
	chmod 644 $(STAGING)/licensing/mlsl/LICENSE
	chmod 644 $(STAGING)/licensing/mlsl/third-party-programs.txt

SWF_NODE = nnvlogin001
SWF_REPO = /nfs/inn/disks/nn-intel_sw_ct
SWF_PKG  = $(MLSL_PRODUCT_GROUP_NAME)/packages/$(MLSL_PRODUCT_NAME)

publish_to_swf: packages archive
	ssh $(SWF_NODE) "cd $(SWF_REPO) && mkdir -p $(SWF_PKG)/webimage && mkdir -p $(SWF_PKG)/toplevelfiles && mkdir -p $(SWF_PKG)/repositories"
	scp $(BASE_DIR)/$(MLSL_ARCHIVE_NAME) $(SWF_NODE):$(SWF_REPO)/$(SWF_PKG)/webimage/
	scp $(BASE_DIR)/_rpm/$(MLSL_RPM_PREFIX_NAME)-$(MLSL_MAJOR_VERSION).$(MLSL_MINOR_VERSION)-$(MLSL_PACKAGE_ID).x86_64.rpm $(SWF_NODE):$(SWF_REPO)/$(SWF_PKG)/webimage/
	scp $(BASE_DIR)/_deb/$(MLSL_RPM_PREFIX_NAME)-$(MLSL_MAJOR_VERSION).$(MLSL_MINOR_VERSION)-$(MLSL_PACKAGE_ID)_amd64.deb $(SWF_NODE):$(SWF_REPO)/$(SWF_PKG)/webimage/
	scp $(SIGNTOOL_DIR)/PUBLIC_KEY.PUB $(SWF_NODE):$(SWF_REPO)/$(SWF_PKG)/webimage/
	scp $(BASE_DIR)/doc/README.txt $(SWF_NODE):$(SWF_REPO)/$(SWF_PKG)/toplevelfiles/
	scp $(BASE_DIR)/LICENSE $(SWF_NODE):$(SWF_REPO)/$(SWF_PKG)/toplevelfiles/
	scp $(BASE_DIR)/third-party-programs.txt $(SWF_NODE):$(SWF_REPO)/$(SWF_PKG)/toplevelfiles/
	scp -r $(BASE_DIR)/_repositories/* $(SWF_NODE):$(SWF_REPO)/$(SWF_PKG)/repositories/
	ssh $(SWF_NODE) "cd $(SWF_REPO)/$(MLSL_PRODUCT_GROUP_NAME)/packages && chmod o+rx -R $(MLSL_PRODUCT_NAME)"

.PHONY: checkoptions examples doxygen dev
