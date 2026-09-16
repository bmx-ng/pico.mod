#ifndef BMX_PICO_LWIPOPTS_H
#define BMX_PICO_LWIPOPTS_H

/* Bare-metal, event-driven lwIP. The native sequential and BSD socket APIs
   require an operating-system layer and remain disabled. Pub.Net supplies its
   compatible Pico API through a compact adapter over lwIP's raw callbacks. */
#define NO_SYS 1
#define SYS_LIGHTWEIGHT_PROT 0
#define LWIP_RAW 1
#define LWIP_NETCONN 0
#define LWIP_SOCKET 0

#define MEM_ALIGNMENT 4
#define MEM_SIZE (8 * 1024)
#define MEMP_NUM_PBUF 8
#define MEMP_NUM_UDP_PCB 4
#define MEMP_NUM_TCP_PCB 4
#define MEMP_NUM_TCP_PCB_LISTEN 2
#define MEMP_NUM_TCP_SEG 16
#define PBUF_POOL_SIZE 8

#define LWIP_IPV4 1
#define LWIP_IPV6 0
#define LWIP_ARP 1
#define LWIP_ETHERNET 1
#define LWIP_ICMP 1
#define LWIP_UDP 1
#define LWIP_TCP 1
#define LWIP_DHCP 1
#define LWIP_DNS 1
#define LWIP_NETIF_HOSTNAME 1
#define LWIP_NETIF_STATUS_CALLBACK 1
#define LWIP_NETIF_LINK_CALLBACK 1
#define LWIP_SINGLE_NETIF 0
#define LWIP_STATS 0

#define TCP_MSS (1500 - 20 - 20)
#define TCP_SND_BUF (4 * TCP_MSS)
#define TCP_SND_QUEUELEN ((2 * TCP_SND_BUF) / TCP_MSS)
#define TCP_WND (4 * TCP_MSS)
#define TCP_QUEUE_OOSEQ 0

#include "pico/rand.h"
#define LWIP_RAND() get_rand_32()

#endif
