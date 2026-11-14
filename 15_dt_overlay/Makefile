DTC := dtc
DTC_FLAGS := -@ -I dts -O dtb

all: my_overlay.dtbo

my_overlay.dtbo: my_overlay.dts
	$(DTC) $(DTC_FLAGS) -o $@ $<

clean:
	rm -f my_overlay.dtbo
