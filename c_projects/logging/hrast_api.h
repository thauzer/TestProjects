/* hrast_api.h - Hrast local header */

/*#######################################################################*/
/*#                                                                     #*/
/*#                                                                     #*/
/*#                    Copyright (c) 2008 Iskratel                      #*/
/*#                                                                     #*/
/*#     Name:           hrast_api.h                                     #*/
/*#                                                                     #*/
/*#     Description:    hrast interface common local header file        #*/
/*#                                                                     #*/
/*#     Code:           QMRP  XAK8292                                   #*/
/*#                                                                     #*/
/*#     Date:           November 2008                                   #*/
/*#                                                                     #*/
/*#     Author:         Dare Oblak, ITPSK9                              #*/
/*#                                                                     #*/
/*#     Design:                                                         #*/
/*#                                                                     #*/
/*#     Revisions:                                                      #*/
/*#                                                                     #*/
/*#     Remarks:        This header is common and is used the same on   #*/
/*#                     client side (MG, IT_UTILS) and on server side   #*/
/*#                     (IT_KERNEL). Synchronization should bee keept   #*/
/*#                     between SVN and sw_conf libraries.              #*/
/*#                                                                     #*/
/*#######################################################################*/

#ifndef _HRAST_API_H
#define _HRAST_API_H

/************************************************************************/
/* UNIX - SCCS  VERSION DESCRIPTION					*/
/************************************************************************/

/* static char unixid_hrast_api_h[] = "@(#)QMRP.z	1.2     10/01/29     hrast_api.h -O-"; */

/* Message structure definition */
struct bcm_msgbuf {
        int lnx_mtype;          /* Linux message type */
        int mtype;              /* private message type */
        pid_t mpid;             /* pid from process/thread */
        unsigned int seq;       /* sequence number */
        unsigned int mcmd;      /* command */
        int mparam[7];          /* integer data to send */
        bcm_pbmp_t pbmp[2];     /* bcm_pbmp_t data to send */
        int bcm_rv;             /* BCM API return value */
        int mstatus;            /* to confirm messages HRAST_MESSAGE_RESPONSE_OK/HRAST_MESSAGE_RESPONSE_ERROR */
        struct bcm_extbuf *ext_msg;   /* link to locally stored extended message */
};

/*
* To send larger amount of data, first ordinary message (struct bcm_msgbuf) 
* with "mtype" set to one of HRAST_EXTENDED_MESSAGE... types is sent, then extended 
* message (struct bcm_extbuf) with data is sent
*/

/* Extended message structure definition */
struct bcm_extbuf {
        int lnx_mtype;     /* Linux message type */
        char mdata[2000];		/* extended data to send */
};

/* defines that is added for rstp */
#define __RSTP__	1

#ifdef __RSTP__
struct bcm_rstpbuf { /* implemented because with mdata[2000] the queue was filled to quickly */
    int lnx_mtype;
    unsigned char mdata[128];
};
#endif /* __RSTP__ */

#define BUFF_SIZE 2024 /*size of hrast message*/
/* logging flags */
#define SYSLOG                 0x00000001	/* syslog */
#define CONSOLE                0x00000002	/* console */

/* hrast message type */
#define HRAST_MSG_TYPE         0x0000d0b1

/* hrast timeout for response message */
#define HRAST_WAIT_FOREWER              0
#define HRAST_SHORT_TIMEOUT           300	/* 300 us *//* changed from 100 us */
#define HRAST_LONG_TIMEOUT           2000	/*   1 ms */ /* changed from 2000 - gstr*/


/* mask for all modules, which have to be initialized  */
/* this value can vary on product and  should be moved */
/* into some product specific header file              */
#define BCM_INITIALIZED_MODULES_MASK        0x0006d7ff		/* all except L3, DMUX and FIELD */

/* command definitions */
#define HRAST_UNIT_SET                  1   /* set unit, on MG there is only unit: 0 */

