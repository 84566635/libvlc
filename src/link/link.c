/* This file contains code which is to be used in the real VLC
   implementation. This code is meant to interface with the hardware.
   This part isn't fully implemented yet.
*/

#include <stdio.h>
#include <stdlib.h>

#include <cvlc/dbg.h>

int get_analog_bit()
{
  FILE *ain1 = NULL;
  ain1 = fopen("/sys/devices/ocp.3/helper.15/AIN1", "r");
  //check(ain1, "Failed tp open AIN1.");

  int int_value = 0;
  char value[5] = {0};
  int len = fread(&value, sizeof(char), 5, ain1);
  check(len == 5, "Failed to get analog read.");
  fclose(ain1);

  int_value = atoi(value);

  //debug("Value: %d", int_value);
  return int_value;

 error:
  if (ain1) fclose(ain1);
  return -1;

}
