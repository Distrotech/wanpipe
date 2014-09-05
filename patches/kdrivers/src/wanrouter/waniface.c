/*****************************************************************************
* waniface.c	WAN Multiprotocol Interface Module. Main code.
*
* Author:	Nenad Corbic
*
* Copyright:	(c) 1995-2002 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
*/

#include <linux/wanpipe_includes.h>
#include <linux/wanpipe_defines.h>
#include <linux/wanpipe_debug.h>
#include <linux/wanpipe_common.h>
#include <linux/wanpipe.h>	/* WAN router API definitions */
#include <linux/if_wanpipe.h>

#include <linux/wanpipe_lapb_kernel.h>
#include <linux/wanpipe_x25_kernel.h>
#include <linux/wanpipe_dsp_kernel.h>
#include <linux/wanpipe_lip_kernel.h>

extern struct wanpipe_lapb_register_struct lapb_protocol;
extern struct wanpipe_x25_register_struct x25_protocol;
extern struct wanpipe_dsp_register_struct dsp_protocol;
extern struct wanpipe_api_register_struct api_socket;
extern struct wanpipe_fw_register_struct  wp_fw_protocol;
extern struct wplip_reg			  wplip_protocol;

/*===================================== 
 * Interfaces to Protocol Modules 
 *====================================*/

int bind_api_to_protocol (netdevice_t *dev, char *devname, unsigned short protocol, void *sk_id)
{
	WAN_ASSERT2((!sk_id),-ENODEV);

	switch (htons(protocol)){
	
		case ETH_P_X25:
	
			WAN_ASSERT2((!dev),-ENODEV);
			
			if (!IS_FUNC_CALL(x25_protocol,x25_bind_api_to_svc))
				return -EPROTONOSUPPORT;
			
			return x25_protocol.x25_bind_api_to_svc(dev,sk_id);

		case DSP_PROT:

			WAN_ASSERT2((!dev),-ENODEV);

			if (!IS_FUNC_CALL(dsp_protocol, dsp_bind_api))
				return -EPROTONOSUPPORT;
			return dsp_protocol.dsp_bind_api(dev, sk_id);

		case WP_X25_PROT:

			WAN_ASSERT2((!devname),-ENODEV);

			if (!IS_FUNC_CALL(wp_fw_protocol,bind_api_to_svc))
				return -EPROTONOSUPPORT;
			
			return wp_fw_protocol.bind_api_to_svc(devname,sk_id);
		
	}

	printk(KERN_INFO "Protocol not supported %i!\n",htons(protocol));
	return -EPROTONOSUPPORT;
}

int bind_api_listen_to_protocol (netdevice_t *dev, char *devname, unsigned short protocol, void *sk_id)
{
	
	WAN_ASSERT2((!sk_id),-ENODEV);

	switch (htons(protocol)){

		case DSP_PROT:
		case ETH_P_X25:

			WAN_ASSERT2((!dev),-ENODEV);
			
			if (!IS_FUNC_CALL(x25_protocol,x25_bind_listen_to_link))
				return -EPROTONOSUPPORT;
			
			return x25_protocol.x25_bind_listen_to_link(dev,sk_id,htons(protocol));

		case WP_X25_PROT:

			WAN_ASSERT2((!devname),-ENODEV);
			
			if (!IS_FUNC_CALL(wp_fw_protocol,bind_listen_to_link))
				return -EPROTONOSUPPORT;
			
			return wp_fw_protocol.bind_listen_to_link(devname,sk_id,htons(protocol));

	}

	printk(KERN_INFO "Protocol not supported %i!\n",htons(protocol));
	return -EPROTONOSUPPORT;
}

int unbind_api_listen_from_protocol (unsigned short protocol, void *sk_id)
{
	WAN_ASSERT2((!sk_id),-ENODEV);
	
	switch (htons(protocol)){
	
		case DSP_PROT:
		case ETH_P_X25:
			if (!IS_FUNC_CALL(x25_protocol,x25_unbind_listen_from_link))
				return -EPROTONOSUPPORT;
			
			return x25_protocol.x25_unbind_listen_from_link(sk_id, htons(protocol));

		case WP_X25_PROT:
			if (!IS_FUNC_CALL(wp_fw_protocol,unbind_listen_from_link))
				return -EPROTONOSUPPORT;
			
			return wp_fw_protocol.unbind_listen_from_link(sk_id, htons(protocol));	
			
	}

	printk(KERN_INFO "Protocol not supported %i!\n",htons(protocol));
	return -EPROTONOSUPPORT;
}


/*================================================
 * Interface to LIP Layer
 */
int wanpipe_lip_rx(void *chan_ptr, void *skb)
{
	wanpipe_common_t *chan = (wanpipe_common_t*)chan_ptr;
	
	if (!chan->lip){
		return -ENODEV;
	}
	if (!IS_FUNC_CALL(wplip_protocol,wplip_rx))
		return -EPROTONOSUPPORT;

	return wplip_protocol.wplip_rx(chan->lip,skb);
}

