#include "bstrlib.h"
#include "dbg.h"
#include "byte_layer.h"
#include "packet_layer.h"

int total_packet_size; //Calculated at init

/* Description: Init the packet layer.
 * Author: Albin Severinson
 * Date: 05/03/15
 */
int init_packet_layer()
{
  total_packet_size = PACKET_DATA_SIZE + PACKET_LENGTH_SIZE;
  return 0;
}

/* Description: Wait for a packet from LINK and return it.
 * Author: Albin Severinson
 * Date: 03/03/15
 */
bstring get_packet()
{
  int rc = 0;
  int i = 0;
  unsigned char data_length = 0;

  //Prepare an empty packet
  bstring packet = bfromcstralloc(total_packet_size, "");

  data_length = get_byte();

  //Get rest of packet
  for(i = PACKET_LENGTH_SIZE;i < PACKET_DATA_SIZE;i++){
    rc = binsertch(packet, i, 1, get_byte());
    check(rc == BSTR_OK, "Failed to get packet.");
  }

  printf("Received packet:-----------------\n%s\n----------------\n", bdata(packet));
  
  return packet;

 error:
  return NULL;
}

/* Description: Recieve a packet and send it.
 * Author: Albin Severinson
 * Date: 03/03/15
 */
int send_packet(bstring packet)
{
  int i = 0;

  //Store packet size
  int data_size = blength(packet);

  //Transmit one byte at a time
  for(i = 5;i < data_size;i++){
    send_byte(bchar(packet, i));
  }
  //printf("Packet of size %d sent.\n", blength(packet));
  return 0;
}
