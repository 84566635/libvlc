#include <unistd.h>

#include "packet_layer.h"
#include "bstrlib.h"
#include "dbg.h"


/* Description: Recieve a file, unpackage it, and store it.
 * Author: Albin Severinson
 * Date: 03/03/15
 */
int get_file(char *output_name)
{
  //Prepare an empty packet
  bstring packet;

  //Prepare EOF statement
  bstring eof = bfromcstr("eof");

  //Open the output file
  FILE *output_file = fopen(output_name, "w");
  check(output_file, "Failed to open output file.");

  while(1){
    //Get a data frame
    packet = get_data_frame();

    //Files are terminated with an eof packet. Check for this.
    if(biseq(eof, packet) == 1) break;

    //Print to file
    fprintf(output_file, "%s", bdata(packet));
  }

  fflush(output_file);
  fclose(output_file);

  return 0;

 error:
  if(output_file) fclose(output_file);
  return -1;
}

/* Description: Read a file, package it, and send it.
 * Author: Albin Severinson
 * Date: 02/03/15
 */
int send_file(char *file_name)
{
  int rc = 0;
  int num_packets = 0;

  //Prepare an empty packet
  bstring packet = NULL;

  //Prepare data buffer
  bstring data_buffer = bfromcstr("");

  //Prepare terminating packet
  bstring eof = bfromcstr("eof");

  FILE *data_file = NULL;
  struct bStream *data_stream = NULL;

  //Open the data file
  data_file = fopen(file_name, "r");
  check(data_file, "Failed to open file for transmit.");

  //Open as bStream
  data_stream = bsopen((bNread) fread, data_file);
  check(data_stream, "Failed to open the data bStream.");

  debug("Reading file: %s\n", file_name);

  //Read the stream in chunks of PACKET_DATA_SIZE
  while(bsread(data_buffer, data_stream, PACKET_DATA_SIZE) == BSTR_OK){

    //Prepend the preamble and packet size.
    packet = create_data_frame(data_buffer);

    //Send the packet
    rc = send_packet(packet);
    check(rc == 0, "Failed to send packet.");

    //Clear the data buffer. The memory is free'd in send_packet.
    data_buffer = bfromcstr("");

    num_packets++;
  }
  
  packet = create_data_frame(eof);
  rc = send_packet(packet);
  check(rc == 0, "Failed to send terminating packet.");

  debug("File sent using %d packets.\n", num_packets);

 error: //fallthrough
  if(packet) bdestroy(packet);
  if(data_buffer) bdestroy(data_buffer);
  if(data_stream) data_file = bsclose(data_stream);
  if(data_file) fclose(data_file);

  return rc;
}
