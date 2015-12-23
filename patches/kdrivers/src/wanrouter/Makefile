#
# Makefile for the Linux network (wan) device drivers.
#


EXTRA_CFLAGS=$(EXTRA_FLAGS)
PRODUCT_DEFINES = -DCONFIG_PRODUCT_WANPIPE_BASE -DCONFIG_PRODUCT_WANPIPE_PPP -DCONFIG_PRODUCT_WANPIPE_AFT -DCONFIG_PRODUCT_WANPIPE_AFT_TE1 -DCONFIG_PRODUCT_WANPIPE_AFT_TE3 -DCONFIG_PRODUCT_WANPIPE_ANNEXG -DCONFIG_WANPIPE_HWEC -DCONFIG_PRODUCT_WANPIPE_SOCK_DATASCOPE -DCONFIG_PRODUCT_WANPIPE_ANNEXG

wanrouter-objs := wanmain.o common/wanproc.o waniface.o 

af_wanpipe-objs		:= af_wanpipe_src.o  af_wanpipe_datascope.o

obj-m += wanrouter.o af_wanpipe.o

