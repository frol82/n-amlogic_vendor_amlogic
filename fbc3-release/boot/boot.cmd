MEMORY {
  ICCM: ORIGIN = 0x00000000 LENGTH = 192K
  DCCM: ORIGIN = 0xffff0000 LENGTH = 64K
  SRAM1: ORIGIN = 0x70020000 LENGTH = 64K
  SRAM2: ORIGIN = 0x80020000 LENGTH = 64K
}
ENTRY(second_boot)
SECTIONS {
  GROUP: {
    .text: { *(.second.boot.entry) *(.text) } 
    .rodata: { *(.rodata) }
  } > SRAM2
  GROUP: {
    .data ADDR(0x80029800): { *(.data) }
    .data1: { *(.data1) }
    .bss: { *(.bss) }
    .tls?: { *(.tls) }
  } > SRAM2
  GROUP: {
    .stack ADDR(0xffffe000) SIZE(8K): {}
  } > DCCM
}
