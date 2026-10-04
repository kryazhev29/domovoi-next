; Доработка: Кряжев А., 2026. CRC16 и два варианта CRC32 для Cortex-M3.
; CRC16 / storage CRC32 adapted from domovoi-asm; bounded unrolled CRC32 loops.
; AAPCS leaf routines: r0-r3/r12 only, no stack, RAM, peripheral state or final XOR.
        PUBLIC  crc16_calc
        PUBLIC  crc16_calcInit
        PUBLIC  CRC32
        PUBLIC  crc32_update

        PRESERVE8
        CFI Names crc_names
        CFI StackFrame CFA R13 DATA
        CFI Resource R0:32, R1:32, R2:32, R3:32, R4:32, R5:32, R6:32, R7:32
        CFI Resource R8:32, R9:32, R10:32, R11:32, R12:32, R13:32, R14:32
        CFI EndNames crc_names
        CFI Common crc_leaf Using crc_names
        CFI CodeAlign 2
        CFI DataAlign 4
        CFI ReturnAddress R14 CODE
        CFI CFA R13+0
        CFI R0 Undefined
        CFI R1 Undefined
        CFI R2 Undefined
        CFI R3 Undefined
        CFI R4 SameValue
        CFI R5 SameValue
        CFI R6 SameValue
        CFI R7 SameValue
        CFI R8 SameValue
        CFI R9 SameValue
        CFI R10 SameValue
        CFI R11 SameValue
        CFI R12 Undefined
        CFI R14 SameValue
        CFI EndCommon crc_leaf

        SECTION .text:CODE:NOROOT(2)
        THUMB

; unsigned short crc16_calc(unsigned char *buf, unsigned short size) - CRC-16 ModBus
        CFI Block crc_default Using crc_leaf
        CFI Function crc16_calc
crc16_calc:
        MOVW    r2, #0xFFFF
        CFI TailCall crc16_calcInit
        B       crc16_calcInit
        CFI EndBlock crc_default
; unsigned short crc16_calcInit(unsigned char *buf, unsigned short size, unsigned short init)
        CFI Block crc_modbus Using crc_leaf
        CFI Function crc16_calcInit
        CFI NoCalls
crc16_calcInit:
        UXTH    r1, r1
        UXTH    r2, r2
        ADR     r3, asm_crc16_table
        CBZ     r1, c16_done
