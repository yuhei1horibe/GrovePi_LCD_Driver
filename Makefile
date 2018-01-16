ARCH=arm
CROSS_COMPILE=arm-linux-gnueabihf-

obj-m=grove_lcd.o
grove_lcd_objs=grove_lcd_core.o grove_lcd_pru.o
KERNDIR=$(HOME)/RaspberryPi/linux
KERNEL_VER=$(shell uname -r)
INSTALL_DIR=/lib/modules/$(KERNEL_VER)/kernel/drivers/grove_lcd
PWD=$(shell pwd)

default:
	$(MAKE) -C $(KERNDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KERNDIR) M=$(PWD) ARCH=$(ARCH) clean

$(INSTALL_DIR):
	mkdir -p $(INSTALL_DIR)

install: grove_lcd.ko $(INSTALL_DIR)
	cp grove_lcd.ko $(INSTALL_DIR)/
	depmod -a
	modprobe grove_lcd
	gpio load i2c
	echo "grove_lcd 0x3e" > /sys/bus/i2c/devices/i2c-1/new_device

