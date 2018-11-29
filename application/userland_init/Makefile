ifeq ($(ZELDA),)
$(error 'please specify env variable ZELDA')
endif

APP = userland_init
SRCS = userland_init.c

MAPS = /usr/bin:userland_init
MAPS += /etc:etc/userland.init

CFLAGS = -g3
include $(ZELDA)/mk/Makefile.application

