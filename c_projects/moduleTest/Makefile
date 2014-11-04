IOCTLTEST_OBJ = main.o
IOCTLTEST_SRC = main.c

obj-m = chardev.o
MODTEST_SRC = chardev.c
KVERSION = $(shell uname -r)

CC = gcc


all: ioctltest modtest

ioctltest: $(IOCTLTEST_OBJ)
	$(CC) -o $@ $(IOCTLTEST_OBJ)

modtest:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules

clean:
	rm -f *.o ioctltest modtest
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
