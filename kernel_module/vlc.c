#include <linux/module.h>   /* Needed by all modules */
#include <linux/kernel.h>   /* Needed for KERN_INFO */
#include <linux/init.h>     /* Needed for the macros */

#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/socket.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/in.h>
#include <linux/init.h>

//#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <linux/inet.h>
#include <linux/netdevice.h>   /* net_device, net_device_stats etc */
#include <linux/etherdevice.h> /* Ethernet device standards. */
#include <linux/skbuff.h>
#include <linux/ethtool.h>
#include <net/sock.h>
#include <net/checksum.h>
#include <linux/if_ether.h>	/* For the statistics structure. */
#include <linux/if_arp.h>	/* For ARPHRD_ETHER */
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/percpu.h>
#include <net/net_namespace.h>
#include <linux/u64_stats_sync.h>

#include <rtdm/rtdm_driver.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>

#include "vlc.h"

MODULE_AUTHOR("Albin Severinson");
MODULE_LICENSE("GPL");

//The frequency of the physical layer.
#define FREQ 5000;

//Some constants
#define PREAMBLE_LEN 1

#define VLC_PREAMBLE 2863311530 /* 10101010101010101010101010101010 in base 10 */

#define SIZE_LEN 1

//GPIO addresses
#define GPIO_INPUT 50
#define GPIO_OUTPUT 60

//Locations of the GPIO in memory for the BBB
#define ADDR_BASE_0 0x44e07000
#define ADDR_BASE_1 0x4804c000

//For storing the locations of GPIO pins in memory
volatile void* gpio1;
volatile void* gpio2;

//The VLC device
struct net_device *vlc_dev = NULL;

//The timer used for calling the timer_handler routine
rtdm_timer_t phy_timer;

//A VLC packet
struct vlc_packet {
  struct vlc_packet *next;
  struct net_device *dev;
  struct sk_buff *skb;
};

//The packets currently being received and transmitted
struct vlc_packet *tx_packet = NULL;
struct vlc_packet *rx_packet = NULL;

//Some private data for the driver. Not currently used (I think).
struct vlc_priv {
  struct net_device_stats stats;
  int status;
  struct vlc_packet *ppool;
  struct vlc_packet *rx_queue;  /* List of incoming packets */
  int rx_int_enabled;
  int tx_packetlen;
  u8 *tx_packetdata;
  struct sk_buff *skb;
  spinlock_t lock;
  struct net_device *dev;
};

//Indexes for keeping track of what to transmit
int tx_bit_index = 0;
int tx_byte_index = 0;

int rx_bit_index = 0;
int rx_byte_index = 0;

//The current preamble collected. Cleared when the actual preamble,
//defined above, is found.
unsigned int vlc_current_preamble = 0;

//The states the transmitter can be in.
#define TX_SENDING_PREAMBLE 0
#define TX_SENDING_PACKET_LEN 1
#define TX_SENDING_PACKET_DATA 2

//Used for keeping track of the current transmitter state.
int tx_state = 0;

//The states the receiver can be in.
#define RX_WAITING_FOR_PREAMBLE 0
#define RX_GETTING_PACKET_LEN 1
#define RX_GETTING_PACKET_DATA 2

//Used for keeping track of the current receiver state.
int rx_state = 0;

//Buffer for storing incoming data before it's moved to a packet.
//Added some padding for safety.
u8 rx_byte_buffer[ETH_DATA_LEN + 10];
unsigned int rx_packet_len = 0;

/* Mask out a specific bit from an int.
 */
unsigned int mask_bit(unsigned int n, int bitnum)
{
  return (n & (1 << bitnum)) >> bitnum;
}

/* Runs at every timer tick. If there's any packets being sent the
 * next bit will be sent. The handler keeps track of which bit to
 * transmit with the tx indexes defined above.
 *
 * The handler will also collect one bit from the physical interface
 * and store it depending on which state it is in.
 *
 * Currently sending preamble and packet length between devices work.
 * The data portion of the packet doesn't yet work.
 */
