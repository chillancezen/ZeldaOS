ifeq ($(ZELDA),)
$(error 'please specify env variable ZELDA')
endif

APP = kill
SRCS = main.c

MAPS = /usr/bin:kill
MAPS += /usr/var:usr/var/signal.txt

CFLAGS = -g3
include $(ZELDA)/mk/Makefile.application

