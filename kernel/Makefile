obj-m := sbsctl.o

UNAME = $(shell uname -r)
LINUX_DIR = /lib/modules/${UNAME}/build
LINUX_HEADERS = ${LINUX_DIR}/include/
MODULE_DIR = $(shell pwd)


.PHONY: kmod
kmod:
	$(MAKE) -C $(LINUX_DIR) M=$(MODULE_DIR) modules

.PHONY: install
install:
	$(MAKE) -C $(LINUX_DIR) M=$(MODULE_DIR) modules_install

.PHONY: load
load:
	-/sbin/rmmod sbsctl
	/sbin/insmod sbsctl.ko

.PHONY: kmod_clean
kmod_clean:
	$(MAKE) -C $(LINUX_DIR) M=$(MODULE_DIR) clean