void timer_handler(rtdm_timer_t *timer)
{
  u8 byte = 0;
  unsigned int bit = 0;
  struct vlc_priv *priv = NULL;
  int i = 0;

  //Check if there's a packet waiting
  if(tx_packet){

    //Check what state the transmitter is in
    if(tx_state == TX_SENDING_PREAMBLE){
      bit = mask_bit((unsigned int) VLC_PREAMBLE, tx_bit_index++);

      if(tx_bit_index == 32){
        tx_state = TX_SENDING_PACKET_LEN;
        tx_bit_index = 0;
        goto end_tx;
      }
    }

    else if(tx_state == TX_SENDING_PACKET_LEN){
      bit = mask_bit(tx_packet->skb->len, tx_bit_index++);
      
      if(tx_bit_index == 32){
        tx_state = TX_SENDING_PACKET_DATA;
        tx_bit_index = 0;
        goto end_tx;
      }
    }
    
    else if(tx_state == TX_SENDING_PACKET_DATA){
      byte = tx_packet->skb->data[tx_byte_index];
      bit = mask_bit(byte, tx_bit_index++);

      //Increment the index
      if(tx_bit_index == 8){
        tx_byte_index++;
        tx_bit_index = 0;
      }

      //If this was the last bit, free the packet and reset the index.
      if(tx_byte_index > tx_packet->skb->len){
        tx_state = TX_SENDING_PREAMBLE;

        //Printout payload for debugging purposes
        /*
        for(i = 0;i < tx_packet->skb->len;i++){
          printk(KERN_INFO "%c", tx_packet->skb->data[i]);
        }
        printk(KERN_INFO "\n");
        */

        //Update stats
        priv = netdev_priv(vlc_dev);
        priv->stats.tx_packets++;
        priv->stats.tx_bytes += tx_packet->skb->len;

        //Free the packet
        dev_kfree_skb(tx_packet->skb);
        kfree(tx_packet);
        tx_packet = NULL;

        //Reset the index
        tx_byte_index = 0;
        tx_bit_index = 0;

        //Accept packets again
        netif_wake_queue(vlc_dev);

        printk(KERN_INFO "VLC: Packet sent!\n");
      }
    }

  end_tx:
    //Set the LED according to current bit
    gpio_set_value(GPIO_OUTPUT, bit);
  }

 start_rx:
  //Get a bit from the interface
  bit = gpio_get_value(GPIO_INPUT);

  //Check which state the reciever is in
  if(rx_state == RX_WAITING_FOR_PREAMBLE){
    vlc_current_preamble = (vlc_current_preamble >> 1) | (bit << 31);
    if(vlc_current_preamble == (unsigned int) VLC_PREAMBLE){
      printk(KERN_INFO "VLC: Found preamble!\n");
      rx_state = RX_GETTING_PACKET_LEN;
      vlc_current_preamble = 0;

      //Create an empty packet
      rx_packet = kmalloc(sizeof(struct vlc_packet), GFP_KERNEL);
      rx_packet_len = 0;

      goto end_rx;
    }
  }

  else if(rx_state == RX_GETTING_PACKET_LEN){
    rx_packet_len |= bit << rx_bit_index++;
    
    if(rx_bit_index == 32){
      printk(KERN_INFO "VLC: Incoming packet length recieved [%d]!\n", rx_packet_len);
      rx_state = RX_GETTING_PACKET_DATA;
      rx_bit_index = 0;

      goto end_rx;
    }
  }

  else if(rx_state == RX_GETTING_PACKET_DATA){
    rx_byte_buffer[rx_byte_index] |= bit << rx_bit_index++;

    if(rx_bit_index == 8){
      rx_byte_index++;
      rx_bit_index = 0;
    }
    
    if(rx_byte_index > rx_packet_len){
      printk(KERN_INFO "VLC: Incoming packet recieved [%d]!\n", rx_packet_len);
      rx_state = RX_WAITING_FOR_PREAMBLE;
      rx_byte_index = 0;
      rx_bit_index = 0;

      priv = netdev_priv(vlc_dev);

      //Create an sk_buff
      rx_packet->skb = dev_alloc_skb(rx_packet_len);
      rx_packet->skb->len = rx_packet_len;

      if(!rx_packet->skb){
        if(printk_ratelimit()) printk(KERN_NOTICE "VLC: Low on mem - packet droped.\n");
        priv->stats.rx_dropped++;
      }

      //Copy the data from the buffer into the sk_buff
      memcpy(skb_put(rx_packet->skb, rx_packet_len), 
             rx_byte_buffer, rx_packet_len);

      //Zero out the buffer
      memset(rx_byte_buffer, 0, ETH_DATA_LEN + 10);

      //Printout payload for debugging purposes
      /*
      for(i = 0;i < rx_packet->skb->len;i++){
        printk(KERN_INFO "%c", rx_packet->skb->data[i]);
      }
      printk(KERN_INFO "\n");
      */
      
      //Update statistics
      priv->stats.rx_packets++;
      priv->stats.rx_bytes += rx_packet->skb->len;

      //Write metadata, and pass to higher layers
      rx_packet->skb->dev = vlc_dev;
      rx_packet->skb->protocol = eth_type_trans(rx_packet->skb, vlc_dev);
      rx_packet->skb->ip_summed = CHECKSUM_NONE;
      if(netif_rx(rx_packet->skb) != NET_RX_SUCCESS) printk(KERN_NOTICE "VLC: Packet droped when passed to upper layers.\n");

      //Cleanup the packet
      kfree(rx_packet);
    }
  }

 end_rx:
  return;
}

/* Open the VLC device. Runs when ifconfig "ups" the device.
 */
int vlc_open(struct net_device *dev)
{
  printk(KERN_INFO "VLC: Opening device.\n");

  memcpy(dev->dev_addr, VLC_MAC_ADDR, ETH_ALEN);
  netif_start_queue(dev);
  return 0;
}

/* Release the VLC device. Runs when ifconfig "downs" the device.
 */
