include kernel/Makefile

C=gcc

# Compilation options
# ===================

# i2c is enabled by default
ENABLE_I2C ?= 1

# Voltage control: enable libgpiod control
GPIO_RPI ?= 0

CFLAGS = -Wall  #-Werror=return-type
# Sanitizer disabled by default

CFLAGS_DEBUG = -g -DENABLE_DEBUG -O2
CFLAGS_RELEASE = -O3 -DNDEBUG


ifeq (${ENABLE_I2C}, 1)
	CFLAGS += -DSBS_ENABLE_I2C
endif

ifeq (${GPIO_RPI}, 1)
	CFLAGS += -DSBS_RPI
endif


# Linux headers
# =============

UNAME = $(shell uname -r)
LINUX_DIR = /lib/modules/${UNAME}/build
LINUX_HEADERS = ${LINUX_DIR}/include/
MODULE_DIR = $(shell pwd)/kernel

ARCH_PATH = /lib/modules/${UNAME}/build/arch
ARCH = $(shell bash -c "ls -1 /lib/modules/$(shell uname -r)/build/arch | grep -v Kconfig")
ARCH_HEADERS = ${ARCH_PATH}/${ARCH}/include/


#SRC_FILES = src/sbsutil.c
TARGETS = sbsutil
#OBJ = $(SRC_FILES:.c=.o)
#INCLUDE = -I. -I${LINUX_HEADERS} -I${LINUX_HEADERS}/uapi/ -I ${ARCH_HEADERS} -I${ARCH_HEADERS}/uapi/ -I${ARCH_HEADERS}/generated/uapi/ -I${ARCH_HEADERS}/generated/
INCLUDE = -I.
LINK =

ifeq (${ENABLE_I2C}, 1)
	LINK += -li2c
endif

ifeq (${GPIO_RPI}, 1)
	LINK += -lgpiod
endif

.DEFAULT_GOAL := release

.PHONY: all
all: release

prerun:
	$(shell mkdir -p build)



.PHONY: debug
debug: CFLAGS += ${CFLAGS_DEBUG}
debug: prerun $(TARGETS)

.PHONY: release
release: CFLAGS += ${CFLAGS_RELEASE}
release: prerun $(TARGETS)

$(TARGETS): %: %.o
	${C} ${CFLAGS} -o build/$@ build/$^ ${LINK}

%.o : src/%.c
	${C} -c ${CFLAGS} $< ${INCLUDE} -o build/$(notdir $@)

.PHONY: clean
clean: kmod_clean
	rm build/*

