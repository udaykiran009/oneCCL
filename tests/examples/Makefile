
BASE_DIR = $(shell pwd)
ENABLE_DEBUG ?= 1
SHELL = bash
DIRS = $(shell ls -d $(BASE_DIR)/*/)

all:
	for DIR in $(DIRS); \
	do \
		$(MAKE) -C $$DIR CC="$(CC)" CXX="$(CXX)" EXTRA_CFLAGS="$(EXTRA_CFLAGS)" EXTRA_LDFLAGS="$(EXTRA_LDFLAGS)" ENABLE_DEBUG=$(ENABLE_DEBUG) all; \
		if [ "$${PIPESTATUS[0]}" -ne 0 ]; then \
			echo "Make FAILED." ; \
		fi ; \
	done;

run:
	for DIR in $(DIRS); \
	do \
		$(MAKE) -C $$DIR run; \
		if [ "$${PIPESTATUS[0]}" -ne 0 ]; then \
			echo "Run FAILED." ; \
		fi ; \
	done;

clean:
	for DIR in $(DIRS); \
	do \
		$(MAKE) -C $$DIR clean; \
	done;

dev:
	make clean && make all && make run
