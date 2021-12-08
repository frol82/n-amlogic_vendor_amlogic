MEMORY {
  ROM: ORIGIN = 0x41000000 LENGTH = 1024K
  CODE_MEM: ORIGIN = 0x00000000 LENGTH = 192K
  DATA_MEM: ORIGIN = 0xffff0000 LENGTH = 64K
  SRAM1: ORIGIN = 0x70020000 LENGTH = 64K
  SRAM2: ORIGIN = 0x80020000 LENGTH = 64K
}

SECTIONS {
  GROUP: {
    .start : { vtable.o (.text) }
    _TEXT_START = .;
    .text : {} 
    * (TEXT): {} 
    _TEXT_END = .;
  } > CODE_MEM
  GROUP: {
	.running.on.spi ADDR(0x410B6000) : {}
	.spi.text : { test_running_spi.o (.text) }
  } > ROM
  GROUP: {
    .rodata ADDR(0x410C6000) SIZE(180K): {}
  } > ROM
  GROUP: {
    .data: { *(.data) }
    .data1: { *(.data1) }
    .tls: { *(.tls) }
    .bss: { *(.bss) }
  } > DATA_MEM
  GROUP: {
    .heap ADDR(0x80028000) SIZE(32K): { _fheap = . ;  *(.heap) ; _eheap = 0x8002ffff ; }  
  } > SRAM2
  GROUP: {
    .stack ADDR(0xffffe000) SIZE(8K-16): {}
  } > DATA_MEM
}
  
