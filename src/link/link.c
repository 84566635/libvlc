/* This file contains wrapper functions for the different LINK
 * implementaions. The different implementations are:
 * - link_emu: Useful for simulating the system. Assumes both the
 * sending and recieving party is running as separate threads on the
 * same computer.
 * - link_win: Uses a simple windowing method for sending and
 * recieving. Whenever a bit is to be sent or recieved the program
 * will wait until the next transmit window.
 */

#include <stdio.h>
#include <stdlib.h>

#include <cvlc/dbg.h>

#include "link_emu.h"

int init_link()
{
  return init_link_emu();
}

char get_bit()
{
  return get_bit_emu();
}

char send_bit(char bit)
{
  return send_bit_emu(bit);
}


