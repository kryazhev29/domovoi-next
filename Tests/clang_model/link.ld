MEMORY { ROM (rx) : ORIGIN = 0x08008200, LENGTH = 0x37E00
         RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 0x4FC0 }
SECTIONS {
  .intvec : { KEEP(*(.intvec)) } > ROM
  .text : { *(.text*) *(.rodata*) . = ALIGN(4); } > ROM
  .data : { . = ALIGN(4); __data_start = .; *(.data*) . = ALIGN(4); __data_end = .; } > RAM AT > ROM
  __data_load = LOADADDR(.data);
  .ramfunc : { . = ALIGN(4); __ramfunc_start = .; *(.ramfunc*) . = ALIGN(4); __ramfunc_end = .; } > RAM AT > ROM
  __ramfunc_load = LOADADDR(.ramfunc);
  .bss (NOLOAD) : { . = ALIGN(4); __bss_start = .; *(.bss*) *(COMMON) . = ALIGN(4); __bss_end = .; } > RAM
  .cstack (NOLOAD) : { . = ALIGN(8); "CSTACK$$Base" = .; . += 0xE00; "CSTACK$$Limit" = .; } > RAM
  .fault_record 0x20004FC0 (NOLOAD) : { *(.fault_record) }
  /DISCARD/ : { *(.ARM.exidx*) *(.comment) }
}
