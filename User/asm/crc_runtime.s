; Cortex-M3 / IAR. Executable production module.
; Reference: Tests/asm/reference/User/crc.c

        RTMODEL "__CPP_Runtime", "1"
        RTMODEL "__SystemLibrary", "DLib"
        RTMODEL "__dlib_version", "6"
        AAPCS BASE,INTERWORK
        PRESERVE8
        REQUIRE8

        #define SHT_PROGBITS 0x1

        EXTERN __aeabi_memcpy4
        EXTERN crc16_calcInit
        EXTERN fast_gf2_apply

        PUBLIC crc16_ffInit
        PUBLIC crc8_calc

          CFI Names cfiNames0
          CFI StackFrame CFA R13 DATA
          CFI Resource R0:32, R1:32, R2:32, R3:32, R4:32, R5:32, R6:32, R7:32
          CFI Resource R8:32, R9:32, R10:32, R11:32, R12:32, R13:32, R14:32
          CFI EndNames cfiNames0

          CFI Common cfiCommon0 Using cfiNames0
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
          CFI EndCommon cfiCommon0

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock0 Using cfiCommon0
          CFI Function crc16_ffInit
        THUMB
crc16_ffInit:
        PUSH     {R3-R8,LR}
          CFI R14 Frame(CFA, -4)
          CFI R8 Frame(CFA, -8)
          CFI R7 Frame(CFA, -12)
          CFI R6 Frame(CFA, -16)
          CFI R5 Frame(CFA, -20)
          CFI R4 Frame(CFA, -24)
          CFI CFA R13+28
        SUB      SP,SP,#+68
          CFI CFA R13+96
        MOV      R4,R0
        MOV      R5,R1
        MOVS     R0,#+0
        STRB     R0,[SP, #+0]
        CBZ.N    R4,??crc16_ffInit_0
        MOVS     R7,#+0
        ADD      R8,SP,#+4
??crc16_ffInit_1:
        MOVS     R2,#+1
        LSLS     R2,R2,R7
        UXTH     R2,R2
        MOVS     R1,#+1
        MOV      R0,SP
          CFI FunCall crc16_calcInit
        BL       crc16_calcInit
        STRH     R0,[R8, R7, LSL #+1]
        ADDS     R7,R7,#+1
        CMP      R7,#+16
        BLT.N    ??crc16_ffInit_1
        MOVS     R2,#+255
        MOVS     R1,#+1
        MOV      R0,SP
          CFI FunCall crc16_calcInit
        BL       crc16_calcInit
        MOV      R7,R0
        B.N      ??crc16_ffInit_2
??crc16_ffInit_3:
        MOV      R1,R7
        ADD      R0,SP,#+4
          CFI FunCall fast_gf2_apply
        BL       fast_gf2_apply
        EORS     R7,R7,R0
        MOVS     R6,#+0
??crc16_ffInit_4:
        LDRH     R1,[R8, R6, LSL #+1]
        ADD      R0,SP,#+4
          CFI FunCall fast_gf2_apply
        BL       fast_gf2_apply
        ADD      R1,SP,#+36
        STRH     R0,[R1, R6, LSL #+1]
        ADDS     R6,R6,#+1
        CMP      R6,#+16
        BLT.N    ??crc16_ffInit_4
        MOVS     R2,#+32
        ADD      R0,SP,#+4
          CFI FunCall __aeabi_memcpy4
        BL       __aeabi_memcpy4
??crc16_ffInit_2:
        LSLS     R0,R4,#+31
        BPL.N    ??crc16_ffInit_5
        MOV      R1,R5
        ADD      R0,SP,#+4
          CFI FunCall fast_gf2_apply
        BL       fast_gf2_apply
        EOR      R5,R7,R0
??crc16_ffInit_5:
        LSRS     R4,R4,#+1
        BNE.N    ??crc16_ffInit_3
??crc16_ffInit_0:
        MOV      R0,R5
        ADD      SP,SP,#+72
          CFI CFA R13+24
        POP      {R4-R8,PC}
          CFI EndBlock cfiBlock0

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock1 Using cfiCommon0
          CFI Function crc8_calc
          CFI NoCalls
        THUMB
crc8_calc:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        MOV      R3,R0
        MOVS     R0,#+0
        B.N      ??crc8_calc_0
??crc8_calc_1:
        ADDS     R3,R3,#+1
??crc8_calc_0:
        MOV      R2,R1
        SUBS     R1,R2,#+1
        UXTB     R2,R2
        CBZ.N    R2,??crc8_calc_2
        LDRB     R4,[R3, #+0]
        MOVS     R2,#+8
??crc8_calc_3:
        EOR      R6,R0,R4
        LSLS     R5,R6,#+31
        ITEE     PL
        LSRPL    R0,R0,#+1
        LSRMI    R0,R0,#+1
        EORMI    R0,R0,#0x8C
        LSRS     R4,R4,#+1
        SUBS     R2,R2,#+1
        BNE.N    ??crc8_calc_3
        B.N      ??crc8_calc_1
??crc8_calc_2:
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock1

        SECTION `.iar_vfe_header`:DATA:NOALLOC:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
        DC32 0

        END
