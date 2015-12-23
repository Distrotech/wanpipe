
#include "sangoma_mgd.h"
static int bridge_threads=0;

#define TEST_SEQ 0

static void *bridge_thread_run(void *obj)
{
	struct smg_tdm_ip_bridge *bridge= (struct smg_tdm_ip_bridge*)obj;
	call_signal_connection_t *mcon = &bridge->mcon;
	wanpipe_tdm_api_t tdm_api;
	int ss = 0;
	char a=0,b=0;
	int bytes=0,err;
	unsigned char data[1024];
	unsigned int fromlen = sizeof(struct sockaddr_in);
	sangoma_api_hdr_t hdrframe;
	unsigned int udp_rx=0,udp_tx=0,tdm_rx=0,tdm_tx=0;
	unsigned int udp_rx_err=0, udp_tx_err=0;
	unsigned int tdm_rx_err=0, tdm_tx_err=0;
	int bridge_ip_sync=0;
	int err_flag=0;
	unsigned char prev_status=0;
#if TEST_SEQ
	unsigned char tx_seq_cnt=0;
	unsigned char rx_seq_cnt=0;
	int i;
	int insync=0;
#endif

	memset(&hdrframe,0,sizeof(hdrframe));
	memset(data,0,sizeof(data));
	memset(&tdm_api,0,sizeof(tdm_api));

	if (bridge->period) {
		err=sangoma_tdm_set_usr_period(bridge->tdm_fd, &tdm_api,bridge->period);
		if (err) {
			log_printf(SMG_LOG_ALL,server.log,"%s: Failed to execute set period %i\n",__FUNCTION__,bridge->period);
		}
	}

	sangoma_tdm_disable_hwec(bridge->tdm_fd, &tdm_api);

	err=sangoma_tdm_flush_bufs(bridge->tdm_fd, &tdm_api);
	if (err) {
		log_printf(SMG_LOG_ALL,server.log,"%s: Failed to execute tdm flush\n",__FUNCTION__);
	}

	while (!bridge->end &&
		   woomera_test_flag(&server.master_connection, WFLAG_RUNNING))  {

		err_flag=0;

		ss = waitfor_2sockets(mcon->socket,
				      bridge->tdm_fd,
				      &a, &b, 1000);

		if (ss > 0) {

			if (a) {
				
				bytes = recvfrom(mcon->socket, &data[0], sizeof(data), MSG_DONTWAIT,
							(struct sockaddr *) &mcon->local_addr, &fromlen);

#if TEST_SEQ
				for (i=0;i<bytes;i++) {
					if (data[i] != rx_seq_cnt) {
						if (insync) {
						log_printf(SMG_LOG_ALL,server.log,"Error: Data rx seq cnt %i expected %i\n",data[i],rx_seq_cnt);
						}
						rx_seq_cnt=data[i];
						insync=0;
					} else if (insync == 0) {
						log_printf(SMG_LOG_ALL,server.log,"In sync\n");
						insync=1;
					}
					rx_seq_cnt++;
				}
				

				for (i=0;i<bytes;i++) {
					data[i]=tx_seq_cnt++;
				}
#endif

				if (bytes > 0) {

					if (bridge_ip_sync == 0) {
						bridge_ip_sync=1;
						log_printf(SMG_LOG_ALL,server.log,"Bridge IP Sync: span=%i chan=%i port=%d len=%i\n",bridge->span,bridge->chan,bridge->mcon.cfg.local_port,bytes);
					}
					udp_rx++;

					err=sangoma_sendmsg_socket(bridge->tdm_fd, 
							&hdrframe, 
							sizeof(hdrframe), 
							data, 
							bytes, 0);
					if (err != bytes) {
						
						unsigned char current_status;
						unsigned char verbose=1;

						sangoma_tdm_get_link_status(bridge->tdm_fd, &tdm_api, &current_status);
						if (current_status != WP_TDMAPI_EVENT_LINK_STATUS_CONNECTED) {
							if (prev_status == current_status) {
								verbose=0;
							} 
						}

						prev_status = current_status;

						if (verbose) {
						log_printf(SMG_LOG_ALL,server.log,"%s: Error: Bridge tdm write failed (span=%i,chan=%i)! len=%i status=%s - %s\n",
							__FUNCTION__,bridge->span,bridge->chan,bytes, current_status==WP_TDMAPI_EVENT_LINK_STATUS_CONNECTED?"Connected":"Disconnected",
							strerror(errno));
						sangoma_tdm_flush_bufs(bridge->tdm_fd, &tdm_api);
						err_flag++;
						}

						tdm_tx_err++;
						
					} else {
						tdm_tx++;
					}
				} else {
					log_printf(SMG_LOG_ALL,server.log,"%s: Error: Bridge sctp read failed (span=%i,chan=%i)! len=%i - %s\n",
							__FUNCTION__,bridge->span,bridge->chan,bytes,strerror(errno));
					udp_rx_err++;
					err_flag++;
				}
			}

			if (b) {
				bytes = sangoma_readmsg_socket(bridge->tdm_fd,
					&hdrframe,
					sizeof(hdrframe),
					data,
					sizeof(data), 0);

				if (bytes > 0) {
					tdm_rx++;
					err=sendto(mcon->socket,
						data,
						bytes, 0,
						(struct sockaddr *) &mcon->remote_addr,
						sizeof(mcon->remote_addr));
					if (err != bytes) {
						log_printf(SMG_LOG_ALL,server.log,"%s: Error: Bridge sctp write failed (span=%i,chan=%i)! len=%i - %s\n",__FUNCTION__,bridge->span,bridge->chan,bytes,strerror(errno));
						udp_tx_err++;
						err_flag++;
					} else {
						udp_tx++;
					}
				} else {
					log_printf(SMG_LOG_ALL,server.log,"%s: Error: Bridge tdm read failed (span=%i,chan=%i)! len=%i - %s\n",
							__FUNCTION__,bridge->span,bridge->chan,bytes,strerror(errno));
					tdm_rx_err++;
					err_flag++;
				}
			}

		} else if (ss < 0) {
			if (!bridge->end) {
				log_printf(SMG_LOG_ALL,server.log,"%s: Poll failed on fd exiting bridge  (span=%i,chan=%i)\n",
							__FUNCTION__,bridge->span,bridge->chan);
			}
			break;

		} else if (ss == 0) {
			
			if (bridge_ip_sync) {
				log_printf(SMG_LOG_ALL,server.log,"Bridge IP Timeout: span=%i chan=%i port=%d \n",
							bridge->span,bridge->chan,bridge->mcon.cfg.local_port);
				err_flag++;
			}
			bridge_ip_sync=0;
		}



#if TEST_SEQ
		if (udp_rx % 1000 == 0) {
			err_flag++;
		}
#endif

		if (err_flag) {
			log_printf(SMG_LOG_ALL,server.log,"Bridge (s%02ic%02i) urx/ttx=(%i/%i) ue/te=(%i/%i) | trx/utx=(%i/%i) te/ue=(%i/%i)  \n",
					bridge->span,bridge->chan,
					udp_rx,tdm_tx,udp_rx_err,tdm_tx_err,
					tdm_rx,udp_tx,tdm_rx_err,udp_tx_err);
		}

	}

	pthread_mutex_lock(&g_smg_ip_bridge_lock);
	bridge_threads--;
	pthread_mutex_unlock(&g_smg_ip_bridge_lock);

	return NULL;
}



