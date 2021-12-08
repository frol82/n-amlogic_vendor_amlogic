MEMORY {
  ICCM: ORIGIN = 0x00000000 LENGTH = 192K
  DCCM: ORIGIN = 0xffff0000 LENGTH = 64K
  SRAM1: ORIGIN = 0x70020000 LENGTH = 64K
  SRAM2: ORIGIN = 0x80020000 LENGTH = 64K
}

ENTRY(main)
SECTIONS {
  GROUP: {
    .text : { *(.text) } 
    .rodata: { *(.rodata) }
  } > ICCM
  GROUP: {
    .data: { *(.data) }
    .data1: { *(.data1) }
    .bss: { *(.bss) }
    .tls?: { *(.tls) }
  } > DCCM
  GROUP: {
    .stack ADDR(0xffffe000) SIZE(8K): {}
  } > DCCM
}
