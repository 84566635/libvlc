/* This file contains code for emulating the LINK layer, and for
 * synchronization of the bit layer.
 */

#include <semaphore.h>
#include <assert.h>

#include "link_emu.h"
#include "dbg.h"

char tx;
char rx;

sem_t ok_to_read_bit;
sem_t ok_to_write_bit;

int bits_waiting = 0;

int init_link()
{
  int rc = 0;
  tx = 0;
  rx = 0;
  rc = sem_init(&ok_to_write_bit, 0, 1);
  check(rc == 0, "Failed to init LINK semaphore.");

  rc = sem_init(&ok_to_read_bit, 0, 0);
  check(rc == 0, "Failed to init LINK semaphore.");

 error:
  return rc;
}

char send_bit(char bit)
{
  int rc = 0;

  debug("Waiting to send bit");
  rc = sem_wait(&ok_to_write_bit);
  check(rc == 0, "Failed to wait for LINK semaphore.");

  tx = bit;
  bits_waiting++;;
  debug("Sent bit");

  sem_getvalue(&ok_to_read_bit, &rc);
  assert(rc == 0 && "ok_to_read_bit has wrong value.");

  rc = sem_post(&ok_to_read_bit);

  return tx;

 error:
  return -1;
}

char get_bit()
{
  int rc = 0;

  debug("Waiting to get bit");
  sem_wait(&ok_to_read_bit);

  char bit = tx;
  bits_waiting--;
  debug("Got bit");

  sem_getvalue(&ok_to_write_bit, &rc);
  assert(rc == 0 && "ok_to_write_bit has wrong value.");

  rc = sem_post(&ok_to_write_bit);
  check(rc == 0, "Failed to post LINK semaphore.");

  return bit;

 error:
  return -1;
}
