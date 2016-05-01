#KERNELDIR :=/lib/modules/$(shell uname -r)/build
KERNELDIR :=/home/linux/linux-3.14

test:
	make -C $(KERNELDIR) M=$(shell pwd) modules

clean:
	rm -rf *.o  *.ko  *.mod*  *.order  *.symvers

obj-m=led.o
