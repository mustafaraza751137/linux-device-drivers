obj-m += open_cdev.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	gcc test.c -o cdev_test

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f cdev_test
