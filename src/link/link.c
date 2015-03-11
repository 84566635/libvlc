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


