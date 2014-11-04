/* hrast.c  - Hrast server source file */
/* dobl_izdaj */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <pthread.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <shared/ipcs_keys.h> 

#include <appl/diag/system.h>

#include <appl/diag/parse.h>
#include <appl/diag/dport.h>
#include <bcm/port.h>
#include <bcm/error.h>
#include <bcm/vlan.h>
#include <bcm/stg.h>
#include <bcm/stat.h>
#include <bcm/link.h>
#include <bcm/rate.h>
#include <bcm/init.h>
#include <bcm/types.h>
#include <bcm/trunk.h>
#include <bcm/stack.h>
#include <bcm/mirror.h>

#include <shared/hrast.h>
#include <shared/it_diag.h>
#include <soc/mem.h>
#include <soc/mcm/memregs.h>
#include <soc/mcm/memacc.h> /*soc_L2X_VALIDm_field_get*/

#include "hrast_api.h"

#undef HRAST_DEBUG

#define MODULE_NAME         "hrast"

#define MAX_CALLBACKS                       10
#define KEEP_ALIVE_PERIOD             20000000	/* 10 sec */
#define KEEP_ALIVE_PERIOD_RX            5000000

#ifdef __RSTP__
#include <bcm/tx.h>
#include <bcm/rx.h>
#include <bcm/pkt.h>
#include <bcm/l2.h>
#include <sal/core/libc.h>
#endif /* __RSTP__ */

/* callback table structure */
typedef struct {
/*	int qid;
	key_t key;*/
	pid_t c_pid; /* client process id */
} callback_t;

/* hrast debug structure:
*    console_on:   TRUE/FALSE  - console printouts on/off
*    syslog_on:    TRUE/FALSE  - syslog logging on/off
*    level:    0 - none
*              1 - errors
*              2 - errors + functions
*              3 - errors + functions + results
*              4 - errors + functions + results + messages
*/
typedef struct {
	int console_on;
	int syslog_on;
	int level;
} hrast_debug;

hrast_debug hrast_dbg;

callback_t Callback[MAX_CALLBACKS];	/* linkscan callback table */
int reg_callbacks;					/* number of registered callbacks */
int cbqid = -1;						/* callback queue id */
static int cong_pkt_thr=HRAST_DIAG_DEFAULT_CONG_PKT_THR; /*number of dropped packets due to internal congestion - threshold*/
static int table_entries=0; /*set CAM table entries - available in BCM menu - for test diagnostic*/
static bool entryFlag=FALSE; /*set CAM table entries - available in BCM menu - for test diagnostic*/
static int BCMdiagDebugSet=0; /*debug mode for BCM diag test*/

#ifdef __RSTP__
callback_t rx_Callback[MAX_CALLBACKS];
int reg_rx_callbacks;
int rxqid = -1;
sal_time_t rx_time = 0;
#endif

/* Command functions prototypes */
static int hrast_switch_init_check(struct bcm_msgbuf *bcm_msg);
static int hrast_switch_attach_check(struct bcm_msgbuf *bcm_msg);

static int hrast_link_status_show(struct bcm_msgbuf *bcm_msg);
static int hrast_port_enable_get(struct bcm_msgbuf *bcm_msg);
static int hrast_port_enable_set(struct bcm_msgbuf *bcm_msg);
static int hrast_port_link_status_get(struct bcm_msgbuf *bcm_msg);
static int hrast_port_advert_remote_get(struct bcm_msgbuf *bcm_msg);
static int hrast_port_info_get(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf);
static int hrast_port_selective_info_get(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf);
static int hrast_port_vlan_info_get(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf);
static int hrast_port_max_speed_get(struct bcm_msgbuf *bcm_msg);
static int hrast_port_speed_set(struct bcm_msgbuf *bcm_msg);
static int hrast_port_speed_get(struct bcm_msgbuf *bcm_msg);
static int hrast_port_duplex_set(struct bcm_msgbuf *bcm_msg);
static int hrast_port_duplex_get(struct bcm_msgbuf *bcm_msg);
static int hrast_port_max_frame_size_get(struct bcm_msgbuf *bcm_msg);
static int hrast_port_max_frame_size_set(struct bcm_msgbuf *bcm_msg);
static int hrast_port_interface_mode_set(struct bcm_msgbuf *bcm_msg);
static int hrast_port_autoneg_set(struct bcm_msgbuf *bcm_msg);
static void hrast_dbg_switch_init_print(int bcm_rv);
static int hrast_port_rate_egress_set(struct bcm_msgbuf *bcm_msg);
static int hrast_port_rate_ingress_set(struct bcm_msgbuf *bcm_msg);
static int hrast_port_rate_egress_get(struct bcm_msgbuf *bcm_msg);
static int hrast_port_rate_ingress_get(struct bcm_msgbuf *bcm_msg);

static int hrast_linkscan_init(struct bcm_msgbuf *bcm_msg);
static int hrast_linkscan_mode_get(struct bcm_msgbuf *bcm_msg);
static int hrast_linkscan_mode_set(struct bcm_msgbuf *bcm_msg);
static int hrast_linkscan_enable_get(struct bcm_msgbuf *bcm_msg);
static int hrast_linkscan_enable_set(struct bcm_msgbuf *bcm_msg);
static int hrast_linkscan_enable_port_get(struct bcm_msgbuf *bcm_msg);
static int hrast_linkscan_register(struct bcm_msgbuf *bcm_msg);
static int hrast_linkscan_unregister(struct bcm_msgbuf *bcm_msg);
static int hrast_linkscan_detach(struct bcm_msgbuf *bcm_msg);

void hrast_linkscan_callback(int unit, int port, bcm_port_info_t *info);
static int hrast_linkscan_connectivity_check(int cbqid, int pid);
static int hrast_linkscan_pull_message(int cbqid, int pid);
static void *hrast_linkscan_keep_alive(void* none);

static int hrast_vlan_show(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf);
static int hrast_vlan_create(struct bcm_msgbuf *bcm_msg);
static int hrast_vlan_destroy(struct bcm_msgbuf *bcm_msg);
static int hrast_vlan_port_get(struct bcm_msgbuf *bcm_msg);
static int hrast_vlan_port_add(struct bcm_msgbuf *bcm_msg);
static int hrast_vlan_port_remove(struct bcm_msgbuf *bcm_msg);
static int hrast_port_untagged_vlan_set(struct bcm_msgbuf *bcm_msg);
static int hrast_port_untagged_vlan_get(struct bcm_msgbuf *bcm_msg);

static int hrast_stg_stp_get(struct bcm_msgbuf *bcm_msg);
static int hrast_stg_stp_set(struct bcm_msgbuf *bcm_msg);

/* Link aggregation (trunking)*/
static int hrast_trunk_init(struct bcm_msgbuf *bcm_msg);
static int hrast_trunk_create(struct bcm_msgbuf *bcm_msg);
static int hrast_trunk_set(struct bcm_msgbuf *bcm_msg);
static int hrast_trunk_detach(struct bcm_msgbuf *bcm_msg);
static int hrast_trunk_psc_get(struct bcm_msgbuf *bcm_msg);
static int hrast_port_ifilter_get(struct bcm_msgbuf *bcm_msg);
static int hrast_port_ifilter_set(struct bcm_msgbuf *bcm_msg);

static int hrast_stat_get32(struct bcm_msgbuf *bcm_msg);
static int hrast_stat_clear(struct bcm_msgbuf *bcm_msg);

static int hrast_diag_mem_set(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf);
static int hrast_diag_mem_get(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf);
static int hrast_diag_filter_set(struct bcm_msgbuf *bcm_msg);
static int hrast_diag_filter_destroy_by_port(struct bcm_msgbuf *bcm_msg);
static int hrast_diag_cam_selective_count_get(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf);
static int hrast_diag_cam_entry_get(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf);
static int hrast_diag_cam_age_timer_get(struct bcm_msgbuf *bcm_msg);
static int hrast_diag_cam_age_timer_set(struct bcm_msgbuf *bcm_msg);
static int hrast_diag_table_size_get(struct bcm_msgbuf *bcm_msg);
static int hrast_diag_table_count_get(struct bcm_msgbuf *bcm_msg);
static int hrast_diag_in_cos_pkt_count_get(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf);
static int hrast_diag_out_cos_pkt_count_get(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf);
static int hrast_diag_congestion_pkt_disc_count_get(struct bcm_msgbuf *bcm_msg);
static int hrast_diag_congestion_pkt_disc_count_get_all(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf);
static int hrast_diag_congestion_pkt_disc_threshold_set(struct bcm_msgbuf *bcm_msg);
static int hrast_diag_table_count_set(struct bcm_msgbuf *bcm_msg);
static int hrast_diag_test_debug_mode_set(struct bcm_msgbuf *bcm_msg);
static int hrast_diag_test_debug_mode_get(struct bcm_msgbuf *bcm_msg);

static int hrast_dbg_level_get(struct bcm_msgbuf *bcm_msg);
static int hrast_dbg_level_set(struct bcm_msgbuf *bcm_msg);

#ifdef __RSTP__
static int hrast_tx_rstp(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf);
static int hrast_rx_register_rstp(struct bcm_msgbuf *bcm_msg);
static bcm_rx_t hrast_rx_receive_rstp_cb(int unit, bcm_pkt_t *pkt, void *vp); /* callback */
static int hrast_rx_connectivity_check(int qid, int pid);
static int hrast_rx_pull_message(int qid, int pid);
static void *hrast_rx_keep_alive(void* none);

static int hrast_rx_start(struct bcm_msgbuf *bcm_msg);
static int hrast_rx_active(struct bcm_msgbuf *bcm_msg);
static int hrast_rx_unregister_rstp(struct bcm_msgbuf *bcm_msg);

static int hrast_port_stp_set(struct bcm_msgbuf * bcm_msg);

static int hrast_l2_addr_delete_by_port(struct bcm_msgbuf *bcm_msg);

static int hrast_sys_msg_send_rstp(int msqid, struct bcm_msgbuf *bcm_msg);
#endif /* __RSTP__ */

/* Local functions prorotypes */
static int hrast_sys_msg_send(int msqid, struct bcm_msgbuf *bcm_msg);
static int hrast_sys_msg_receive(int msqid, int m_timout,
						struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf);

/* Debug functions prototypes */
static void hrast_dbg_print(int level, int flags, char *format, ...);
static void hrast_dbg_port_info_print(bcm_port_info_t *info);
static void hrast_dbg_ability_print(bcm_port_abil_t ability_mask);
static void hrast_dbg_ability_get(bcm_port_abil_t ability_mask, char *str);
static void hrast_dbg_callbacks_print(void);
#ifdef __RSTP__
static void hrast_dbg_callbacks_print_rstp(void);
#endif
static void hrast_dbg_msg_print(char* str, int qid, struct bcm_msgbuf *msgbuf);
static void hrast_dbg_ext_msg_print(char* str, int qid, struct bcm_extbuf *extbuf);

/* Mirroring prototypes */
static int hrast_switch_control_get(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf);
static int hrast_switch_control_set(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf);
static int hrast_mirror_init(struct bcm_msgbuf *bcm_msg);
static int hrast_mirror_mode_get(struct bcm_msgbuf *bcm_msg);
static int hrast_mirror_mode_set(struct bcm_msgbuf *bcm_msg);
static int hrast_mirror_to_get(struct bcm_msgbuf *bcm_msg);
static int hrast_mirror_to_set(struct bcm_msgbuf *bcm_msg);
static int hrast_mirror_ingress_get(struct bcm_msgbuf *bcm_msg);
static int hrast_mirror_ingress_set(struct bcm_msgbuf *bcm_msg);
static int hrast_mirror_egress_get(struct bcm_msgbuf *bcm_msg);
static int hrast_mirror_egress_set(struct bcm_msgbuf *bcm_msg);
static int hrast_mirror_port_get(struct bcm_msgbuf *bcm_msg);
static int hrast_mirror_port_set(struct bcm_msgbuf *bcm_msg);
static int hrast_mirror_port_dest_add(struct bcm_msgbuf *bcm_msg);


int hrast_init(void)
{
	
  struct bcm_msgbuf bcm_buf;
  struct bcm_extbuf ext_buf;
  int msqid, qid_0;
  int rv, i;
  int shutdown = 0;
  pid_t pid;
  pthread_t keep_alive_tid;
#ifdef __RSTP__
  pthread_t keep_alive_rx_tid;
#endif /* __RSTP__ */

	/* default error logging settings */
	hrast_dbg.console_on = TRUE;
	hrast_dbg.syslog_on = TRUE;
	hrast_dbg.level = 1;

	hrast_dbg_print(0, SYSLOG | CONSOLE, "Initializing hrast server");
	
	/* check if we have process ID 1,2,4 or 8. This is not vaild! */
	pid = getpid();
	if ((pid == 1) || (pid == 2) || (pid == 4) || (pid == 8)) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "can not work with process number %d!", pid);
		return -1;
	}

	if ((qid_0 = msgget((key_t)MSGKEY_QID_ZERO, 0644 | IPC_CREAT )) == -1) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "error getting ZERO queue - %s", strerror(errno));
	}
	hrast_dbg_print(2, SYSLOG | CONSOLE, "ZERO queue created (qid=%d)", qid_0);

	/* create the message queue and connect to it */
	if ((msqid = msgget((key_t)MSGKEY_HRAST_MSG, 0644 | IPC_CREAT )) == -1) { 
		hrast_dbg_print(1, SYSLOG | CONSOLE, "error creating message queue - %s", strerror(errno));
		return -1;
	}
	hrast_dbg_print(0, SYSLOG | CONSOLE, "message queue created (qid=%d)", msqid);
	
	/* init linkscan resources */
	if (bcm_linkscan_enable_get(0, &rv) == BCM_E_INIT) {
		reg_callbacks = -1;
	}
	else {
		if ((cbqid = msgget((key_t)MSGKEY_HRAST_CB, 0644 | IPC_CREAT )) == -1) { 
			hrast_dbg_print(1, SYSLOG | CONSOLE, "error creating callback queue - %s", strerror(errno));
			return -1;
		}
		hrast_dbg_print(0, SYSLOG | CONSOLE, "callback queue created (qid=%d)", cbqid);
		
		for (i = 0; i < MAX_CALLBACKS; i++) {
			Callback[i].c_pid = 0;
		}
		reg_callbacks = 0;
		
		/* create keep-alive thread for registered callbacks */
		rv = pthread_create(&keep_alive_tid, NULL, hrast_linkscan_keep_alive, NULL);
		if (rv != 0) {
			hrast_dbg_print(1, SYSLOG | CONSOLE, "error creating keep-alive thread - %s", strerror(errno));
			/* hrast_linkscan_resources_clear();
			hrast_mutex_unlock();*/
			return -1;
		}
		hrast_dbg_print(2, SYSLOG | CONSOLE, "keep-alive thread created (tid=%d)", (int)keep_alive_tid);

	}
#ifdef __RSTP__
	/* receive callback thread */
	if ((rxqid = msgget((key_t)MSGKEY_HRAST_RX_CB, 0644 | IPC_CREAT )) == -1) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "error creating receive callback queue - %s", strerror(errno));
		return -1;
	}
	hrast_dbg_print(0, SYSLOG | CONSOLE, "RX callback queue created (qid=%d)", rxqid);

	for (i = 0; i < MAX_CALLBACKS; i++) {
		rx_Callback[i].c_pid = 0;
	}
	reg_rx_callbacks = 0;

	/* create keep-alive thread for RX registered callbacks */
	rv = pthread_create(&keep_alive_rx_tid, NULL, hrast_rx_keep_alive, NULL);
	if (rv != 0) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "error creating RX keep-alive thread - %s", strerror(errno));
		return -1;
	}
	hrast_dbg_print(2, SYSLOG | CONSOLE, "keep-alive RX thread created (tid=%d)", (int)keep_alive_rx_tid);
