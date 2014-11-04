#include <stdlib.h>
#include <stdio.h>

#include "libusb.h"

static void print_devs(libusb_device **devs)
{
	libusb_device *dev;
	int i=0;
	
	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);
		if (r > 0) {
			fprintf(stderr, "Failed to get device descriptor\n");
			return;
		}
		
		printf("%04x:%04x (bus %d, device %d)\n",
			desc.idVendor, desc.idProduct,
			libusb_get_bus_number(dev), libusb_get_device_address(dev));
	}
}

static struct libusb_device_handle* handle;

int initUsb()
{
	int ret;
	
	ret = libusb_init(NULL);
	if (ret < 0)
		return ret;
		
	if ((handle = libusb_open_device_with_vid_pid(NULL, VID, PID)) == NULL) {
		fprintf(stdout, "Problem acquireing handle\n");
		return -1;
	}
	
	libusb_claim_interface(handle, 0);
	
	
	libusb_release_interface(devh, 0);
	libusb_close(devh);
	
	return 0;
}

int main(int argc, char** argv)
{
	libusb_device **devs;
	int r;
	size_t cnt;
	

	
	cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0)
		return (int) cnt;
		
	print_devs(devs);
	libusb_free_device_list(devs, 1);
	
	libusb_exit(NULL);
	return 0;
}