static int launch_bridge_thread(int idx)
{
    pthread_attr_t attr;
    int result = 0;
    struct sched_param param;

    param.sched_priority = 9;
    result = pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, MGD_STACK_SIZE);

    result = pthread_attr_setschedparam (&attr, &param);
    
    log_printf(SMG_LOG_ALL,server.log,"%s: Bridge Priority=%i res=%i \n",__FUNCTION__,
			 param.sched_priority,result);

	bridge_threads++;

    result = pthread_create(&g_smg_ip_bridge_idx[idx].thread, &attr, bridge_thread_run, &g_smg_ip_bridge_idx[idx]);
    if (result) {
	log_printf(SMG_LOG_ALL, server.log, "%s: Error: Creating Thread! %s\n",
			 __FUNCTION__,strerror(errno));
		g_smg_ip_bridge_idx[idx].end=1;
		bridge_threads--;
    } 
    pthread_attr_destroy(&attr);

    return result;
}

static int create_udp_socket(call_signal_connection_t *ms, char *local_ip, int local_port, char *ip, int port)
{
    int rc;
    struct hostent *result, *local_result;
    char buf[512], local_buf[512];
    int err = 0;

    log_printf(SMG_LOG_DEBUG_9,server.log,"LocalIP %s:%d IP %s:%d \n",local_ip, local_port, ip, port);

    memset(&ms->remote_hp, 0, sizeof(ms->remote_hp));
    memset(&ms->local_hp, 0, sizeof(ms->local_hp));
    if ((ms->socket = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
		gethostbyname_r(ip, &ms->remote_hp, buf, sizeof(buf), &result, &err);
		gethostbyname_r(local_ip, &ms->local_hp, local_buf, sizeof(local_buf), &local_result, &err);
		if (result && local_result) {
			ms->remote_addr.sin_family = ms->remote_hp.h_addrtype;
			memcpy((char *) &ms->remote_addr.sin_addr.s_addr, ms->remote_hp.h_addr_list[0], ms->remote_hp.h_length);
			ms->remote_addr.sin_port = htons(port);

			ms->local_addr.sin_family = ms->local_hp.h_addrtype;
			memcpy((char *) &ms->local_addr.sin_addr.s_addr, ms->local_hp.h_addr_list[0], ms->local_hp.h_length);
			ms->local_addr.sin_port = htons(local_port);
    
			rc = bind(ms->socket, (struct sockaddr *) &ms->local_addr, sizeof(ms->local_addr));
			if (rc < 0) {
				close(ms->socket);
				ms->socket = -1;
    			
				log_printf(SMG_LOG_DEBUG_9,server.log,
					"Failed to bind LocalIP %s:%d IP %s:%d (%s)\n",
						local_ip, local_port, ip, port,strerror(errno));
			} 

			/* OK */

		} else {
    			log_printf(SMG_LOG_ALL,server.log,
				"Failed to get hostbyname LocalIP %s:%d IP %s:%d (%s)\n",
					local_ip, local_port, ip, port,strerror(errno));
		}
    } else {
    	log_printf(SMG_LOG_ALL,server.log,
		"Failed to create/allocate UDP socket\n");
    }

    return ms->socket;
}



int smg_ip_bridge_start(void)
{
	int i;
	int err;
	struct smg_tdm_ip_bridge *bridge;
	call_signal_connection_t *mcon;

	pthread_mutex_init(&g_smg_ip_bridge_lock,NULL);

	for (i=0;i<MAX_SMG_BRIDGE;i++) {
		if (!g_smg_ip_bridge_idx[i].init) {
			continue;
		}
		bridge=&g_smg_ip_bridge_idx[i];
		mcon=&bridge->mcon;

		log_printf(SMG_LOG_ALL, server.log, "Opening Bridge MCON Socket [%d] local %s/%d  remote %s/%d \n",
						mcon->socket,mcon->cfg.local_ip,mcon->cfg.local_port,mcon->cfg.remote_ip,mcon->cfg.remote_port);

#if 0
		if (call_signal_connection_open(mcon, 
						mcon->cfg.local_ip, 
						mcon->cfg.local_port,
						mcon->cfg.remote_ip,
						mcon->cfg.remote_port) < 0) {
				log_printf(SMG_LOG_ALL, server.log, "Error: Opening Bridge MCON Socket [%d] local %s/%d  remote %s/%d %s\n", 
						mcon->socket,mcon->cfg.local_ip,mcon->cfg.local_port,mcon->cfg.remote_ip,mcon->cfg.remote_port,strerror(errno));
				bridge->end=1;
				return -1;
		}
#else
		 if (create_udp_socket(mcon,
		 				   mcon->cfg.local_ip,
						   mcon->cfg.local_port,
						   mcon->cfg.remote_ip,
						   mcon->cfg.remote_port) < 0) {
				log_printf(SMG_LOG_ALL, server.log, "Error: Opening Bridge MCON Socket [%d] local %s/%d  remote %s/%d %s\n", 
						mcon->socket,mcon->cfg.local_ip,mcon->cfg.local_port,mcon->cfg.remote_ip,mcon->cfg.remote_port,strerror(errno));
				bridge->end=1;
				return -1;
		}
	
#endif

		bridge->tdm_fd=open_span_chan(bridge->span, bridge->chan);
		if (bridge->tdm_fd < 0) {
			log_printf(SMG_LOG_ALL, server.log, "Error: Failed to open span=%i chan=%i - %s\n",
						bridge->span,bridge->chan,strerror(errno));
			return -1;
		}

		err=launch_bridge_thread(i);
		if (err) {
			bridge->end=1;
			return -1;
		}
	}

	return 0;
}


int smg_ip_bridge_stop(void)
{
	int i;
	int timeout=10;

	for (i=0;i<MAX_SMG_BRIDGE;i++) {
		if (g_smg_ip_bridge_idx[i].init) {
			g_smg_ip_bridge_idx[i].end=1;
			g_smg_ip_bridge_idx[i].init=0;
		}
	}

	while (bridge_threads) {
		log_printf(SMG_LOG_ALL, server.log, "Waiting for bridge threads %i\n",
						bridge_threads);
		sleep(1);
		timeout--;
		if (timeout == 0) {
			break;
		}
	}

	pthread_mutex_destroy(&g_smg_ip_bridge_lock);

	return 0;
}

