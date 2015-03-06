#include "link_emu.h"

/* Description: Read an int and return the indicated bit.
 * Author: Unknown
 * Date: Unknown
 */
unsigned int mask_bit(char n, int bitnum)
{
  return (n & (1 << bitnum)) >> bitnum;
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
