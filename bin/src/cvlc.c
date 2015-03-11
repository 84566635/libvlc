#include <stdio.h>
#include <pthread.h>

#include <cvlc/packet_layer.h>
#include <cvlc/dbg.h>



int main(int argc, char *argv[])
{
  int i = 0;
  int rc = 0;

  //Init link layer
  rc = init_link();
  check(rc == 0, "Failed to init link layer.");

  rc = init_packet_layer();
  check(rc == 0, "Failed to init packet layer.");

  //Set up packet and send it
  bstring payload = bfromcstr("vlc");
  bstring packet = create_data_frame(payload);
  send_packet(payload);

  /*
  //recieve packet
  bstring packet = get_data_frame();
  debug("Got packet: %s", bdata(packet));
  */

 error:
  return 0;
}
