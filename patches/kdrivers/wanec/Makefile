# Makefile for hello world kernel 2.6 module.              -*-makefile-*-
#
# Copyright (C) 2004 Joachim Nilsson <joachim.nilsson@member.fsf.org>
#
# Licensed under the GNU General Public License, v2.0 or later,
# at your option.  See the file COPYING for details, or the web
# page http://www.gnu.org/copyleft/gpl.html
#

OBJS = 
MODULE_NAME = 
EXTRA_CFLAGS = 
KDIR =	
$(MODULE_NAME)-objs = $(OBJS)

RM      = @rm -rf
JUNK	= *~ *.bak DEADJOE

PWD	:= $(shell pwd)
SUBDIRS=$(PWD)

# First pass, kernel Makefile reads module objects
ifneq ($(KERNELRELEASE),)
obj-m	:= $(MODULE_NAME).o


# Second pass, the actual build.
else
KVER    ?= $(shell uname -r)
PWD	:= $(shell pwd)
KBUILD_VERBOSE=
	
all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(SUBDIRS) CC=$(CC) KBUILD_VERBOSE=$(KBUILD_VERBOSE) modules

clean:
	$(shell find  $(SUBDIRS) -name '*.*o'  | xargs rm)
	$(shell find $(SUBDIRS) -name '.*.o.cmd' | xargs rm)
	$(shell rm -f build.sh)
	$(MAKE) -C $(KDIR) SUBDIRS=$(SUBDIRS) clean

distclean: clean
	$(RM) $(JUNK) $(OBJS)

help:
	$(MAKE) -C $(KDIR) SUBDIRS=$(SUBDIRS) help


# Indents the kernel source the way linux/Documentation/CodingStyle.txt
# wants it to be.
indent:
	indent -kr -i8 $($(MODULE_NAME)-objs:.o=.c)

install:
	$(MAKE) -C $(KDIR) M=$(SUBDIRS) modules_install

endif