/* Initialization */
#define HRAST_SWITCH_INIT_CHECK         3   /* bcm_init_check */
#define HRAST_SWITCH_ATTACH_CHECK       4   /* bcm_attach_check */
#define HRAST_BCM_SHUTDOWN              5

/* Port configuration */
#define HRAST_SHOW_ME_PIC               9
#define HRAST_PORT_STATUS_SHOW         10
#define HRAST_PORT_ENABLE_GET          11
#define HRAST_PORT_ENABLE_SET          12   /* bcm_port_enable_set */
#define HRAST_PORT_LINK_STATUS_GET     13
#define HRAST_PORT_ADVERT_REMOTE_GET   14
#define HRAST_PORT_INFO_GET            15
#define HRAST_PORT_SELECTIVE_INFO_GET  16
#define HRAST_PORT_VLAN_INFO_GET       17
#define HRAST_PORT_MAX_SPEED_GET       18
#define HRAST_PORT_SPEED_SET           19
#define HRAST_PORT_SPEED_GET           1009
#define HRAST_PORT_DUPLEX_SET          190
#define HRAST_PORT_DUPLEX_GET          1010
#define HRAST_PORT_MAX_FRAME_SIZE_GET  1011
#define HRAST_PORT_INTERFACE_MODE_SET  1012
#define HRAST_PORT_MAX_FRAME_SIZE_SET  191
#define HRAST_PORT_AUTONEG_SET         192
#define HRAST_PORT_RATE_EGRESS_SET     195
#define HRAST_PORT_RATE_INGRESS_SET    196
#define HRAST_PORT_RATE_EGRESS_GET     1015
#define HRAST_PORT_RATE_INGRESS_GET    1016

/* Link Monitoring and Notification */
#define HRAST_LINKSCAN_INIT            20   /* bcm_linkscan_init */
#define HRAST_LINKSCAN_MODE_GET        21   /* bcm_linkscan_mode_get */
#define HRAST_LINKSCAN_MODE_SET        22   /* bcm_linkscan_mode_set */
#define HRAST_LINKSCAN_ENABLE_GET      23   /* bcm_linkscan_enable_get */
#define HRAST_LINKSCAN_ENABLE_SET      24   /* bcm_linkscan_enable_set */
#define HRAST_LINKSCAN_ENABLE_PORT_GET 25   /* bcm_linkscan_enable_port_get */
#define HRAST_LINKSCAN_REGISTER        26   /* bcm_linkscan_register */
#define HRAST_LINKSCAN_UNREGISTER      27   /* bcm_linkscan_unregister */
#define HRAST_LINKSCAN_DETACH          28   /* bcm_linkscan_detach */

#define HRAST_LINKSKAN_SHOW            30
#define HRAST_LINKSCAN_CALLBACK        31   /* callback from BCM API */
#define HRAST_LINKSCAN_ECHO            32

/* VLAN Management */
#define HRAST_VLAN_SHOW                40
#define HRAST_VLAN_CREATE              41   /* bcm_vlan_create */
#define HRAST_VLAN_DESTROY             42   /* bcm_vlan_destroy */
#define HRAST_PORT_VLAN_GET            43   /* bcm_vlan_port_get */
#define HRAST_PORT_VLAN_ADD            44   /* bcm_vlan_port_add */
#define HRAST_PORT_VLAN_REMOVE         45
#define HRAST_PORT_UNTAG_VLAN_SET      46   /* bcm_port_untagged_vlan_set */
#define HRAST_PORT_UNTAG_VLAN_GET      47   /* bcm_port_untagged_vlan_get */


/* Spanning Tree Groups */
#define HRAST_PORT_STP_STATE_GET       50
#define HRAST_PORT_STP_STATE_SET       51

