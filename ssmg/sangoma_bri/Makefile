
INSTALLPREFIX=
ifndef ARCH
       ARCH=$(shell uname -m)
endif

SANGOMA_BRID=sangoma_brid.i686
ifeq ($(ARCH),x86_64)
	SANGOMA_BRID=sangoma_brid.x86_64
endif
all:

install:
	install -D $(SANGOMA_BRID) $(INSTALLPREFIX)/usr/sbin/sangoma_brid

	@echo "Installing smg_ctrl scripts"
	@cp -f ../sangoma_mgd.trunk/smg_ctrl $(DESTDIR)/usr/sbin/smg_ctrl
	@cp -f ../sangoma_mgd.trunk/safe_sangoma $(DESTDIR)/usr/sbin/safe_sangoma
	@cp -f ../sangoma_mgd.trunk/rc/smg.rc.bri_only $(DESTDIR)/etc/wanpipe/smg.rc

	@echo
	@echo "Sangoma BRI Install Done"
	@echo "Run: /usr/sbin/wancfg_fs to configure wanpipe BRI"
	@echo


