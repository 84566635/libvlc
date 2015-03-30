/* This file contains code which is to be used in the real VLC
   implementation. This code is meant to interface with the hardware.
   This part isn't fully implemented yet.
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <cvlc/dbg.h>

//How frequently the transmit windows occurs. The window occurs every
//FREQ clock cycles.
#define FREQ 1000000

clock_t start_rx_time;
clock_t start_tx_time;
clock_t start_time;
clock_t prev_window;

int init_link_win()
{
  start_time = clock();
  debug("Windowing start time: %d", (int)start_time);
  return 0;
}

/* Description: Wait for the next transmit window to occur.
 * Author: Albin Severinson
 * Date: 110315
 */
int wait_for_window()
{
  clock_t now = 1;

  //Wait for previous window to close
  while((now = clock()) == prev_window);

  debug("Waiting for window at %d", (int)now);

  //Wait for next window
  while(((now = clock()) % FREQ) != 0);
  prev_window = now;

  debug("Window started at %d", (int)now);

  return 0; 
}

int wait_for_tx_window()
{
  return wait_for_window();
}

int wait_for_rx_window()
{
  return wait_for_window();
}