int vlc_release(struct net_device *dev)
{
  printk(KERN_INFO "VLC: Releasing device.\n");

  netif_stop_queue(dev);
  return 0;
}

/* Called by the kernel whenever it wants to send a packet.
 */
int vlc_tx(struct sk_buff *skb, struct net_device *dev)
{
  printk(KERN_INFO "VLC: Outgoing packet recieved [%d]!\n", skb->len);

  //Check if we currently have a packet queued
  if(tx_packet == NULL){

    //Allocate a packet
    tx_packet = kmalloc(sizeof(struct vlc_packet), GFP_KERNEL);
    if(tx_packet == NULL) goto out_free_skb;

    //Save the timestamp
    dev->trans_start = jiffies;

    //Make sure we own the sk_buff
    skb_orphan(skb);
    
    //Store the sk_buff in our queued packet
    tx_packet->skb = skb;

    netif_stop_queue(vlc_dev);
  }

  return 0;

 out_free_skb:
  dev_kfree_skb(skb);
  return -1;
}

/* Should be used to send received packets to upper layers. Not
 * currently used.
 */
void vlc_rx(struct net_device *dev, struct vlc_packet _pkt)
{
  return;
}

/* Struct for keeping track of the device stats.
 */
struct net_device_stats *vlc_stats(struct net_device *dev)
{
  struct vlc_priv *priv = netdev_priv(dev);
  return &priv->stats;
}

/* Struct for keeping track of which functions the kernel should call
 * for what job.
 */
static const struct net_device_ops vlc_netdev_ops = {
  .ndo_open = vlc_open,
  .ndo_stop = vlc_release,
  .ndo_start_xmit = vlc_tx,
  .ndo_get_stats = vlc_stats,
};

/* 
 * Initialize the VLC driver module. Runs at module insertion.
 */
static int __init vlc_init_module(void)
{
  int ret = -ENOMEM;
  struct vlc_priv *priv = NULL;
  long int slot_ns = 0;

  printk(KERN_INFO "VLC: Initializing module...\n");

  //Allocate a net_device. This function allocates it with the
  //etherdevice standard values.
  vlc_dev = alloc_etherdev(sizeof(struct vlc_priv));
  if(!vlc_dev) goto out_free_netdev;

  //Override some of the values.
  vlc_dev->netdev_ops = &vlc_netdev_ops;
  //vlc_dev->flags |= IFF_NOARP;
  vlc_dev->features |= NETIF_F_HW_CSUM;

  //Init the vlc_priv struct
  priv = netdev_priv(vlc_dev);
  memset(priv, 0, sizeof(struct vlc_priv));
  priv->dev = vlc_dev;
  spin_lock_init(&priv->lock);

  //Setup the GPIO
  if ( gpio_request(GPIO_OUTPUT, "GPIO_OUTPUT")
       || gpio_request(GPIO_INPUT, "GPIO_INPUT") ) {
    printk("VLC: Request GPIO failed!\n");
    goto out_free_netdev;
  }

  gpio_direction_output(GPIO_OUTPUT, GPIOF_INIT_HIGH);
  gpio_direction_input(GPIO_INPUT);

  gpio1 = ioremap(ADDR_BASE_0, 4);
  gpio2 = ioremap(ADDR_BASE_1, 4);

  if (!(gpio1 && gpio2)){
    printk(KERN_INFO "VLC: Failed to remap I/O\n");
    goto out_free_netdev;
  }

  //Init the timer
  ret = rtdm_timer_init(&phy_timer, timer_handler, "phy timer");
  if(ret) goto out_free_netdev;

  //Start the timer
  slot_ns = 1000000000 / FREQ;
  ret = rtdm_timer_start(&phy_timer, slot_ns, slot_ns, RTDM_TIMERMODE_RELATIVE);
  if(ret) goto out_free_netdev;

  //Register the net device.
  ret = register_netdev(vlc_dev);
  if(ret) goto out_free_netdev;
  
  printk(KERN_INFO "VLC: Module initialized!\n");
  return 0;

 out_free_netdev:
  if(gpio1) iounmap(gpio1);
  if(gpio2) iounmap(gpio2);

  free_netdev(vlc_dev);

 out:
  return ret;
}

/*
 * Cleanup the VLC driver module. Runs at module removal.
 */
static void __exit vlc_cleanup_module(void)
{
  printk(KERN_INFO "VLC: Removing module...\n");

  iounmap(gpio1);
  iounmap(gpio2);

  gpio_free(GPIO_OUTPUT);
  gpio_free(GPIO_INPUT);

  //Unregister and free net device
  if(vlc_dev){
    unregister_netdev(vlc_dev);
    free_netdev(vlc_dev);
  }

  //Free the current packet
  if(tx_packet){
    kfree(tx_packet);
  }

  //Destroy the timer
  rtdm_timer_destroy(&phy_timer);

  printk(KERN_INFO "VLC: Module removed\n");

  return;
}

/* Define the routines to be run when the module is inserted and
 * removed respetively.
 */
module_init(vlc_init_module);
module_exit(vlc_cleanup_module);
