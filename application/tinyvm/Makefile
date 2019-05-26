ifeq ($(ZELDA),)
$(error 'please specify env variable ZELDA')
endif

APP = tinyvm
SRCS = tvm_program.c tvm_lexer.c tvm.c tvm_htab.c tvm_memory.c \
	tvm_preprocessor.c tvm_parser.c tvm_file.c tvmi.c

MAPS = /usr/bin:tinyvm
MAPS += /usr/var:usr/var/stack_bench.vm
MAPS += /usr/var:usr/var/primes.vm


CFLAGS = -Wall -pipe -Iinclude/ -std=gnu11 -Werror -pedantic -pedantic-errors -O3
include $(ZELDA)/mk/Makefile.application