/* Link aggregation (trunking) */
#define HRAST_TRUNK_INIT               55   /*bcm_trunk_init*/
#define HRAST_TRUNK_CREATE             56   /*bcm_trunk_create*/
#define HRAST_TRUNK_SET                57   /*bcm_trunk_set*/
#define HRAST_TRUNK_DETACH             58   /*bcm_trunk_detach*/
#define HRAST_TRUNK_PSC_GET            59   /*bcm_trunk_psc_get*/

/* Statistics */
#define HRAST_STAT_GET                 60
#define HRAST_STAT_GET32               61
#define HRAST_STAT_CLEAR               62 /*bcm_stat_clear*/

/*IFILTER*/
#define HRAST_PORT_IFILTER_GET          70 /*bcm_port_ifilter_get*/
#define HRAST_PORT_IFILTER_SET          71 /*bcm_port_ifilter_set*/

/* Iskratel Diagnostics */
#define HRAST_DIAG_CAM_SEL_COUNT_GET                 80	/* bcm_diag_cam_selective_count_get */
#define HRAST_DIAG_CAM_ENTRY_GET                     81	/* bcm_diag_cam_entry_get */
#define HRAST_DIAG_CAM_AGE_TIMER_GET                 82	/* bcm_diag_cam_age_timer_get */
#define HRAST_DIAG_CAM_AGE_TIMER_SET                 83	/* bcm_diag_cam_age_timer_set */
#define HRAST_DIAG_TABLE_SIZE_GET                    84	/* bcm_diag_table_size_get */
#define HRAST_DIAG_TABLE_COUNT_GET                   85	/* bcm_diag_table_count_get */
#define HRAST_DIAG_IN_COS_PKT_COUNT_GET              86 /* bcm_diag_in_cos_pkt_count_get */
#define HRAST_DIAG_OUT_COS_PKT_COUNT_GET             87 /* bcm_diag_out_cos_pkt_count_get */
#define HRAST_DIAG_CONGESTION_PKT_DISC_COUNT_GET     88 /* bcm_diag_congestion_pkt_disc_count_get */
#define HRAST_DIAG_CONGESTION_PKT_DISC_COUNT_GET_ALL 89 /* hrast_diag_congestion_pkt_disc_count_get_all */
#define HRAST_DIAG_CONGESTION_PKT_DISC_THRESHOLD_SET 90 /* bcm_diag_congestion_pkt_disc_threshold_set */
#define HRAST_DIAG_TABLE_COUNT_SET                   91 /* hrast_api_diag_table_count_set */
#define HRAST_DIAG_TEST_DEBUG_MODE_SET               92	/* hrast_diag_debug_mode_set */
#define HRAST_DIAG_TEST_DEBUG_MODE_GET               93	/* hrast_diag_debug_mode_get */

/* Iskratel filter */
#define HRAST_DIAG_FILTER_SET                       800	/* bcm_diag_DHCP_filter_set */
#define HRAST_DIAG_MEM_SET                          801	/* bcm_diag_mem_set */
#define HRAST_DIAG_MEM_GET                          802	/* bcm_diag_mem_get */
#define HRAST_DIAG_FILTER_DESTROY_BY_PORT           803	/* bcm_diag_filter_destroy_by_port */

/* Debug */
#define HRAST_DEBUG_LEVEL_SET         100
#define HRAST_DEBUG_LEVEL_GET         101
#define HRAST_DIAG_DEBUG_FLAG_ON      105	/* hrast_api_diag_debug_flag_on */
#define HRAST_DIAG_DEBUG_FLAG_OFF     106	/* hrast_api_diag_debug_flag_off */
#define HRAST_DIAG_TEST_DEBUG_MODE_ON 107	/* BCMdiagDebugSet = FALSE */
#define HRAST_DIAG_TEST_DEBUG_MODE_OFF 108	/* BCMdiagDebugSet = TRUE */

