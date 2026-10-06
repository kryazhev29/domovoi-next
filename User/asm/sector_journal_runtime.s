        EXTERN stage5_sj_destination
; Cortex-M3 / IAR. Executable production module.
; Reference: Tests/asm/reference/User/sector_journal.c

        RTMODEL "__CPP_Runtime", "1"
        RTMODEL "__SystemLibrary", "DLib"
        RTMODEL "__dlib_version", "6"
        AAPCS BASE,INTERWORK
        PRESERVE8
        REQUIRE8

        #define SHT_PROGBITS 0x1

        EXTERN __aeabi_memcpy
        EXTERN crc32_update
        EXTERN memcmp
        EXTERN sj_io_erase
        EXTERN sj_io_read
        EXTERN sj_io_service
        EXTERN sj_io_write

        PUBLIC sj_destination
        PUBLIC sj_recover
        PUBLIC sj_replace
        PUBLIC sj_replace_pages
        PUBLIC sj_span

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

          CFI Common cfiCommon1 Using cfiNames0
          CFI CodeAlign 2
          CFI DataAlign 4
          CFI ReturnAddress R14 CODE
          CFI CFA R13+0
          CFI R0 SameValue
          CFI R1 SameValue
          CFI R2 SameValue
          CFI R3 SameValue
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R7 SameValue
          CFI R8 SameValue
          CFI R9 SameValue
          CFI R10 SameValue
          CFI R11 SameValue
          CFI R12 SameValue
          CFI R14 SameValue
          CFI EndCommon cfiCommon1

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock0 Using cfiCommon0
          CFI Function sj_destination
          CFI NoCalls
        THUMB
sj_destination:
        B.W stage5_sj_destination
        LSLS     R1,R0,#+20
        BEQ.N    ??sj_destination_0
        MOVS     R0,#+0
        BX       LR
??sj_destination_0:
        SUB      R2,R0,#+266240
        LDR.W    R1,??DataTable5
        CMP      R2,R1
        BCC.N    ??sj_destination_1
        LDR.W    R1,??DataTable5_1
        CMP      R0,R1
        ITT      NE
        CMPNE    R0,#+1875968
        CMPNE    R0,#+1884160
        BEQ.N    ??sj_destination_1
        SUB      R0,R0,#+1998848
        CMP      R0,#+53248
        SBCS     R0,R0,R0
        LSRS     R0,R0,#+31
        BX       LR
??sj_destination_1:
        MOVS     R0,#+1
        BX       LR
          CFI EndBlock cfiBlock0

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock1 Using cfiCommon0
          CFI Function sj_span
        THUMB
sj_span:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
        LSRS     R2,R0,#+12
        LSLS     R2,R2,#+12
        MOVS     R3,R1
        BEQ.N    ??sj_span_0
        RSB      R0,R0,#+4096
        ADDS     R0,R2,R0
        CMP      R0,R1
        BCC.N    ??sj_span_0
        LDR.W    R0,??DataTable5_2
        CMP      R2,R0
        IT       NE
        CMPNE    R2,#+2072576
        BEQ.N    ??sj_span_1
        MOV      R0,R2
          CFI FunCall sj_destination
        BL       sj_destination
        CBZ.N    R0,??sj_span_2
??sj_span_1:
        MOVS     R0,#+1
        POP      {R1,PC}
??sj_span_0:
        MOVS     R0,#+0
??sj_span_2:
        POP      {R1,PC}
          CFI EndBlock cfiBlock1

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock2 Using cfiCommon0
          CFI Function sj_flash_crc
        THUMB
sj_flash_crc:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        SUB      SP,SP,#+256
          CFI CFA R13+280
        MOV      R7,R0
        MOV      R4,R1
        MOV      R5,#+4294967295
        MOVS     R6,#+0
