obj-m += my_dev_driver.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

DTC := dtc
DTC_FLAGS := -@ -I dts -O dtb

all: my_overlay.dtbo
	$(MAKE) -C $(KDIR) M=$(PWD) modules

my_overlay.dtbo: my_overlay.dts
	$(DTC) $(DTC_FLAGS) -o $@ $<

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f my_overlay.dtbo
