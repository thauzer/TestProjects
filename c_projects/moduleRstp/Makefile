RSTPCOM_OBJ = rstp_mod_com.o
RSTPCOM_SRC = rstp_mod_com.c

obj-m = mod_rstp.o
RSTPMOD_SRC = mod_rstp.c
KERNEL_VERSION = $(shell uname -r)

CC = gcc

all: rstpmodcom rstpmod

rstpmodcom: $(RSTPCOM_OBJ)
	$(CC) -o $@ $(RSTPCOM_OBJ)

rstpmod: 
	make -C /lib/modules/$(KERNEL_VERSION)/build M=$(PWD) modules
clean:
	rm -f *.o rstpmodcom rstpmod
	make -C /lib/modules/$(KERNEL_VERSION)/build M=$(PWD) clean
