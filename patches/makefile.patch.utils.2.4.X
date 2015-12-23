--- linux/drivers/net/wan/Makefile.orig	Thu Dec  5 10:48:52 2002
+++ linux/drivers/net/wan/Makefile	Thu Dec  5 10:59:07 2002
@@ -9,10 +9,10 @@
 
 O_TARGET := wan.o
 
-export-objs =	z85230.o syncppp.o comx.o sdladrv.o cycx_drv.o hdlc.o
+export-objs =	z85230.o syncppp.o comx.o sdladrv.o cycx_drv.o hdlc.o wanpipe_syncppp.o
 list-multi =	wanpipe.o cyclomx.o
 
-wanpipe-objs = sdlamain.o sdla_ft1.o sdla_te1.o sdla_56k.o $(wanpipe-y)
+wanpipe-objs = sdlamain.o sdla_ft1.o sdla_te1.o  sdla_te3.o sdla_56k.o wanpipe_linux_iface.o wanpipe_utils.o wanpipe_abstr.o $(wanpipe-y)
 wanpipe-$(CONFIG_WANPIPE_X25)	+= sdla_x25.o
 wanpipe-$(CONFIG_WANPIPE_FR)	+= sdla_fr.o
 wanpipe-$(CONFIG_WANPIPE_CHDLC)	+= sdla_chdlc.o
@@ -59,9 +59,9 @@
 obj-$(CONFIG_DLCI)		+= dlci.o 
 obj-$(CONFIG_SDLA)		+= sdla.o
 ifeq ($(CONFIG_WANPIPE_MULTPPP),y)
-  obj-$(CONFIG_VENDOR_SANGOMA) += sdladrv.o wanpipe.o syncppp.o	
+  obj-$(CONFIG_VENDOR_SANGOMA) += sdladrv.o wanpipe.o wanpipe_syncppp.o	
 else
-  obj-$(CONFIG_VENDOR_SANGOMA) += sdladrv.o wanpipe.o
+  obj-$(CONFIG_VENDOR_SANGOMA) += sdladrv.o wanpipe.o wanpipe_syncppp.o
 endif
 obj-$(CONFIG_CYCLADES_SYNC)	+= cycx_drv.o cyclomx.o
 obj-$(CONFIG_LAPBETHER)		+= lapbether.o
