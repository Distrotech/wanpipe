#
# Makefile for the Linux network (wan) device drivers.
#
# 3 Aug 2000, Christoph Hellwig <hch@infradead.org>
# Rewritten to use lists instead of if-statements.
#


PWD=$(shell pwd)
WAN_MODULES = sdladrv.o wanrouter.o wanpipe.o wanpipe_syncppp.o af_wanpipe.o wanec.o wan_aften.o

CONFIG_WANPIPE_X25=n
CONFIG_WANPIPE_FR=n
CONFIG_WANPIPE_CHDLC=n
CONFIG_WANPIPE_PPP=n
CONFIG_WANPIPE_MULTPPP=n 

EXTRA_CFLAGS=$(EXTRA_FLAGS) 

wanpipe-y			:= sdlamain.o 

ifneq (,$(ZAPDIR))
PRODUCT_DEFINES			+= -DCONFIG_PRODUCT_WANPIPE_TDM_VOICE 
wanpipe-y			+= sdla_tdmv.o sdla_remora_tdmv.o sdla_bri_tdmv.o sdla_gsm_tdmv.o sdla_tdmv_dummy.o  sdla_usb_remora_tdmv.o

ifneq (,$(wildcard $(ZAPHDLC)))
PRODUCT_DEFINES+= -DCONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN_ZAPTEL
endif
endif       

wanpipe-y			+= sdla_ft1.o sdla_te1.o sdla_te3.o sdla_56k.o sdla_8te1.o 
wanpipe-y			+= wanpipe_tdm_api.o 
wanpipe-y                       += aft_core.o aft_core_api_events.o aft_core_prot.o aft_core_utils.o
wanpipe-y                       += sdla_xilinx.o aft_a104.o aft_t116.o
wanpipe-y			+= aft_gsm.o aft_analog.o sdla_aft_te3.o wanpipe_utils.o sdla_remora_analog.o 
wanpipe-y			+= wanpipe_abstr.o wanpipe_linux_iface.o 
wanpipe-y			+= wanpipe_tdm_api.o sdla_remora.o 
wanpipe-y			+= sdla_gsm.o sdla_bri.o aft_bri.o sdla_serial.o wanpipe_mtp1.o

wanpipe-y                       += wanpipe_codec.o wanpipe_codec_law.o
wanpipe-y                       += wanpipe_usb.o sdla_usb_remora.o 
wanpipe-y                       += wanpipe_timer_dev.o

EXTRA_CFLAGS 			+= $(PRODUCT_DEFINES)

wanpipe-objs := $(wanpipe-y)


WANECDIR=../../wanec
BTDIR=$(WANECDIR)/oct6100_api/apilib/bt
LARGMATHDIR=$(WANECDIR)/oct6100_api/apilib/largmath
LLMANDIR=$(WANECDIR)/oct6100_api/apilib/llman
OCTAPIDIR=$(WANECDIR)/oct6100_api/octdeviceapi/oct6100api/oct6100_api
OCTAPIMIDIR=$(WANECDIR)/oct6100_api/octdeviceapi/oct6100api/oct6100_apimi
HOME=$(HOMEDIR)/patches/kdrivers/wanec

EXTRA_CFLAGS += -I$(HOME) -I$(HOME)/oct6100_api -I$(HOME)/oct6100_api/include -I$(HOME)/oct6100_api/include/oct6100api -I$(HOME)/oct6100_api/include  -I$(HOME)/oct6100_api/include/apilib -I$(HOME)/oct6100_api/include/apilib -I$(HOME)/oct6100_api/include/octrpc -I$(HOME)/oct6100_api/include/oct6100api -I$(HOME)/oct6100_api/octdeviceapi/oct6100api -DENABLE_TONE_PLAY

wanec-y := $(WANECDIR)/wanec_iface.o $(WANECDIR)/wanec_cmd.o $(WANECDIR)/wanec_utils.o $(WANECDIR)/wanec_dev.o 
wanec-y += $(BTDIR)/octapi_bt0.o $(LARGMATHDIR)/octapi_largmath.o 
wanec-y += $(LLMANDIR)/octapi_llman.o $(OCTAPIMIDIR)/oct6100_mask_interrupts.o 
wanec-y += $(OCTAPIDIR)/oct6100_adpcm_chan.o $(OCTAPIDIR)/oct6100_channel.o 
wanec-y += $(OCTAPIDIR)/oct6100_chip_open.o $(OCTAPIDIR)/oct6100_chip_stats.o 
wanec-y += $(OCTAPIDIR)/oct6100_conf_bridge.o $(OCTAPIDIR)/oct6100_debug.o 
wanec-y += $(OCTAPIDIR)/oct6100_events.o $(OCTAPIDIR)/oct6100_interrupts.o 
wanec-y += $(OCTAPIDIR)/oct6100_memory.o $(OCTAPIDIR)/oct6100_miscellaneous.o 
wanec-y += $(OCTAPIDIR)/oct6100_mixer.o $(OCTAPIDIR)/oct6100_phasing_tsst.o 
wanec-y += $(OCTAPIDIR)/oct6100_playout_buf.o $(OCTAPIDIR)/oct6100_remote_debug.o 
wanec-y += $(OCTAPIDIR)/oct6100_tlv.o $(OCTAPIDIR)/oct6100_tone_detection.o 
wanec-y += $(OCTAPIDIR)/oct6100_tsi_cnct.o $(OCTAPIDIR)/oct6100_tsst.o 
wanec-y += $(OCTAPIDIR)/oct6100_user.o 

wanec-objs	:= $(wanec-y)     	

sdladrv-objs := sdladrv_src.o sdladrv_fe.o sdladrv_utils.o sdladrv_usb.o wan_mem_debug.o wanpipe_cdev_linux.o wanpipe_logger.o

wanrouter-objs := ../wanrouter/wanmain.o ../wanrouter/wanproc.o ../wanrouter/waniface.o ../wanrouter/wandev.o 

af_wanpipe-objs	:= ../wanrouter/af_wanpipe_src.o  ../wanrouter/af_wanpipe_datascope.o  

wan_aften-objs := ../wan_aften/wan_aften_src.o ../wan_aften/wanpipe_linux_iface.o

obj-m += $(WAN_MODULES)      

