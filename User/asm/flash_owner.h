/* Flash transaction owners for the byte flash_owner (memory_runtime.s).
   0 = free. Bit 7 set = this owner also forbids table reads.
   Shared by the IAR assembler (#include) and C tests. A claim and the
   release of the same owner use the same constant. */
#ifndef FLASH_OWNER_H
#define FLASH_OWNER_H

#define FO_READ_BLOCK   0x80
#define FO_IDCLEAR      0x81
#define FO_EXPIRY       0x82
#define FO_FWSERVICE    0x83
#define FO_UPLOAD       0x84
#define FO_INDEX        0x85
#define FO_CRC          0x86
#define FO_TABLE        0x87
#define FO_PHOTO        0x08
#define FO_FWBLOCK      0x09
#define FO_LOG          0x0A

#endif
