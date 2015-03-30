/* This file contains code related to creating, sending, and getting
 * packages.
 */


#include <assert.h>

#include "bstrlib.h"
#include "dbg.h"
#include "byte_layer.h"
#include "packet_layer.h"

#define ACK 'h'
#define MAX_ACK_WAIT 3
#define MAX_PACKET_RESEND 3

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
  //rc = binsertch(packet, 0, 1, data_length);
  //check(rc == BSTR_OK, "Failed to add packet length.");

  debug("Got packet size: %d", data_length);

  //Get rest of packet
  for(i = 0;i < data_length;i++){
    byte = get_byte();

    rc = binsertch(packet, i, 1, byte);
    check(rc == BSTR_OK, "Failed to get packet.");

    //debug("Got [%d = %d]", i, byte);
  }
  
  //Send ACK frame
  if(data_length != 0){
    rc = send_ack();
    check(rc == 0, "Failed to send ACK frame.");
  }

  debug("[GOT PACKET]: %s", bdata(packet));

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
  int j = 0;
  char byte = 0;

  debug("[SENDING]: %s", bdata(packet));

  //Store packet size
  int data_size = blength(packet);

  for(j = 0;j < MAX_PACKET_RESEND;j++){
    debug("[SENDING PACKET] size: %d try: %d", data_size, j);

    //Transmit one byte at a time
    for(i = 0;i < data_size;i++){
      byte = bchar(packet, i);
      send_byte(byte);
      //debug("Sent [%d = %d]", i, byte);
    }

    //Wait for ACK frame
    if(data_size != 1){
      rc = get_ack();
      
      //Return if we got the ACK
      if(rc == 0){
        bdestroy(packet);
        return 0;
      }
    }
  }

  bdestroy(packet);
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
  //TODO Improve ACK. Current ACK isn't well designed for full duplex
  //transmissions. ACK should specify which packet was ACK'd instead
  //of sending a general signal.
  debug("Sending ACK.");
  send_byte(ACK);
  return 0;
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
  debug("Waiting for ACK...");
  char ack = 0;
  int i = 0;

  //Wait for the ACK frame
  for(i = 0;i < MAX_ACK_WAIT;i++){
    ack = get_byte();
    if(ack == ACK){
      debug("Got ACK!");
      return 0;
    }
  }
  return -1;
}
