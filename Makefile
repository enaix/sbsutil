C=gcc

CFLAGS = -Wall #-Werror=return-type
# Sanitizer disabled by default

CFLAGS_DEBUG = -g -DENABLE_DEBUG
CFLAGS_RELEASE = -O3 -DNDEBUG

# Linux headers
# =============

UNAME = $(shell uname -r)
LINUX_HEADERS = /lib/modules/${UNAME}/build/include/

ARCH_PATH = /lib/modules/${UNAME}/build/arch
ARCH = $(shell bash -c "ls -1 /lib/modules/$(shell uname -r)/build/arch | grep -v Kconfig")
ARCH_HEADERS = ${ARCH_PATH}/${ARCH}/include/


#SRC_FILES = src/sbsutil.c
TARGETS = sbsutil
#OBJ = $(SRC_FILES:.c=.o)
#INCLUDE = -I. -I${LINUX_HEADERS} -I${LINUX_HEADERS}/uapi/ -I ${ARCH_HEADERS} -I${ARCH_HEADERS}/uapi/ -I${ARCH_HEADERS}/generated/uapi/ -I${ARCH_HEADERS}/generated/
INCLUDE = -I.
LINK = -li2c

prerun:
	$(shell mkdir -p build)

.PHONY: all
all: release

.PHONY: debug
debug: CFLAGS += ${CFLAGS_DEBUG}
debug: prerun $(TARGETS)

.PHONY: release
release: CFLAGS += ${CFLAGS_RELEASE}
release: prerun $(TARGETS)

$(TARGETS): %: %.o
	${C} ${CFLAGS} ${LINK} -o build/$@ build/$^

%.o : src/%.c
	${C} -c ${CFLAGS} $< ${INCLUDE} -o build/$(notdir $@)

.PHONY: clean
clean:
	rm build/*