int wanpipe_lip_connect(void *chan_ptr, int reason)
{
	wanpipe_common_t *chan = (wanpipe_common_t*)chan_ptr;
	
	if (!chan->lip){
		return -ENODEV;
	}
	if (!IS_FUNC_CALL(wplip_protocol,wplip_connect))
		return -EPROTONOSUPPORT;

	 wplip_protocol.wplip_connect(chan->lip,reason);

	 return 0;
}

int wanpipe_lip_disconnect(void *chan_ptr, int reason)
{
	wanpipe_common_t *chan = (wanpipe_common_t*)chan_ptr;
	
	if (!chan->lip){
		return -ENODEV;
	}
	if (!IS_FUNC_CALL(wplip_protocol,wplip_disconnect))
		return -EPROTONOSUPPORT;

	wplip_protocol.wplip_disconnect(chan->lip,reason);
	return 0;
}

int wanpipe_lip_kick(void *chan_ptr,int reason)
{
	wanpipe_common_t *chan = (wanpipe_common_t*)chan_ptr;
	
	if (!chan->lip){
		return -ENODEV;
	}
	if (!IS_FUNC_CALL(wplip_protocol,wplip_kick))
		return -EPROTONOSUPPORT;

	wplip_protocol.wplip_kick(chan->lip,reason);
	return 0;
}
	
int wanpipe_lip_get_if_status(void *chan_ptr,void *m)
{
	wanpipe_common_t *chan = (wanpipe_common_t*)chan_ptr;
	
	if (!chan->lip){
		return -ENODEV;
	}
	if (!IS_FUNC_CALL(wplip_protocol,wplip_get_if_status))
		return -EPROTONOSUPPORT;

	wplip_protocol.wplip_get_if_status(chan->lip,m);
	return 0;


}

/*==========================================
 * Interface to socket API 
 *=========================================*/

int protocol_connected (netdevice_t *dev, void *sk_id)
{
	WAN_ASSERT2((!sk_id),-ENODEV);
	WAN_ASSERT2((!dev),-ENODEV);
	
	if (!IS_FUNC_CALL(api_socket,wanpipe_api_connected))
		return -ENODEV;

	return api_socket.wanpipe_api_connected(dev,sk_id);
}

int protocol_connecting (netdevice_t *dev, void *sk_id)
{
	WAN_ASSERT2((!sk_id),-ENODEV);
	WAN_ASSERT2((!dev),-ENODEV);
	
	if (!IS_FUNC_CALL(api_socket,wanpipe_api_connecting))
		return -ENODEV;

	return api_socket.wanpipe_api_connecting(dev,sk_id);
}


int protocol_disconnected (void *sk_id)
{
	WAN_ASSERT2((!sk_id),-ENODEV);
	
	if (!IS_FUNC_CALL(api_socket,wanpipe_api_disconnected))
		return -ENODEV;

	return api_socket.wanpipe_api_disconnected(sk_id);
}

int wanpipe_api_sock_rx (struct sk_buff *skb, netdevice_t *dev, void *sk_id)
{
	WAN_ASSERT2((!sk_id),-ENODEV);
	WAN_ASSERT2((!dev),-ENODEV);
	WAN_ASSERT2((!skb),-ENOBUFS);

	if (!IS_FUNC_CALL(api_socket,wanpipe_api_sock_rcv)){
		return -ENODEV;
	}
	
	return api_socket.wanpipe_api_sock_rcv(skb, dev, (struct sock *)sk_id);
}

int wanpipe_api_listen_rx (struct sk_buff *skb, void *sk_id)
{
	WAN_ASSERT2((!sk_id),-ENODEV);
	
	/* It is ok if skb==NULL, the wanpipe_listen_rcv function
	 * supports it */
	
	if (!IS_FUNC_CALL(api_socket,wanpipe_listen_rcv)){
		printk(KERN_INFO "WANIFACE: No listen_rcv binded !\n");
		return -ENODEV;
	}

	return api_socket.wanpipe_listen_rcv(skb, (struct sock *)sk_id);
}

int wanpipe_api_buf_check (void *sk_id, int len)
{
	WAN_ASSERT2((!sk_id),-ENODEV);

	if (!IS_FUNC_CALL(api_socket,sk_buf_check))
		return -ENODEV;
	
	return api_socket.sk_buf_check((struct sock *)sk_id,len);
}


int wanpipe_api_poll_wake (void *sk_id)
{
	WAN_ASSERT2((!sk_id),-ENODEV);

	if (!IS_FUNC_CALL(api_socket,sk_poll_wake))
		return -ENODEV;

	return api_socket.sk_poll_wake((struct sock *)sk_id);
}

EXPORT_SYMBOL(bind_api_to_protocol);
EXPORT_SYMBOL(wanpipe_api_sock_rx);
EXPORT_SYMBOL(protocol_disconnected);
EXPORT_SYMBOL(protocol_connected);
EXPORT_SYMBOL(protocol_connecting);
EXPORT_SYMBOL(wanpipe_api_listen_rx);
EXPORT_SYMBOL(unbind_api_listen_from_protocol);
EXPORT_SYMBOL(bind_api_listen_to_protocol);
EXPORT_SYMBOL(wanpipe_api_buf_check);
EXPORT_SYMBOL(wanpipe_api_poll_wake);

