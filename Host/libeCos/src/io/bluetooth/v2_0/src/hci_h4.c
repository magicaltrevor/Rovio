/* 
   BlueZ - Bluetooth protocol stack for Linux
   Copyright (C) 2000-2001 Qualcomm Incorporated

   Written 2000,2001 by Maxim Krasnyansky <maxk@qualcomm.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation;

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
   IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY
   CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES 
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF 
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

   ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS, 
   COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS 
   SOFTWARE IS DISCLAIMED.
*/

/*
 * BlueZ HCI UART(H4) protocol.
 *
 * $Id: hci_h4.c,v 1.3 2002/09/09 01:17:32 maxk Exp $    
 */
#ifdef CYGPKG_NET_BLUEZ_STACK

#define VERSION "1.2"

#include "pkgconf/io.h"

#include "cyg/io/io.h"
#include "cyg/io/devtab.h"
#include "cyg/io/ttyio.h"

//#include <linux/config.h>
//#include <linux/module.h>

//#include <linux/version.h>
#include "linux/kernel.h"
//#include <linux/init.h>
#include "linux/sched.h"
#include "linux/types.h"
//#include <linux/fcntl.h>
//#include <linux/interrupt.h>
//#include <linux/ptrace.h>
#include "asm/poll.h"

#include "linux/slab.h"
//#include <linux/tty.h>
#include "linux/errno.h"
//#include <linux/string.h>
//#include <linux/signal.h>
#include "sys/ioctl.h"
//#include <linux/skbuff.h>

#include "net/bluetooth/bluetooth.h"
#include "net/bluetooth/hci_core.h"
#include "hci_uart.h"
#include "hci_h4.h"

//#define HCI_UART_DEBUG

#ifndef HCI_UART_DEBUG
#undef  BT_DBG
#define BT_DBG( A... )
#undef  BT_DMP
#define BT_DMP( A... )
#endif

/* Initialize protocol */
static int h4_open(struct hci_uart *hu)
{
	struct h4_struct *h4;
	
	BT_DBG("hu %p", hu);
	
	h4 = malloc(sizeof(*h4));
	if (!h4)
		return -ENOMEM;
	memset(h4, 0, sizeof(*h4));

	skb_queue_head_init(&h4->txq);

	hu->priv = h4;
	return 0;
}

/* Flush protocol data */
static int h4_flush(struct hci_uart *hu)
{
	struct h4_struct *h4 = hu->priv;

	BT_DBG("hu %p", hu);
	skb_queue_purge(&h4->txq);
	return 0;
}

/* Close protocol */
static int h4_close(struct hci_uart *hu)
{
	struct h4_struct *h4 = hu->priv;
	hu->priv = NULL;

	BT_DBG("hu %p", hu);

	skb_queue_purge(&h4->txq);
	if (h4->rx_skb)
		kfree_skb(h4->rx_skb);

	hu->priv = NULL;
	free(h4);
	return 0;
}

/* Enqueue frame for transmittion (padding, crc, etc) */
static int h4_enqueue(struct hci_uart *hu, struct sk_buff *skb)
{
	struct h4_struct *h4 = hu->priv;

	BT_DBG("hu %p skb %p", hu, skb);

	/* Prepend skb with frame type */
	memcpy(skb_push(skb, 1), &skb->pkt_type, 1);
	skb_queue_tail(&h4->txq, skb);
	return 0;
}

static inline int h4_check_data_len(struct h4_struct *h4, int len)
{
	register int room = skb_tailroom(h4->rx_skb);

	BT_DBG("len %d room %d", len, room);
	if (!len) {
		BT_DMP(h4->rx_skb->data, h4->rx_skb->len);
		hci_recv_frame(h4->rx_skb);
	} else if (len > room) {
		//BT_ERR("Data length is too large");
		printf("%s: ""Data length is too large" "\n" , __func__ );
		kfree_skb(h4->rx_skb);
	} else {
		h4->rx_state = H4_W4_DATA;
		h4->rx_count = len;
		return len;
	}

	h4->rx_state = H4_W4_PACKET_TYPE;
	h4->rx_skb   = NULL;
	h4->rx_count = 0;
	return 0;
}

