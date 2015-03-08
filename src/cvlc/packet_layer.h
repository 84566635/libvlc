#ifndef _packet_layer_h
#define _packet_layer_h

#include "bstrlib.h"

#define PACKET_LENGTH_SIZE 1
#define PACKET_DATA_SIZE 100

int init_packet_layer();
bstring get_packet();
int send_packet(bstring );
bstring create_data_frame(bstring );
bstring create_ack_frame();
int send_ack();
int get_ack();

#endif
