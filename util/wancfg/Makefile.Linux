
# ============================================================================
# Makefile	Make script for building Linux WAN router utilities.
# ----------------------------------------------------------------------------
# Copyright (c) 1995-1998 Sangoma Technologies Inc.  All Rights Reserved.
# ============================================================================

####### MACROS ###############################################################

# Build options.
OS_TYPE	= __LINUX__
EBUG	= 2
#these protocols defined only for Linux, not for BSD:
PROTOCOL_DEFINES = -DCONFIG_PRODUCT_WANPIPE_LIP_ATM -DCONFIG_PRODUCT_WANPIPE_AFT_BRI
#PROTOCOL_DEFINES += -DZAPTEL_PARSER

WLINC=../../patches/kdrivers/include
WEC=../../patches/kdrivers/wanec
WECOCT=../../patches/kdrivers/wanec/oct6100_api/include

# Project file paths. Initilize to compile in develpment environment.
# Changed when Setup script is running.
WAN_VIRTUAL=
ENABLE_ZAPTEL_PARSER=
ZAPINC=/usr/src/zaptel

ifndef SYSINC
	SYSINC=$(WAN_VIRTUAL)/usr/include/wanpipe
endif

# Tools options.
#CFLAGS = -Wall -O2 -D$(OS_TYPE) -D_DEBUG_=$(DEBUG) -D_GNUC_ -I. \
         -D__LINUX__ -DSTATIC_LIBS=$(GD_STATIC) $(OGR_STATIC) $(PROTOCOL_DEFINES)

CFLAGS = -Wall  -Wno-write-strings -O2 -D$(OS_TYPE) -D_DEBUG_=$(DEBUG) -D_GNUC_ -I. -D__LINUX__  -I$(WLINC) -I$(WEC) -I$(WECOCT) -I$(SYSINC) $(PROTOCOL_DEFINES)

#uncomment this to compile from CVS, also may change to your zaptel installation directory
#CFLAGS += -I/usr/src/zaptel

#CFLAGS += -Wall -O2 -D$(OS_TYPE) -D_DEBUG_=$(DEBUG) -D_GNUC_ -I. \
         -DSTATIC_LIBS= $(GD_STATIC) $(OGR_STATIC) $(PROTOCOL_DEFINES)

CFLAGS += $(EXTRA_FLAGS)
CFLAGS += -lstdc++
CFLAGS += -lncurses 

#For gcc 4.6 to be added later 
#-Wno-write-strings -Wno-unused-but-set-variable -Wno-int-to-pointer-cast

ifeq "${ENABLE_ZAPTEL_PARSER}" "YES"
CFLAGS += -DZAPTEL_PARSER -I$(ZAPINC)
endif

ifeq "${DAHDI_ISSUES}" "YES"
CFLAGS += -DDAHDI_ISSUES
endif

FILES=cpp_string.cpp\
conf_file_reader.cpp\
text_box_yes_no.cpp\
dialog_yes_no.cpp\
text_box.cpp\
menu_base.cpp\
input_box.cpp\
dialog_base.cpp\
menu_main_configuration_options.cpp\
menu_list_existing_wanpipes.cpp\
menu_list_all_wanpipes.cpp\
menu_new_device_configuration.cpp\
menu_hardware_setup.cpp\
menu_hardware_card_type.cpp\
menu_select_card_type_manualy.cpp\
menu_hardware_serial_card_advanced_options.cpp\
menu_hardware_select_serial_clock_source.cpp\
menu_hardware_te1_card_advanced_options.cpp\
menu_hardware_te3_card_advanced_options.cpp\
menu_hardware_select_comms_port.cpp\
menu_te1_select_media.cpp\
menu_te3_select_media.cpp\
menu_hardware_serial_select_medium.cpp\
menu_s508_io_port_select.cpp\
menu_s508_irq_select.cpp\
menu_s508_memory_addr.cpp\
menu_te_select_line_decoding.cpp\
menu_te_select_framing.cpp\
menu_t1_lbo.cpp\
input_box_active_channels.cpp\
menu_te1_clock_mode.cpp\
menu_hardware_probe.cpp\
menu_aft_logical_channels_list.cpp\
input_box_number_of_logical_channels.cpp\
menu_aft_logical_channel_cfg.cpp\
net_interface_file_reader.cpp\
menu_advanced_pci_configuration.cpp\
menu_hardware_cpu_number.cpp\
menu_wan_channel_cfg.cpp\
menu_frame_relay_basic_cfg.cpp\
menu_frame_relay_signalling.cpp\
input_box_number_of_dlcis.cpp\
menu_frame_relay_dlci_configuration.cpp\
menu_frame_relay_advanced_dlci_configuration.cpp\
menu_frame_relay_cir_configuration.cpp\
menu_frame_relay_arp.cpp\
menu_frame_relay_manual_or_auto_dlci_cfg.cpp\
input_box_frame_relay_dlci_number.cpp\
menu_frame_relay_dlci_list.cpp\
menu_frame_relay_advanced_global_configuration.cpp\
menu_select_protocol.cpp\
menu_ppp_basic_cfg.cpp\
menu_ppp_ip_mode.cpp\
menu_ppp_select_authentication_protocol.cpp\
menu_net_interfaces_list.cpp\
menu_net_interface_setup.cpp\
menu_net_interface_ip_configuration.cpp\
menu_net_interface_operation_mode.cpp\
menu_net_interface_miscellaneous_options.cpp\
menu_chdlc_basic_cfg.cpp\
menu_chdlc_advanced_cfg.cpp\
menu_device_miscellaneous_options.cpp\
conf_file_writer.cpp\
menu_adsl_encapsulation.cpp\
menu_adsl_advanced_cfg.cpp\
menu_adsl_standard.cpp\
menu_adsl_trellis.cpp\
menu_adsl_coding_gain.cpp\
menu_adsl_rx_bin_adjust.cpp\
menu_adsl_framing_struct.cpp\
menu_adsl_exchange_type.cpp\
menu_adsl_clock_type.cpp\
wanrouter_rc_file_reader.cpp\
menu_wan_channel_cfg_v1.cpp\
menu_lapb_basic_cfg.cpp\
menu_atm_basic_cfg.cpp\
menu_atm_interface_configuration.cpp\
message_box.cpp\
menu_tdmv_law.cpp\
menu_e1_lbo.cpp\
main.cpp

ifeq "${ENABLE_ZAPTEL_PARSER}" "YES"
FILES+=zaptel_conf_file_reader.cpp\
sangoma_card_list.cpp
endif

####### RULES ################################################################

all:	wancfg
	@echo "Ok."

wancfg: 
	$(CC) $(FILES) $(CFLAGS) -o $@ $^ -lfl -lm

clean:
	rm -f wancfg
	rm -f *.o
	
install:
	@echo "Installing WANCFG in $(WAN_VIRTUAL)/usr/sbin"
	install -D wancfg $(WAN_VIRTUAL)/usr/sbin/wancfg

uninstall:
	@echo "Uninstalling WANCFG in $(WAN_VIRTUAL)/usr/sbin"
	rm -f $(WAN_VIRTUAL)/usr/sbin/wancfg