/* Recv data */
static int h4_recv(struct hci_uart *hu, void *data, int count)
{
	struct h4_struct *h4 = hu->priv;
	register char *ptr;
	hci_event_hdr *eh;
	hci_acl_hdr   *ah;
	hci_sco_hdr   *sh;
	register int len, type, dlen;
	char *p;

	BT_DBG("hu %p count %d rx_state %ld rx_count %ld", 
			hu, count, h4->rx_state, h4->rx_count);
p = data;
printf("h4: %x,%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x,count=%d,rx_count=%d",p,
			p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7],p[8],p[9],p[10],p[11],
			p[12],p[13],p[14],p[15],count,h4->rx_count);
	ptr = data;
	while (count) {
		if (h4->rx_count) {
			len = MIN(h4->rx_count, count);
			memcpy(skb_put(h4->rx_skb, len), ptr, len);
			h4->rx_count -= len; count -= len; ptr += len;
//printf("tail=%x,end=%x",h4->rx_skb->tail,h4->rx_skb->end);
			if (h4->rx_count)
				continue;

			switch (h4->rx_state) {
			case H4_W4_DATA:
				//BT_DBG
				printf("Complete data",h4->rx_skb->data);

				BT_DMP(h4->rx_skb->data, h4->rx_skb->len);

				hci_recv_frame(h4->rx_skb);

				h4->rx_state = H4_W4_PACKET_TYPE;
				h4->rx_skb = NULL;
				continue;

			case H4_W4_EVENT_HDR:
				eh = (hci_event_hdr *) h4->rx_skb->data;

				//BT_DBG
				printf("h4:Event header: evt 0x%2.2x plen %d", eh->evt, eh->plen);

				h4_check_data_len(h4, eh->plen);
				continue;

			case H4_W4_ACL_HDR:
				ah = (hci_acl_hdr *) h4->rx_skb->data;
				dlen = __le16_to_cpu(ah->dlen);

				//BT_DBG
				printf("h4:ACL header: dlen %d", dlen);

				h4_check_data_len(h4, dlen);
				continue;

			case H4_W4_SCO_HDR:
				sh = (hci_sco_hdr *) h4->rx_skb->data;

				BT_DBG("SCO header: dlen %d", sh->dlen);

				h4_check_data_len(h4, sh->dlen);
				continue;
			}
		}

		/* H4_W4_PACKET_TYPE */
		switch (*ptr) {
		case HCI_EVENT_PKT:
			//BT_DBG
			printf("h4:Event packet\n");
			h4->rx_state = H4_W4_EVENT_HDR;
			h4->rx_count = HCI_EVENT_HDR_SIZE;
			type = HCI_EVENT_PKT;
			break;

		case HCI_ACLDATA_PKT:
			//BT_DBG
			printf("h4:ACL packet\n");
			h4->rx_state = H4_W4_ACL_HDR;
			h4->rx_count = HCI_ACL_HDR_SIZE;
			type = HCI_ACLDATA_PKT;
			break;

		case HCI_SCODATA_PKT:
			//BT_DBG
			printf("h4:SCO packet\n");
			h4->rx_state = H4_W4_SCO_HDR;
			h4->rx_count = HCI_SCO_HDR_SIZE;
			type = HCI_SCODATA_PKT;
			break;

		default:
			//BT_ERR
			printf("h4:Unknown HCI packet type %2.2x\n", (cyg_uint8)*ptr);
			hu->hdev.stat.err_rx++;
			ptr++; count--;
			continue;
		};
		ptr++; count--;

		/* Allocate packet */
		h4->rx_skb = bluez_skb_alloc(HCI_MAX_FRAME_SIZE, 0);
		//printf("h4->rx_skb=%x,data=%x,head=%x,end=%x;",h4->rx_skb,h4->rx_skb->data,h4->rx_skb->head,h4->rx_skb->end);
		if (!h4->rx_skb) {
			//BT_ERR
			printf("Can't allocate mem for new packet:%d",h4->rx_skb);
			h4->rx_state = H4_W4_PACKET_TYPE;
			h4->rx_count = 0;
			return 0;
		}
		h4->rx_skb->dev = (void *) &hu->hdev;
		h4->rx_skb->pkt_type = type;
	}
	return count;
}

static struct sk_buff *h4_dequeue(struct hci_uart *hu)
{
	struct h4_struct *h4 = hu->priv;
	return skb_dequeue(&h4->txq);
}

static struct hci_uart_proto h4p = {
	/*id:      */HCI_UART_H4,
	/*open:    */h4_open,
	/*close:   */h4_close,
	/*flush:   */h4_flush,
	/*recv:    */h4_recv,
	/*enqueue: */h4_enqueue,
	/*dequeue: */h4_dequeue,
	
};
	      
int h4_init(void)
{
	return hci_uart_register_proto(&h4p);
}

int h4_deinit(void)
{
	return hci_uart_unregister_proto(&h4p);
}
#endif
