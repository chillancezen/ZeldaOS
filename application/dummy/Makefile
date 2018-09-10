ifeq ($(ZELDA),)
$(error 'please specify env variable ZELDA')
endif
APP = dummy
SRCS = main.c

MAPS = /usr/bin:dummy
MAPS += /etc:etc/dummy.cfg

CFLAGS = -g3
include $(ZELDA)/mk/Makefile.application


