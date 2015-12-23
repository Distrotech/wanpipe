PWD=$(shell pwd)

ifndef WAN_VIRTUAL
	WAN_VIRTUAL=$(PREFIX)
endif

ifndef WANINCDIR
	WANINCDIR=$(WAN_VIRTUAL)/usr/include/wanpipe
endif

ifneq (,$(wildcard $(PWD)/../patches/kdrivers/include))
	SYSINC=$(PWD)/../patches/kdrivers/include
else
	SYSINC=$(WANINCDIR)
endif

ARCH=$(shell uname -m)

all:
	make -C lxdialog all CC=$(CC) SYSINC=$(SYSINC) EXTRA_FLAGS="$(EXTRA_FLAGS)" ARCH=$(ARCH)
	make -C libsangoma CC=$(CC) SYSINC=$(SYSINC) EXTRA_FLAGS="$(EXTRA_FLAGS)" ARCH=$(ARCH)
	make -C libsangoma install DESTDIR=$(PWD)/.lib
	make -C wanconfig all CC=$(CC) SYSINC=$(SYSINC) EXTRA_FLAGS="$(EXTRA_FLAGS)" ARCH=$(ARCH)
	make -C wanec_client all CC=$(CC)  SYSINC=$(SYSINC) WANINCDIR=$(WANINCDIR) EXTRA_FLAGS="$(EXTRA_FLAGS)" ARCH=$(ARCH)
	make -C wan_plxctrl all CC=$(CC)  SYSINC=$(SYSINC) EXTRA_FLAGS="$(EXTRA_FLAGS)" ARCH=$(ARCH)
	make -C wanpipemon all  CC=$(CC) SYSINC=$(SYSINC) PROTOCOL_DEFS="$(PROTOCOL_DEFS)" ENABLE_WANPIPEMON_ZAP=$(ENABLE_WANPIPEMON_ZAP) EXTRA_FLAGS="$(EXTRA_FLAGS)" ARCH=$(ARCH)
	make -C wanpipemon_legacy all CC=$(CC) SYSINC=$(SYSINC) PROTOCOL_DEFS="$(PROTOCOL_DEFS)" EXTRA_FLAGS="$(EXTRA_FLAGS)" ARCH=$(ARCH)
	make -C bwm all SYSINC=$(SYSINC) CC=$(CC) PROTOCOL_DEFS="$(PROTOCOL_DEFS)" EXTRA_FLAGS="$(EXTRA_FLAGS)" ARCH=$(ARCH)
	make -C wan_aftup all CC=$(CC) SYSINC=$(SYSINC) PROTOCOL_DEFS="$(PROTOCOL_DEFS)" EXTRA_FLAGS="$(EXTRA_FLAGS)" ARCH=$(ARCH)

all_wancfg:
	make -C wancfg all CC=$(CC) SYSINC=$(SYSINC) PROTOCOL_DEFS="$(PROTOCOL_DEFS)" EXTRA_FLAGS="$(EXTRA_FLAGS)" ARCH=$(ARCH) 2> /dev/null

install:
	@if [ ! -e $(WAN_VIRTUAL)/etc/wanpipe ]; then \
		mkdir -p $(WAN_VIRTUAL)/etc/wanpipe; \
	fi
	make -C wancfg_legacy install WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C lxdialog install WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C wanconfig install WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C wanec_client install WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C wan_plxctrl install WAN_VIRTUAL=$(WAN_VIRTUAL) 
	make -C wanpipemon install WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C wanpipemon_legacy install WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C wan_aftup install WAN_VIRTUAL=$(WAN_VIRTUAL) 
	make -C bwm install WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C wancfg_zaptel install WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C libsangoma install DESTDIR=$(WAN_VIRTUAL)

install_wancfg:
	make -C wancfg install WAN_VIRTUAL=$(WAN_VIRTUAL)

uninstall:
	make -C ft1 uninstall WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C wancfg_legacy uninstall WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C lxdialog uninstall WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C sdladump uninstall WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C wanconfig uninstall WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C wanec_client uninstall WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C wan_plxctrl uninstall WAN_VIRTUAL=$(WAN_VIRTUAL) 
	make -C wanpipemon uninstall WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C wanpipemon_legacy uninstall WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C wancfg uninstall WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C wan_aftup uninstall WAN_VIRTUAL=$(WAN_VIRTUAL) 
	make -C bwm uninstall   WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C wancfg_zaptel uninstall WAN_VIRTUAL=$(WAN_VIRTUAL)  

clean:
	make -C ft1 clean WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C lxdialog clean WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C sdladump clean WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C wanconfig clean WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C wanec_client clean WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C wan_plxctrl clean WAN_VIRTUAL=$(WAN_VIRTUAL) 
	make -C wanpipemon clean WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C wanpipemon_legacy clean WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C wancfg clean WAN_VIRTUAL=$(WAN_VIRTUAL)
	make -C wan_aftup clean WAN_VIRTUAL=$(WAN_VIRTUAL) 
	make -C bwm clean  WAN_VIRTUAL=$(WAN_VIRTUAL) 
	make -C wanctl clean  WAN_VIRTUAL=$(WAN_VIRTUAL) 
	make -C wan_fxotune clean  WAN_VIRTUAL=$(WAN_VIRTUAL) 
	@rm -rf .lib
