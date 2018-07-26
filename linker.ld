ENTRY(loader)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)

SECTIONS
{
  . = 0x0100000;


  _kernel_text_start = .;
  .text BLOCK(4K) : ALIGN(4K)
  {
    *(.multiboot)
    *(.text*)
    *(.rodata)
  }
  _kernel_text_end = .;
  _kernel_data_start = ALIGN(4096);
  .data BLOCK(4K) : ALIGN(4K)
  {
    start_ctors = .;
    KEEP(*( .init_array ));
    KEEP(*(SORT_BY_INIT_PRIORITY( .init_array.* )));
    end_ctors = .;
    *(.data)
  }
  _kernel_data_end = .;
  _kernel_bss_start = ALIGN(4096);
  .bss BLOCK(4K) : ALIGN(4K)
  {
    *(.bss)
  }
  _kernel_bss_end = .;
  . = ALIGN(4096);
  _kernel_end = .;
  /DISCARD/ : { *(.fini_array*) *(.comment) }
}

