#ifndef _MOD_RSTP_H_
#define _MOD_RSTP_H_

#include <linux/ioctl.h>

#define MAJOR_NUM 199

#define CMD_CHANGE_PORT 0
#define CMD_FLUSH_ADDRESSES 1

typedef enum {
	SW_PS_DISABLED = 0,
	SW_PS_DISCARDING,
	SW_PS_LEARNING,
	SW_PS_FORWARDING,
	SW_PS_NON_STP
} SW_RSTP_PORT_STATE;

typedef struct change_port_status {
	 /* switch port indication */
	int					portNo;
	/* port state */
	SW_RSTP_PORT_STATE	portState;
} SW_PORT_CHANGE_T;

#define IOCTL_CHANGE_PORT _IOW(MAJOR_NUM, CMD_CHANGE_PORT, SW_PORT_CHANGE_T*)
#define IOCTL_FLUSH_ADDRESSES _IOW(MAJOR_NUM, CMD_FLUSH_ADDRESSES, int)

#define DEVICE_FILE_NAME "/dev/mv88E6097F"

#endif
