; Доработка: Кряжев А., 2026. Ассемблерная версия User/crc.c (сборки ASM и гибрид).
; Эталон - функции на C в crc.c; результаты совпадают байт в байт (Tests/asm/test_asm_crc.py).
; Расчёт по таблице на полбайта: та же CRC, в несколько раз быстрее побитового.
        PUBLIC  crc16_calc
        PUBLIC  crc16_calcInit
        PUBLIC  crc8_calc

        SECTION .text:CODE:NOROOT(2)
        THUMB

; unsigned short crc16_calc(unsigned char *buf, unsigned short size) - CRC-16 ModBus
crc16_calc:
        MOVW    r2, #0xFFFF
; unsigned short crc16_calcInit(unsigned char *buf, unsigned short size, unsigned short init)
crc16_calcInit:
        UXTH    r1, r1
        UXTH    r2, r2
        ADR     r3, t16
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

        DATA
        ALIGNROM 2
t16:    DC16    0x0000,0xCC01,0xD801,0x1400,0xF001,0x3C00,0x2800,0xE401
        DC16    0xA001,0x6C00,0x7800,0xB401,0x5000,0x9C01,0x8801,0x4400

; unsigned char crc8_calc(unsigned char *in, unsigned char num) - CRC Dallas
        SECTION .text:CODE:NOROOT(2)
        THUMB
crc8_calc:
        UXTB    r1, r1
        MOVS    r2, #0
        ADR     r3, t8
        CBZ     r1, c8_done
c8_loop:
        LDRB    r12, [r0], #1
        EOR     r2, r2, r12
        AND     r12, r2, #15
        LDRB    r12, [r3, r12]
        EOR     r2, r12, r2, LSR #4
        AND     r12, r2, #15
        LDRB    r12, [r3, r12]
        EOR     r2, r12, r2, LSR #4
        SUBS    r1, r1, #1
        BNE     c8_loop
c8_done:
        MOV     r0, r2
        BX      lr

        DATA
t8:     DC8     0x00,0x9D,0x23,0xBE,0x46,0xDB,0x65,0xF8
        DC8     0x8C,0x11,0xAF,0x32,0xCA,0x57,0xE9,0x74
        END
