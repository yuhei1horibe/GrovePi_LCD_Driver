ARCH=arm
CROSS_COMPILE=arm-linux-gnueabihf-

obj-m=grove_lcd.o
grove_lcd_objs=grove_lcd_core.o grove_lcd_pru.o
KERNDIR=$(HOME)/RaspberryPi/linux
PWD=$(shell pwd)

default:
	$(MAKE) -C $(KERNDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KERNDIR) M=$(PWD) ARCH=$(ARCH) clean

