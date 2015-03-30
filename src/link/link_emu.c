/* This file contains code for emulating the LINK layer, and for
 * synchronization of the bit layer.
 */

#include <semaphore.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <cvlc/dbg.h>

#include "link_emu.h"

char tx;
char rx;

sem_t ok_to_read_bit;
sem_t ok_to_write_bit;

int bits_waiting = 0;

/* Description: Init the link layer and the semaphores used for
 * synchronization when simulating.
 * Author: Albin Severinson
 * Date: 11/03/15
 */
int init_link_emu()
{
  int rc = 0;
  tx = 0;
  rx = 0;

  //Init synchronization semaphores
  rc = sem_init(&ok_to_write_bit, 0, 1);
  check(rc == 0, "Failed to init LINK semaphore.");

  rc = sem_init(&ok_to_read_bit, 0, 0);
  check(rc == 0, "Failed to init LINK semaphore.");

 error:
  return rc;
}

/* Description: Send a bit. This method is to be used when simulating
 * the system.
 * Author: Albin Severinson
 * Date: 11/03/15
 */
char send_bit_emu(char bit)
{
  int rc = 0;
  //debug("Waiting to send bit");

  rc = sem_wait(&ok_to_write_bit);
  check(rc == 0, "Failed to wait for LINK semaphore.");

  tx = bit;
  bits_waiting++;;
  //debug("Sent bit");

  sem_getvalue(&ok_to_read_bit, &rc);
  assert(rc == 0 && "ok_to_read_bit has wrong value.");

  rc = sem_post(&ok_to_read_bit);

  return tx;

 error:
  return -1;
}

/* Description: Get a bit. This function is to be used when simulating
 * the system.
 * Author: Albin Severinson
 * Date: 11/03/15
 */
char get_bit_emu()
{
  int rc = 0;
  //debug("Waiting to get bit");
  sem_wait(&ok_to_read_bit);

  char bit = tx;
  bits_waiting--;
  //debug("Got bit");

  sem_getvalue(&ok_to_write_bit, &rc);
  assert(rc == 0 && "ok_to_write_bit has wrong value.");

  rc = sem_post(&ok_to_write_bit);
  check(rc == 0, "Failed to post LINK semaphore.");

  return bit;

 error:
  return -1;
}