c16_loop:
        LDRB    r12, [r0], #1
        EOR     r2, r2, r12
        AND     r12, r2, #15
        LDRH    r12, [r3, r12, LSL #1]
        EOR     r2, r12, r2, LSR #4
        AND     r12, r2, #15
        LDRH    r12, [r3, r12, LSL #1]
        EOR     r2, r12, r2, LSR #4
        SUBS    r1, r1, #1
        BNE     c16_loop
c16_done:
        MOV     r0, r2
        BX      lr

        CFI EndBlock crc_modbus

        DATA
        ALIGNROM 2
asm_crc16_table:    DC16    0x0000,0xCC01,0xD801,0x1400,0xF001,0x3C00,0x2800,0xE401
        DC16    0xA001,0x6C00,0x7800,0xB401,0x5000,0x9C01,0x8801,0x4400

; uint32_t crc32_update(uint32_t crc, const uint8_t *p, uint16_t size) - CRC-32 0xEDB88320,
; без начального и конечного XOR. Эталон - crc32_update в sector_journal.c
        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block crc_storage Using crc_leaf
        CFI Function crc32_update
        CFI NoCalls
crc32_update:
        UXTH    r2, r2
        ADR     r3, asm_crc32_table
; r2 holds remaining length minus 4. Borrow selects the 0..3-byte tail.
        SUBS    r2, r2, #4
        BLO     c32_tail
c32_loop:
; Four bytes per iteration; byte loads also support unaligned input.
        LDRB    r12, [r1], #1
        EOR     r0, r0, r12
        AND     r12, r0, #15
        LDR     r12, [r3, r12, LSL #2]
        EOR     r0, r12, r0, LSR #4
        AND     r12, r0, #15
        LDR     r12, [r3, r12, LSL #2]
        EOR     r0, r12, r0, LSR #4
        LDRB    r12, [r1], #1
        EOR     r0, r0, r12
        AND     r12, r0, #15
        LDR     r12, [r3, r12, LSL #2]
        EOR     r0, r12, r0, LSR #4
        AND     r12, r0, #15
        LDR     r12, [r3, r12, LSL #2]
        EOR     r0, r12, r0, LSR #4
        LDRB    r12, [r1], #1
        EOR     r0, r0, r12
        AND     r12, r0, #15
        LDR     r12, [r3, r12, LSL #2]
        EOR     r0, r12, r0, LSR #4
        AND     r12, r0, #15
        LDR     r12, [r3, r12, LSL #2]
        EOR     r0, r12, r0, LSR #4
        LDRB    r12, [r1], #1
        EOR     r0, r0, r12
        AND     r12, r0, #15
        LDR     r12, [r3, r12, LSL #2]
        EOR     r0, r12, r0, LSR #4
        AND     r12, r0, #15
        LDR     r12, [r3, r12, LSL #2]
        EOR     r0, r12, r0, LSR #4
        SUBS    r2, r2, #4
        BHS     c32_loop
c32_tail:
        ADDS    r2, r2, #4
        CBZ     r2, c32_done
c32_byte:
        LDRB    r12, [r1], #1
        EOR     r0, r0, r12
        AND     r12, r0, #15
        LDR     r12, [r3, r12, LSL #2]
        EOR     r0, r12, r0, LSR #4
        AND     r12, r0, #15
        LDR     r12, [r3, r12, LSL #2]
        EOR     r0, r12, r0, LSR #4
        SUBS    r2, r2, #1
        BNE     c32_byte
c32_done:
        BX      lr

        CFI EndBlock crc_storage

        DATA
        ALIGNROM 2
asm_crc32_table:    DC32    0x00000000,0x1DB71064,0x3B6E20C8,0x26D930AC,0x76DC4190,0x6B6B51F4,0x4DB26158,0x5005713C
        DC32    0xEDB88320,0xF00F9344,0xD6D6A3E8,0xCB61B38C,0x9B64C2B0,0x86D3D2D4,0xA00AE278,0xBDBDF21C


; uint32_t CRC32(uint32_t crc, uint32_t word), STM32 word order, no final XOR.
        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block crc_firmware Using crc_leaf
        CFI Function CRC32
        CFI NoCalls
CRC32:
        EOR     r0, r0, r1
        ADR     r2, asm_crc32_stm_table
; Eight fixed nibble steps; no branch/counter for each firmware word.
        LSRS    r3, r0, #28
        LDR     r3, [r2, r3, LSL #2]
        EOR     r0, r3, r0, LSL #4
        LSRS    r3, r0, #28
        LDR     r3, [r2, r3, LSL #2]
        EOR     r0, r3, r0, LSL #4
        LSRS    r3, r0, #28
        LDR     r3, [r2, r3, LSL #2]
        EOR     r0, r3, r0, LSL #4
        LSRS    r3, r0, #28
        LDR     r3, [r2, r3, LSL #2]
        EOR     r0, r3, r0, LSL #4
        LSRS    r3, r0, #28
        LDR     r3, [r2, r3, LSL #2]
        EOR     r0, r3, r0, LSL #4
        LSRS    r3, r0, #28
        LDR     r3, [r2, r3, LSL #2]
        EOR     r0, r3, r0, LSL #4
        LSRS    r3, r0, #28
        LDR     r3, [r2, r3, LSL #2]
        EOR     r0, r3, r0, LSL #4
        LSRS    r3, r0, #28
        LDR     r3, [r2, r3, LSL #2]
        EOR     r0, r3, r0, LSL #4
        BX      lr

        CFI EndBlock crc_firmware

        DATA
        ALIGNROM 2
asm_crc32_stm_table: DC32    0x00000000,0x04C11DB7,0x09823B6E,0x0D4326D9
        DC32    0x130476DC,0x17C56B6B,0x1A864DB2,0x1E475005
        DC32    0x2608EDB8,0x22C9F00F,0x2F8AD6D6,0x2B4BCB61
        DC32    0x350C9B64,0x31CD86D3,0x3C8EA00A,0x384FBDBD
        END
