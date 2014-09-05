--- kernel-source.orig/drivers/net/Makefile	Sun Mar 25 11:37:34 2001
+++ kernel-source/drivers/net/Makefile	Thu Aug 30 11:32:57 2001
@@ -1059,6 +1059,78 @@
   endif
 endif
 
+
+ifeq ($(CONFIG_VENDOR_SANGOMA),y)
+  LX_OBJS += sdladrv.o wanpipe_syncppp.o
+  L_OBJS += sdlamain.o
+  L_OBJS += sdla_ft1.o
+  L_OBJS += sdla_te1.o
+  L_OBJS += sdla_56k.o wanpipe_utils.o
+  ifeq ($(CONFIG_WANPIPE_X25),y)
+    L_OBJS += sdla_x25.o
+  endif
+  ifeq ($(CONFIG_WANPIPE_FR),y)
+    L_OBJS += sdla_fr.o
+  endif
+  ifeq ($(CONFIG_WANPIPE_PPP),y)
+    L_OBJS += sdla_ppp.o
+  endif
+  ifeq ($(CONFIG_WANPIPE_CHDLC),y)
+    L_OBJS += sdla_chdlc.o
+  endif    
+  ifeq ($(CONFIG_WANPIPE_MULTPPP),y)
+    L_OBJS += wanpipe_multppp.o
+    CONFIG_SYNCPPP_BUILTIN=y
+  endif
+  ifeq ($(CONFIG_WANPIPE_BITSTRM),y)
+    L_OBJS += sdla_bitstrm.o
+  endif  
+  ifeq ($(CONFIG_WANPIPE_EDU), y)
+    L_OBJS += sdla_edu.o
+  endif
+  ifeq ($(CONFIG_WANPIPE_BSC), y)
+    L_OBJS += sdla_bsc.o
+  endif
+endif
+
+ifeq ($(CONFIG_VENDOR_SANGOMA),m)
+  MX_OBJS += sdladrv.o wanpipe_syncppp.o
+  M_OBJS += wanpipe.o
+  WANPIPE_OBJS = sdlamain.o
+  WANPIPE_OBJS += sdla_ft1.o
+  WANPIPE_OBJS += sdla_te1.o
+  WANPIPE_OBJS += sdla_56k.o wanpipe_utils.o
+  ifeq ($(CONFIG_WANPIPE_X25),y)
+    WANPIPE_OBJS += sdla_x25.o
+  endif
+  ifeq ($(CONFIG_WANPIPE_FR),y)
+    WANPIPE_OBJS += sdla_fr.o
+  endif
+  ifeq ($(CONFIG_WANPIPE_PPP),y)
+    WANPIPE_OBJS += sdla_ppp.o
+  endif
+  ifeq ($(CONFIG_WANPIPE_CHDLC),y)
+    WANPIPE_OBJS += sdla_chdlc.o
+  endif    
+  ifeq ($(CONFIG_WANPIPE_MULTPPP),y)
+    WANPIPE_OBJS += wanpipe_multppp.o
+    CONFIG_SYNCPPP_MODULE=y
+  endif
+  ifeq ($(CONFIG_WANPIPE_BITSTRM),y)
+    WANPIPE_OBJS += sdla_bitstrm.o
+  endif
+  ifeq ($(CONFIG_WANPIPE_EDU), y)
+    WANPIPE_OBJS += sdla_edu.o
+  endif
+  ifeq ($(CONFIG_WANPIPE_BSC), y)
+    WANPIPE_OBJS += sdla_bsc.o
+  endif
+endif
+
+ifeq ($(CONFIG_ETHER_FR),m)
+  M_OBJS += ether_fr.o
+endif
+
 # If anything built-in uses syncppp, then build it into the kernel also.
 # If not, but a module uses it, build as a module.
 
@@ -1354,43 +1414,6 @@
   ifeq ($(CONFIG_NCR885E),m)
   M_OBJS += ncr885e.o
   endif
-endif
-
-ifeq ($(CONFIG_VENDOR_SANGOMA),y)
-  LX_OBJS += sdladrv.o
-  L_OBJS += sdlamain.o
-  ifeq ($(CONFIG_WANPIPE_X25),y)
-    L_OBJS += sdla_x25.o
-    L_OBJS += sdla_x25api.o
-  endif
-  ifeq ($(CONFIG_WANPIPE_FR),y)
-    L_OBJS += sdla_fr.o
-  endif
-  ifeq ($(CONFIG_WANPIPE_PPP),y)
-    L_OBJS += sdla_ppp.o
-  endif
-  ifeq ($(CONFIG_WANPIPE_CHDLC),y)
-    L_OBJS += sdla_chdlc.o
-  endif    
-endif
-
-ifeq ($(CONFIG_VENDOR_SANGOMA),m)
-  MX_OBJS += sdladrv.o
-  M_OBJS += wanpipe.o
-  WANPIPE_OBJS = sdlamain.o
-  ifeq ($(CONFIG_WANPIPE_X25),y)
-    WANPIPE_OBJS += sdla_x25.o
-    WANPIPE_OBJS += sdla_x25api.o
-  endif
-  ifeq ($(CONFIG_WANPIPE_FR),y)
-    WANPIPE_OBJS += sdla_fr.o
-  endif
-  ifeq ($(CONFIG_WANPIPE_PPP),y)
-    WANPIPE_OBJS += sdla_ppp.o
-  endif
-  ifeq ($(CONFIG_WANPIPE_CHDLC),y)
-    WANPIPE_OBJS += sdla_chdlc.o
-  endif    
 endif
 
 ifeq ($(CONFIG_X25_ASY),y)