#endif /* __RSTP__ */
	
	while (shutdown != 1) {
		bcm_buf.lnx_mtype = HRAST_MSG_TYPE;
		/* wait for message */
		if (hrast_sys_msg_receive(msqid, HRAST_WAIT_FOREWER, &bcm_buf, &ext_buf) != 0) {
			hrast_dbg_print(1, SYSLOG, "error receiving message (qid=%d)", msqid);
			continue;
		}
		bcm_buf.mstatus = HRAST_MESSAGE_CMD_OK;			

		switch (bcm_buf.mcmd) {
			case HRAST_SWITCH_INIT_CHECK: 
			{
				rv = hrast_switch_init_check(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_SWITCH_ATTACH_CHECK: 
			{
				rv = hrast_switch_attach_check(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_LINK_STATUS_GET: 
			{
				rv = hrast_port_link_status_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_INFO_GET:
			{
				rv = hrast_port_info_get(&bcm_buf, &ext_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM_E;
				break;
			}
			case HRAST_PORT_SELECTIVE_INFO_GET:
			{
				rv = hrast_port_selective_info_get(&bcm_buf, &ext_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM_E;
				break;
			}
			case HRAST_PORT_VLAN_INFO_GET:
			{
				rv = hrast_port_vlan_info_get(&bcm_buf, &ext_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM_E;
				break;
			}
			/*port settings-gstr*/
			case HRAST_PORT_MAX_SPEED_GET: 
			{
				rv = hrast_port_max_speed_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_SPEED_SET: 
			{
				rv = hrast_port_speed_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_SPEED_GET: 
			{
				rv = hrast_port_speed_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_DUPLEX_SET: 
			{
				rv = hrast_port_duplex_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_DUPLEX_GET: 
			{
				rv = hrast_port_duplex_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_MAX_FRAME_SIZE_GET: 
			{
				rv = hrast_port_max_frame_size_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_INTERFACE_MODE_SET: 
			{
				rv = hrast_port_interface_mode_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_MAX_FRAME_SIZE_SET: 
			{
				rv = hrast_port_max_frame_size_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_AUTONEG_SET: 
			{
				rv = hrast_port_autoneg_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_RATE_EGRESS_SET: 
			{
				rv = hrast_port_rate_egress_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_RATE_INGRESS_SET: 
			{
				rv = hrast_port_rate_ingress_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_RATE_EGRESS_GET: 
			{
				rv = hrast_port_rate_egress_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_RATE_INGRESS_GET: 
			{
				rv = hrast_port_rate_ingress_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			/*end port settings-gstr*/
			case HRAST_PORT_ADVERT_REMOTE_GET:
			{
				rv = hrast_port_advert_remote_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_STP_STATE_GET: 
			{
				rv = hrast_stg_stp_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_STP_STATE_SET: 
			{
				rv = hrast_stg_stp_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			/* Link aggregation (trunking) */
			case HRAST_TRUNK_INIT: 
			{
				rv = hrast_trunk_init(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_TRUNK_CREATE: 
			{
				rv = hrast_trunk_create(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_TRUNK_SET: 
			{
				rv = hrast_trunk_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_TRUNK_DETACH: 
			{
				rv = hrast_trunk_detach(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_TRUNK_PSC_GET: 
			{
				rv = hrast_trunk_psc_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			/* end Link aggregation (trunking) */
			case HRAST_PORT_IFILTER_GET: 
			{
				rv = hrast_port_ifilter_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_IFILTER_SET: 
			{
				rv = hrast_port_ifilter_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_VLAN_CREATE: 
			{
				rv = hrast_vlan_create(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_VLAN_DESTROY: 
			{
				rv = hrast_vlan_destroy(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_VLAN_GET: 
			{
				rv = hrast_vlan_port_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_VLAN_ADD: 
			{

				rv = hrast_vlan_port_add(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_VLAN_REMOVE: 
			{
				rv = hrast_vlan_port_remove(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_UNTAG_VLAN_SET: 
			{
				rv = hrast_port_untagged_vlan_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_UNTAG_VLAN_GET: 
			{
				rv = hrast_port_untagged_vlan_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_ENABLE_SET: 
			{
				rv = hrast_port_enable_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_ENABLE_GET: 
			{
				rv = hrast_port_enable_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_STAT_GET32: 
			{
				rv = hrast_stat_get32(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_STAT_CLEAR: 
			{
				rv = hrast_stat_clear(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_LINKSCAN_INIT: 
			{
				rv = hrast_linkscan_init(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_LINKSCAN_MODE_GET: 
			{
				rv = hrast_linkscan_mode_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_LINKSCAN_MODE_SET: 
			{
				rv = hrast_linkscan_mode_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_LINKSCAN_ENABLE_GET: 
			{
				rv = hrast_linkscan_enable_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_LINKSCAN_ENABLE_SET: 
			{
				rv = hrast_linkscan_enable_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_LINKSCAN_ENABLE_PORT_GET: 
			{
				rv = hrast_linkscan_enable_port_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_LINKSCAN_REGISTER: 
			{
				rv = hrast_linkscan_register(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_LINKSCAN_UNREGISTER: 
			{
				rv = hrast_linkscan_unregister(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_LINKSCAN_DETACH: 
			{
				rv = hrast_linkscan_detach(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_PORT_STATUS_SHOW: 
			{
				rv = hrast_link_status_show(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_VLAN_SHOW: 
			{
				rv = hrast_vlan_show(&bcm_buf, &ext_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM_E;
				break;
			}
			case HRAST_DIAG_DEBUG_FLAG_ON: 
			{
				bcm_diag_dbg_on();
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				rv = 0;
				break;
			}
			case HRAST_DIAG_DEBUG_FLAG_OFF: 
			{
				bcm_diag_dbg_off();
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				rv = 0;
				break;
			}
			case HRAST_DIAG_MEM_SET:
			{
				rv = hrast_diag_mem_set(&bcm_buf, &ext_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM_E;
				break;
			}
			case HRAST_DIAG_MEM_GET:
			{
				rv = hrast_diag_mem_get(&bcm_buf, &ext_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM_E;
				break;
			}
			case HRAST_DIAG_FILTER_SET:
			{
				rv = hrast_diag_filter_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_DIAG_FILTER_DESTROY_BY_PORT:
			{
				rv = hrast_diag_filter_destroy_by_port(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_DIAG_CAM_SEL_COUNT_GET:
			{
				rv = hrast_diag_cam_selective_count_get(&bcm_buf, &ext_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM_E;
				break;
			}
			case HRAST_DIAG_CAM_ENTRY_GET:
			{
				rv = hrast_diag_cam_entry_get(&bcm_buf, &ext_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM_E;
				break;
			}
			case HRAST_DIAG_CAM_AGE_TIMER_GET:
			{
				rv = hrast_diag_cam_age_timer_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_DIAG_CAM_AGE_TIMER_SET:
			{
				rv = hrast_diag_cam_age_timer_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_DIAG_TABLE_SIZE_GET:
			{
				rv = hrast_diag_table_size_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_DIAG_TABLE_COUNT_GET:
			{
				rv = hrast_diag_table_count_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_DIAG_IN_COS_PKT_COUNT_GET:
			{
				rv = hrast_diag_in_cos_pkt_count_get(&bcm_buf, &ext_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM_E;
				break;
			}
			case HRAST_DIAG_OUT_COS_PKT_COUNT_GET:
			{
				rv = hrast_diag_out_cos_pkt_count_get(&bcm_buf, &ext_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM_E;
				break;
			}
			case HRAST_DIAG_CONGESTION_PKT_DISC_COUNT_GET:
			{
				rv = hrast_diag_congestion_pkt_disc_count_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_DIAG_CONGESTION_PKT_DISC_COUNT_GET_ALL:
			{
				rv = hrast_diag_congestion_pkt_disc_count_get_all(&bcm_buf, &ext_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM_E;
				break;
			}
			case HRAST_DIAG_CONGESTION_PKT_DISC_THRESHOLD_SET:
			{
				rv = hrast_diag_congestion_pkt_disc_threshold_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_DIAG_TABLE_COUNT_SET:
			{
				rv = hrast_diag_table_count_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_DIAG_TEST_DEBUG_MODE_SET:
			{
				rv = hrast_diag_test_debug_mode_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_DIAG_TEST_DEBUG_MODE_GET:
			{
				rv = hrast_diag_test_debug_mode_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_BCM_SHUTDOWN: 
			{
				hrast_dbg_print(0, SYSLOG | CONSOLE, "shutdown BCM application");
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				shutdown = 1;
				rv = 0;
				break;
			}
			case HRAST_DEBUG_LEVEL_GET: 
			{
				rv = hrast_dbg_level_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_DEBUG_LEVEL_SET: 
			{
				rv = hrast_dbg_level_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_MIRROR_INIT: 
			{
				rv = hrast_mirror_init(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}	
			case HRAST_MIRROR_MODE_GET: 
			{
				rv = hrast_mirror_mode_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}	
			case HRAST_MIRROR_MODE_SET: 
			{
				rv = hrast_mirror_mode_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_MIRROR_TO_GET: 
			{
				rv = hrast_mirror_to_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_MIRROR_TO_SET: 
			{
				rv = hrast_mirror_to_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_MIRROR_INGRESS_GET: 
			{
				rv = hrast_mirror_ingress_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_MIRROR_INGRESS_SET: 
			{
				rv = hrast_mirror_ingress_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_MIRROR_EGRESS_GET: 
			{
				rv = hrast_mirror_egress_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_MIRROR_EGRESS_SET: 
			{
				rv = hrast_mirror_egress_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_SWITCH_CONTROL_GET: 
			{
				rv = hrast_switch_control_get(&bcm_buf, &ext_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}	
			case HRAST_SWITCH_CONTROL_SET: 
			{
				rv = hrast_switch_control_set(&bcm_buf, &ext_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_MIRROR_PORT_GET: 
			{
				rv = hrast_mirror_port_get(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_MIRROR_PORT_SET: 
			{
				rv = hrast_mirror_port_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_MIRROR_PORT_DEST_ADD: 
			{
				rv = hrast_mirror_port_dest_add(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
#ifdef __RSTP__
			case HRAST_TX_RSTP:
			{
				rv = hrast_tx_rstp(&bcm_buf, &ext_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_RX_REGISTER_RSTP:
			{
				rv = hrast_rx_register_rstp(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_RX_UNREGISTER_RSTP:
            {
                rv = hrast_rx_unregister_rstp(&bcm_buf);
                bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
                break;
            }
			case HRAST_RX_START:
            {
                rv = hrast_rx_start(&bcm_buf);
                bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
                break;
            }
			case HRAST_RX_ACTIVE:
            {
                rv = hrast_rx_active(&bcm_buf);
                bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
                break;
            }
			case HRAST_PORT_STP_SET:
			{
				rv = hrast_port_stp_set(&bcm_buf);
				bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
			case HRAST_L2_ADDR_DEL_PORT:
			{
			    rv = hrast_l2_addr_delete_by_port(&bcm_buf);
			    bcm_buf.mtype = HRAST_MESSAGE_FROM_APPL_CONFIRM;
				break;
			}
#endif /* __RSTP__ */
			default:
			{
				hrast_dbg_print(1, SYSLOG | CONSOLE, "unknown command: %d", (int) bcm_buf.mcmd);
				continue; /* don't send the message, continue to the message receive function */
			}
		}
		if (rv != 0) {
			bcm_buf.mstatus = HRAST_MESSAGE_CMD_ERROR;
			hrast_dbg_print(1, SYSLOG | CONSOLE, "error at executing command: %d", (int) bcm_buf.mcmd);
		}
		
		/* send confirm message */
		bcm_buf.lnx_mtype = bcm_buf.mpid;
		if (hrast_sys_msg_send(msqid, &bcm_buf) != 0)
			hrast_dbg_print(1, SYSLOG, "error sending confirm message for command: %d", (int) bcm_buf.mcmd);
	}

	if (msgctl(msqid, IPC_RMID, NULL) == -1) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "error removing message queue - %s", strerror(errno));
		return -1;
	}
	
	if (msgctl(cbqid, IPC_RMID, NULL) == -1) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "error removing callback queue - %s", strerror(errno));
		return -1;
	}
	
	if (msgctl(qid_0, IPC_RMID, NULL) == -1) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "error removing ZERO queue - %s", strerror(errno));
		return -1;
	}
	
	return 0;

}

/* 
Input data
bcm_msg->mparam[0] - unit number
Output data
-
Return: -1 on error
		0 on on success
*/ 

static int hrast_switch_init_check(struct bcm_msgbuf *bcm_msg)
{
  int status;

	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_init_check(bcm_msg->mparam[0]);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_init_check(%d) - \"0x%x\"", /* %s */
					bcm_msg->mparam[0],
					status);	/* bcm_errmsg(status) */

	bcm_msg->bcm_rv = status;
	
	hrast_dbg_switch_init_print(status);
									
	return 0;
}

/* 
Input data
bcm_msg->mparam[0] - unit number
Output data
-
Return: -1 on error
		0 on on success
*/ 

static int hrast_switch_attach_check(struct bcm_msgbuf *bcm_msg)
{
  int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_attach_check(bcm_msg->mparam[0]);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_attach_check(%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_errmsg(status));

	bcm_msg->bcm_rv = status;
									
	return 0;
}

/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - port number 
Output data
bcm_msg->mparam[2] - port status
Return: -1 on error
		0 on on success
*/ 

static int hrast_port_link_status_get(struct bcm_msgbuf *bcm_msg)
{
  int port_link_status0;
  int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_port_link_status_get(bcm_msg->mparam[0], bcm_msg->mparam[1], &port_link_status0);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_port_link_status_get(%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					port_link_status0,
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error reading link status\n");
	else
		bcm_msg->mparam[2] = port_link_status0;

	return 0;
}

/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - port number 
Output data
bcm_msg->mparam[2] - BCM_E_DISABLED - AN disabled
					 BCM_E_BUSY     - AN not completed
					 BCM_E_NONE     - AN completed
bcm_msg->mparam[3] - ability mask		
Return: -1 on error
		0 on on success
*/ 

static int hrast_port_advert_remote_get(struct bcm_msgbuf *bcm_msg)
{
  bcm_port_abil_t ability_mask;
  int status;

	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_port_advert_remote_get(bcm_msg->mparam[0], (bcm_port_t)bcm_msg->mparam[1],
																					&ability_mask);
																					
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_port_advert_remote_get(%d,%d,0x%08x) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					ability_mask,
					bcm_errmsg(status));
																								
	bcm_msg->bcm_rv = status;
																				
	if (status == BCM_E_NONE) { 			
		hrast_dbg_print(3, SYSLOG | CONSOLE, "autonegotiation COMPLETED");
		hrast_dbg_ability_print(ability_mask);
		bcm_msg->mparam[2] = (int)ability_mask;
	}
	else if (status == BCM_E_DISABLED) {	
		hrast_dbg_print(3, SYSLOG | CONSOLE, "autonegotiation DISABLED");
		bcm_msg->mparam[2] = 0;
	}
	else if (status == BCM_E_BUSY) {	
		hrast_dbg_print(3, SYSLOG | CONSOLE, "autonegotiation NOT COMPLETED");
		bcm_msg->mparam[2] = 0;
	}
	else {
		soc_cm_debug(DK_VERBOSE, "Error reading link status\n");
		hrast_dbg_print(1, SYSLOG | CONSOLE, "error reading link status");
	}

	return 0;
}

/* 
* Function
*      hrast_port_info_get
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
* Output
*      -
* Return:
*      -1 on error
*		0 on on success
*/ 

static int hrast_port_info_get(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf)
{
	bcm_port_info_t *info;
	int status;

	if (bcm_msg == NULL)
		return -1;
		
	if (ext_buf == NULL)
		return -1;

	/* clear extended data buffer */
	memset(ext_buf, 0, sizeof(struct bcm_extbuf));
	bcm_msg->ext_msg = ext_buf;
	
	info = (bcm_port_info_t*)ext_buf->mdata;
	
	status = bcm_port_info_get(bcm_msg->mparam[0], bcm_msg->mparam[1], info);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_port_info_get(%d,%d,info) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_errmsg(status));
	
	bcm_msg->bcm_rv = status;
	
/*	hrast_dbg_port_info_print(info);*/
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error reading port info (error: %d)\n", status);
	
	return 0;
}

/* 
* Function
*      hrast_port_selective_info_get
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
*      bcm_msg->mparam[2] - action_mask
* Output
*      -
* Return:
*      -1 on error
*		0 on on success
*/ 

static int hrast_port_selective_info_get(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf)
{
	bcm_port_info_t *info;
	int status;

	if (bcm_msg == NULL)
		return -1;
		
	if (ext_buf == NULL)
		return -1;

	/* clear extended data buffer */
	memset(ext_buf, 0, sizeof(struct bcm_extbuf));
	bcm_msg->ext_msg = ext_buf;
	
	info = (bcm_port_info_t*)ext_buf->mdata;
	
	/* set action mask */
	info->action_mask = bcm_msg->mparam[2];
	
	if (info->action_mask == 0)
		hrast_dbg_print(2, SYSLOG | CONSOLE, "nothing to be read, action mask empty");
		
	status = bcm_port_selective_get(bcm_msg->mparam[0], bcm_msg->mparam[1], info);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_port_selective_get(%d,%d,info) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_errmsg(status));
	
	bcm_msg->bcm_rv = status;
	
	hrast_dbg_port_info_print(info);
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error reading port info (error: %d)\n", status);
	
	return 0;
}

/* 
* Function
*      hrast_port_vlan_info_get
* Input data
*      bcm_msg->mparam[0] - unit number
* Output
*      -
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_port_vlan_info_get(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf)
{
    int status = BCM_E_NONE, str_len=0, size;
    pbmp_t pbm;
    soc_port_t port;
    vlan_id_t vid = BCM_VLAN_VID_DISABLE;
    char buf[64];
    char log_buf[64];
    
    if (bcm_msg == NULL)
		return -1;
	
	if (ext_buf == NULL)
		return -1;

	/* clear extended data buffer */
	memset(ext_buf, 0, sizeof(struct bcm_extbuf));
	bcm_msg->ext_msg = ext_buf;
	
	size=bcm_msg->mparam[1];
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_port_vlan_info_get(%d,%d,info) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_errmsg(status));
	
	pbm = PBMP_PORT_ALL(bcm_msg->mparam[0]);
	PBMP_ITER(pbm, port) 
	{
	    if ((status = bcm_port_untagged_vlan_get(bcm_msg->mparam[0], port, &vid)) < 0) 
	    {
	        sprintf(buf, "Error retrieving info for port %s: %s\n", SOC_PORT_NAME(bcm_msg->mparam[0], port), bcm_errmsg(status));
	        strcat(ext_buf->mdata, buf);
	        break;
	    }
	    sprintf(buf, "Port %s default VLAN is %d%s\n",
	                  SOC_PORT_NAME(bcm_msg->mparam[0], port), vid,
	                  (vid == BCM_VLAN_VID_DISABLE) ? " (disabled)":"");
	    str_len = str_len + strlen(buf);
	    if (str_len >= (size - 1))
	    {
			sprintf(log_buf,"%s: Port VLAN info report larger then buffer space", MODULE_NAME);
			printk(log_buf);
			syslog(LOG_ERR, log_buf);	/* log to: /var/log/messages */
			break;
		}
		strcat(ext_buf->mdata, buf);
	}
	
	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error reading port vlan info (error: %d)\n", status);
	
	return 0;
}

/*port settings-gstr*/
/* 
* Function
*      hrast_port_max_speed_get
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
* Output
*      bcm_msg->mparam[2] - speed
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_port_max_speed_get(struct bcm_msgbuf *bcm_msg)
{
    int speed;
    int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_port_speed_max(bcm_msg->mparam[0], bcm_msg->mparam[1], &speed);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_port_max_speed_get(%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					speed,
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error reading max port speed\n");
	else
		bcm_msg->mparam[2] = speed;

	return 0;
}

/* 
* Function
*      hrast_port_speed_set
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
*      bcm_msg->mparam[2] - speed
* Output
*      -
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_port_speed_set(struct bcm_msgbuf *bcm_msg)
{
    int status;

	if (bcm_msg == NULL)
		return -1;
		
	status = bcm_port_speed_set(bcm_msg->mparam[0], bcm_msg->mparam[1], bcm_msg->mparam[2]);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_port_speed_set(%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error setting port speed (error: %d)\n", status); 

	return 0;
}

/* 
* Function
*      hrast_port_speed_get
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
* Output
*      bcm_msg->mparam[2] - speed
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_port_speed_get(struct bcm_msgbuf *bcm_msg)
{
    int speed;
    int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_port_speed_get(bcm_msg->mparam[0], bcm_msg->mparam[1], &speed);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_port_speed_get(%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					speed,
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error reading port speed\n");
	else
		bcm_msg->mparam[2] = speed;

	return 0;
}

/* 
* Function
*      hrast_port_duplex_set
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
*      bcm_msg->mparam[2] - speed
* Output
*      -
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_port_duplex_set(struct bcm_msgbuf *bcm_msg)
{
    int status;

	if (bcm_msg == NULL)
		return -1;
	
	if(bcm_msg->mparam[2]==0)
	    bcm_msg->mparam[2]=BCM_PORT_DUPLEX_HALF;
	else if(bcm_msg->mparam[2]==1)
	    bcm_msg->mparam[2]=BCM_PORT_DUPLEX_FULL;
	status = bcm_port_duplex_set(bcm_msg->mparam[0], bcm_msg->mparam[1], bcm_msg->mparam[2]);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_port_duplex_set(%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error setting port duplex (error: %d)\n", status); 

	return 0;
}

/* 
* Function
*      hrast_port_duplex_get
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
* Output
*      bcm_msg->mparam[2] - duplex
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_port_duplex_get(struct bcm_msgbuf *bcm_msg)
{
    int duplex;
    int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_port_duplex_get(bcm_msg->mparam[0], bcm_msg->mparam[1], &duplex);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_port_duplex_get(%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					duplex,
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error reading port duplex\n");
	else
	    bcm_msg->mparam[2]=duplex;

	return 0;
}

/* 
* Function
*      hrast_port_max_frame_size_get
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
* Output
*      bcm_msg->mparam[2] - frame_size
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_port_max_frame_size_get(struct bcm_msgbuf *bcm_msg)
{
    int frame_size;
    int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_port_frame_max_get(bcm_msg->mparam[0], bcm_msg->mparam[1], &frame_size);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_port_max_frame_size_get(%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					frame_size,
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error reading port max frame size\n");
	else
		bcm_msg->mparam[2] = frame_size;

	return 0;
}

/* 
* Function
*      hrast_port_interface_mode_set
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
*      bcm_msg->mparam[2] - mode
* Output
*      -
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_port_interface_mode_set(struct bcm_msgbuf *bcm_msg)
{
    int status=0;

	if (bcm_msg == NULL)
		return -1;
	
	if(bcm_msg->mparam[2]==1)
	    status = bcm_port_interface_set(bcm_msg->mparam[0], bcm_msg->mparam[1], BCM_PORT_IF_SGMII);
	else if(bcm_msg->mparam[2]==0)
	    status = bcm_port_interface_set(bcm_msg->mparam[0], bcm_msg->mparam[1], BCM_PORT_IF_GMII);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_port_interface_mode_set(%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error setting port interface (error: %d)\n", status); 

	return 0;
}

/* 
* Function
*      hrast_port_max_frame_size_set
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
*      bcm_msg->mparam[2] - frame_size
* Output
*      -
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_port_max_frame_size_set(struct bcm_msgbuf *bcm_msg)
{
    int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_port_frame_max_set(bcm_msg->mparam[0], bcm_msg->mparam[1], bcm_msg->mparam[2]);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_port_max_frame_size_set(%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error setting port max frame size (error: %d)\n", status); 

	return 0;
}

/* 
* Function
*      hrast_port_autoneg_set
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
*      bcm_msg->mparam[2] - enable
* Output
*      -
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_port_autoneg_set(struct bcm_msgbuf *bcm_msg)
{
    int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_port_autoneg_set(bcm_msg->mparam[0], bcm_msg->mparam[1], bcm_msg->mparam[2]);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_port_autoneg_set(%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error setting auto-negotiation (error: %d)\n", status); 

	return 0;
}

/* 
* Function
*      hrast_port_rate_egress_set
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
*      bcm_msg->mparam[2] - speed
*      bcm_msg->mparam[3] - speedBurst
* Output
*      -
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_port_rate_egress_set(struct bcm_msgbuf *bcm_msg)
{
    int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_port_rate_egress_set(bcm_msg->mparam[0], bcm_msg->mparam[1], bcm_msg->mparam[2], bcm_msg->mparam[3]);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_port_rate_egress_set(%d,%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_msg->mparam[3],
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error setting egress port rate (error: %d)\n", status); 

	return 0;
}

/* 
* Function
*      hrast_port_rate_ingress_set
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
*      bcm_msg->mparam[2] - speed
*      bcm_msg->mparam[3] - speedBurst
* Output
*      -
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_port_rate_ingress_set(struct bcm_msgbuf *bcm_msg)
{
    int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_port_rate_ingress_set(bcm_msg->mparam[0], bcm_msg->mparam[1], bcm_msg->mparam[2], bcm_msg->mparam[3]);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_port_rate_ingress_set(%d,%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_msg->mparam[3],
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error setting ingress port rate (error: %d)\n", status); 

	return 0;
}

/* 
* Function
*      hrast_port_rate_egress_get
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
* Output
*      bcm_msg->mparam[2] - speed
*      bcm_msg->mparam[3] - speedBurst
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_port_rate_egress_get(struct bcm_msgbuf *bcm_msg)
{
    unsigned int speed;
    unsigned int speedBurst;
    int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_port_rate_egress_get(bcm_msg->mparam[0], bcm_msg->mparam[1], &speed, &speedBurst);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_port_rate_egress_get(%d,%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					speed,
					speedBurst,
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error getting egress port rate\n");
	else
	{
		bcm_msg->mparam[2] = (int) speed;
		bcm_msg->mparam[3] = (int) speedBurst;
	}

	return 0;
}

/* 
* Function
*      hrast_port_rate_ingress_get
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
* Output
*      bcm_msg->mparam[2] - speed
*      bcm_msg->mparam[3] - speedBurst
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_port_rate_ingress_get(struct bcm_msgbuf *bcm_msg)
{
    unsigned int speed;
    unsigned int speedBurst;
    int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_port_rate_ingress_get(bcm_msg->mparam[0], bcm_msg->mparam[1], &speed, &speedBurst);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_port_rate_ingress_get(%d,%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					speed,
					speedBurst,
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error getting ingress port rate\n");
	else
	{
		bcm_msg->mparam[2] = (int) speed;
		bcm_msg->mparam[3] = (int) speedBurst;
	}

	return 0;
}
/*end port settings-gstr*/

/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - stg
bcm_msg->mparam[2] - port number
Output data
bcm_msg->mparam[3] - state
Return: -1 on error
		0 on on success
*/ 

static int hrast_stg_stp_get(struct bcm_msgbuf *bcm_msg)
{
  int port_stp_status0;
  int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_stg_stp_get(bcm_msg->mparam[0], (bcm_stg_t) bcm_msg->mparam[1],
							(bcm_port_t) bcm_msg->mparam[2], &port_stp_status0); 

	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_stg_stp_get(%d,%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					port_stp_status0,
					bcm_errmsg(status));
					
	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error reading STP port state (error: %d)\n", status);
	else
		bcm_msg->mparam[3] = port_stp_status0;

	return 0;
}

/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - stg id
bcm_msg->mparam[2] - port number 
bcm_msg->mparam[3] - state
Return: -1 on error
		0 on on success
*/ 

static int hrast_stg_stp_set(struct bcm_msgbuf *bcm_msg)
{
  int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_stg_stp_set(bcm_msg->mparam[0], (bcm_stg_t) bcm_msg->mparam[1],
							(bcm_port_t) bcm_msg->mparam[2], bcm_msg->mparam[3]);
							
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_stg_stp_set(%d,%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_msg->mparam[3],
					bcm_errmsg(status));

	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error setting STP port state (error: %d)\n", status); 

	return 0;	
}

/* Link aggregation (trunking) */
/* 
Input data
bcm_msg->mparam[0] - unit number
Return: -1 on error
		0 on on success
*/ 
static int hrast_trunk_init(struct bcm_msgbuf *bcm_msg)
{
  int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_trunk_init(bcm_msg->mparam[0]);
							
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_trunk_init(%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_errmsg(status));

	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error init trunk mode (error: %d)\n", status); 

	return 0;	
}

/* 
Input data
bcm_msg->mparam[0] - unit number
Output
bcm_msg->mparam[1] - trunk id
Return: -1 on error
		0 on on success
*/ 
static int hrast_trunk_create(struct bcm_msgbuf *bcm_msg)
{
  int status;
  bcm_trunk_t tid;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_trunk_create(bcm_msg->mparam[0],&tid);
							
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_trunk_create(%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					tid,
					bcm_errmsg(status));

	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error create trunk (error: %d)\n", status);
	else
	    bcm_msg->mparam[1] = tid;

	return 0;	
}

/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - trunk id
bcm_msg->mparam[2] - port number 1
bcm_msg->mparam[3] - port number 2
Return: -1 on error
		0 on on success
*/ 
static int hrast_trunk_set(struct bcm_msgbuf *bcm_msg)
{
  int status, j, i, dport, r;
  bcm_trunk_add_info_t t_add_info;
  bcm_pbmp_t pbmp;
  bcm_module_t modid;

	if (bcm_msg == NULL)
		return -1;
	
	/*set data for trunk info*/
	r = bcm_stk_my_modid_get(bcm_msg->mparam[0], &modid);
    if ((r < 0) && (r != BCM_E_UNAVAIL)) {
        printk("BCMX: Could not get mod id %d: %s\n", r, bcm_errmsg(r));
        return r;
    }
    
    if (r < 0) {  /* If unavailable, indicate unknown */
        modid = -1;
    }
    
    BCM_PBMP_CLEAR(pbmp);
    BCM_PBMP_PORT_ADD(pbmp,bcm_msg->mparam[2]);
    BCM_PBMP_PORT_ADD(pbmp,bcm_msg->mparam[3]);
	bcm_trunk_add_info_t_init(&t_add_info);
	j = 0;
	DPORT_BCM_PBMP_ITER(bcm_msg->mparam[0], pbmp, dport, i)
	{
	    t_add_info.tm[j] = (soc_feature(bcm_msg->mparam[0], soc_feature_mod1) && i > 31) ? modid + 1 : modid;
	    t_add_info.tp[j] = i;
	    r = bcm_stk_modmap_map(bcm_msg->mparam[0], BCM_STK_MODMAP_GET, t_add_info.tm[j],
                                   t_add_info.tp[j], &(t_add_info.tm[j]), 
                                   &(t_add_info.tp[j]));
        if (r < 0) {
            return CMD_FAIL;
        }
        j += 1;
	};	
	t_add_info.num_ports = j;
	t_add_info.psc = 0;
	t_add_info.dlf_index = BCM_TRUNK_UNSPEC_INDEX;
	t_add_info.mc_index = BCM_TRUNK_UNSPEC_INDEX;
	t_add_info.ipmc_index = BCM_TRUNK_UNSPEC_INDEX;	
	
	status = bcm_trunk_set(bcm_msg->mparam[0],bcm_msg->mparam[1], &t_add_info);
							
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_trunk_set(%d,%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_msg->mparam[3],
					bcm_errmsg(status));

	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error setting trunk (error: %d)\n", status);

	return 0;	
}

/* 
Input data
bcm_msg->mparam[0] - unit number
Return: -1 on error
		0 on on success
*/ 
static int hrast_trunk_detach(struct bcm_msgbuf *bcm_msg)
{
  int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_trunk_detach(bcm_msg->mparam[0]);
							
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_trunk_detach(%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_errmsg(status));

	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error shutting down trunk mode (error: %d)\n", status); 

	return 0;	
}

/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - trunk ID
Return: -1 on error
		0 on on success
*/ 
static int hrast_trunk_psc_get(struct bcm_msgbuf *bcm_msg)
{
  int status;
  int psc;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_trunk_psc_get(bcm_msg->mparam[0], bcm_msg->mparam[1], &psc);
	/* int bcm_trunk_psc_get(int unit, bcm_trunk_t tid, int *psc); */
							
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_trunk_psc_get(%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					psc,
					bcm_errmsg(status));

	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error getting trunk psc (error: %d)\n", status); 
	else
	    bcm_msg->mparam[2] = psc;

	return 0;	
}
/* end Link aggregation (trunking) */

/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - port number
Output
bcm_msg->mparam[2] - mode
Return: -1 on error
		0 on on success
*/ 
static int hrast_port_ifilter_get(struct bcm_msgbuf *bcm_msg)
{
  int status;
  int mode;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_port_ifilter_get(bcm_msg->mparam[0], bcm_msg->mparam[1], &mode);
							
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_port_ifilter_get(%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					mode,
					bcm_errmsg(status));

	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error hrast_port_ifilter_get (error: %d)\n", status);
	else
	    bcm_msg->mparam[2] = mode;

	return 0;	
}

/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - port number
bcm_msg->mparam[2] - mode
Return: -1 on error
		0 on on success
*/ 
static int hrast_port_ifilter_set(struct bcm_msgbuf *bcm_msg)
{
  int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_port_ifilter_set(bcm_msg->mparam[0], bcm_msg->mparam[1], bcm_msg->mparam[2]);
							
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_port_ifilter_set(%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_errmsg(status));

	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error hrast_port_ifilter_set (error: %d)\n", status);

	return 0;	
}

/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - VLAN id 

Return: -1 on error
		0 on on success
*/ 

static int hrast_vlan_create(struct bcm_msgbuf *bcm_msg)
{
  int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_vlan_create(bcm_msg->mparam[0], (bcm_vlan_t) bcm_msg->mparam[1]);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_vlan_create(%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_errmsg(status));
					
	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error creating new VLAN (error: %d)\n", status);

	return 0;	
}


/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - VLAN id 

Return: -1 on error
		0 on on success
*/ 
static int hrast_vlan_destroy(struct bcm_msgbuf *bcm_msg)
{
  int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_vlan_destroy(bcm_msg->mparam[0], (bcm_vlan_t) bcm_msg->mparam[1]);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_vlan_destroy(%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_errmsg(status));
	
	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error destroying VLAN (error: %d)\n", status); 

	return 0;	
}

/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - VLAN id
bcm_msg->pbmp[0] - pbmp 
bcm_msg->pbmp[1] - ubmp

Return: -1 on error
		0 on on success
*/ 
static int hrast_vlan_port_get(struct bcm_msgbuf *bcm_msg)
{
  int status=0;
  int unit, vlanid;
  bcm_pbmp_t port_bit_map,untag_bit_map;

	if (bcm_msg == NULL)
		return -1;

	unit = bcm_msg->mparam[0];
	vlanid = bcm_msg->mparam[1];

	status = bcm_vlan_port_get(unit, (bcm_vlan_t) vlanid, &port_bit_map, &untag_bit_map);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_vlan_port_get(%d,%d,0x%08x,0x%08x) - \"%s\"",
					unit,
					vlanid,
					port_bit_map.pbits[0],
					untag_bit_map.pbits[0],
					bcm_errmsg(status));

	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error getting port from VLAN (error %d)\n", status);
	else {
		bcm_msg->pbmp[0] = port_bit_map;
		bcm_msg->pbmp[1] = untag_bit_map;
	}

	return 0;
}


/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - VLAN id
bcm_msg->pbmp[0] - pbmp 
bcm_msg->pbmp[1] - ubmp

Return: -1 on error
		0 on on success
*/ 
static int hrast_vlan_port_add(struct bcm_msgbuf *bcm_msg)
{
  int status=0;

	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_vlan_port_add(bcm_msg->mparam[0], (bcm_vlan_t) bcm_msg->mparam[1], 
								bcm_msg->pbmp[0], bcm_msg->pbmp[1]);
								
	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_vlan_port_add(%d,%d,0x%08x,0x%08x) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->pbmp[0].pbits[0],
					bcm_msg->pbmp[1].pbits[0],
					bcm_errmsg(status));
													
	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error adding port to VLAN (error %d)\n", status); 

	return 0;
}

/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - VLAN id
bcm_msg->pbmp[0] - pbmp 

Return: -1 on error
		0 on on success
*/ 
static int hrast_vlan_port_remove(struct bcm_msgbuf *bcm_msg)
{
  int status=0;
  
#ifdef CHECK_PORT_VLAN_PRESENCE
  int unit, vlanid;
  bcm_pbmp_t port_bit_map,untag_bit_map;
#endif /* CHECK_PORT_VLAN_PRESENCE */

	if (bcm_msg == NULL)
		return -1;
	
#ifdef CHECK_PORT_VLAN_PRESENCE
	/* first check if ports are actually members of VLAN, if not return BCM_E_NOT_FOUND */
	unit = bcm_msg->mparam[0];
	vlanid = bcm_msg->mparam[1];
	status = bcm_vlan_port_get(unit, (bcm_vlan_t) vlanid, &port_bit_map, &untag_bit_map);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_vlan_port_get(%d,%d,0x%08x,0x%08x) - \"%s\"",
					unit,
					vlanid,
					port_bit_map.pbits[0],
					untag_bit_map.pbits[0],
					bcm_errmsg(status));
					
	if ( (status == BCM_E_NONE) && !(port_bit_map.pbits[0] & bcm_msg->pbmp[0].pbits[0]) )
		status = BCM_E_NOT_FOUND;
	else
#endif /* CHECK_PORT_VLAN_PRESENCE */
	
	status = bcm_vlan_port_remove(bcm_msg->mparam[0], (bcm_vlan_t) bcm_msg->mparam[1], 
									bcm_msg->pbmp[0]);
		
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_vlan_port_remove(%d,%d,0x%08x) - \"%s\"",
						bcm_msg->mparam[0],
						bcm_msg->mparam[1],
						bcm_msg->pbmp[0].pbits[0],
						bcm_errmsg(status));
						
	bcm_msg->bcm_rv = status;
		
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error removing port from VLAN (error: %d)\n", status); 

	return 0;
}


/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - port number
bcm_msg->mparam[2] - VLAN id

Return: -1 on error
		0 on on success
*/ 
static int hrast_port_untagged_vlan_set(struct bcm_msgbuf *bcm_msg)
{
  int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_port_untagged_vlan_set(bcm_msg->mparam[0], (bcm_port_t) bcm_msg->mparam[1], 
								(bcm_vlan_t) bcm_msg->mparam[2]); 
								
	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_port_untagged_vlan_set(%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_errmsg(status));
					
	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error setting default port untagged vlan (error: %d)\n", status);

	return 0;
}

/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - port number
Output
bcm_msg->mparam[2] - VLAN id

Return: -1 on error
		0 on on success
*/ 
static int hrast_port_untagged_vlan_get(struct bcm_msgbuf *bcm_msg)
{
  int status;
  bcm_vlan_t vlan;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_port_untagged_vlan_get(bcm_msg->mparam[0], (bcm_port_t) bcm_msg->mparam[1],&vlan); 
								
	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_port_untagged_vlan_set(%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					vlan,
					bcm_errmsg(status));
					
	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error getting default port untagged vlan (error: %d)\n", status);
	else
		bcm_msg->mparam[2] = vlan;

	return 0;
}


/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - port number
bcm_msg->mparam[2] - enable TRUE/FALSE

Return: -1 on error
		0 on on success
*/ 
static int hrast_port_enable_set(struct bcm_msgbuf *bcm_msg)
{
  int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_port_enable_set( bcm_msg->mparam[0], (bcm_port_t) bcm_msg->mparam[1], 
								   bcm_msg->mparam[2]);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_port_enable_set(%d,%d,%s) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2] ? "enable" : "disable",
					bcm_errmsg(status));

	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error setting enable/dissable port\n");

	return 0;
}


/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - port number
Output
bcm_msg->mparam[2] - enable TRUE/FALSE

Return: -1 on error
		0 on on success
*/ 
static int hrast_port_enable_get(struct bcm_msgbuf *bcm_msg)
{
  int status, enable;

	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_port_enable_get(bcm_msg->mparam[0], (bcm_port_t) bcm_msg->mparam[1], 
								   &enable);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_port_enable_get(%d,%d,%s) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					enable ? "enabled" : "disabled",
					bcm_errmsg(status));								   

	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error setting enable/dissable port\n");
	else
		bcm_msg->mparam[2] = enable;

	return 0;
}


/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - port number
bcm_msg->mparam[2] - statistic type
Output
bcm_msg->mparam[3] - value

Return: -1 on error
		0 on on success
*/ 
static int hrast_stat_get32(struct bcm_msgbuf *bcm_msg)
{
  int status;
  unsigned int value;

	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_stat_get32( bcm_msg->mparam[0], (bcm_port_t) bcm_msg->mparam[1], 
								   (bcm_stat_val_t) bcm_msg->mparam[2],  &value);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_stat_get32(%d,%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					value,
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error reading statistic (error: %d)\n", status);
	else
		bcm_msg->mparam[3] = (int) value;

	return 0;
}

/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - port number

Return: -1 on error
		0 on on success
*/ 
static int hrast_stat_clear(struct bcm_msgbuf *bcm_msg)
{
  int status;

	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_stat_clear( bcm_msg->mparam[0], (bcm_port_t) bcm_msg->mparam[1]);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_stat_clear(%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error clearing port statistic (error: %d)\n", status);

	return 0;
}

/* 
Input data
bcm_msg->mparam[0] - unit number

Return: -1 on error
		0 on on success
*/

static int hrast_linkscan_init(struct bcm_msgbuf *bcm_msg)
{
  int status, i;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_linkscan_init(bcm_msg->mparam[0]);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_linkscan_init(%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_errmsg(status));

	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error initializing linkscan (error: %d)\n", status);
	else {
		for (i = 0; i < MAX_CALLBACKS; i++) {
			Callback[i].c_pid = 0;
		}
		reg_callbacks = 0;
	}

	return 0;
}

/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - port number
Output data
bcm_msg->mparam[2] - mode

Return: -1 on error
		0 on on success
*/ 
static int hrast_linkscan_mode_get(struct bcm_msgbuf *bcm_msg)
{
  int status, mode;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_linkscan_mode_get(bcm_msg->mparam[0], bcm_msg->mparam[1], &mode);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_linkscan_mode_get(%d,%d,%s) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					(mode == BCM_LINKSCAN_MODE_HW) ? "HW" :
					((mode == BCM_LINKSCAN_MODE_SW) ? "SW" :
					((mode == BCM_LINKSCAN_MODE_NONE) ? "none" : "N/A")),
					bcm_errmsg(status));

	bcm_msg->bcm_rv = status;

	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error getting linkscan parameter (error: %d)\n", status);
	else
		bcm_msg->mparam[2] = mode;
		
	return 0;
}

/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - port number

Return: -1 on error
		0 on on success
*/ 
static int hrast_linkscan_mode_set(struct bcm_msgbuf *bcm_msg)
{
  int status;

	status = bcm_linkscan_mode_set(bcm_msg->mparam[0], bcm_msg->mparam[1], bcm_msg->mparam[2]);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_linkscan_mode_set(%d,%d,%s) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					(bcm_msg->mparam[2] == BCM_LINKSCAN_MODE_HW) ? "HW" :
					((bcm_msg->mparam[2] == BCM_LINKSCAN_MODE_SW) ? "SW" :
					((bcm_msg->mparam[2] == BCM_LINKSCAN_MODE_NONE) ? "none" : "N/A")),
					bcm_errmsg(status));

	bcm_msg->bcm_rv = status;

	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error setting linkscan parameter (error: %d)\n", status);
	
	return 0;
}

/* 
Input data
bcm_msg->mparam[0] - unit number
Output data
bcm_msg->mparam[1] - time between linkscan cycles

Return: -1 on error
		0 on on success
*/ 
static int hrast_linkscan_enable_get(struct bcm_msgbuf *bcm_msg)
{
  int status, scan_time;

	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_linkscan_enable_get(bcm_msg->mparam[0], &scan_time); 

	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_linkscan_enable_get(%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					scan_time,
					bcm_errmsg(status));

	bcm_msg->bcm_rv = status;

	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error getting linkscan parameter (error: %d)\n", status);
	else
		bcm_msg->mparam[1] = scan_time;
		
	return 0;
}

/* 
Input data
bcm_msg->mparam[0] - unit number
bcm_msg->mparam[1] - time between linkscan cycles

Return: -1 on error
		0 on on success
*/ 
static int hrast_linkscan_enable_set(struct bcm_msgbuf *bcm_msg)
{
  int status;

	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_linkscan_enable_set(bcm_msg->mparam[0], bcm_msg->mparam[1]); 

	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_linkscan_enable_set(%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_errmsg(status));

	bcm_msg->bcm_rv = status;

	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error setting linkscan parameter (error: %d)\n", status);

	return 0;
}


/*
Input data
bcm_msg->mparam[0] - unit number
Output data
bcm_msg->mparam[1] - time between linkscan cycles

Return: -1 on error
		0 on on success
*/ 
static int hrast_linkscan_enable_port_get(struct bcm_msgbuf *bcm_msg)
{
  int status;

	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_linkscan_enable_port_get(bcm_msg->mparam[0], bcm_msg->mparam[1]); 

	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_linkscan_enable_port_get(%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_errmsg(status));

	bcm_msg->bcm_rv = status;
	
	if (status == BCM_E_NONE)		
		hrast_dbg_print(3, SYSLOG | CONSOLE, "linkscan enabled");
	else if (status == BCM_E_DISABLED)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "linkscan disabled");
	else if (status == BCM_E_PORT)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "invalid port number");
	else {
		soc_cm_debug(DK_VERBOSE, "Error getting linkscan parameter (error: %d)\n", status);
		hrast_dbg_print(1, SYSLOG | CONSOLE, "error reading linkscan status");
	}

	return 0;
}

/* 
Input data
bcm_msg->mparam[0] - unit number 
bcm_msg->mparam[1] - client side thread id

Return: -1 on error
		0 on on success
*/ 
static int hrast_linkscan_register(struct bcm_msgbuf *bcm_msg)
{
  int status, i, empty_idx = MAX_CALLBACKS;
  
	if (bcm_msg == NULL)
		return -1;
	
	/* check if linkscan initialized */
	if (reg_callbacks < 0) {
			hrast_dbg_print(1, SYSLOG | CONSOLE, "linkscan not initialized");
			status = BCM_E_INIT;
			goto reg_end;
	}
	/* check if c_pid already registered (only one registration per client allowed) */
	for (i = 0; i < MAX_CALLBACKS; i++) {
		if (bcm_msg->mpid == Callback[i].c_pid) {
			hrast_dbg_print(1, SYSLOG | CONSOLE, "duplicated callback registration");
			return -1;
		}
	}

	/* find epmty index in Callback array if there is any */
	for (i = 0; i < MAX_CALLBACKS; i++) {
		if (Callback[i].c_pid == 0) {
			empty_idx = i;
			break;	
		}
	}
	if (empty_idx == MAX_CALLBACKS) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "max number of callbacks exceeded");
		status = BCM_E_MEMORY;
		goto reg_end;
	}
			
	/* register callback at BCM API */
	if (reg_callbacks > 1) /* nothing to do - registered already */
		status = BCM_E_NONE;
	else {
		/* do register */
		status = bcm_linkscan_register(bcm_msg->mparam[0],
			(bcm_linkscan_handler_t)hrast_linkscan_callback);
			
		hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_linkscan_register(%d,&CB_func) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_errmsg(status));
	}

reg_end:
		
	bcm_msg->bcm_rv = status;
	bcm_msg->mparam[1] = empty_idx;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "hrast: callback registration error: %d\n", status);
	else {
		/* save callback info */
		Callback[empty_idx].c_pid = bcm_msg->mpid;
		reg_callbacks++;
		hrast_dbg_print(0, SYSLOG | CONSOLE, "Callback added (pid=%d), altogether %d registered",
					Callback[empty_idx].c_pid,
					reg_callbacks);
	}
	hrast_dbg_callbacks_print();
	
	return 0;
}


/* 
Input data
bcm_msg->mparam[0] - unit number 
bcm_msg->mparam[1] - client side thread id

Return: -1 on error
		0 on on success
*/ 
static int hrast_linkscan_unregister(struct bcm_msgbuf *bcm_msg)
{
  int status;

	if (bcm_msg == NULL)
		return -1;
	
	/* check if linkscan initialized */
	if (reg_callbacks < 0) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "linkscan not initialized");
		status = BCM_E_INIT;
		goto unreg_end;
	}
	/* clear registered c_pid */
	if (Callback[bcm_msg->mparam[1]].c_pid != bcm_msg->mpid) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "linkscan unregistration error - entry not found");
		return -1;
	}
	
	if (reg_callbacks > 1) {
		/* don't unregister hrast callback at BCM API yet */
		status = BCM_E_NONE;
		hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_linkscan_unregister() skipped and returned - \"%s\"",
					bcm_errmsg(status));
		goto unreg_end;
	}
	
	/* unregister hrast callback at BCM API */
	status = bcm_linkscan_unregister(bcm_msg->mparam[0],
		(bcm_linkscan_handler_t)hrast_linkscan_callback);
		
	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_linkscan_unregister(%d,&CB_func) - \"%s\"",
				bcm_msg->mparam[0],
				bcm_errmsg(status));
		
unreg_end:
	
	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "hrast: callback unregistration error: %d\n", status);
	else {
		
		reg_callbacks--;
		hrast_dbg_print(0, SYSLOG | CONSOLE, "Callback removed (pid=%d), remaining %d registered",
					Callback[bcm_msg->mparam[1]].c_pid,
					reg_callbacks);
		Callback[bcm_msg->mparam[1]].c_pid = 0;	
	}
	hrast_dbg_callbacks_print();
	
	return 0;  
}

/*
Return: -1 on error
		0 on on success
*/ 
void hrast_linkscan_callback(int unit, int port, bcm_port_info_t *info)
{
  struct bcm_msgbuf msg_buf;
  struct bcm_extbuf ext_buf;
  int rv, i;

	/* prepare message */
	memset(&msg_buf, 0, sizeof(struct bcm_msgbuf));
	msg_buf.mcmd = HRAST_LINKSCAN_CALLBACK;
	msg_buf.mtype = HRAST_MESSAGE_FROM_BCM_E;
	msg_buf.mparam[0] = unit;
	msg_buf.mparam[1] = port;
	
	/* prepare extended message */
	
	if (memcpy(ext_buf.mdata, info, sizeof(bcm_port_info_t)) != ext_buf.mdata) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "memcpy error at linkscan callback");
		return;
	}

	/* send data to all registered */
	for (i = 0; i < MAX_CALLBACKS; i++) {
		
		/* send callback data to all registered clients */
		if (Callback[i].c_pid != 0) {
			msg_buf.lnx_mtype = Callback[i].c_pid;
			msg_buf.ext_msg = &ext_buf;

			/* send message */
			rv = hrast_sys_msg_send(cbqid, &msg_buf);
			if (rv == -1) {
				hrast_dbg_print(1, SYSLOG, "error sending callback (idx=%d)", i);
				continue;
			}
			hrast_dbg_print(1, SYSLOG, "callback (idx=%d) sent to queue: %d", i, cbqid);
		}
	}
	return;
}

static void *hrast_linkscan_keep_alive(void* none)
{
  int res;
  int i, rv, unit = 0;
  int num_cleared;
  pid_t c_pid;

	res = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if (res != 0) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "pthread_setcancelstate error - %s (tid=%d)",
		strerror(errno),
		pthread_self());
		exit(EXIT_FAILURE);
	}
	res = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	if (res != 0) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "pthread_setcanceltype error - %s (tid=%d)",
		strerror(errno),
		pthread_self());
		exit(EXIT_FAILURE);
	}

	while(1){
		if (reg_callbacks > 0) {
			/* 
			Check connectivity for all hrast callback threads and unregister
			callbacks for threads, which are not accessible
			*/
			for (i = 0; i < MAX_CALLBACKS; i++) {
				if (Callback[i].c_pid != 0) {
					c_pid = Callback[i].c_pid;
					/* check connectivity to hrast_api callback thread */
					rv = hrast_linkscan_connectivity_check(cbqid, c_pid);
					
					if (rv != 0) {
						/* clear entry at callback table */ 
						reg_callbacks--;
						hrast_dbg_print(0, SYSLOG | CONSOLE, "Unaccessible callback removed (pid=%d), %d remaining registered",
										c_pid,
										reg_callbacks);
						Callback[i].c_pid = 0;
						
						/* client callback thread not accessible, unregister callback */
						if (reg_callbacks == 0) {
							/* unregister hrast callback at BCM API */
							rv = bcm_linkscan_unregister(unit,	(bcm_linkscan_handler_t)hrast_linkscan_callback);
							hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_linkscan_unregister(%d,&CB_func) - \"%s\"",
										unit,
										bcm_errmsg(rv));
						}
											
						/*clear sent messages from callback queue */
						num_cleared = 0;
						do {
							rv = hrast_linkscan_pull_message(cbqid, c_pid);
							if (rv == 0)
								num_cleared++;
						} while (rv == 0);
						if (num_cleared > 0)
							hrast_dbg_print(2, SYSLOG | CONSOLE, "%d messages pulled from callback queue", num_cleared);
							
						hrast_dbg_callbacks_print();	
					}
					else
						hrast_dbg_print(2, SYSLOG | CONSOLE, "callback thread %d accessible (pid=%d)",
							i, Callback[i].c_pid);
				}
			}
		}
		usleep(KEEP_ALIVE_PERIOD);
	}


}



/* 
Check connectivity with hrast_api process

Input data: cbqid - callback queue id
	        pid - client pid

Return: -1 on error
		0 on on success
*/ 
static int hrast_linkscan_connectivity_check(int cbqid, int pid)
{
	struct bcm_msgbuf msg_buf;

	memset(&msg_buf, 0, sizeof(struct bcm_msgbuf));
	msg_buf.lnx_mtype = pid;
	msg_buf.mcmd = HRAST_LINKSCAN_ECHO;
	msg_buf.mtype = HRAST_MESSAGE_FROM_BCM;
	
	/* send echo request to registered thread */
	if (hrast_sys_msg_send(cbqid, &msg_buf) == -1) {
		hrast_dbg_print(2, SYSLOG | CONSOLE, "error sending echo request (pid=%d)", pid);
		return -1;
	}

	hrast_dbg_print(2, SYSLOG | CONSOLE, "echo request sent to client (pid=%d)", pid);
	
	/* wait for echo response from registered thread */
	msg_buf.lnx_mtype = HRAST_MSG_TYPE;
	if (hrast_sys_msg_receive(cbqid, HRAST_LONG_TIMEOUT, &msg_buf, NULL) != 0) {
		hrast_dbg_print(1, SYSLOG, "error receiving echo response (pid=%d)", msg_buf.mpid);
		return -1;
	}

	hrast_dbg_print(2, SYSLOG | CONSOLE, "echo response received (pid=%d)", msg_buf.mpid);
	
	if (msg_buf.mstatus != HRAST_MESSAGE_CMD_OK)
		hrast_dbg_print(2, SYSLOG | CONSOLE, "echo error (pid=%d)", pid);
		
	return 0;
}

/* 
Pull one callback message from callback queue sent to certain client

Input data: cbqid - callback queue id
	        pid - client pid

Return: -1 on error
		0 on on success
*/ 
static int hrast_linkscan_pull_message(int cbqid, int pid)
{
	struct bcm_msgbuf msg_buf;
	struct bcm_extbuf ext_buf;

	memset(&msg_buf, 0, sizeof(struct bcm_msgbuf));
	msg_buf.lnx_mtype = pid;
	
	/* pull one message */
	if (hrast_sys_msg_receive(cbqid, HRAST_SHORT_TIMEOUT, &msg_buf, &ext_buf) != 0)
		return -1;
			
	return 0;
}

/* 
Input data
bcm_msg->mparam[0] - unit number

Return: -1 on error
		0 on on success
*/

static int hrast_linkscan_detach(struct bcm_msgbuf *bcm_msg)
{
  int status;

	if (bcm_msg == NULL)
		return -1;

	status = bcm_linkscan_detach(bcm_msg->mparam[0]);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_linkscan_detach(%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_errmsg(status));

	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error detaching linkscan (error: %d)\n", status);
	else
		reg_callbacks = -1;


	return 0;
}

/* 
Return: -1 on error
		0 on on success
*/ 
static int hrast_link_status_show(struct bcm_msgbuf *bcm_msg)
{
  int status;
  args_t a;
  
	if (bcm_msg == NULL)
		return -1;

	memset(&a,0,sizeof(args_t));

	status = if_esw_port_stat(bcm_msg->mparam[0], &a);  
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "if_port_stat(%d)", bcm_msg->mparam[0]);

	bcm_msg->bcm_rv = status;
									
	if (status != CMD_OK)
		soc_cm_debug(DK_VERBOSE, "Error port status show (error: %d)\n", status); 

	return 0;
}


/* 
Return: -1 on error
		0 on on success
*/ 
static int hrast_vlan_show(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf)
{
  int status = BCM_E_NONE;
  
	bcm_vlan_data_t	*list;
	int		count, i, str_len=0, size;
	char		bufp[FORMAT_PBMP_MAX], bufu[FORMAT_PBMP_MAX];
	char		pfmtp[SOC_PBMP_FMT_LEN];
	char		pfmtu[SOC_PBMP_FMT_LEN];
	int			r = 0;
	int         unit = 0;
	char        *bcm_vlan_mcast_flood_str[] = BCM_VLAN_MCAST_FLOOD_STR;
	char        buf[256];
    char        log_buf[64];

	if (bcm_msg == NULL)
		return -1;
	
	if (ext_buf == NULL)
		return -1;
		
	hrast_dbg_print(2, SYSLOG | CONSOLE, "vlan_show()");
	
	/* clear extended data buffer */
	memset(ext_buf, 0, sizeof(struct bcm_extbuf));
	bcm_msg->ext_msg = ext_buf;
	
	size=bcm_msg->mparam[1];
	if ((r = bcm_vlan_list(unit, &list, &count)) < 0) {
		status = BCM_E_UNAVAIL;
		goto bcm_err;
	}

	for (i = 0; i < count; i++) {
		bcm_vlan_mcast_flood_t  mode;
		if ((r = bcm_vlan_mcast_flood_get(unit, list[i].vlan_tag, &mode)) < 0) {
			if (r == BCM_E_UNAVAIL) {
				mode = BCM_VLAN_MCAST_FLOOD_COUNT;
			} else {
				status = BCM_E_UNAVAIL;
				goto bcm_err;
			}
		}
		format_pbmp(unit, bufp, sizeof(bufp), list[i].port_bitmap);
		format_pbmp(unit, bufu, sizeof(bufu), list[i].ut_port_bitmap);
		hrast_dbg_print(0, SYSLOG | CONSOLE, "vlan %4d  ports %s (%s), untagged %s (%s) %s",
			   list[i].vlan_tag,
			   bufp, SOC_PBMP_FMT(list[i].port_bitmap, pfmtp),
			   bufu, SOC_PBMP_FMT(list[i].ut_port_bitmap, pfmtu),
					   bcm_vlan_mcast_flood_str[mode]
					   );
        sprintf(buf, "vlan %4d  ports %s (%s), untagged %s (%s) %s\n", list[i].vlan_tag,
			   bufp, SOC_PBMP_FMT(list[i].port_bitmap, pfmtp),
			   bufu, SOC_PBMP_FMT(list[i].ut_port_bitmap, pfmtu),
					   bcm_vlan_mcast_flood_str[mode]);
				   
		str_len = str_len + strlen(buf);
		if (str_len >= (size - 1)) {
			sprintf(log_buf,"%s: Congestion report larger then buffer space", MODULE_NAME);
			printk(log_buf);
			syslog(LOG_ERR, log_buf);	/* log to: /var/log/messages */
			break;
		}
		strcat(ext_buf->mdata, buf);
	}

	bcm_vlan_list_destroy(unit, list, count);

bcm_err:
	
	bcm_msg->bcm_rv = status;

	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error vlan status show (error: %d)\n", status);
	
	return 0;
}

/* 
* Function
*      hrast_diag_mem_set
* Input data
*      bcm_msg->mparam[0] - unit number
*      ext_buf->mdata - string for set the memory
*
* Output
*
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_diag_mem_set(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf)
{
	int status;
	
	if (bcm_msg == NULL)
		return -1;
		
	if (ext_buf == NULL)
		return -1;
	
	status = bcm_diag_mem_set(bcm_msg->mparam[0], ext_buf->mdata);
			
	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_diag_mem_set(%d,%s,mdata) - \"%s\"",
					bcm_msg->mparam[0],
					ext_buf->mdata,
					bcm_errmsg(status));
			
	bcm_msg->bcm_rv = status;

	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error setting memory/table (error: %d)\n", status);

	return 0;
}

/* 
* Function
*      hrast_diag_mem_get
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
*      bcm_msg->mparam[2] - mode
*      bcm_msg->mparam[3] - buffer size
*      bcm_msg->mparam[4] - buffer address
* Output
*
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_diag_mem_get(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf)
{
	int status;
	
	if (bcm_msg == NULL)
		return -1;
		
	if (ext_buf == NULL)
		return -1;
		
	/* clear extended data buffer */
	memset(ext_buf, 0, sizeof(struct bcm_extbuf));
	bcm_msg->ext_msg = ext_buf;
	
	status = bcm_diag_mem_get(bcm_msg->mparam[0], bcm_msg->mparam[1], bcm_msg->mparam[2], bcm_msg->mparam[3], ext_buf->mdata);
			
	hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_diag_mem_get(%d,%d,%d,%d,mdata) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_msg->mparam[3],
					bcm_errmsg(status));
			
	bcm_msg->bcm_rv = status;

	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error reading filter (error: %d)\n", status);

	return 0;
}

/* 
* Function
*      hrast_diag_filter_set
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
*      bcm_msg->mparam[2] - mode: 0-ingress,1-egress
*      bcm_msg->mparam[3] - filter id
*
* Output
*
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_diag_filter_set(struct bcm_msgbuf *bcm_msg)
{
	int status;
	
	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_diag_filter_set(bcm_msg->mparam[0], bcm_msg->mparam[1], bcm_msg->mparam[2], &bcm_msg->mparam[3]);
			
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_diag_filter_set(%d,%d,%d,%d,mdata) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_msg->mparam[3],
					bcm_errmsg(status));
			
	bcm_msg->bcm_rv = status;

	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error setting filter (error: %d)\n", status);

	return 0;
}

/* 
* Function
*      hrast_diag_filter_destroy_by_port
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port number/fid
*
* Output
*
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_diag_filter_destroy_by_port(struct bcm_msgbuf *bcm_msg)
{
	int status;
	
	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_diag_filter_destroy_by_port(bcm_msg->mparam[0], bcm_msg->mparam[1]);
			
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_diag_filter_destroy_by_port(%d,%d,mdata) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_errmsg(status));
			
	bcm_msg->bcm_rv = status;

	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error destroying filter by port (error: %d)\n", status);

	return 0;
}

/* 
* Function
*      hrast_diag_cam_selective_count_get
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - buffer size
*      bcm_msg->mparam[2] - buffer address
* Output
*
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_diag_cam_selective_count_get(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf)
{
	int status;
	
	if (bcm_msg == NULL)
		return -1;
		
	if (ext_buf == NULL)
		return -1;
		
	/* clear extended data buffer */
	memset(ext_buf, 0, sizeof(struct bcm_extbuf));
	bcm_msg->ext_msg = ext_buf;
	
	status = bcm_diag_cam_selective_count_get(bcm_msg->mparam[0],
			bcm_msg->mparam[1], ext_buf->mdata);
			
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_diag_cam_selective_count_get(%d,%d,mdata) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_errmsg(status));
			
	bcm_msg->bcm_rv = status;

	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error reading CAM table (error: %d)\n", status);

	return 0;
}

/* 
* Function
*      hrast_diag_cam_entry_get
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port number
*      bcm_msg->mparam[2] - buffer size
*      bcm_msg->mparam[3] - buffer address
* Output
*
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_diag_cam_entry_get(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf)
{
	int status;
	
	if (bcm_msg == NULL)
		return -1;
		
	if (ext_buf == NULL)
		return -1;
		
	/* clear extended data buffer */
	memset(ext_buf, 0, sizeof(struct bcm_extbuf));
	bcm_msg->ext_msg = ext_buf;
	
	status = bcm_diag_cam_entry_get(bcm_msg->mparam[0],
			bcm_msg->mparam[1], bcm_msg->mparam[2], ext_buf->mdata);
			
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_diag_cam_entry_get(%d,%d,%d,mdata) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_errmsg(status));
			
	bcm_msg->bcm_rv = status;

	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error reading CAM table entry (error: %d)\n", status);

	return 0;
}

/* 
* Function
*      hrast_diag_cam_age_timer_get
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - age timer (seconds)
* Output
*
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_diag_cam_age_timer_get(struct bcm_msgbuf *bcm_msg)
{
	int status = BCM_E_NONE;
	
	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_l2_age_timer_get(bcm_msg->mparam[0], &bcm_msg->mparam[1]);
			
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_diag_cam_age_timer_get(%d,%d,mdata) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_errmsg(status));
			
	bcm_msg->bcm_rv = status;

	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error get CAM age timer (error: %d)\n", status);

	return 0;
}

/* 
* Function
*      hrast_diag_cam_age_timer_set
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - age timer (seconds)
* Output
*
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_diag_cam_age_timer_set(struct bcm_msgbuf *bcm_msg)
{
	int status = BCM_E_NONE;
	
	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_l2_age_timer_set(bcm_msg->mparam[0], bcm_msg->mparam[1]);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_diag_cam_age_timer_set(%d,%d,mdata) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_errmsg(status));
			
	bcm_msg->bcm_rv = status;

	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error set CAM age timer (error: %d)\n", status);

	return 0;
}

/* 
* Function
*      hrast_diag_table_size_get
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - table index (CAM table is L2Xm - 113 in \include\soc\mcm\allenum.h)
* Output
*      bcm_msg->mparam[0] - max table size
*
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_diag_table_size_get(struct bcm_msgbuf *bcm_msg)
{
	int status = BCM_E_NONE;
	
	if (bcm_msg == NULL)
		return -1;
		
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_diag_table_size_get(unit: %d, table: %d, table name: %s)",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					SOC_MEM_UFNAME(bcm_msg->mparam[0], bcm_msg->mparam[1]));
					
	bcm_msg->mparam[2] = soc_mem_index_count(bcm_msg->mparam[0],bcm_msg->mparam[1]);
	
	if(bcm_msg->mparam[2] < 0)
	    status = BCM_E_PARAM;
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_diag_table_size_get - size: %d",
					bcm_msg->mparam[2]);
					
	bcm_msg->bcm_rv = status;
	
	return (0);
}

/*
 * Function:
 *      hrast_diag_table_count_get
 * Purpose:
 *      
 * Input data
 *      bcm_msg->mparam[0] - unit - StrataSwitch unit number.
 *      bcm_msg->mparam[1] - mem - table index (L2X_VALIDm - 119 \include\soc\mcm\allenum.h)
 * Output
 *      bcm_msg->mparam[0] - count of valid entries in table
 *      bcm_msg->bcm_rv = BCM_E_NOT_FOUND - if table not supported yet
 * Returns:
 *      0 - on success
 *      
 * Notes:
 *     
 */
int hrast_diag_table_count_get(struct bcm_msgbuf *bcm_msg)
{
    uint32 idx_min, idx_max, idx, idx_curr, found_count = 0;
    uint32 buf;
    
    if (bcm_msg == NULL)
		return -1;
		
    idx_min = soc_mem_index_min(bcm_msg->mparam[0], bcm_msg->mparam[1]);
    idx_max = soc_mem_index_max(bcm_msg->mparam[0], bcm_msg->mparam[1]);
    idx = idx_max-idx_min;
    
    hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_diag_table_count_get (unit: %d, table: %d, table name: %s, indexes: %d)",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					SOC_MEM_UFNAME(bcm_msg->mparam[0], bcm_msg->mparam[1]),
					idx);
					
    switch(bcm_msg->mparam[1])
    {
        case L2X_VALIDm: /*L2_VALID_BITS table*/
        {
            l2x_valid_entry_t l2x_valid_entry[idx];
            
            memset(&l2x_valid_entry[0], 0, idx * sizeof(l2x_valid_entry_t));
            for (idx_curr=idx_min; idx_curr <= idx_max; idx_curr++)
            {
                soc_mem_read(bcm_msg->mparam[0], bcm_msg->mparam[1], MEM_BLOCK_ANY, idx_curr, &l2x_valid_entry[idx_curr]);
                if(!soc_L2X_VALIDm_field_get(bcm_msg->mparam[0], &l2x_valid_entry[idx_curr], BUCKET_BITMAPf, &buf))
                {
                    hrast_dbg_print(1, SYSLOG | CONSOLE, "soc_L2X_VALIDm_field_get failed - unit %d, index %d, value %d!", 
                                    bcm_msg->mparam[0], 
                                    idx_curr, 
                                    l2x_valid_entry[idx_curr]);
                    continue;
                }
                else
                {
                    while(buf>0)
                    {
                        if(buf%2==1)
                            found_count++;
                        buf = buf/2;
                    }
                }
            }
            hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_diag_table_count_get - table %s, entries %d!",
                            SOC_MEM_UFNAME(bcm_msg->mparam[0],bcm_msg->mparam[1]), found_count);
            break;
        }
        default:
        {
            hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_diag_table_count_get - table %s not supported yet",
					SOC_MEM_UFNAME(bcm_msg->mparam[0], bcm_msg->mparam[1]));
			bcm_msg->bcm_rv = BCM_E_NOT_FOUND;
			break;
		}
    }
    if(entryFlag==TRUE)
    {
        bcm_msg->mparam[2] = table_entries;
        entryFlag=FALSE;
        
    }
    else
        bcm_msg->mparam[2] = found_count;
        
	return (0);
}

/* 
* Function
*      hrast_diag_in_cos_pkt_count_get
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - buffer size
*      bcm_msg->mparam[2] - buffer address
*
* Output
*      bcm_msg->mparam[3] - type - 0 - in cos pkt count get
*                                  1 - out cos pkt count get
*      
*
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_diag_in_cos_pkt_count_get(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf)
{
	int status;
		
	if (bcm_msg == NULL)
		return -1;
		
	if (ext_buf == NULL)
		return -1;
		
	/* clear extended data buffer */
	memset(ext_buf, 0, sizeof(struct bcm_extbuf));
	bcm_msg->ext_msg = ext_buf;
	
	/*set param for in_cos_pkt_get*/
	bcm_msg->mparam[3]=HRAST_DIAG_IN_COS_PKT_GET;
	
	status = bcm_diag_in_cos_pkt_count_get(bcm_msg->mparam[0], bcm_msg->mparam[1], bcm_msg->mparam[3], ext_buf->mdata);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_diag_in_cos_pkt_count_get(%d,%d,%d,mdata) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[3],
					bcm_errmsg(status));
	
	bcm_msg->bcm_rv = status;
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error reading CAM table (error: %d)\n", status);
		
	return 0;
}

/* 
* Function
*      hrast_diag_out_cos_pkt_count_get
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - buffer size
*      bcm_msg->mparam[2] - buffer address
*
* Output
* in bcm_msg
*      bcm_msg->mparam[3] - type - 0 - in cos pkt count get
*                                  1 - out cos pkt count get
*
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_diag_out_cos_pkt_count_get(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf)
{
	int status;
	
	if (bcm_msg == NULL)
		return -1;
		
	if (ext_buf == NULL)
		return -1;
		
	/* clear extended data buffer */
	memset(ext_buf, 0, sizeof(struct bcm_extbuf));
	bcm_msg->ext_msg = ext_buf;
	
	/*set param for out_cos_pkt_get*/
	bcm_msg->mparam[3]=HRAST_DIAG_OUT_COS_PKT_GET;
	
	status = bcm_diag_in_cos_pkt_count_get(bcm_msg->mparam[0], bcm_msg->mparam[1], bcm_msg->mparam[3], ext_buf->mdata);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_diag_out_cos_pkt_count_get(%d,%d,%d,mdata) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[3],
					bcm_errmsg(status));
					
	bcm_msg->bcm_rv = status;
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error reading CAM table (error: %d)\n", status);
		
	return 0;
}

/* 
* Function
*      hrast_diag_congestion_pkt_disc_count_get
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - ingress port
*
* Output
*      bcm_msg->mparam[2] - egress port
*      bcm_msg->mparam[3] - pkts
*      bcm_msg->mparam[4] - congestion packet threshold - can be set in BCM menu
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_diag_congestion_pkt_disc_count_get(struct bcm_msgbuf *bcm_msg)
{
	int status;
	int egress_port;
	int nr_pkts;
	
	if (bcm_msg == NULL)
		return -1;
		
	status = bcm_diag_congestion_pkt_disc_count_get(bcm_msg->mparam[0],bcm_msg->mparam[1], &egress_port, &nr_pkts);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_diag_congestion_pkt_disc_count_get(%d,%d,%d,%d)",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_msg->mparam[3]);
					
	bcm_msg->bcm_rv = status;
	bcm_msg->mparam[2]=egress_port;
	bcm_msg->mparam[3]=nr_pkts;
	bcm_msg->mparam[4]=cong_pkt_thr;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error reading CAM table (error: %d)\n", status);

	return 0;
}

/* 
* Function
*      hrast_diag_congestion_pkt_disc_count_get_all
*
*   Available in BCM menu
*
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - ingress port
*      bcm_msg->mparam[2] - buffer size
*      bcm_msg->mparam[3] - buffer address
*
* Output
*
* Return:
*      -1 on error
*		0 on on success
*/ 
static int hrast_diag_congestion_pkt_disc_count_get_all(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf)
{
	int status;
	
	if (bcm_msg == NULL)
		return -1;
		
	if (ext_buf == NULL)
		return -1;
		
	/* clear extended data buffer */
	memset(ext_buf, 0, sizeof(struct bcm_extbuf));
	bcm_msg->ext_msg = ext_buf;
	status = bcm_diag_congestion_pkt_disc_count_get_all(bcm_msg->mparam[0],bcm_msg->mparam[1], bcm_msg->mparam[2], ext_buf->mdata);
	hrast_dbg_print(2, SYSLOG | CONSOLE, "hrast_diag_congestion_pkt_disc_count_get_all(%d,%d,%d,mdata) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_errmsg(status));
					
	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "Error getting congestion packet count (error: %d)\n", status);
		
	return 0;
}

/*-------------------------------------*/
/*function for test BCM diagnostic test*/
/*-------------------------------------*/
static int hrast_diag_congestion_pkt_disc_threshold_set(struct bcm_msgbuf *bcm_msg)
{
    if (bcm_msg == NULL)
		return -1;
		
    cong_pkt_thr=bcm_msg->mparam[1];
    bcm_msg->bcm_rv = BCM_E_NONE;
    
	return 0;
}

static int hrast_diag_table_count_set(struct bcm_msgbuf *bcm_msg)
{
    if (bcm_msg == NULL)
		return -1;
		
    table_entries=bcm_msg->mparam[1];
    entryFlag=TRUE;
    bcm_msg->bcm_rv = BCM_E_NONE;
    
	return 0;
}

/* BCM diagnostic debug functions */

static int hrast_diag_test_debug_mode_set(struct bcm_msgbuf *bcm_msg)
{
    if (bcm_msg == NULL)
		return -1;
		
    BCMdiagDebugSet=bcm_msg->mparam[0];
    bcm_msg->bcm_rv = BCM_E_NONE;
    
	return 0;
}

static int hrast_diag_test_debug_mode_get(struct bcm_msgbuf *bcm_msg)
{
    if (bcm_msg == NULL)
        return -1;
		
    bcm_msg->mparam[0]=BCMdiagDebugSet;
    bcm_msg->bcm_rv = BCM_E_NONE;
    
	return 0;
}

/* debug functions */

static int hrast_dbg_level_get(struct bcm_msgbuf *bcm_msg)
{
	if (bcm_msg == NULL)
		return -1;
	
	bcm_msg->mparam[0] = hrast_dbg.console_on;
	bcm_msg->mparam[1] = hrast_dbg.syslog_on;
	bcm_msg->mparam[2] = hrast_dbg.level;
	bcm_msg->bcm_rv = 0;
	
	return 0;
}

static int hrast_dbg_level_set(struct bcm_msgbuf *bcm_msg)
{
	if (bcm_msg == NULL)
		return -1;
		
	hrast_dbg.console_on = bcm_msg->mparam[0];
	hrast_dbg.syslog_on = bcm_msg->mparam[1];
	hrast_dbg.level = bcm_msg->mparam[2];
	bcm_msg->bcm_rv = 0;
	
	if (hrast_dbg.level > 0)
		hrast_dbg_print(1, SYSLOG | CONSOLE, "debug prints ON (level %d)", hrast_dbg.level);
	else
		hrast_dbg_print(0, SYSLOG | CONSOLE, "debug prints OFF", hrast_dbg.level);
	
	return 0;
}

#ifdef __RSTP__
static int hrast_tx_rstp(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf)
{
	int status;
	bcm_pkt_t *pkt = NULL;
	int unit, port, data_size;

	if (bcm_msg == NULL) {
	    hrast_dbg_print(1, SYSLOG | CONSOLE, "bcm_msg == NULL");
	    return -1;
	}
	if (ext_buf == NULL) {
	    hrast_dbg_print(1, SYSLOG | CONSOLE, "ext_buf == NULL");
	    return -1;
	}

	unit = bcm_msg->mparam[0];
	port = bcm_msg->mparam[1];
	data_size = bcm_msg->mparam[2];

	hrast_dbg_print(4, SYSLOG | CONSOLE, "hrast_tx_rstp - unit: %d, port: %d, data size: %d", unit, port, data_size);

	status = bcm_pkt_alloc(unit, data_size, BCM_TX_CRC_REGEN | BCM_TX_NO_PAD, &pkt);
	if (status != BCM_E_NONE) {
	    hrast_dbg_print(0, SYSLOG | CONSOLE, "Error bcm_pkt_alloc (error: %d)", status);
	    goto tx_end;
	}
	
	sal_memset(pkt->pkt_data[0].data, 0, data_size);
	sal_memcpy(pkt->pkt_data[0].data, ext_buf->mdata, data_size);
        SOC_PBMP_PORT_SET(pkt->tx_pbmp, port);

	status = bcm_tx(unit, pkt, NULL);
	
	if (status != BCM_E_NONE) {
	    hrast_dbg_print(1, SYSLOG | CONSOLE, "hrast_tx_rstp - Error transmiting (error: %d)\n", status);
                soc_cm_debug(DK_VERBOSE, "Error transmiting (error: %d)\n", status);
	}
	bcm_pkt_free(unit, pkt);

tx_end:
	bcm_msg->bcm_rv = status;

	return 0;
}

static int hrast_rx_start(struct bcm_msgbuf *bcm_msg)
{
	hrast_dbg_print(1, SYSLOG | CONSOLE, "hrast_rx_start");
	if (bcm_msg == NULL)
		return -1;

	int status, unit;
        unit = bcm_msg->mparam[0];

        status = bcm_rx_start(unit, NULL);
        if (status != BCM_E_NONE) {
                hrast_dbg_print(1, SYSLOG | CONSOLE, "Error RX start: %d - %s",
                        status, bcm_errmsg(status));
        }
        bcm_msg->bcm_rv = status;
        return 0;
}

static int hrast_rx_active(struct bcm_msgbuf *bcm_msg)
{
	hrast_dbg_print(1, SYSLOG | CONSOLE, "hrast_rx_active");
        if (bcm_msg == NULL)
                return -1;

	int status, unit;
	unit = bcm_msg->mparam[0];
	
	status = bcm_rx_active(unit);
	if (status != BCM_E_NONE) {
                hrast_dbg_print(1, SYSLOG | CONSOLE, "Error RX active: %d - %s",
                        status, bcm_errmsg(status));
	}
	bcm_msg->bcm_rv = status;
        return 0;
}

static bcm_rx_t hrast_rx_receive_rstp_cb(int unit, bcm_pkt_t *pkt, void *vp)
{
    int i;
    if (rx_time == 0)
        rx_time = sal_time();
    
    sal_time_t new_time = sal_time();
/*    hrast_dbg_print(1, SYSLOG | CONSOLE, "hrast RX callback - rx_time: %u, new_time: %u", rx_time, new_time); */
    if ((rx_time+2) > new_time) {
        return BCM_RX_NOT_HANDLED;
    }

    uint8 *pkt_data = BCM_PKT_IEEE(pkt);
	int len = pkt->pkt_data[0].len;
    int reason = BCM_RX_REASON_GET(pkt->rx_reasons, bcmRxReasonProtocol);
    
    int count = 0;
    BCM_RX_REASON_COUNT(pkt->rx_reasons, count);
    
    hrast_dbg_print(4, SYSLOG | CONSOLE, 
        "hrast_rx_receive_rstp_cb - pkt_data: %p, len: %d, rx_unit: %d, rx_port: %d, rx_reasons: %d, rx_reason: %d, rx_reasons count: %d", 
        pkt_data, len, pkt->rx_unit, pkt->rx_port, pkt->rx_reasons, pkt->rx_reason, count);
    if (reason == 0) {
        hrast_dbg_print(1, SYSLOG | CONSOLE, "not a protocol packet");
        return BCM_RX_NOT_HANDLED;
    }
    
    if ((pkt_data[21] != 0) || (pkt_data[22] != 0)) {
        hrast_dbg_print(1, SYSLOG | CONSOLE, "stp protocol identifier not correct: %x %x", pkt_data[21], pkt_data[22]);
        return BCM_RX_NOT_HANDLED;
    }

	struct bcm_msgbuf msg_buf;
	struct bcm_rstpbuf rstp_buf;
	int rv;

	memset(&msg_buf, 0, sizeof(struct bcm_msgbuf));
	msg_buf.mcmd = HRAST_RX_CALLBACK;
	msg_buf.mtype = HRAST_MESSAGE_FROM_BCM_E;
	msg_buf.mparam[0] = pkt->rx_unit;
	msg_buf.mparam[1] = pkt->rx_port;
	msg_buf.mparam[2] = pkt->pkt_len;

	if (memcpy(rstp_buf.mdata, pkt_data, pkt->pkt_len) != rstp_buf.mdata) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "memcpy error at receive callback");
		return BCM_RX_NOT_HANDLED;
	}
	
	for (i = 0; i < MAX_CALLBACKS; i++) {
		
		/* send callback data to all registered clients */
		if (rx_Callback[i].c_pid != 0) {
			msg_buf.lnx_mtype = rx_Callback[i].c_pid;
			msg_buf.ext_msg = (struct bcm_extbuf*)&rstp_buf;

			/* send message */
			rv = hrast_sys_msg_send_rstp(rxqid, &msg_buf);
			if (rv == -1) {
				hrast_dbg_print(1, SYSLOG, "error sending RX callback (idx=%d)", i);
				continue;
			}
			hrast_dbg_print(2, SYSLOG, "RX callback (idx=%d) sent to queue: %d", i, cbqid);
		}
	}
	return BCM_RX_HANDLED;
}

static void *hrast_rx_keep_alive(void* none)
{
	int res;
  int i, rv, unit = 0;
  int num_cleared;
  pid_t c_pid;

	res = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if (res != 0) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "pthread_setcancelstate error - %s (tid=%d)",
		strerror(errno),
		pthread_self());
		exit(EXIT_FAILURE);
	}
	res = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	if (res != 0) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "pthread_setcanceltype error - %s (tid=%d)",
		strerror(errno),
		pthread_self());
		exit(EXIT_FAILURE);
	}

	while (1) {
		if (reg_rx_callbacks > 0) {
			for (i = 0; i < MAX_CALLBACKS; i++) {
				if (rx_Callback[i].c_pid != 0) {
					c_pid = rx_Callback[i].c_pid;
					/* check connectivity to hrast_api callback thread */
					rv = hrast_rx_connectivity_check(rxqid, c_pid);
					
					if (rv != 0) {
						reg_rx_callbacks--;
						hrast_dbg_print(0, SYSLOG | CONSOLE, "Unaccessible RX callback removed (pid=%d), %d remaining registered",
										c_pid,
										reg_rx_callbacks);
						rx_Callback[i].c_pid = 0;

						/* client callback thread not accessible, unregister callback */
                	   if (reg_rx_callbacks == 0) {
        	           /* unregister hrast callback at BCM API */

	                        rv = bcm_rx_unregister(unit, hrast_rx_receive_rstp_cb, 100);
	                        rx_time = 0;
                            hrast_dbg_print(2, SYSLOG | CONSOLE, "bcm_rx_unregister(%d,&CB_func) - \"%s\"",
                                            unit,
                                            bcm_errmsg(rv));
                        }
						
						/*clear sent messages from callback queue */
						hrast_dbg_print(2, SYSLOG | CONSOLE, "clear message from RX callback queue");
						num_cleared = 0;
						do {
							rv = hrast_rx_pull_message(rxqid, c_pid);
							if (rv == 0)
								num_cleared++;
						} while (rv == 0);
						if (num_cleared > 0)
							hrast_dbg_print(2, SYSLOG | CONSOLE, "%d messages pulled from RX callback queue", num_cleared);
							
						hrast_dbg_callbacks_print_rstp();
					} else 
						hrast_dbg_print(2, SYSLOG | CONSOLE, "RX callback thread %d accessible (pid=%d)",
							i, Callback[i].c_pid);
				}
			}
		}
		usleep(KEEP_ALIVE_PERIOD_RX);
	}
}

static int hrast_rx_connectivity_check(int qid, int pid)
{
	struct bcm_msgbuf msg_buf;

	memset(&msg_buf, 0, sizeof(struct bcm_msgbuf));
	msg_buf.lnx_mtype = pid;
	msg_buf.mcmd = HRAST_RX_ECHO;
	msg_buf.mtype = HRAST_MESSAGE_FROM_BCM;
	
	/* send echo request to registered thread */
	if (hrast_sys_msg_send(qid, &msg_buf) == -1) {
		hrast_dbg_print(2, SYSLOG | CONSOLE, "error sending RX echo request (pid=%d)", pid);
		return -1;
	}

	hrast_dbg_print(2, SYSLOG | CONSOLE, "RX echo request sent to client (pid=%d)", pid);
	
	/* wait for echo response from registered thread */
	msg_buf.lnx_mtype = HRAST_MSG_TYPE;
	if (hrast_sys_msg_receive(qid, HRAST_LONG_TIMEOUT, &msg_buf, NULL) != 0) {
		hrast_dbg_print(1, SYSLOG, "error receiving RX echo response (pid=%d)", msg_buf.mpid);
		return -1;
	}

	hrast_dbg_print(2, SYSLOG | CONSOLE, "echo RX response received (pid=%d)", msg_buf.mpid);
	
	if (msg_buf.mstatus != HRAST_MESSAGE_CMD_OK)
		hrast_dbg_print(2, SYSLOG | CONSOLE, "echo error (pid=%d)", pid);
		
	return 0;
}

static int hrast_rx_pull_message(int qid, int pid)
{
	struct bcm_msgbuf msg_buf;
	struct bcm_extbuf ext_buf;

	memset(&msg_buf, 0, sizeof(struct bcm_msgbuf));
	msg_buf.lnx_mtype = pid;
	
	/* pull one message */
	if (hrast_sys_msg_receive(qid, HRAST_SHORT_TIMEOUT, &msg_buf, &ext_buf) != 0)
		return -1;
			
	return 0;
}

static int hrast_rx_register_rstp(struct bcm_msgbuf *bcm_msg)
{
	hrast_dbg_print(1, SYSLOG | CONSOLE, "hrast_rx_register_rstp");
	int status, i, empty_idx = MAX_CALLBACKS;

	if (bcm_msg == NULL)
		return -1;

	/* check if rx initialized */
	if (reg_rx_callbacks < 0) {
			hrast_dbg_print(1, SYSLOG | CONSOLE, "RX not initialized");
			status = BCM_E_INIT;
			goto end_rx_register;
	}
	/* check if c_pid already registered (only one registration per client allowed) */
	for (i = 0; i < MAX_CALLBACKS; i++) {
		if (bcm_msg->mpid == rx_Callback[i].c_pid) {
			hrast_dbg_print(1, SYSLOG | CONSOLE, "duplicated callback registration");
			return -1;
		}
	}

	/* find epmty index in Callback array if there is any */
	for (i = 0; i < MAX_CALLBACKS; i++) {
		if (rx_Callback[i].c_pid == 0) {
			empty_idx = i;
			break;	
		}
	}
	if (empty_idx == MAX_CALLBACKS) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "max number of callbacks exceeded");
		status = BCM_E_MEMORY;
		goto end_rx_register;
	}

	int unit;
	unit = bcm_msg->mparam[0];

	status = bcm_rx_active(unit);

	// start rx thread
	if (status == 0) {
		status = bcm_rx_start(unit, NULL);
        if (status != BCM_E_NONE) {
            hrast_dbg_print(1, SYSLOG | CONSOLE, "Error RX start: %d - %s", status, bcm_errmsg(status));
		    goto end_rx_register;
        }
	}

	status = bcm_rx_register(unit, "rx_rstp_cb", hrast_rx_receive_rstp_cb, 100, NULL, BCM_RCO_F_ALL_COS); 
	if (status != BCM_E_NONE) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "Error RX register: %d - %s", 
			status, bcm_errmsg(status));
		goto end_rx_register;
	}

end_rx_register:
	bcm_msg->bcm_rv = status;
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "hrast: callback registration error: %d\n", status);
	else {
		/* save callback info */
		rx_Callback[empty_idx].c_pid = bcm_msg->mpid;
		reg_rx_callbacks++;
		hrast_dbg_print(0, SYSLOG | CONSOLE, "RX Callback added (pid=%d), altogether %d registered",
					rx_Callback[empty_idx].c_pid,
					reg_rx_callbacks);
	}

	return 0;
}

static int hrast_rx_unregister_rstp(struct bcm_msgbuf *bcm_msg)
{
	hrast_dbg_print(1, SYSLOG | CONSOLE, ">>>>>>>>>>>> hrast_rx_unregister_rstp");
	if (bcm_msg == NULL)
        return -1;

    int unit, status;
    unit = bcm_msg->mparam[0];
    
    status = bcm_rx_stop(unit, NULL);
    if (status != BCM_E_NONE) {
        hrast_dbg_print(1, SYSLOG | CONSOLE, "Error RX stop: %d - %s",
                status, bcm_errmsg(status));
    }

	status = bcm_rx_unregister(unit, hrast_rx_receive_rstp_cb, 100);
    if (status != BCM_E_NONE) {
        hrast_dbg_print(1, SYSLOG | CONSOLE, "Error RX unregister: %d - %s",
                status, bcm_errmsg(status));
    }

/*        rx_Callback[0].c_pid = 0;*/

    bcm_msg->bcm_rv = status;
    return 0;
}

static int hrast_port_stp_set(struct bcm_msgbuf *bcm_msg)
{
	if (bcm_msg == NULL)
		return -1;

	int unit, port, state, status;
	unit = bcm_msg->mparam[0];
	port = bcm_msg->mparam[1];
	state = bcm_msg->mparam[2];

	hrast_dbg_print(4, SYSLOG | CONSOLE, "hrast_port_stp_state: unit: %d, port: %d, state: %d", unit, port, state);

	status = bcm_port_stp_set(unit, port, state);
	if (status != BCM_E_NONE) {
                hrast_dbg_print(1, SYSLOG | CONSOLE, "Error stp port set: %d - %s",
                        status, bcm_errmsg(status));
        }

	bcm_msg->bcm_rv = status;
	return 0;
}

static hrast_l2_addr_delete_by_port(struct bcm_msgbuf *bcm_msg)
{
    if (bcm_msg == NULL)
		return -1;

	int unit, port, status;
	unit = bcm_msg->mparam[0];
	port = bcm_msg->mparam[1];

	hrast_dbg_print(4, SYSLOG | CONSOLE, "hrast_l2_addr_delete_by_port: unit: %d, port: %d", unit, port);

	status = bcm_l2_addr_delete_by_port(unit, -1, port, BCM_L2_DELETE_NO_CALLBACKS);
	if (status != BCM_E_NONE) {
        hrast_dbg_print(1, SYSLOG | CONSOLE, "Error l2 address delete by port: %d - %s",
                        status, bcm_errmsg(status));
    }

	bcm_msg->bcm_rv = status;
	return 0;
}
#endif /* __RSTP__ */

/*
* description:
*   Print if global hrast_dbg.level is greater or equal to current level
*
*   level: 0 - print unconditionally
*          1 - errors
*          2 - function calls
*          3 - function results
*          4 - messages
*   flags  SYSLOG
*          CONSOLE
*/
static void hrast_dbg_print(int level, int flags, char *format, ...)
{
	va_list arg_ptr;
	char output[250];

	if (format == NULL)
		return;
	
	if ((hrast_dbg.level >= level) || (level <= 2))
	{
		va_start(arg_ptr, format);
		vsprintf(output, format, arg_ptr);
		va_end(arg_ptr);

		if (hrast_dbg.level >= level) {
			if (((SYSLOG & flags) != 0 ) && (hrast_dbg.syslog_on))
				syslog( LOG_ERR, "%s: %s", MODULE_NAME, output);
	
			if (((CONSOLE & flags) != 0 ) && (hrast_dbg.console_on))
				printf(" %s: %s\n", MODULE_NAME, output);
		}
		if (level <= 2)
			soc_cm_debug(DK_PORT, "%s: %s\n", MODULE_NAME, output);
		
		fflush(stdout);
	}
	
} 


/* nice port info printout */
static void hrast_dbg_port_info_print(bcm_port_info_t *info)
{
	unsigned int mask;
	char str[150];
	
	mask = info->action_mask;
	
	hrast_dbg_print(3, SYSLOG | CONSOLE, "port info:");
	
	hrast_dbg_print(3, SYSLOG | CONSOLE, "  action_mask         : 0x%08x", mask);

	if (mask & BCM_PORT_ATTR_ENABLE_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  enable              : %s",
			(info->enable == TRUE) ? "TRUE" : "FALSE");

	if (mask & BCM_PORT_ATTR_LINKSTAT_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  linkstatus          : %s",
			(info->linkstatus == TRUE) ? "up" : "down");

	if (mask & BCM_PORT_ATTR_AUTONEG_MASK) 
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  autoneg             : %s",
			(info->autoneg == TRUE) ? "enabled" : "disabled");

	if (mask & BCM_PORT_ATTR_SPEED_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  speed               : %d Mb/s", info->speed);

	if (mask & BCM_PORT_ATTR_DUPLEX_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  duplex              : %s",
			(info->duplex == BCM_PORT_DUPLEX_FULL) ? "full" :
			((info->duplex == BCM_PORT_DUPLEX_HALF) ? "half" : "N/A"));

	if (mask & BCM_PORT_ATTR_LINKSCAN_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  linkscan            : %s",
			(info->linkscan == BCM_LINKSCAN_MODE_HW) ? "HW" :
			((info->linkscan == BCM_LINKSCAN_MODE_SW) ? "SW" :
			((info->linkscan == BCM_LINKSCAN_MODE_NONE) ? "none" : "N/A")));

	if (mask & BCM_PORT_ATTR_LEARN_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  learn               : %s%s%s",
			(info->learn & BCM_PORT_LEARN_ARL) ? "ARL " : "",
			(info->learn & BCM_PORT_LEARN_CPU) ? "CPU " : "",
			(info->learn & BCM_PORT_LEARN_FWD) ? "FWD" : "");

	if (mask & BCM_PORT_ATTR_DISCARD_MASK) 
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  discard             : %s",
			(info->discard == BCM_PORT_DISCARD_TAG) ? "all tagged" :
			((info->discard == BCM_PORT_DISCARD_UNTAG) ? "all untagged" : 
			((info->discard == BCM_PORT_DISCARD_ALL) ? "all" :
			((info->discard == BCM_PORT_DISCARD_NONE) ? "none" : "N/A"))));

	if (mask & BCM_PORT_ATTR_VLANFILTER_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  vlanfilter          : %s%s%s",
			(info->vlanfilter & BCM_PORT_VLAN_MEMBER_INGRESS) ? "ingrees " : "",
			(info->vlanfilter & BCM_PORT_VLAN_MEMBER_EGRESS) ? "egress " : "",
			(info->vlanfilter & (BCM_PORT_VLAN_MEMBER_INGRESS | BCM_PORT_VLAN_MEMBER_EGRESS)) ? "" : "none");

	if (mask & BCM_PORT_ATTR_UNTAG_PRI_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  untagged_priority   : %d",	info->untagged_priority);

	if (mask & BCM_PORT_ATTR_UNTAG_VLAN_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  untagged_vlan       : %d", info->untagged_vlan);

	if (mask & BCM_PORT_ATTR_STP_STATE_MASK) 
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  stp_state           : %s",
			(info->stp_state == BCM_STG_STP_FORWARD) ? "forwarding" :
			((info->stp_state == BCM_STG_STP_LEARN) ? "learning" :
			((info->stp_state == BCM_STG_STP_LISTEN) ? "listening" : 
			((info->stp_state == BCM_STG_STP_BLOCK) ? "blocking" : 
			((info->stp_state == BCM_STG_STP_DISABLE) ? "disabled" : "N/A")))));

	if (mask & BCM_PORT_ATTR_PFM_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  pfm                 : %s",
			(info->pfm == BCM_PORT_MCAST_FLOOD_NONE) ? "flood none" :
			((info->pfm == BCM_PORT_MCAST_FLOOD_UNKNOWN) ? "flood unknown" :
			((info->pfm == BCM_PORT_MCAST_FLOOD_ALL) ? "flood all" : "N/A")));

	if (mask & BCM_PORT_ATTR_LOOPBACK_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  loopback            : %s",
			(info->loopback == BCM_PORT_LOOPBACK_PHY) ? "PHY" :
			((info->loopback == BCM_PORT_LOOPBACK_MAC) ? "MAC" :
			((info->loopback == BCM_PORT_LOOPBACK_NONE) ? "none" : "N/A")));

	if (mask & BCM_PORT_ATTR_PHY_MASTER_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  phy_master          : %s",
			(info->phy_master == BCM_PORT_MS_NONE) ? "none" :
			((info->phy_master == BCM_PORT_MS_AUTO) ? "auto" : 
			((info->phy_master == BCM_PORT_MS_MASTER) ? "master" :
			((info->phy_master == BCM_PORT_MS_SLAVE) ? "slave" : "N/A"))));

	if (mask & BCM_PORT_ATTR_INTERFACE_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  interface           : %s",
			(info->interface == BCM_PORT_IF_RGMII) ? "RGMII" :
			((info->interface == BCM_PORT_IF_XGMII) ? "XGMII" :
			((info->interface == BCM_PORT_IF_TBI) ? "TBI" : 
			((info->interface == BCM_PORT_IF_SGMII) ? "SGMII" : 
			((info->interface == BCM_PORT_IF_GMII) ? "GMII" :
			((info->interface == BCM_PORT_IF_MII) ? "MII" :
			((info->interface == BCM_PORT_IF_NULL) ? "NULL" :
			((info->interface == BCM_PORT_IF_NOCXN) ? "NOCXN" : "N/A"))))))));

	if (mask & BCM_PORT_ATTR_PAUSE_TX_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  pause_tx            : %s",
			(info->pause_tx == TRUE) ? "enabled" : "disabled");

	if (mask & BCM_PORT_ATTR_PAUSE_RX_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  pause_rx            : %s",
			(info->pause_rx == TRUE) ? "enabled" : "disabled");

	if (mask & BCM_PORT_ATTR_ENCAP_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  encap_mode          : %s",
			(info->encap_mode == BCM_PORT_ENCAP_B5632) ? "Broadcom BCM5632 format" :
			((info->encap_mode == BCM_PORT_ENCAP_HIGIG) ? "Broadcom HiGig format" :
			((info->encap_mode == BCM_PORT_ENCAP_IEEE) ? "Standard IEEE" : "N/A")));

	if (mask & BCM_PORT_ATTR_PAUSE_MAC_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  pause_mac           : %02x:%02x:%02x:%02x:%02x:%02x",
			info->pause_mac[0], info->pause_mac[1], info->pause_mac[2],
			info->pause_mac[3], info->pause_mac[4], info->pause_mac[5]);

	if (mask & BCM_PORT_ATTR_LOCAL_ADVERT_MASK) {
		hrast_dbg_ability_get(info->local_advert, str);
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  local_advert        : %s", str);
		
	}
	if (mask & BCM_PORT_ATTR_REMOTE_ADVERT_MASK) {
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  remote_advert_valid : %s",
			(info->remote_advert_valid == TRUE) ? "TRUE" : "FALSE");
		if (info->remote_advert_valid == TRUE) {
			hrast_dbg_ability_get(info->remote_advert, str);
			hrast_dbg_print(3, SYSLOG | CONSOLE, "  remote_advert       : %s", str);
		}
	}
	if (mask & BCM_PORT_ATTR_RATE_MCAST_MASK) {
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  mcast_limit_enable  : %s",
			(info->mcast_limit_enable == BCM_RATE_MCAST) ? "TRUE" : "FALSE");
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  mcast_limit         : %d pps", info->mcast_limit);
	}
	if (mask & BCM_PORT_ATTR_RATE_BCAST_MASK) {
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  bcast_limit_enable  : %s",
			(info->bcast_limit_enable == BCM_RATE_BCAST) ? "TRUE" : "FALSE");
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  bcast_limit         : %d pps", info->bcast_limit);
	}
	if (mask & BCM_PORT_ATTR_RATE_DLFBC_MASK) {
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  dlfbc_limit_enable  : %s",
			(info->dlfbc_limit_enable == BCM_RATE_DLF) ? "TRUE" : "FALSE");
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  dlfbc_limit         : %d pps", info->dlfbc_limit);
	}
	if (mask & BCM_PORT_ATTR_SPEED_MAX_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  speed_max           : %d Mb/s", info->speed_max);

	if (mask & BCM_PORT_ATTR_ABILITY_MASK) {
		hrast_dbg_ability_get(info->ability, str);
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  ability             : %s", str);		
	}
	if (mask & BCM_PORT_ATTR_FRAME_MAX_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  frame_max           : %d Bytes", info->frame_max);

	if (mask & BCM_PORT_ATTR_MDIX_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  mdix                : %s",
			(info->mdix == BCM_PORT_MDIX_XOVER) ? "force crossed over" :
			((info->mdix == BCM_PORT_MDIX_NORMAL) ? "force normal" : 
			((info->mdix == BCM_PORT_MDIX_FORCE_AUTO) ? "auto always" :
			((info->mdix == BCM_PORT_MDIX_AUTO) ? "auto only when autonegotiation" : "N/A"))));

	if (mask & BCM_PORT_ATTR_MDIX_STATUS_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  mdix_status         : %s",
			(info->mdix_status == BCM_PORT_MDIX_STATUS_XOVER) ? "crossed over" :
			((info->mdix_status == BCM_PORT_MDIX_STATUS_NORMAL) ? "normal" : "N/A"));

	if (mask & BCM_PORT_ATTR_MEDIUM_MASK)			
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  medium              : %s",
			(info->medium == BCM_PORT_MEDIUM_FIBER) ? "fiber" :
			((info->medium == BCM_PORT_MEDIUM_COPPER) ? "copper" : 
			((info->medium == BCM_PORT_MEDIUM_NONE) ? "none" : "N/A")));

	if (mask & BCM_PORT_ATTR_FAULT_MASK)
		hrast_dbg_print(3, SYSLOG | CONSOLE, "  fault               : %s%s",
			(info->learn & BCM_PORT_FAULT_LOCAL) ? "local " : "",
			(info->learn & BCM_PORT_FAULT_REMOTE) ? "remote " : "");

	return;
}

/* print BCM switch initialization status by modules */
static void hrast_dbg_switch_init_print(int bcm_rv)
{
	int i, initialized;
	char *_bcm_module_names[] = BCM_MODULE_NAMES_INITIALIZER;
	
	hrast_dbg_print(3, CONSOLE, "Switch initialization status:");
	for (i = 0; i < BCM_MODULE__COUNT; i++) {
		if ((BCM_INITIALIZED_MODULES_MASK & (1 << i)) != 0) {
			if ((bcm_rv & (1 << i)) != 0)
				initialized = TRUE;
			else {
				initialized = FALSE;
			}
			hrast_dbg_print(3, CONSOLE, "  %10s : %s",
				_bcm_module_names[i],
				initialized ? "initialized" : "not initialized" );
		}
	}
}

static void hrast_dbg_ability_print(bcm_port_abil_t ability_mask)
{
	char str[150];

	hrast_dbg_ability_get(ability_mask, str);
	
	hrast_dbg_print(3, SYSLOG | CONSOLE, "port ability: %s", str);
	
	return;
}

/* print message header */
static void hrast_dbg_msg_print(char* str, int qid, struct bcm_msgbuf *msgbuf)
{
	hrast_dbg_print(4, SYSLOG | CONSOLE, "%s[%d]: lnx_mtype=0x%08x mtype=0x%08x mpid=%d seq=%d mcmd=%d bcm_rv=%d mstatus=%d", 
					str,
					qid,
					msgbuf->lnx_mtype,
					msgbuf->mtype,
					msgbuf->mpid,
					msgbuf->seq,
					msgbuf->mcmd,
					msgbuf->bcm_rv,
					msgbuf->mstatus);
}

/* print extended messages header */
static void hrast_dbg_ext_msg_print(char* str, int qid, struct bcm_extbuf *extbuf)
{
	hrast_dbg_print(4, SYSLOG | CONSOLE, "%s[%d]: lnx_mtype=0x%08x", 
					str,
					qid,
					extbuf->lnx_mtype);
}

/* form port ability string from ability mask */
static void hrast_dbg_ability_get(bcm_port_abil_t ability_mask, char *str)
{
	sprintf(str, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		(ability_mask & BCM_PORT_ABIL_10MB_HD) ? "10MB-HD " : "",
		(ability_mask & BCM_PORT_ABIL_10MB_FD) ? "10MB-FD " : "",
		(ability_mask & BCM_PORT_ABIL_100MB_HD) ? "100MB-HD " : "",
		(ability_mask & BCM_PORT_ABIL_100MB_FD) ? "100MB-FD " : "",
		(ability_mask & BCM_PORT_ABIL_1000MB_HD) ? "1000MB-HD " : "",
		(ability_mask & BCM_PORT_ABIL_1000MB_FD) ? "1000MB-FD " : "",
		(ability_mask & BCM_PORT_ABIL_10GB_HD) ? "10GB-HD " : "",
		(ability_mask & BCM_PORT_ABIL_10GB_FD) ? "10GB-FD " : "",
		(ability_mask & BCM_PORT_ABIL_TBI) ? "TBI " : "",
		(ability_mask & BCM_PORT_ABIL_MII) ? "MII " : "",
		(ability_mask & BCM_PORT_ABIL_GMII) ? "GMII " : "",
		(ability_mask & BCM_PORT_ABIL_SGMII) ? "SGMII " : "",
		(ability_mask & BCM_PORT_ABIL_XGMII) ? "XGMII " : "",
		(ability_mask & BCM_PORT_ABIL_LB_MAC) ? "LB-MAC " : "",
		(ability_mask & BCM_PORT_ABIL_LB_PHY) ? "LB_PHY " : "",
		(ability_mask & BCM_PORT_ABIL_PAUSE_TX) ? "PAUSE-TX " : "",
		(ability_mask & BCM_PORT_ABIL_PAUSE_RX) ? "PAUSE_RX " : "");
	
	return;
}

static void hrast_dbg_callbacks_print(void)
{
	int i;
	
	hrast_dbg_print(3, SYSLOG | CONSOLE, "registered callbacks: %d", reg_callbacks);
	if (reg_callbacks > 0) {
		hrast_dbg_print(3, SYSLOG | CONSOLE, "idx    pid");
		for (i = 0; i < MAX_CALLBACKS; i++) {
			if (Callback[i].c_pid != 0)
				hrast_dbg_print(3, SYSLOG | CONSOLE, " %2d   %d", i, Callback[i].c_pid);
		}
	}
}

static void hrast_dbg_callbacks_print_rstp(void)
{
	int i;
	
	hrast_dbg_print(3, SYSLOG | CONSOLE, "registered callbacks: %d", reg_rx_callbacks);
	if (reg_rx_callbacks > 0) {
		hrast_dbg_print(3, SYSLOG | CONSOLE, "idx    pid");
		for (i = 0; i < MAX_CALLBACKS; i++) {
			if (rx_Callback[i].c_pid != 0)
				hrast_dbg_print(3, SYSLOG | CONSOLE, " %2d   %d", i, rx_Callback[i].c_pid);
		}
	}
}

/* local functions for manipulating with messages */

/*
Description
Common function for sending messages to message queue

Input data
msqid 				- message queue id
bcm_msg->ext_msg 	- pointer to extended buffer (in case of HRAST_EXTENDED_MESSAGE, otherwise NULL)
bcm_msg->lnx_mtype 	- sending process id
bcm_msg->mtype		- message type

Return: -1 on error
		0 on on success
*/ 

static int hrast_sys_msg_send(int msqid, struct bcm_msgbuf *bcm_msg)
{
	struct bcm_extbuf *ext_buf;
	static unsigned int seq = 1;
	
    if (bcm_msg == NULL) {
        hrast_dbg_print(1, SYSLOG | CONSOLE, "hrast_sys_msg_send: no message buffer");
		return -1;	
	}
		
	ext_buf = bcm_msg->ext_msg;
	bcm_msg->mpid = getpid();
	
	/* increase and set sequence number if new message is sending, otherwise take received sequence number */
	if (bcm_msg->mtype & HRAST_MESSAGE) {
		seq++;
		bcm_msg->seq = seq;
	}
	bcm_msg->ext_msg = NULL;

/*    hrast_dbg_print(1, SYSLOG | CONSOLE, "hrast_sys_msg_send: HRAST_MESSAGE\n");*/
	if (msgsnd(msqid, bcm_msg, sizeof(struct bcm_msgbuf), 0) == -1) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "msgsnd error - %s", strerror(errno));
		return -1;
	}
	hrast_dbg_msg_print("SEND", msqid, bcm_msg);
	
	if (bcm_msg->mtype & HRAST_EXTENDED_MESSAGE) {
		if (ext_buf == NULL) {
			hrast_dbg_print(1, SYSLOG | CONSOLE, "hrast_sys_msg_send: no extended buffer");
			return -1;
		}
		
		/* send extended message */
		ext_buf->lnx_mtype = bcm_msg->lnx_mtype;
	
		if (msgsnd(msqid, ext_buf, sizeof(struct bcm_extbuf), 0) == -1) {
			hrast_dbg_print(1, SYSLOG | CONSOLE, "msgsnd error - %s", strerror(errno));
			return -1;
		}
		hrast_dbg_ext_msg_print("SEND", msqid, ext_buf);
	}
	
	return 0;
}

#ifdef __RSTP__
static int hrast_sys_msg_send_rstp(int msqid, struct bcm_msgbuf *bcm_msg)
{
    struct bcm_rstpbuf *rstp_buf;
	static unsigned int seq_rstp = 1;
	
    if (bcm_msg == NULL) {
		hrast_dbg_print(4, SYSLOG | CONSOLE, "hrast_sys_msg_send_rstp: no message buffer");
		return -1;	
	}
		
	rstp_buf = (struct bcm_rstpbuf*)bcm_msg->ext_msg;
	bcm_msg->mpid = getpid();
	
	/* increase and set sequence number if new message is sending, otherwise take received sequence number */
	if (bcm_msg->mtype & HRAST_MESSAGE) {
		seq_rstp++;
		bcm_msg->seq = seq_rstp;
	}
	bcm_msg->ext_msg = NULL;

	if (msgsnd(msqid, bcm_msg, sizeof(struct bcm_msgbuf), 0) == -1) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "msgsnd error - %s", strerror(errno));
		return -1;
	}
	
	hrast_dbg_msg_print("SEND", msqid, bcm_msg);
	
	
	if (bcm_msg->mtype & HRAST_EXTENDED_MESSAGE) {
		
		if (rstp_buf == NULL) {
			hrast_dbg_print(1, SYSLOG | CONSOLE, "hrast_sys_msg_send: no extended buffer");
			return -1;
		}
		
		/* send extended message */
		rstp_buf->lnx_mtype = bcm_msg->lnx_mtype;
	
		if (msgsnd(msqid, rstp_buf, sizeof(struct bcm_rstpbuf), 0) == -1) {
			hrast_dbg_print(1, SYSLOG | CONSOLE, "msgsnd error - %s", strerror(errno));
			return -1;
		}
/*		hrast_dbg_ext_msg_print("SEND", msqid, _buf);*/
	}
    return 0;
}
#endif /* __RSTP__ */

/*
Description
Common function for receiving messages from message queue

Input data
msqid 			- message queue id
m_timout		- waiting for message timeout 
buf->mcmd 		- expected command (in case of CONFIRM message)
buf->seq		- expected sequence (in case of CONFIRM message)
ext_buf			- pointer to extended message (NULL if message is not extended)

Return: -1 on error
		0 on on success
*/ 
static int hrast_sys_msg_receive(int msqid, int m_timout,
						struct bcm_msgbuf *buf, struct bcm_extbuf *ext_buf)
{
	unsigned int mcmd, seq, lnx_mtype, msg_arrived = FALSE;
	
	if (buf == NULL) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "hrast_sys_msg_receive: no message buffer");
		return -1;
	}
	
	lnx_mtype = buf->lnx_mtype;
	mcmd = buf->mcmd;
	seq = buf->seq;
	
	/* clear data buffer */
	memset(buf, 0, sizeof(struct bcm_msgbuf));

	do {
		/* wait for message to arrive */
		if (msgrcv(msqid, buf, sizeof(struct bcm_msgbuf), lnx_mtype, m_timout ? IPC_NOWAIT : 0) == -1) {
			if (errno != ENOMSG) { /* message received with error */
				hrast_dbg_print(1, SYSLOG | CONSOLE, "msgrcv error msg- %s", strerror(errno));
				return -1;
			}
			else { /* no message received */
				m_timout--;
				usleep(1);                
			}
		}
		else { /* message arrived - check it */
			msg_arrived = TRUE;
			
			if (buf->mtype & HRAST_CONFIRM) { /* if CONFIRM message, check it */			
				if ((buf->seq == seq) && (buf->mcmd == mcmd)) { /* sequence, command  */
					hrast_dbg_msg_print("RECV", msqid, buf);
					break; /* break loop */
				}
				else { /* throw message away and wait for next message */
					hrast_dbg_msg_print("RECV&THROW_AWAY", msqid, buf);
					if (m_timout > 0)								/* in case of HRAST_SHORT/LONG_TIMEOUT */
						memset(buf, 0, sizeof(struct bcm_msgbuf));	/* clear buffer and continue waiting until timed out */ 
					else											/* in case of HRAST_WAIT_FOREWER */
						return -1;									/* exit */
				}
			}
			else {	/* if not CONFIRM message, just take it */
				hrast_dbg_msg_print("RECV", msqid, buf);
				break; /* break loop */
			}
		}
	} while( m_timout > 0);
	
	if  (buf->mtype & HRAST_EXTENDED_MESSAGE) {
		
		if (ext_buf == NULL) {
			hrast_dbg_print(1, SYSLOG | CONSOLE, "hrast_sys_msg_receive: no extended buffer");
			return -1;
		}
		
		/* clear extended data buffer */
		memset(ext_buf, 0, sizeof(struct bcm_extbuf));

		do {			
			/* wait for extended message to arrive */
			if (msgrcv(msqid, ext_buf, sizeof(struct bcm_extbuf), lnx_mtype, m_timout ? IPC_NOWAIT : 0) == -1) {
				if (errno != ENOMSG) { /* message received with error */
					hrast_dbg_print(1, SYSLOG | CONSOLE, "msgrcv error ext - %s", strerror(errno));
					return -1;
				}
				else { /* no message received */
					m_timout--;
					usleep(1);                
				}
			}
			else {
				buf->ext_msg = ext_buf;
				hrast_dbg_ext_msg_print("RECV", msqid, ext_buf);
				break; /* break loop */
			}
		} while( m_timout > 0);
	}
	else
		buf->ext_msg = NULL;
	
	/* check for timeout */
	if ((msg_arrived == FALSE) && (m_timout == 0)) {
		hrast_dbg_print(1, SYSLOG | CONSOLE, "RECV timeout", buf);
		return -1;
	}

	return 0;
}

/* 
* Function
*      hrast_mirror_init
* Input data
*      bcm_msg->mparam[0] - unit number
* Output
*
* Return
*      -1 on error
*		0 on on success
*/ 
static int hrast_mirror_init(struct bcm_msgbuf *bcm_msg)
{
	int status;

	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_mirror_init(bcm_msg->mparam[0]);
	/* int bcm_mirror_init(int unit); */
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "BCMmirror: hrast_mirror_init(%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "BCMmirror: Error initializing BCM mirror\n");

	return 0;
}

/* 
* Function
*      hrast_mirror_mode_get
* Input data
*      bcm_msg->mparam[0] - unit number
* Output
*      bcm_msg->mparam[2] = mode;
* Return
*      -1 on error
*		0 on on success
*/ 
static int hrast_mirror_mode_get(struct bcm_msgbuf *bcm_msg)
{	
	int status, mode;

	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_mirror_mode_get(bcm_msg->mparam[0], &mode);
	/* int bcm_mirror_mode_get(int unit, int *mode); */
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "BCMmirror: bcm_mirror_mode_get(%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					mode,
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "BCMmirror: Error mirror mode get\n");
	else
		bcm_msg->mparam[2] = mode;
		
	return 0;
}

/* 
* Function
*      hrast_mirror_mode_set
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - mode
* Output
*
* Return
*      -1 on error
*		0 on on success
*/ 
static int hrast_mirror_mode_set(struct bcm_msgbuf *bcm_msg)
{	
	int status;

	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_mirror_mode_set(bcm_msg->mparam[0], bcm_msg->mparam[1]);
	/* int bcm_mirror_mode_set(int unit, int mode); */
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "BCMmirror: bcm_mirror_mode_set(%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "BCMmirror: Error mirror mode set\n");
		
	return 0;
}

/* 
* Function
*      hrast_mirror_to_get
* Input data
*      bcm_msg->mparam[0] - unit number
* Output
*      bcm_msg->mparam[2] - port
* Return
*      -1 on error
*		0 on on success
*/ 
static int hrast_mirror_to_get(struct bcm_msgbuf *bcm_msg)
{	
	int status;
	bcm_port_t port;

	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_mirror_to_get(bcm_msg->mparam[0], &port);
	/* int bcm_mirror_to_get(int unit, bcm_port_t *port); */
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "BCMmirror: bcm_mirror_to_get(%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					port,
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "BCMmirror: Error mirror-to port get\n");
	else
		bcm_msg->mparam[2] = port;
		
	return 0;
}

/* 
* Function
*      hrast_mirror_to_set
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
* Output
*
* Return
*      -1 on error
*		0 on on success
*/ 
static int hrast_mirror_to_set(struct bcm_msgbuf *bcm_msg)
{	
	int status;

	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_mirror_to_set(bcm_msg->mparam[0], bcm_msg->mparam[1]);
	/* int bcm_mirror_to_set(int unit, bcm_port_t *port); */
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "BCMmirror: bcm_mirror_to_set(%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "BCMmirror: Error mirror-to port set\n");
		
	return 0;
}

/* 
* Function
*      hrast_mirror_ingress_get
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
* Output
*      bcm_msg->mparam[2] - enable/disable flag    
* Return
*      -1 on error
*		0 on on success
*/ 
static int hrast_mirror_ingress_get(struct bcm_msgbuf *bcm_msg)
{	
	int status;
	int val;

	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_mirror_ingress_get(bcm_msg->mparam[0], bcm_msg->mparam[1], &val);
	/* int bcm_mirror_ingress_get(int unit, bcm_port_t port, int *val); */
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "BCMmirror: bcm_mirror_ingress_get(%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					val,
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "BCMmirror: Error mirror port ingress get\n");
	else
		bcm_msg->mparam[2] = val;
		
	return 0;
}

/* 
* Function
*      hrast_mirror_ingress_set
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
*      bcm_msg->mparam[2] - enable/disable flag 
* Output
*         
* Return
*      -1 on error
*		0 on on success
*/ 
static int hrast_mirror_ingress_set(struct bcm_msgbuf *bcm_msg)
{	
	int status;

	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_mirror_ingress_set(bcm_msg->mparam[0], bcm_msg->mparam[1], bcm_msg->mparam[2]);
	/* int bcm_mirror_ingress_set(int unit, bcm_port_t port, int val); */
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "BCMmirror: bcm_mirror_ingress_set(%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "BCMmirror: Error mirror port ingress set\n");
		
	return 0;
}

/* 
* Function
*      hrast_mirror_egress_get
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
* Output
*      bcm_msg->mparam[2] - enable/disable flag    
* Return
*      -1 on error
*		0 on on success
*/ 
static int hrast_mirror_egress_get(struct bcm_msgbuf *bcm_msg)
{	
	int status;
	int val;

	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_mirror_egress_get(bcm_msg->mparam[0], bcm_msg->mparam[1], &val);
	/* int bcm_mirror_egress_get(int unit, bcm_port_t port, int *val); */
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "BCMmirror: bcm_mirror_egress_get(%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					val,
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "BCMmirror: Error mirror port egress get\n");
	else
		bcm_msg->mparam[2] = val;
		
	return 0;
}

/* 
* Function
*      hrast_mirror_egress_set
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
*      bcm_msg->mparam[2] - enable/disable flag 
* Output
*         
* Return
*      -1 on error
*		0 on on success
*/ 
static int hrast_mirror_egress_set(struct bcm_msgbuf *bcm_msg)
{	
	int status;

	if (bcm_msg == NULL)
		return -1;
	
	status = bcm_mirror_egress_set(bcm_msg->mparam[0], bcm_msg->mparam[1], bcm_msg->mparam[2]);
	/* int bcm_mirror_egress_set(int unit, bcm_port_t port, int val); */
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "BCMmirror: bcm_mirror_egress_set(%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "BCMmirror: Error mirror port egress set\n");
		
	return 0;
}

/* 
* Function
*      hrast_switch_control_get
* Input data
*      bcm_msg->mparam[0] - unit number
*      ext_buf->mdata - type string
* Output
*      bcm_msg->mparam[2] = mode;
* Return
*      -1 on error
*		0 on on success
*/ 
static int hrast_switch_control_get(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf)
{
	int status, mode;
	char type[50];

	if (bcm_msg == NULL)
		return -1;
		
	if (ext_buf == NULL)
		return -1;	
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "BCMmirror: hrast_switch_control_get: type:(%s)",
					ext_buf->mdata); 
	
	/* hand typed variable name bcmSwitchModuleType(ext_buf->mdata) */
	status = bcm_switch_control_get(bcm_msg->mparam[0], bcmSwitchModuleType, &mode);
	/* int bcm_switch_control_get(bcm_msg->mparam[0], ext_buf->mdata, &mode); */
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "BCMmirror: bcm_switch_control_get(%d,%s,%d) - \"%s\"",
					bcm_msg->mparam[0],
					ext_buf->mdata,
					mode,
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "BCMmirror: Error switch control get\n");
	else
		bcm_msg->mparam[2] = mode;
		
	return 0;
}

/* 
* Function
*      hrast_switch_control_set
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - mode;
*      ext_buf->mdata - type string
* Output
*     
* Return
*      -1 on error
*		0 on on success
*/ 
static int hrast_switch_control_set(struct bcm_msgbuf *bcm_msg, struct bcm_extbuf *ext_buf)
{
	int status;
	char type[50];

	if (bcm_msg == NULL)
		return -1;
		
	if (ext_buf == NULL)
		return -1;	
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "BCMmirror: hrast_switch_control_set: type:(%s)",
					ext_buf->mdata); 
	
	/* hand typed variable name bcmSwitchModuleType(ext_buf->mdata) */
	status = bcm_switch_control_set(bcm_msg->mparam[0], bcmSwitchModuleType, bcm_msg->mparam[1]);
	/* int bcm_switch_control_set(bcm_msg->mparam[0], ext_buf->mdata, bcm_msg->mparam[1]); */
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "BCMmirror: bcm_switch_control_set(%d,%s,%d) - \"%s\"",
					bcm_msg->mparam[0],
					ext_buf->mdata,
					bcm_msg->mparam[1],
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;

	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "BCMmirror: Error switch control set\n");
	
	return 0;
}

/* 
* Function
*      hrast_mirror_port_get
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
* Output
*      bcm_msg->mparam[2] - dest_mod
*      bcm_msg->mparam[3] - dest_port
*      bcm_msg->mparam[4] - flags
* Return
*      -1 on error
*		0 on on success
*/ 
static int hrast_mirror_port_get(struct bcm_msgbuf *bcm_msg)
{	
	int status;
	bcm_module_t dest_mod;
	bcm_port_t dest_port; 
	uint32 flags;

	if (bcm_msg == NULL)
		return -1;
#if 0	
	/* TEST - check for device id*/
	int unit=0;
	int modid=10; //test
	status = bcm_stk_modid_get(unit, &modid);
	hrast_dbg_print(2, SYSLOG | CONSOLE, "BCMmirror: bcm_stk_modid_get(%d) - \"%s\"",
					modid,
					bcm_errmsg(status));
	/* END TEST */
#endif	
	status = bcm_mirror_port_get(bcm_msg->mparam[0], bcm_msg->mparam[1], &dest_mod, &dest_port, &flags);
	/* int bcm_mirror_port_get(int unit, bcm_port_t port, bcm_module_t dest_mod, bcm_port_t dest_port, uint32 flags); */
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "BCMmirror: bcm_mirror_port_get(%d,%d,%d,%d,0x%08x) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					dest_mod,
					dest_port,
					flags,
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "BCMmirror: Error mirror port get\n");
	else
	{
		bcm_msg->mparam[2] = dest_mod;
		bcm_msg->mparam[3] = dest_port;
		bcm_msg->mparam[4] = (int)flags;
	}
		
	return 0;
}

/* 
* Function
*      hrast_mirror_port_set
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port
*      bcm_msg->mparam[2] - dest_mod
*      bcm_msg->mparam[3] - dest_port
*      bcm_msg->mparam[4] - flags
* Output
*
* Return
*      -1 on error
*		0 on on success
*/ 
static int hrast_mirror_port_set(struct bcm_msgbuf *bcm_msg)
{	
	int status;

	if (bcm_msg == NULL)
		return -1;
	
	if(bcm_msg->mparam[4]==1)
	{
		status = bcm_mirror_port_set(bcm_msg->mparam[0], bcm_msg->mparam[1], bcm_msg->mparam[2], bcm_msg->mparam[3], BCM_MIRROR_PORT_INGRESS);
		/* int bcm_mirror_port_set(int unit, bcm_port_t port, bcm_module_t dest_mod, bcm_port_t dest_port, uint32 flags); */
	
		hrast_dbg_print(2, SYSLOG | CONSOLE, "BCMmirror: bcm_mirror_port_set(%d,%d,%d,%d,0x%08x) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_msg->mparam[3],
					BCM_MIRROR_PORT_INGRESS,
					bcm_errmsg(status));
	}
	else if(bcm_msg->mparam[4]==2)
	{
		status = bcm_mirror_port_set(bcm_msg->mparam[0], bcm_msg->mparam[1], bcm_msg->mparam[2], bcm_msg->mparam[3], BCM_MIRROR_PORT_EGRESS);
		/* int bcm_mirror_port_set(int unit, bcm_port_t port, bcm_module_t dest_mod, bcm_port_t dest_port, uint32 flags); */
	
		hrast_dbg_print(2, SYSLOG | CONSOLE, "BCMmirror: bcm_mirror_port_set(%d,%d,%d,%d,0x%08x) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_msg->mparam[3],
					BCM_MIRROR_PORT_EGRESS,
					bcm_errmsg(status));
	}
	else
	{
		return 1;	
	}	

	bcm_msg->bcm_rv = status;
	
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "BCMmirror: Error mirror port set\n");
		
	return 0;
}

/* 
* Function
*      hrast_mirror_port_dest_add
* Input data
*      bcm_msg->mparam[0] - unit number
*      bcm_msg->mparam[1] - port number
*      bcm_msg->mparam[2] - flag number
*      bcm_msg->mparam[3] - destination port number
* Output
*
* Return
*      -1 on error
*		0 on on success
*/ 
static int hrast_mirror_port_dest_add(struct bcm_msgbuf *bcm_msg)
{

	int status;

	if (bcm_msg == NULL)
		return -1;
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "BCMmirror: before hrast_mirror_port_dest_add(%d,%d,%d,%d)%d",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_msg->mparam[3],
					BCM_MIRROR_PORT_ENABLE);
	
	status = bcm_mirror_port_dest_add(bcm_msg->mparam[0], bcm_msg->mparam[1], BCM_MIRROR_PORT_ENABLE, bcm_msg->mparam[3]);
	
	hrast_dbg_print(2, SYSLOG | CONSOLE, "BCMmirror: hrast_mirror_port_dest_add(%d,%d,%d,%d) - \"%s\"",
					bcm_msg->mparam[0],
					bcm_msg->mparam[1],
					bcm_msg->mparam[2],
					bcm_msg->mparam[3],
					bcm_errmsg(status)); 

	bcm_msg->bcm_rv = status;
									
	if (status != BCM_E_NONE)
		soc_cm_debug(DK_VERBOSE, "BCMmirror: Error adding mirroring destination to a port\n");

	return 0;
}
