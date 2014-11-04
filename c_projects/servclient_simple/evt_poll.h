#ifndef _EVT_POLL_H_
#define _EVT_POLL_H_

int init_event_poll(void);

int close_event_poll(void);

int add_event_poll(int fd);

int evt_loop(int serverfd);

#endif
