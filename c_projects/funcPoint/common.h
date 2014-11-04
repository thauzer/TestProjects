#ifndef _COMMON_H_
#define _COMMON_H_

struct SReportStatus {
    char firm_version[12];
    char firm_version_updated[24];
    char ip_address[24];
    int dhcp;
    int wps;
    int rssi;
    int snr;
    char ssid[32];
    int security_type;
};

#define MAX_REGISTERS 24

struct SRegister {
    char address[8];
    char value[8];
};

struct SHandlers {
	struct SReportStatus* (*statusHandler)();
	struct SRegister* (*registerHandler)();
};

#endif /* _COMMON_H_ */

