#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include "mod_rstp.h"

#define SUCCESS 0
#define BUF_LEN 80

static int Device_Open = 0;

static char Message[BUF_LEN];

static char* Message_Ptr;

static int device_open(struct inode* inode, struct file* filep)
{
#ifdef DEBUG
	printk(KERN_INFO "device_open - %p\n", filep);
#endif
	if (Device_Open)
		return -EBUSY;

	Device_Open++;
	Message_Ptr = Message;
	try_module_get(THIS_MODULE);
	return SUCCESS;
}

static int device_release(struct inode* inode, struct file* filep)
{
#ifdef DEBUG
	printk(KERN_ALERT "device_release - %p,%p\n", inode, filep);
#endif
	Device_Open--;
	module_put(THIS_MODULE);
	return SUCCESS;
}

static ssize_t device_read(struct inode* inode, char __user* buffer, size_t length, loff_t offset)
{
	int bytes_read = 0;
#ifdef DEBUG
	printk(KERN_ALERT "device_read - %p, %p, %d\n", filep, buffer, length);
#endif
	
	if (*Message_Ptr == 0)
		return 0;

	while(length && *Message_Ptr) {
		put_user(*(Message_Ptr++), buffer++);
		length--;
		bytes_read++;
	}

#ifdef DEBUG
	printk(KERN_ALERT "Bytes read: %d, left to read: %d\n", bytes_read, length);
#endif

	return bytes_read;
}

static ssize_t device_write(struct inode* inode, const char __user* buffer, size_t length, loff_t offset)
{
	int i;

	for (i=0; i < length && i < BUF_LEN; i++) {
		get_user(Message[i], buffer+i);
	}

	Message_Ptr = Message;

	return i;
}

static int device_ioctl(struct inode* inode, struct file* filep, unsigned int ioctl_num, unsigned long ioctl_param)
{
	switch (ioctl_num)
	{
	case CMD_CHANGE_PORT:
	{
		SW_PORT_CHANGE_T* portChange = (SW_PORT_CHANGE_T *)ioctl_param;
		printk(KERN_INFO "rstp module - change port: %d, state: %d\n", portChange->portNo, portChange->portState);
	}
		break;
	case CMD_FLUSH_ADDRESSES:
	{
		printk(KERN_INFO "rstp module - flush addresses on port: %d\n", (int)ioctl_param);
	}
		break;
	}

	return SUCCESS;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = device_read,
	.write = device_write,
	.ioctl = device_ioctl,
	.open = device_open,
	.release = device_release,
};

static int __init rstp_init(void)
{
	int ret;
	printk(KERN_INFO "Rstp module initialization\n");
	ret = register_chrdev(MAJOR_NUM, DEVICE_FILE_NAME, &fops);
	if (ret < 0) {
		printk(KERN_INFO "%s failed with %d\n", "Registering device", ret);
		return ret;
	}
#ifdef DEBUG
	printk(KERN_INFO "rstpmod %p, return value: %d\n", fops.open, ret);
#endif
	
	return 0;
}

static void __exit rstp_cleanup(void)
{
/*	int ret; */
	printk(KERN_INFO "Rstp module exit");
	unregister_chrdev(MAJOR_NUM, DEVICE_FILE_NAME);
/*	if (ret < 0) {
		printk(KERN_ALERT "Unregitering device failed\n");
	} */
}

module_init(rstp_init);
module_exit(rstp_cleanup);
