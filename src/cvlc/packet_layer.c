/* This file contains code related to creating, sending, and getting
 * packages.
 */


#include <assert.h>

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
  char byte = 0;

  //Prepare an empty packet
  bstring packet = bfromcstralloc(total_packet_size, "");

  //Get packet
  data_length = get_byte();
  rc = binsertch(packet, 0, 1, data_length);
  check(rc == BSTR_OK, "Failed to add packet length.");

  debug("Got packet size: %d", data_length);

  //Get rest of packet
  for(i = PACKET_LENGTH_SIZE;i < data_length + PACKET_LENGTH_SIZE;i++){
    byte = get_byte();

    rc = binsertch(packet, i, 1, byte);
    check(rc == BSTR_OK, "Failed to get packet.");

    debug("Got [%d = %d]", i, byte);
  }
  
  //Send ACK frame
  if(data_length != 0){
    rc = send_ack();
  }

  return packet;

 error:
  bdestroy(packet);
  return NULL;
}

/* Description: Recieve a packet and send it.
 * Author: Albin Severinson
 * Date: 03/03/15
 */
int send_packet(bstring packet)
{
  int rc = 0;
  int i = 0;
  char byte = 0;

  //Store packet size
  int data_size = blength(packet);

  debug("Sent packet size: %d", data_size);

  //Transmit one byte at a time
  for(i = 0;i < data_size;i++){
    byte = bchar(packet, i);
    send_byte(byte);
    debug("Sent [%d = %d]", i, byte);
  }

  bdestroy(packet);

  //Wait for ACK frame
  if(data_size != 1){
    rc = get_ack();
    check(rc == 0, "Error when waiting for ACK.");
    //TODO Make a retransmission if the ACK fails.
  }

  return 0;

 error:
  return -1;
}

/* Description: Create a packet for sending data.
 * Author: Albin Severinson
 * Date: 07/03/15
 */
bstring create_data_frame(bstring payload)
{
  int rc = 0;
  unsigned char payload_length = blength(payload);
  assert(payload_length <= PACKET_DATA_SIZE && "Payload exceeded max size.");
  
  //Prepare packet
  bstring packet = bfromcstr("");

  //If it's a DATA frame, add the preable
  if(payload_length != 0){
    rc = binsertch(packet, 0, 1, preamble);
    check(rc == BSTR_OK, "Failed to insert preamble.");

    //Insert payload length
    rc = binsertch(packet, 1, 1, payload_length);
    check(rc == 0, "Failed to add package size.");
    debug("Preamble added");
  }
  else{
    //Insert payload length
    rc = binsertch(packet, 0, 1, payload_length);
    check(rc == 0, "Failed to add package size.");
  }

  //Concat with payload
  bconcat(packet, payload);

  //Cleanup payload
  bdestroy(payload);

  return packet;

 error:
  bdestroy(packet);
  return NULL;
  
}

/* Description: Create a packet for sending an ACK.
 * Author: Albin Severinson
 * Date: 07/03/15
 */
bstring create_ack_frame()
{
  bstring ack_payload = bfromcstr("");
  bstring packet = create_data_frame(ack_payload);
  return packet;
}

/* Description: Create and send an ACK frame.
 * Author: Albin Severinson
 * Date: 07/03/15
 */
int send_ack()
{
  //TODO Improve ACK. Current ACK is all zeros which is a problem.

  int rc = 0;
  debug("Sending ACK frame.");
  bstring ack_frame = create_ack_frame();
  rc = send_packet(ack_frame);
  check(rc == 0, "Failed to send ACK frame.");
  return 0;

 error:
  return -1;
}

/* Description: Wait for a DATA frame and store the payload.
 * Author: Albin Severinson
 * Date: 08/03/15
 */
bstring get_data_frame()
{
  int rc = 0;
  bstring packet;

  //Wait for preamble to occur
  rc = wait_for_preamble();
  check(rc == 0, "Failed to get preamble.");

  //Get packet
  packet = get_packet();
  return packet;

 error:
  return NULL;
}

/* Description: Wait for an ACK frame.
 * Author: Albin Severinson
 * Date: 07/03/15
 */
int get_ack()
{
  debug("Waiting for ACK frame.");
  int rc = 0;
  bstring packet = get_packet();
  check(packet, "Failed to get ACK.");

  rc = blength(packet);
  bdestroy(packet);

  if(rc == 1){
    debug("Got ACK frame.");
    return 0;
  }

 error:
  return -1;
}
