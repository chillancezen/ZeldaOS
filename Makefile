GCCPARAMS = -m32 -std=c11 -O0 -g -Wall -Wextra -Wpedantic -Werror  -g -Wno-error=missing-field-initializers -nostdlib -ffreestanding
ASMPARAMS = -f elf32
ASPARAMS = --32
LDPARAMS = -melf_i386 -Map=Zelda.map

DIRS = x86 kernel lib device memory
KERNEL_IMAGE = Zelda.bin
OS_PACKAGE = Zelda.iso

C_FILES = $(foreach item,$(DIRS),$(wildcard $(item)/*.c))
C_OBJS = $(patsubst %.c,%.o,$(C_FILES))

ASM_FILES = $(foreach item,$(DIRS),$(wildcard $(item)/*.asm))
ASM_OBJS = $(patsubst %.asm,%.o,$(ASM_FILES))

AS_FILES = $(foreach item,$(DIRS),$(wildcard $(item)/*.s))
AS_OBJS = $(patsubst %.s,%.o,$(AS_FILES))

KERNEL_DEPENDS = $(C_OBJS) $(ASM_OBJS) $(AS_OBJS)

%.o: %.c
	@echo "[CC] $<"
	@gcc $(GCCPARAMS) -I .  -o $@ -c $<

%.o: %.asm
	@echo "[AS] $<"
	@nasm $(ASMPARAMS) -o $@ $<

%.o: %.s
	@echo "[AS] $<"
	@as $(ASPARAMS) -o $@ $<

$(OS_PACKAGE):

$(KERNEL_IMAGE): $(KERNEL_DEPENDS)
	@echo "[LD] $@"
	@ld $(LDPARAMS) -T linker.ld -o $@ $(KERNEL_DEPENDS)
clean:
	@rm -f $(KERNEL_IMAGE) $(KERNEL_DEPENDS)
	@rm -f $(OS_PACKAGE)
	@rm -f iso/boot/$(KERNEL_IMAGE)
$(OS_PACKAGE): $(KERNEL_IMAGE)
	@cp $(KERNEL_IMAGE) iso/boot
	@grub2-mkrescue -o $(OS_PACKAGE) iso

install:$(OS_PACKAGE)
	@cp $(OS_PACKAGE) /mnt/projects
