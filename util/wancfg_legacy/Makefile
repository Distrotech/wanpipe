install:
	install -D -m 755 wancfg_legacy $(WAN_VIRTUAL)/usr/sbin/wancfg_legacy
	install -D -m 755 pppconfig $(WAN_VIRTUAL)/usr/sbin/wp_pppconfig
	cp -rf lib $(WAN_VIRTUAL)/etc/wanpipe/

uninstall:
	rm -f $(WAN_VIRTUAL)/usr/sbin/wancfg_legacy
	rm -f $(WAN_VIRTUAL)/usr/sbin/wp_pppconfig