#ifdef __RSTP__
/* BCM Packet, Transmit and Receive API functions */
#define HRAST_TX_RSTP			    120 /* bcm_tx_rstp */
#define HRAST_RX_REGISTER_RSTP		121 /* bcm_rx_register */
#define HRAST_RX_START			    122 /* bcm_rx_start */
#define HRAST_RX_ACTIVE             123 /* bcm_rx_active */
#define HRAST_RX_UNREGISTER_RSTP	124 /* bcm_rx_unregister */
#define HRAST_PORT_STP_SET		    125 /* bcm_port_stp_set */
#define HRAST_L2_ADDR_DEL_PORT      126 /* bcm_l2_addr_delete_by_port */

#define HRAST_RX_CALLBACK		    130 /* rx callback message */
#define HRAST_RX_ECHO               131 /* rx echo message */
#endif /* __RSTP__ */

/* mstatus values for message delivery */
#define HRAST_MESSAGE_CMD_OK            0
#define HRAST_MESSAGE_CMD_ERROR        -1

/*default congestion packet threshold*/
#define HRAST_DIAG_DEFAULT_CONG_PKT_THR 50

/* mtype definitions */
#define HRAST_MESSAGE_FROM_BCM              0x00000001
#define HRAST_MESSAGE_FROM_BCM_CONFIRM      0x00000002
#define HRAST_MESSAGE_FROM_APPL             0x00000004
#define HRAST_MESSAGE_FROM_APPL_CONFIRM     0x00000008

#define HRAST_MESSAGE_FROM_BCM_E            0x00000010
#define HRAST_MESSAGE_FROM_BCM_CONFIRM_E    0x00000020
#define HRAST_MESSAGE_FROM_APPL_E           0x00000040
#define HRAST_MESSAGE_FROM_APPL_CONFIRM_E   0x00000080


#define HRAST_EXTENDED_MESSAGE	(HRAST_MESSAGE_FROM_BCM_E | \
								HRAST_MESSAGE_FROM_BCM_CONFIRM_E | \
								HRAST_MESSAGE_FROM_APPL_E | \
								HRAST_MESSAGE_FROM_APPL_CONFIRM_E)
								
#define HRAST_MESSAGE			(HRAST_MESSAGE_FROM_BCM | \
								HRAST_MESSAGE_FROM_APPL | \
								HRAST_MESSAGE_FROM_BCM_E | \
								HRAST_MESSAGE_FROM_APPL_E)
								
#define HRAST_CONFIRM			(HRAST_MESSAGE_FROM_BCM_CONFIRM | \
								HRAST_MESSAGE_FROM_APPL_CONFIRM | \
								HRAST_MESSAGE_FROM_BCM_CONFIRM_E | \
								HRAST_MESSAGE_FROM_APPL_CONFIRM_E)

/* mirror */
#define HRAST_MIRROR_INIT                       201	/* bcm mirror initialize */
#define HRAST_MIRROR_MODE_GET                   202 /* bcm mirror mode get */
#define HRAST_MIRROR_MODE_SET                   203 /* bcm mirror mode set */
#define HRAST_MIRROR_TO_GET                     211 /* bcm mirror-to port get */
#define HRAST_MIRROR_TO_SET                     212 /* bcm mirror-to port set */
#define HRAST_MIRROR_INGRESS_GET                213 /* bcm mirror port ingress get */
#define HRAST_MIRROR_INGRESS_SET                214 /* bcm mirror port ingress set */
#define HRAST_MIRROR_EGRESS_GET                 215 /* bcm mirror port egress get */
#define HRAST_MIRROR_EGRESS_SET                 216 /* bcm mirror port egress set */
#define HRAST_SWITCH_CONTROL_GET                221 /* bcm witch control get */
#define HRAST_SWITCH_CONTROL_SET                222 /* bcm witch control get */
#define HRAST_MIRROR_PORT_GET                   223 /* bcm mirror port get */
#define HRAST_MIRROR_PORT_SET                   224 /* bcm mirror port set */
#define HRAST_MIRROR_PORT_DEST_ADD              225 /* bcm mirror port destination add */

#endif /* _HRAST_API_H */




