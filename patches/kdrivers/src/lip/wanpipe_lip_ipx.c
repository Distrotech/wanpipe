#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
# include <wanpipe_lip.h>
#elif defined(__WINDOWS__)
#include "wanpipe_lip.h"
#else
# include <linux/wanpipe_lip.h>
#endif

#define CVHexToAscii(b) (((unsigned char)(b) > (unsigned char)9) ? ((unsigned char)'A' + ((unsigned char)(b) - (unsigned char)10)) : ((unsigned char)'0' + (unsigned char)(b)))


/*
   If incoming is 0 (outgoing)- if the net numbers is ours make it 0
   if incoming is 1 - if the net number is 0 make it ours 

*/
void wplip_ipxwan_restore_net_num(unsigned char *sendpacket, 
				  unsigned long orig_dnet,
				  unsigned long orig_snet)
{
	sendpacket[6] = (unsigned char)(orig_dnet >> 24);
	sendpacket[7] = (unsigned char)((orig_dnet & 
					 0x00FF0000) >> 16);
	sendpacket[8] = (unsigned char)((orig_dnet & 
				 0x0000FF00) >> 8);
	sendpacket[9] = (unsigned char)(orig_dnet & 
				 0x000000FF);

	sendpacket[18] = (unsigned char)(orig_snet >> 24);
	sendpacket[19] = (unsigned char)((orig_snet & 
				 0x00FF0000) >> 16);
	sendpacket[20] = (unsigned char)((orig_snet & 
				 0x0000FF00) >> 8);
	sendpacket[21] = (unsigned char)(orig_snet & 
				 0x000000FF);
	
}


void wplip_ipxwan_switch_net_num(unsigned char *sendpacket, 
		                unsigned long network_number, 
				unsigned long *orig_dnet,
				unsigned long *orig_snet,
			        unsigned char incoming)
{
	unsigned long pnetwork_number;

	pnetwork_number = (unsigned long)((sendpacket[6] << 24) + 
			  (sendpacket[7] << 16) + (sendpacket[8] << 8) + 
			   sendpacket[9]);
	
	*orig_dnet=pnetwork_number;
	
	
	if (!incoming) {
		/* If the destination network number is ours, make it 0 */
		if( pnetwork_number == network_number) {
			sendpacket[6] = sendpacket[7] = sendpacket[8] = 
					 sendpacket[9] = 0x00;
		}
	} else {
		/* If the incoming network is 0, make it ours */
		if( pnetwork_number == 0) {
			sendpacket[6] = (unsigned char)(network_number >> 24);
			sendpacket[7] = (unsigned char)((network_number & 
					 0x00FF0000) >> 16);
			sendpacket[8] = (unsigned char)((network_number & 
					 0x0000FF00) >> 8);
			sendpacket[9] = (unsigned char)(network_number & 
					 0x000000FF);
		}
	}


	pnetwork_number = (unsigned long)((sendpacket[18] << 24) + 
			  (sendpacket[19] << 16) + (sendpacket[20] << 8) + 
			  sendpacket[21]);

	*orig_snet=pnetwork_number;
	
	if( !incoming ) {
		/* If the source network is ours, make it 0 */
		if( pnetwork_number == network_number) {
			sendpacket[18] = sendpacket[19] = sendpacket[20] = 
					 sendpacket[21] = 0x00;
		}
	} else {
		/* If the source network is 0, make it ours */
		if( pnetwork_number == 0 ) {
			sendpacket[18] = (unsigned char)(network_number >> 24);
			sendpacket[19] = (unsigned char)((network_number & 
					 0x00FF0000) >> 16);
			sendpacket[20] = (unsigned char)((network_number & 
					 0x0000FF00) >> 8);
			sendpacket[21] = (unsigned char)(network_number & 
					 0x000000FF);
		}
	}
} /* switch_net_numbers */


int wplip_handle_ipxwan(wplip_dev_t *lip_dev, void *skb)
{
	int i;
	unsigned char *sendpacket=wan_skb_data(skb);
	unsigned long network_number = lip_dev->ipx_net_num;
	unsigned long dnet,snet;

	if( sendpacket[16] == 0x90 && sendpacket[17] == 0x04){
		/* It's IPXWAN */

		if( sendpacket[2] == 0x02 && sendpacket[32] == 0x00){

			/* It's a timer request packet */
			DEBUG_EVENT( "%s: Received IPXWAN Timer Request packet\n",
					lip_dev->name);

			/* Go through the routing options and answer no to every
			 * option except Unnumbered RIP/SAP
			 */
			for(i = 41; sendpacket[i] == 0x00; i += 5){
				/* 0x02 is the option for Unnumbered RIP/SAP */
				if( sendpacket[i + 4] != 0x02){
					sendpacket[i + 1] = 0;
				}
			}

			/* Skip over the extended Node ID option */
			if( sendpacket[i] == 0x04 ){
				i += 8;
			}

			/* We also want to turn off all header compression opt.
			 */
			for(; sendpacket[i] == 0x80 ;){
				sendpacket[i + 1] = 0;
				i += (sendpacket[i + 2] << 8) + (sendpacket[i + 3]) + 4;
			}

			/* Set the packet type to timer response */
			sendpacket[34] = 0x01;

			DEBUG_EVENT( "%s: Sending IPXWAN Timer Response\n",
					lip_dev->name);

		} else if( sendpacket[34] == 0x02 ){

			/* This is an information request packet */
			DEBUG_EVENT( 
				"%s: Received IPXWAN Information Request packet\n",
						lip_dev->name);

			/* Set the packet type to information response */
			sendpacket[34] = 0x03;

			/* Set the router name */
			sendpacket[51] = 'F';
			sendpacket[52] = 'P';
			sendpacket[53] = 'I';
			sendpacket[54] = 'P';
			sendpacket[55] = 'E';
			sendpacket[56] = '-';
			sendpacket[57] = CVHexToAscii(network_number >> 28);
			sendpacket[58] = CVHexToAscii((network_number & 0x0F000000)>> 24);
			sendpacket[59] = CVHexToAscii((network_number & 0x00F00000)>> 20);
			sendpacket[60] = CVHexToAscii((network_number & 0x000F0000)>> 16);
			sendpacket[61] = CVHexToAscii((network_number & 0x0000F000)>> 12);
			sendpacket[62] = CVHexToAscii((network_number & 0x00000F00)>> 8);
			sendpacket[63] = CVHexToAscii((network_number & 0x000000F0)>> 4);
			sendpacket[64] = CVHexToAscii(network_number & 0x0000000F);
			for(i = 65; i < 99; i+= 1)
			{
				sendpacket[i] = 0;
			}

			DEBUG_EVENT( "%s: Sending IPXWAN Information Response packet\n",
					lip_dev->name);
		} else {

			DEBUG_EVENT( "%s: Unknown IPXWAN packet!\n",lip_dev->name);
			return 0;
		}

		/* Set the WNodeID to our network address */
		sendpacket[35] = (unsigned char)(network_number >> 24);
		sendpacket[36] = (unsigned char)((network_number & 0x00FF0000) >> 16);
		sendpacket[37] = (unsigned char)((network_number & 0x0000FF00) >> 8);
		sendpacket[38] = (unsigned char)(network_number & 0x000000FF);

		return 1;
	}

	/* If we get here, its an IPX-data packet so it'll get passed up the 
	 * stack.
	 * switch the network numbers 
	 */
	wplip_ipxwan_switch_net_num(&sendpacket[0], network_number,  
			           &dnet, &snet ,1);
	return 0;
}

