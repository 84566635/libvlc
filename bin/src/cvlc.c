#include <stdio.h>
#include <pthread.h>

#include <cvlc/link_emu.h>
#include <cvlc/byte_layer.h>
#include <cvlc/dbg.h>

char out[8] = {0};
char in[8] = {0};



int main(int argc, char *argv[])
{
  int i = 0;
  int rc = 0;

  //Init link layer
  rc = init_link();
  check(rc == 0, "Failed to init link layer.");

  //Set up sequence to send
  out[0] = 1;
  out[2] = 1;
  out[7] = 1;

  //Create threads for sending and receiving
  pthread_t rx_tid;
  pthread_t tx_tid;

  pthread_attr_t rx_attr;
  pthread_attr_t tx_attr;

  rc = pthread_attr_init(&rx_attr);
  check(rc == 0, "Failed to init rx thread.");

  rc = pthread_attr_init(&tx_attr);
  check(rc == 0, "Failed to init tx thread.");

  //pthread_create(&rx_tid, &rx_attr, send_bytes, "");
  //pthread_create(&tx_tid, &tx_attr, get_bytes, "");

  pthread_join(rx_tid, NULL);
  pthread_join(tx_tid, NULL);

 error:
  return 0;
}
