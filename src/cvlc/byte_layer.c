/* This file contains code related to creating, sending, and getting
 * bytes.
 */

#include "link_emu.h"
#include "byte_layer.h"
#include "dbg.h"

/* Description: Read an int and return the indicated bit.
 * Author: Unknown
 * Date: Unknown
 */
unsigned int mask_bit(char n, int bitnum)
{
  return (n & (1 << bitnum)) >> bitnum;
}

/* Description: Wait for the preamble to occur.
 * Author: Philip Holgersson 
 * Revised: Albin Severinson
 * Date: 08/03/15
 */
int wait_for_preamble()
{
  debug("Waiting for preamble...");

  unsigned char preamble_found = 0;
  char bit = 0;
  int i = 0;
  char preamble_bits[9] = {0};

  while(preamble_found != preamble){
    bit = get_bit();
    preamble_found = (preamble_found >> 1) | (bit << 7);
    for(i = 0;i < 8;i++) preamble_bits[i] = (mask_bit(preamble_found, i) == 0) ? '0' : '1';
    preamble_bits[8] = '\0';
    debug("Preamble found: %s = %c", preamble_bits, preamble_found);
  }

  debug("Got preamble!");
  return 0;
}

/* Description: Recieve a byte from LINK and return.
 * Author: Albin Severinson
 * Date: 03/03/15
 */
char get_byte()
{
  int i = 0;
  int j = 0;
  char c = 0;
  char bit;

  for(i = 0;i < 8;i++){
    bit = get_bit();    
    for(j = 0;j < i;j++) bit *= 2;
    c += bit;
  }

  return c;
}

/* Description: Recieve a byte and transmit it.
 * Author: Albin Severinson
 * Date: 03/03/15
 */
int send_byte(char byte)
{
  int i = 0;
  for(i = 0;i < 8;i++){
    send_bit(mask_bit(byte, i));
  }
  return 0;
}
