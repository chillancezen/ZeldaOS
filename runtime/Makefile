SRCS = $(wildcard ./*.c)

OBJS = $(patsubst %.c,%.o,$(SRCS))

CFLAGS = -g3

%.o:%.c
	@echo "[CC] $<"
	@gcc -m32 $(CFLAGS) -I./include -c -o $@ $<

all:$(OBJS)

.PHONY:all

clean:
	@rm -rf $(OBJS)
