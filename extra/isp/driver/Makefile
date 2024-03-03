ifeq ($(KERNELRELEASE), )
KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD :=$(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR)  M=$(PWD)  
clean:
	rm -rf *.mk .tmp_versions Module.symvers *.mod.c *.o *.ko .*.cmd Module.markers modules.order *.a *.mod
load:
	insmod ch37x.ko
unload:
	rmmod ch37x
install: default
	mkdir -p /lib/modules/$(shell uname -r)/kernel/drivers/usb/misc
	cp -f ./ch37x.ko /lib/modules/$(shell uname -r)/kernel/drivers/usb/misc/
	depmod -a
uninstall:
	rm -rf /lib/modules/$(shell uname -r)/kernel/drivers/usb/misc/ch37x.ko
	depmod -a
else
	obj-m := ch37x.o
endif
