#ifndef _byte_layer_h
#define _byte_layer_h

//#define preamble 2863311530 //10101010101010101010101010101010
#define preamble 'g'


int wait_for_preamble();
char get_byte();
int send_byte(char byte);

#endif
