#ifndef _link_win_h
#define _link_win_h

int init_link_win();
int wait_for_window(long start);
int wait_for_tx_window();
int wait_for_rx_window();

#endif
