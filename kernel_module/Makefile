obj-m += vlc.o

### Default to the source files of the current kernel. Make sure you
### have kernel headers for your kernel installed with "sudo apt-get
### install linux-headers-$(uname -r)".
KERNEL_SRC=/lib/modules/$(shell uname -r)/build

### Add the Xenomai headers to the compilation CFLAGS. Requires that
### you have a kernel with Xenomai support built-in.
EXTRA_CFLAGS =-I$(KERNEL_SRC)/include/xenomai -I$(KERNEL_SRC)/include/xenomai/native

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
	sudo insmod vlc.ko
	sudo ifconfig eth1 192.168.10.1

remove:
	sudo ifconfig eth1 down
	sudo rmmod vlc.ko