??sj_flash_crc_0:
          CFI FunCall sj_io_service
        BL       sj_io_service
        MOV      R2,#+256
        MOV      R1,SP
        ADDS     R0,R6,R7
          CFI FunCall sj_io_read
        BL       sj_io_read
        CBZ.N    R0,??sj_flash_crc_1
        MOV      R2,#+256
        MOV      R1,SP
        MOV      R0,R5
          CFI FunCall crc32_update
        BL       crc32_update
        MOV      R5,R0
        BL       ?Subroutine1
??CrossCallReturnLabel_2:
        BCC.N    ??sj_flash_crc_0
        B.N      ??sj_flash_crc_2
??sj_flash_crc_1:
        ADD      SP,SP,#+260
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI CFA R13+280
??sj_flash_crc_2:
        MVNS     R5,R5
        STR      R5,[R4, #+0]
        MOVS     R0,#+1
        B.N      ??sj_flash_crc_1
          CFI EndBlock cfiBlock2

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock3 Using cfiCommon0
          CFI Function sj_blank
        THUMB
sj_blank:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        SUB      SP,SP,#+256
          CFI CFA R13+272
        MOV      R4,R0
        MOVS     R5,#+0
??sj_blank_0:
          CFI FunCall sj_io_service
        BL       sj_io_service
        MOV      R2,#+256
        MOV      R1,SP
        ADDS     R0,R5,R4
          CFI FunCall sj_io_read
        BL       sj_io_read
        CBZ.N    R0,??sj_blank_1
        MOVS     R1,#+0
??sj_blank_2:
        UXTH     R0,R1
        LDRB     R0,[SP, R0]
        CMP      R0,#+255
        BNE.N    ??sj_blank_1
        ADDS     R1,R1,#+1
        UXTH     R0,R1
        CMP      R0,#+256
        BCC.N    ??sj_blank_2
        B.N      ??sj_blank_3
??sj_blank_1:
        MOVS     R0,#+0
        B.N      ??sj_blank_4
??sj_blank_3:
        ADD      R5,R5,#+256
        CMP      R5,#+4096
        BCC.N    ??sj_blank_0
        MOVS     R0,#+1
??sj_blank_4:
        ADD      SP,SP,#+260
          CFI CFA R13+12
        POP      {R4,R5,PC}
          CFI EndBlock cfiBlock3

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock4 Using cfiCommon0
          CFI Function sj_page_equal
        THUMB
sj_page_equal:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        SUB      SP,SP,#+32
          CFI CFA R13+56
        MOV      R4,R0
        MOV      R5,R1
        MOV      R6,R2
        MOVS     R0,#+1
        STRB     R0,[R6, #+0]
        MOVS     R7,#+0
??sj_page_equal_0:
        MOVS     R2,#+32
        MOV      R1,SP
        ADDS     R0,R7,R4
          CFI FunCall sj_io_read
        BL       sj_io_read
        CBZ.N    R0,??sj_page_equal_1
        MOVS     R2,#+32
        ADDS     R1,R5,R7
        MOV      R0,SP
          CFI FunCall memcmp
        BL       memcmp
        CBZ.N    R0,??sj_page_equal_2
        MOVS     R0,#+0
        STRB     R0,[R6, #+0]
??sj_page_equal_2:
        ADDS     R7,R7,#+32
        CMP      R7,#+256
        BCC.N    ??sj_page_equal_0
        B.N      ??sj_page_equal_3
??sj_page_equal_1:
        ADD      SP,SP,#+36
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI CFA R13+56
??sj_page_equal_3:
        MOVS     R0,#+1
        B.N      ??sj_page_equal_1
          CFI EndBlock cfiBlock4

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock5 Using cfiCommon0
          CFI Function sj_pages_equal
        THUMB
sj_pages_equal:
        PUSH     {R3-R8,LR}
          CFI R14 Frame(CFA, -4)
          CFI R8 Frame(CFA, -8)
          CFI R7 Frame(CFA, -12)
          CFI R6 Frame(CFA, -16)
          CFI R5 Frame(CFA, -20)
          CFI R4 Frame(CFA, -24)
          CFI CFA R13+28
        SUB      SP,SP,#+260
          CFI CFA R13+288
        MOV      R4,R0
        MOV      R5,R1
        MOV      R8,R2
        MOV      R7,R3
        MOVS     R0,#+1
        STRB     R0,[R7, #+0]
        MOVS     R6,#+0
??sj_pages_equal_0:
          CFI FunCall sj_io_service
        BL       sj_io_service
        ADD      R2,SP,#+4
        UXTH     R1,R6
        MOV      R0,R8
          CFI IndirectCall
        BLX      R5
        CBZ.N    R0,??sj_pages_equal_1
        MOV      R2,SP
        ADD      R1,SP,#+4
        ADDS     R0,R6,R4
          CFI FunCall sj_page_equal
        BL       sj_page_equal
        CBZ.N    R0,??sj_pages_equal_1
        LDRB     R0,[SP, #+0]
        CBNZ.N   R0,??sj_pages_equal_2
        STRB     R0,[R7, #+0]
??sj_pages_equal_2:
        BL       ?Subroutine1
??CrossCallReturnLabel_3:
        BCC.N    ??sj_pages_equal_0
        B.N      ??sj_pages_equal_3
??sj_pages_equal_1:
        MOVS     R0,#+0
        B.N      ??sj_pages_equal_4
??sj_pages_equal_3:
        MOVS     R0,#+1
??sj_pages_equal_4:
        ADD      SP,SP,#+264
          CFI CFA R13+24
        POP      {R4-R8,PC}
          CFI EndBlock cfiBlock5

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond6 Using cfiCommon0
          CFI Function sj_flash_crc
          CFI Conditional ??CrossCallReturnLabel_2
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+280
          CFI Block cfiCond7 Using cfiCommon0
          CFI (cfiCond7) Function sj_pages_equal
          CFI (cfiCond7) Conditional ??CrossCallReturnLabel_3
          CFI (cfiCond7) R4 Frame(CFA, -24)
          CFI (cfiCond7) R5 Frame(CFA, -20)
          CFI (cfiCond7) R6 Frame(CFA, -16)
          CFI (cfiCond7) R7 Frame(CFA, -12)
          CFI (cfiCond7) R8 Frame(CFA, -8)
          CFI (cfiCond7) R14 Frame(CFA, -4)
          CFI (cfiCond7) CFA R13+288
          CFI Block cfiCond8 Using cfiCommon0
          CFI (cfiCond8) Function sj_recover
          CFI (cfiCond8) Conditional ??CrossCallReturnLabel_4
          CFI (cfiCond8) R4 Frame(CFA, -16)
          CFI (cfiCond8) R5 Frame(CFA, -12)
          CFI (cfiCond8) R6 Frame(CFA, -8)
          CFI (cfiCond8) R14 Frame(CFA, -4)
          CFI (cfiCond8) CFA R13+312
          CFI Block cfiPicker9 Using cfiCommon1
          CFI (cfiPicker9) NoFunction
          CFI (cfiPicker9) Picker
        THUMB
?Subroutine1:
        ADD      R6,R6,#+256
        CMP      R6,#+4096
        BX       LR
          CFI EndBlock cfiCond6
          CFI EndBlock cfiCond7
          CFI EndBlock cfiCond8
          CFI EndBlock cfiPicker9

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock10 Using cfiCommon0
          CFI Function sj_flash_equal
        THUMB
sj_flash_equal:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        SUB      SP,SP,#+64
          CFI CFA R13+88
        MOV      R4,R0
        MOV      R5,R1
        MOV      R7,R2
        MOVS     R6,#+0
??sj_flash_equal_0:
          CFI FunCall sj_io_service
        BL       sj_io_service
        MOVS     R2,#+32
        ADD      R1,SP,#+32
        ADDS     R0,R6,R4
          CFI FunCall sj_io_read
        BL       sj_io_read
        CBZ.N    R0,??sj_flash_equal_1
        MOVS     R2,#+32
        MOV      R1,SP
        ADDS     R0,R6,R5
          CFI FunCall sj_io_read
        BL       sj_io_read
        CMP      R0,#+0
        BEQ.N    ??sj_flash_equal_1
        MOVS     R1,#+0
        B.N      ??sj_flash_equal_2
??sj_flash_equal_1:
        MOVS     R0,#+0
        B.N      ??sj_flash_equal_3
??sj_flash_equal_4:
        ADDS     R1,R1,#+1
        CMP      R1,#+32
        BCS.N    ??sj_flash_equal_5
??sj_flash_equal_2:
        ADD      R0,SP,#+32
        LDRB     R0,[R0, R1]
        LDRB     R2,[SP, R1]
        CMP      R0,R2
        BEQ.N    ??sj_flash_equal_4
        MOVS     R0,#+0
        STRB     R0,[R7, #+0]
        MOVS     R0,#+1
        B.N      ??sj_flash_equal_3
??sj_flash_equal_5:
        ADDS     R6,R6,#+32
        CMP      R6,#+4096
        BCC.N    ??sj_flash_equal_0
        MOVS     R0,#+1
        STRB     R0,[R7, #+0]
??sj_flash_equal_3:
        ADD      SP,SP,#+68
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI EndBlock cfiBlock10

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock11 Using cfiCommon0
          CFI Function sj_header
        THUMB
sj_header:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        MOV      R4,R0
        LDR      R1,[R4, #+0]
        LDR.N    R0,??DataTable5_3
        CMP      R1,R0
        ITT      EQ
        LDREQ    R0,[R4, #+4]
        CMPEQ    R0,#+1
        BNE.N    ??sj_header_0
        LDR      R0,[R4, #+8]
          CFI FunCall sj_destination
        BL       sj_destination
        CBZ.N    R0,??sj_header_0
        LDR      R0,[R4, #+12]
        CMP      R0,#+4096
        ITTTT    EQ
        LDREQ    R1,[R4, #+20]
        LDREQ    R0,[R4, #+16]
        MVNEQ    R0,R0
        CMPEQ    R1,R0
        BNE.N    ??sj_header_0
        MOVS     R2,#+24
        MOV      R1,R4
        MOV      R0,#+4294967295
          CFI FunCall crc32_update
        BL       crc32_update
        LDR      R1,[R4, #+24]
        MVNS     R0,R0
        CMP      R1,R0
        ITTT     EQ
        LDREQ    R1,[R4, #+28]
        LDREQ.N  R0,??DataTable5_4
        CMPEQ    R1,R0
        BNE.N    ??sj_header_0
        MOVS     R0,#+1
        POP      {R4,PC}
??sj_header_0:
        MOVS     R0,#+0
        POP      {R4,PC}
          CFI EndBlock cfiBlock11

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock12 Using cfiCommon0
          CFI Function sj_recover
        THUMB
sj_recover:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        SUB      SP,SP,#+296
          CFI CFA R13+312
        MOVS     R2,#+32
        ADD      R1,SP,#+8
        MOV      R0,#+2072576
          CFI FunCall sj_io_read
        BL       sj_io_read
        CMP      R0,#+0
        BEQ.N    ??sj_recover_0
        ADD      R0,SP,#+8
          CFI FunCall sj_header
        BL       sj_header
        CMP      R0,#+0
        BEQ.N    ??sj_recover_1
        LDR.N    R4,??DataTable5_2
        ADD      R1,SP,#+4
        MOV      R0,R4
          CFI FunCall sj_flash_crc
        BL       sj_flash_crc
        CMP      R0,#+0
        BEQ.N    ??sj_recover_0
        ADD      R5,SP,#+8
        BL       ?Subroutine3
??CrossCallReturnLabel_7:
        BNE.N    ??sj_recover_0
        ADD      R1,SP,#+4
        LDR      R0,[R5, #+8]
          CFI FunCall sj_flash_crc
        BL       sj_flash_crc
        CMP      R0,#+0
        BEQ.N    ??sj_recover_0
        BL       ?Subroutine3
??CrossCallReturnLabel_8:
        BNE.N    ??sj_recover_2
        BL       ?Subroutine2
??CrossCallReturnLabel_6:
        CMP      R0,#+0
        BEQ.N    ??sj_recover_0
??sj_recover_2:
        BL       ?Subroutine3
??CrossCallReturnLabel_9:
        BNE.N    ??sj_recover_3
        LDRB     R0,[SP, #+0]
        CBNZ.N   R0,??sj_recover_4
??sj_recover_3:
        LDR      R0,[R5, #+8]
          CFI FunCall sj_io_erase
        BL       sj_io_erase
        CMP      R0,#+0
        BEQ.N    ??sj_recover_0
        LDR      R0,[R5, #+8]
          CFI FunCall sj_blank
        BL       sj_blank
        CMP      R0,#+0
        BEQ.N    ??sj_recover_0
        MOVS     R6,#+0
??sj_recover_5:
          CFI FunCall sj_io_service
        BL       sj_io_service
        MOV      R2,#+256
        ADD      R1,SP,#+40
        ADDS     R0,R4,R6
          CFI FunCall sj_io_read
        BL       sj_io_read
        CBZ.N    R0,??sj_recover_0
        MOV      R2,#+256
        ADD      R1,SP,#+40
        LDR      R0,[R5, #+8]
        ADDS     R0,R6,R0
          CFI FunCall sj_io_write
        BL       sj_io_write
        CBZ.N    R0,??sj_recover_0
        BL       ?Subroutine1
??CrossCallReturnLabel_4:
        BCC.N    ??sj_recover_5
??sj_recover_4:
        ADD      R1,SP,#+4
        LDR      R0,[R5, #+8]
          CFI FunCall sj_flash_crc
        BL       sj_flash_crc
        CBZ.N    R0,??sj_recover_0
        BL       ?Subroutine3
??CrossCallReturnLabel_10:
        BNE.N    ??sj_recover_0
        BL       ?Subroutine2
??CrossCallReturnLabel_5:
        CMP      R0,#+0
        ITT      NE
        LDRBNE   R0,[SP, #+0]
        CMPNE    R0,#+0
        BEQ.N    ??sj_recover_0
        MOV      R0,#+2072576
          CFI FunCall sj_io_erase
        BL       sj_io_erase
        CBZ.N    R0,??sj_recover_6
        MOV      R0,#+2072576
          CFI FunCall sj_blank
        BL       sj_blank
        CBZ.N    R0,??sj_recover_6
??sj_recover_1:
        MOVS     R0,#+1
        B.N      ??sj_recover_6
??sj_recover_0:
        MOVS     R0,#+0
??sj_recover_6:
        ADD      SP,SP,#+296
          CFI CFA R13+16
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock12

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond13 Using cfiCommon0
          CFI Function sj_recover
          CFI Conditional ??CrossCallReturnLabel_7
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+312
          CFI Block cfiCond14 Using cfiCommon0
          CFI (cfiCond14) Function sj_recover
          CFI (cfiCond14) Conditional ??CrossCallReturnLabel_8
          CFI (cfiCond14) R4 Frame(CFA, -16)
          CFI (cfiCond14) R5 Frame(CFA, -12)
          CFI (cfiCond14) R6 Frame(CFA, -8)
          CFI (cfiCond14) R14 Frame(CFA, -4)
          CFI (cfiCond14) CFA R13+312
          CFI Block cfiCond15 Using cfiCommon0
          CFI (cfiCond15) Function sj_recover
          CFI (cfiCond15) Conditional ??CrossCallReturnLabel_9
          CFI (cfiCond15) R4 Frame(CFA, -16)
          CFI (cfiCond15) R5 Frame(CFA, -12)
          CFI (cfiCond15) R6 Frame(CFA, -8)
          CFI (cfiCond15) R14 Frame(CFA, -4)
          CFI (cfiCond15) CFA R13+312
          CFI Block cfiCond16 Using cfiCommon0
          CFI (cfiCond16) Function sj_recover
          CFI (cfiCond16) Conditional ??CrossCallReturnLabel_10
          CFI (cfiCond16) R4 Frame(CFA, -16)
          CFI (cfiCond16) R5 Frame(CFA, -12)
          CFI (cfiCond16) R6 Frame(CFA, -8)
          CFI (cfiCond16) R14 Frame(CFA, -4)
          CFI (cfiCond16) CFA R13+312
          CFI Block cfiPicker17 Using cfiCommon1
          CFI (cfiPicker17) NoFunction
          CFI (cfiPicker17) Picker
        THUMB
?Subroutine3:
        LDR      R1,[SP, #+4]
        LDR      R0,[R5, #+16]
        CMP      R1,R0
        BX       LR
          CFI EndBlock cfiCond13
          CFI EndBlock cfiCond14
          CFI EndBlock cfiCond15
          CFI EndBlock cfiCond16
          CFI EndBlock cfiPicker17

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond18 Using cfiCommon0
          CFI Function sj_recover
          CFI Conditional ??CrossCallReturnLabel_6
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+312
          CFI Block cfiCond19 Using cfiCommon0
          CFI (cfiCond19) Function sj_recover
          CFI (cfiCond19) Conditional ??CrossCallReturnLabel_5
          CFI (cfiCond19) R4 Frame(CFA, -16)
          CFI (cfiCond19) R5 Frame(CFA, -12)
          CFI (cfiCond19) R6 Frame(CFA, -8)
          CFI (cfiCond19) R14 Frame(CFA, -4)
          CFI (cfiCond19) CFA R13+312
          CFI Block cfiPicker20 Using cfiCommon1
          CFI (cfiPicker20) NoFunction
          CFI (cfiPicker20) Picker
        THUMB
?Subroutine2:
        MOV      R2,SP
        MOV      R1,R4
        LDR      R0,[R5, #+8]
          CFI (cfiCond18) FunCall sj_recover sj_flash_equal
          CFI (cfiCond18) FunCall sj_recover sj_flash_equal
        B.N      sj_flash_equal
          CFI EndBlock cfiCond18
          CFI EndBlock cfiCond19
          CFI EndBlock cfiPicker20

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock21 Using cfiCommon0
          CFI Function sj_replace_pages
        THUMB
sj_replace_pages:
        PUSH     {R3-R9,LR}
          CFI R14 Frame(CFA, -4)
          CFI R9 Frame(CFA, -8)
          CFI R8 Frame(CFA, -12)
          CFI R7 Frame(CFA, -16)
          CFI R6 Frame(CFA, -20)
          CFI R5 Frame(CFA, -24)
          CFI R4 Frame(CFA, -28)
          CFI CFA R13+32
        SUB      SP,SP,#+328
          CFI CFA R13+360
        MOV      R4,R0
        MOV      R9,R1
        MOV      R8,R2
        MOV      R5,#+4294967295
        MOVS     R0,R9
        BEQ.N    ??sj_replace_pages_0
        MOV      R0,R4
          CFI FunCall sj_destination
        BL       sj_destination
        CMP      R0,#+0
        BEQ.N    ??sj_replace_pages_0
          CFI FunCall sj_recover
        BL       sj_recover
        CMP      R0,#+0
        BEQ.N    ??sj_replace_pages_0
        MOV      R3,SP
        MOV      R2,R8
        MOV      R1,R9
        MOV      R0,R4
          CFI FunCall sj_pages_equal
        BL       sj_pages_equal
        CMP      R0,#+0
        BEQ.N    ??sj_replace_pages_0
        LDRB     R0,[SP, #+0]
        CBZ.N    R0,??sj_replace_pages_1
        MOVS     R0,#+1
        B.N      ??sj_replace_pages_2
??sj_replace_pages_1:
        MOV      R0,#+2072576
          CFI FunCall sj_io_erase
        BL       sj_io_erase
        CMP      R0,#+0
        BEQ.N    ??sj_replace_pages_0
        MOV      R0,#+2072576
          CFI FunCall sj_blank
        BL       sj_blank
        CMP      R0,#+0
        BEQ.N    ??sj_replace_pages_0
        LDR.N    R6,??DataTable5_2
        MOV      R0,R6
          CFI FunCall sj_io_erase
        BL       sj_io_erase
        CMP      R0,#+0
??sj_replace_pages_0:
        BEQ.N    ??sj_replace_pages_3
        MOV      R0,R6
          CFI FunCall sj_blank
        BL       sj_blank
        CMP      R0,#+0
        BEQ.N    ??sj_replace_pages_3
        MOVS     R7,#+0
??sj_replace_pages_4:
          CFI FunCall sj_io_service
        BL       sj_io_service
        ADD      R2,SP,#+72
        UXTH     R1,R7
        MOV      R0,R8
          CFI IndirectCall
        BLX      R9
        CMP      R0,#+0
        BEQ.N    ??sj_replace_pages_3
        MOV      R2,#+256
        ADD      R1,SP,#+72
        MOV      R0,R5
          CFI FunCall crc32_update
        BL       crc32_update
        MOV      R5,R0
        MOV      R2,#+256
        ADD      R1,SP,#+72
        ADDS     R0,R6,R7
          CFI FunCall sj_io_write
        BL       sj_io_write
        CMP      R0,#+0
        BEQ.N    ??sj_replace_pages_3
        ADD      R7,R7,#+256
        CMP      R7,#+4096
        BCC.N    ??sj_replace_pages_4
        LDR.N    R0,??DataTable5_3
        STR      R0,[SP, #+8]
        ADD      R7,SP,#+8
        MOVS     R0,#+1
        STR      R0,[R7, #+4]
        STR      R4,[SP, #+16]
        MOV      R0,#+4096
        STR      R0,[SP, #+20]
        MVNS     R5,R5
        STR      R5,[R7, #+16]
        MVNS     R0,R5
        STR      R0,[SP, #+28]
        MOVS     R2,#+24
        ADD      R1,SP,#+8
        MOV      R0,#+4294967295
          CFI FunCall crc32_update
        BL       crc32_update
        MVNS     R0,R0
        STR      R0,[R7, #+24]
        LDR.N    R0,??DataTable5_4
        STR      R0,[R7, #+28]
        ADD      R1,SP,#+4
        MOV      R0,R6
          CFI FunCall sj_flash_crc
        BL       sj_flash_crc
        CMP      R0,#+0
        BEQ.N    ??sj_replace_pages_3
        LDR      R1,[SP, #+4]
        LDR      R0,[R7, #+16]
        CMP      R1,R0
        BNE.N    ??sj_replace_pages_3
        MOV      R3,SP
        MOV      R2,R8
        MOV      R1,R9
        MOV      R0,R6
          CFI FunCall sj_pages_equal
        BL       sj_pages_equal
        CMP      R0,#+0
        ITT      NE
        LDRBNE   R0,[SP, #+0]
        CMPNE    R0,#+0
        BEQ.N    ??sj_replace_pages_3
        MOVS     R2,#+28
        ADD      R1,SP,#+8
        MOV      R0,#+2072576
          CFI FunCall sj_io_write
        BL       sj_io_write
        CBZ.N    R0,??sj_replace_pages_3
        BL       ?Subroutine0
??CrossCallReturnLabel_1:
        CBZ.N    R0,??sj_replace_pages_3
        MOVS     R3,#+0
        ADD      R2,SP,#+40
??sj_replace_pages_5:
        LDR      R0,[R2, R3, LSL #+2]
        LDR      R1,[R7, R3, LSL #+2]
        CMP      R0,R1
        BNE.N    ??sj_replace_pages_3
        ADDS     R3,R3,#+1
        CMP      R3,#+7
        BLT.N    ??sj_replace_pages_5
        LDR      R0,[R2, #+28]
        CMN      R0,#+1
        BNE.N    ??sj_replace_pages_3
        MOVS     R2,#+4
        ADD      R1,SP,#+36
        LDR.N    R0,??DataTable5_5
          CFI FunCall sj_io_write
        BL       sj_io_write
        CBZ.N    R0,??sj_replace_pages_3
        BL       ?Subroutine0
??CrossCallReturnLabel_0:
        CBZ.N    R0,??sj_replace_pages_3
        ADD      R0,SP,#+40
          CFI FunCall sj_header
        BL       sj_header
        CBZ.N    R0,??sj_replace_pages_3
          CFI FunCall sj_recover
        BL       sj_recover
??sj_replace_pages_2:
        ADD      SP,SP,#+332
          CFI CFA R13+28
        POP      {R4-R9,PC}
          CFI CFA R13+360
??sj_replace_pages_3:
        MOVS     R0,#+0
        B.N      ??sj_replace_pages_2
          CFI EndBlock cfiBlock21

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond22 Using cfiCommon0
          CFI Function sj_replace_pages
          CFI Conditional ??CrossCallReturnLabel_1
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+360
          CFI Block cfiCond23 Using cfiCommon0
          CFI (cfiCond23) Function sj_replace_pages
          CFI (cfiCond23) Conditional ??CrossCallReturnLabel_0
          CFI (cfiCond23) R4 Frame(CFA, -28)
          CFI (cfiCond23) R5 Frame(CFA, -24)
          CFI (cfiCond23) R6 Frame(CFA, -20)
          CFI (cfiCond23) R7 Frame(CFA, -16)
          CFI (cfiCond23) R8 Frame(CFA, -12)
          CFI (cfiCond23) R9 Frame(CFA, -8)
          CFI (cfiCond23) R14 Frame(CFA, -4)
          CFI (cfiCond23) CFA R13+360
          CFI Block cfiPicker24 Using cfiCommon1
          CFI (cfiPicker24) NoFunction
          CFI (cfiPicker24) Picker
        THUMB
?Subroutine0:
        MOVS     R2,#+32
        ADD      R1,SP,#+40
        MOV      R0,#+2072576
          CFI (cfiCond22) FunCall sj_replace_pages sj_io_read
          CFI (cfiCond22) FunCall sj_replace_pages sj_io_read
        B.W      sj_io_read
          CFI EndBlock cfiCond22
          CFI EndBlock cfiCond23
          CFI EndBlock cfiPicker24

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock25 Using cfiCommon0
          CFI Function sj_ram_page
        THUMB
sj_ram_page:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
        MOV      R3,R2
        MOV      R2,#+256
        ADD      R1,R0,R1
        MOV      R0,R3
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        MOVS     R0,#+1
        POP      {R1,PC}
          CFI EndBlock cfiBlock25

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock26 Using cfiCommon0
          CFI Function sj_replace
        THUMB
sj_replace:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
        MOVS     R2,R1
        BEQ.N    ??sj_replace_0
        LDR.N    R1,??DataTable5_6
          CFI FunCall sj_replace_pages
        BL       sj_replace_pages
        CBZ.N    R0,??sj_replace_1
        MOVS     R0,#+1
        POP      {R1,PC}
??sj_replace_0:
        MOVS     R0,#+0
??sj_replace_1:
        POP      {R1,PC}
          CFI EndBlock cfiBlock26

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable5:
        DATA32
        DC32     0x187000

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable5_1:
        DATA32
        DC32     0x1c9000

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable5_2:
        DATA32
        DC32     0x1f9000

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable5_3:
        DATA32
        DC32     0x314a4353

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable5_4:
        DATA32
        DC32     0x314b4f53

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable5_5:
        DATA32
        DC32     0x1fa01c

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable5_6:
        DATA32
        DC32     sj_ram_page

        SECTION `.iar_vfe_header`:DATA:NOALLOC:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
        DC32 0

        END
