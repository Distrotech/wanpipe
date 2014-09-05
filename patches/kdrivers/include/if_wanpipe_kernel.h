#ifndef _IF_WANPIPE_KERNEL_
#define _IF_WANPIPE_KERNEL_

#ifdef __KERNEL__

#ifdef LINUX_2_6

#define pkt_sk(__sk) ((struct packet_opt *)(__sk)->sk_protinfo)

# if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,11)) || defined(AF_WANPIPE_2612_FORCE_UPDATE)
#define AF_WANPIPE_2612_UPDATE
#define wansk_set_zapped(__sk)	  sock_set_flag(__sk, SOCK_ZAPPED)
#define wansk_reset_zapped(__sk)  sock_reset_flag(__sk, SOCK_ZAPPED)
#define wansk_is_zapped(__sk)	  sock_flag(__sk, SOCK_ZAPPED)
#define sk_debug	sk_type
#else
#define wansk_set_zapped(__sk)		__sk->sk_zapped=1	
#define wansk_reset_zapped(__sk)	__sk->sk_zapped=0
#define wansk_is_zapped(__sk)		__sk->sk_zapped
#endif

#else

#define pkt_sk(__sk) (__sk)

#define sk_zapped 	zapped
#define sk_state 	state
#define sk_num 		num
#define sk_wmem_alloc	wmem_alloc
#define sk_family	family
#define sk_type		type
#define sk_socket	socket
#define sk_priority	priority
#define sk_protocol	protocol
#define sk_num		num
#define sk_rcvbuf	rcvbuf
#define sk_sndbuf	sndbuf
#define sk_sleep	sleep
#define sk_data_ready	data_ready
#define sk_rmem_alloc	rmem_alloc
#define sk_bound_dev_if	bound_dev_if
#define sk_ack_backlog	ack_backlog
#define sk_data_ready	data_ready
#define sk_receive_queue receive_queue
#define sk_max_ack_backlog max_ack_backlog	
#define sk_debug	debug
#define sk_pair		pair
#define sk_error_queue  error_queue
#define sk_refcnt	refcnt
#define sk_state_change	state_change
#define sk_error_queue	error_queue
#define sk_reuse	reuse
#define sk_err		err
#define sk_shutdown	shutdown
#define sk_reuse	reuse	
#define sk_write_queue  write_queue
#define sk_user_data	user_data

#define wansk_set_zapped(__sk)		__sk->sk_zapped=1	
#define wansk_reset_zapped(__sk)	__sk->sk_zapped=0
#define wansk_is_zapped(__sk)		__sk->sk_zapped


#endif

#endif

#endif
