ENTRY(Reset_Handler)
MEMORY {
 FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 0x7ffc
 ID (r) : ORIGIN = 0x08007ffc, LENGTH = 4
 RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}
_stack_top = ORIGIN(RAM) + LENGTH(RAM);
SECTIONS {
 .intvec : { KEEP(*(.intvec)) } > FLASH
 .text : { *(.text*) *(.rodata*) . = ALIGN(4); } > FLASH
 .ARM.exidx : { *(.ARM.exidx*) } > FLASH
 .boot_version : { KEEP(*(.boot_version)) } > ID
 .data : { . = ALIGN(4); _sdata = .; *(.data*) . = ALIGN(4); _edata = .; } > RAM AT> FLASH
 _sidata = LOADADDR(.data);
 .bss (NOLOAD) : { . = ALIGN(4); _sbss = .; *(.bss*) *(COMMON) . = ALIGN(4); _ebss = .; } > RAM
 ASSERT(_ebss + 0x800 <= _stack_top, "Bootloader RAM overlaps stack")
 ASSERT(SIZEOF(.boot_version) == 4, "Invalid boot identity")
 /DISCARD/ : { *(.comment*) *(.note*) }
}
