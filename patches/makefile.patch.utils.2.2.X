--- linux/drivers/net/Makefile.orig	Thu Dec  5 11:53:14 2002
+++ linux/drivers/net/Makefile	Thu Dec  5 11:50:22 2002
@@ -1061,11 +1061,11 @@
 
 
 ifeq ($(CONFIG_VENDOR_SANGOMA),y)
-  LX_OBJS += sdladrv.o 
+  LX_OBJS += sdladrv.o wanpipe_syncppp.o
   L_OBJS += sdlamain.o
   L_OBJS += sdla_ft1.o
   L_OBJS += sdla_te1.o
-  L_OBJS += sdla_56k.o 
+  L_OBJS += sdla_56k.o wanpipe_utils.o
   ifeq ($(CONFIG_WANPIPE_X25),y)
     L_OBJS += sdla_x25.o
   endif
@@ -1094,12 +1094,12 @@
 endif
 
 ifeq ($(CONFIG_VENDOR_SANGOMA),m)
-  MX_OBJS += sdladrv.o 
+  MX_OBJS += sdladrv.o wanpipe_syncppp.o
   M_OBJS += wanpipe.o
   WANPIPE_OBJS = sdlamain.o
   WANPIPE_OBJS += sdla_ft1.o
   WANPIPE_OBJS += sdla_te1.o
-  WANPIPE_OBJS += sdla_56k.o 
+  WANPIPE_OBJS += sdla_56k.o wanpipe_utils.o
   ifeq ($(CONFIG_WANPIPE_X25),y)
     WANPIPE_OBJS += sdla_x25.o
   endif
