; Cortex-M3 / IAR. Executable production module.
; Reference: Tests/asm/reference/User/flash_w25.c
        PUBLIC w25_job

        RTMODEL "__CPP_Runtime", "1"
        RTMODEL "__SystemLibrary", "DLib"
        RTMODEL "__dlib_file_descriptor", "0"
        RTMODEL "__dlib_full_locale_support", "0"
        RTMODEL "__dlib_version", "6"
        AAPCS BASE,INTERWORK
        PRESERVE8
        REQUIRE8

        #define SHT_PROGBITS 0x1

        EXTERN spi_read
        EXTERN spi_read_byte
        EXTERN spi_ready
        EXTERN spi_send
        EXTERN spi_send_byte
        EXTERN wdt_reload

        PUBLIC w25_async_active
        PUBLIC w25_async_poll
        PUBLIC w25_async_start
        PUBLIC w25_clear_status
        PUBLIC w25_erase_chip
        PUBLIC w25_erase_sector
        PUBLIC w25_read_available
        PUBLIC w25_read_data
        PUBLIC w25_read_devID
        PUBLIC w25_read_status
        PUBLIC w25_write_data

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

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
w25_job:
        DS8 20

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock0 Using cfiCommon0
          CFI Function w25_async_active
          CFI NoCalls
        THUMB
w25_async_active:
        LDR.W    R0,??DataTable14
        LDRB     R0,[R0, #+16]
        CBZ.N    R0,??w25_async_active_0
        MOVS     R0,#+1
??w25_async_active_0:
        BX       LR
          CFI EndBlock cfiBlock0

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock1 Using cfiCommon0
          CFI Function w25_read_available
        THUMB
w25_read_available:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI FunCall spi_ready
        BL       spi_ready
        CBZ.N    R0,??w25_read_available_0
          CFI FunCall w25_read_status
        BL       w25_read_status
        LSLS     R0,R0,#+31
        BMI.N    ??w25_read_available_0
          CFI FunCall spi_ready
        BL       spi_ready
        CBZ.N    R0,??w25_read_available_1
        MOVS     R0,#+1
        POP      {R1,PC}
??w25_read_available_0:
        MOVS     R0,#+0
??w25_read_available_1:
        POP      {R1,PC}
          CFI EndBlock cfiBlock1

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock2 Using cfiCommon0
          CFI Function w25_async_start
        THUMB
w25_async_start:
        PUSH     {R3-R9,LR}
          CFI R14 Frame(CFA, -4)
          CFI R9 Frame(CFA, -8)
          CFI R8 Frame(CFA, -12)
          CFI R7 Frame(CFA, -16)
          CFI R6 Frame(CFA, -20)
          CFI R5 Frame(CFA, -24)
          CFI R4 Frame(CFA, -28)
          CFI CFA R13+32
        MOV      R5,R0
        MOV      R4,R1
        MOV      R6,R2
        MOV      R8,R3
        CMP      R5,#+16777216
        BCS.N    ??w25_async_start_0
        RSB      R1,R5,#+16777216
        CMP      R1,R6
        BCC.N    ??w25_async_start_0
        LDR.W    R7,??DataTable14
        LDRB     R0,[R7, #+16]
        CMP      R0,#+0
        BNE.N    ??w25_async_start_0
          CFI FunCall w25_read_available
        BL       w25_read_available
        CMP      R0,#+0
        BEQ.N    ??w25_async_start_0
        CBZ.N    R4,??w25_async_start_1
        MOVS     R0,R6
        BEQ.N    ??w25_async_start_0
        CMP      R6,#+65
        BCS.N    ??w25_async_start_0
        AND      R1,R5,#0xFF
        ADDS     R1,R6,R1
        MOVW     R0,#+257
        CMP      R1,R0
        BCC.N    ??w25_async_start_2
        B.N      ??w25_async_start_0
??w25_async_start_1:
        LSLS     R0,R5,#+20
        BNE.N    ??w25_async_start_0
??w25_async_start_2:
          CFI FunCall w25_WREN
        BL       w25_WREN
          CFI FunCall w25_read_status
        BL       w25_read_status
        LSLS     R0,R0,#+30
        BPL.N    ??w25_async_start_0
          CFI FunCall spi_ready
        BL       spi_ready
        CBZ.N    R0,??w25_async_start_0
        MOV      R9,#+4096
        LDR.W    R0,??DataTable14_1
        STR      R9,[R0, #+0]
          CFI FunCall w25_delay_ON
        BL       w25_delay_ON
        CMP      R4,#+0
        ITE      NE
        MOVNE    R0,#+2
        MOVEQ    R0,#+32
          CFI FunCall spi_send_byte
        BL       spi_send_byte
        LSLS     R0,R5,#+8
        BL       ??Subroutine2_0
??CrossCallReturnLabel_8:
        LSLS     R0,R5,#+16
        BL       ??Subroutine2_0
??CrossCallReturnLabel_7:
        UXTB     R0,R5
          CFI FunCall spi_send_byte
        BL       spi_send_byte
        CBZ.N    R4,??w25_async_start_3
        MOV      R1,R6
        MOV      R0,R4
          CFI FunCall spi_send
        BL       spi_send
??w25_async_start_3:
        LDR.W    R0,??DataTable14_2
        STR      R9,[R0, #+0]
          CFI FunCall w25_delay_OFF
        BL       w25_delay_OFF
          CFI FunCall spi_ready
        BL       spi_ready
        CBNZ.N   R0,??w25_async_start_4
??w25_async_start_0:
        MOVS     R0,#+0
        B.N      ??w25_async_start_5
??w25_async_start_4:
        STR      R4,[R7, #+0]
        STR      R5,[R7, #+4]
        CBNZ.N   R4,??w25_async_start_6
        MOV      R6,R9
??w25_async_start_6:
        STRH     R6,[R7, #+12]
        MOVS     R0,#+0
        STRH     R0,[R7, #+14]
        STR      R8,[R7, #+8]
        MOVS     R0,#+1
        STRB     R0,[R7, #+16]
??w25_async_start_5:
        POP      {R1,R4-R9,PC}
          CFI EndBlock cfiBlock2

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock3 Using cfiCommon0
          CFI Function w25_async_poll
        THUMB
w25_async_poll:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        SUB      SP,SP,#+64
          CFI CFA R13+80
        MOV      R5,R0
        LDR.W    R4,??DataTable14
        LDRB     R0,[R4, #+16]
        CMP      R0,#+0
        BEQ.N    ??w25_async_poll_0
          CFI FunCall w25_read_status
        BL       w25_read_status
        MOV      R6,R0
          CFI FunCall spi_ready
        BL       spi_ready
        CMP      R0,#+0
        BEQ.N    ??w25_async_poll_1
        LSLS     R0,R6,#+31
        BPL.N    ??w25_async_poll_2
        LDR      R0,[R4, #+0]
        CMP      R0,#+0
        ITE      NE
        MOVNE    R1,#+20
        MOVEQ    R1,#+1000
        LDR      R0,[R4, #+8]
        SUBS     R5,R5,R0
        CMP      R5,R1
        BCS.N    ??w25_async_poll_1
        B.N      ??w25_async_poll_3
??w25_async_poll_2:
        LDRH     R5,[R4, #+12]
        LDRH     R0,[R4, #+14]
        SUBS     R5,R5,R0
        UXTH     R5,R5
        CMP      R5,#+65
        IT       CS
        MOVCS    R5,#+64
        MOV      R2,R5
        MOV      R1,SP
        LDR      R0,[R4, #+4]
        LDRH     R3,[R4, #+14]
        ADDS     R0,R3,R0
          CFI FunCall w25_read_data
        BL       w25_read_data
        CBZ.N    R0,??w25_async_poll_1
        MOVS     R1,#+0
        B.N      ??w25_async_poll_4
??w25_async_poll_5:
        MOVS     R0,#+255
        B.N      ??w25_async_poll_6
??w25_async_poll_7:
        LDR      R0,[R4, #+0]
        CMP      R0,#+0
        BEQ.N    ??w25_async_poll_5
        MOV      R2,R0
        LDRH     R0,[R4, #+14]
        ADDS     R0,R1,R0
        LDRB     R0,[R2, R0]
??w25_async_poll_6:
        LDRB     R2,[SP, R1]
        CMP      R2,R0
        BNE.N    ??w25_async_poll_1
        ADDS     R1,R1,#+1
??w25_async_poll_4:
        CMP      R1,R5
        BLT.N    ??w25_async_poll_7
        LDRH     R0,[R4, #+14]
        ADDS     R5,R5,R0
        STRH     R5,[R4, #+14]
        LDRH     R1,[R4, #+14]
        LDRH     R0,[R4, #+12]
        CMP      R1,R0
        BEQ.N    ??w25_async_poll_8
??w25_async_poll_3:
        MOVS     R0,#+1
        B.N      ??w25_async_poll_0
??w25_async_poll_8:
        MOVS     R0,#+0
        STRB     R0,[R4, #+16]
        MOVS     R0,#+2
        B.N      ??w25_async_poll_0
??w25_async_poll_1:
        MOVS     R0,#+0
        STRB     R0,[R4, #+16]
        MOVS     R0,#+3
??w25_async_poll_0:
        ADD      SP,SP,#+64
          CFI CFA R13+16
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock3

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock4 Using cfiCommon0
          CFI Function w25_read_data
        THUMB
w25_read_data:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        MOV      R4,R0
        MOV      R5,R1
        MOV      R7,R2
          CFI FunCall w25_busy
        BL       w25_busy
        LSLS     R0,R0,#+31
        BPL.N    ??w25_read_data_0
        MOVS     R0,#+0
        POP      {R1,R4-R7,PC}
??w25_read_data_0:
        MOV      R6,#+4096
        LDR.N    R0,??DataTable14_1
        STR      R6,[R0, #+0]
          CFI FunCall w25_delay_ON
        BL       w25_delay_ON
        MOVS     R0,#+3
          CFI FunCall spi_send_byte
        BL       spi_send_byte
        BL       ?Subroutine2
??CrossCallReturnLabel_11:
        LSLS     R0,R4,#+16
        BL       ??Subroutine2_0
??CrossCallReturnLabel_6:
        UXTB     R0,R4
          CFI FunCall spi_send_byte
        BL       spi_send_byte
        MOV      R1,R7
        MOV      R0,R5
          CFI FunCall spi_read
        BL       spi_read
        LDR.N    R0,??DataTable14_2
        STR      R6,[R0, #+0]
          CFI FunCall w25_delay_OFF
        BL       w25_delay_OFF
          CFI FunCall spi_ready
        BL       spi_ready
        CBZ.N    R0,??w25_read_data_1
        MOVS     R0,#+1
??w25_read_data_1:
        POP      {R1,R4-R7,PC}
          CFI EndBlock cfiBlock4

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock5 Using cfiCommon0
          CFI Function w25_write_data
        THUMB
w25_write_data:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        MOV      R5,R0
        MOV      R6,R1
        MOV      R7,R2
        LDR.N    R0,??DataTable14
        LDRB     R0,[R0, #+16]
        CBNZ.N   R0,??w25_write_data_0
        ASRS     R0,R5,#+7
        ADD      R4,R5,R0, LSR #+24
        BIC      R4,R4,#0xFF
        SUBS     R4,R5,R4
        BEQ.N    ??w25_write_data_1
        RSB      R4,R4,#+256
        CMP      R4,R7
        IT       CS
        MOVCS    R4,R7
        UXTH     R2,R4
        MOV      R0,R5
          CFI FunCall w25_page_program
        BL       w25_page_program
        CBZ.N    R0,??w25_write_data_0
??w25_write_data_2:
        UXTH     R0,R4
        ADDS     R5,R0,R5
        ADD      R6,R6,R0
        SUBS     R7,R7,R4
??w25_write_data_1:
        UXTH     R0,R7
        CBZ.N    R0,??w25_write_data_3
        UXTH     R1,R7
        MOVW     R0,#+257
        CMP      R1,R0
        ITE      GE
        MOVGE    R4,#+256
        MOVLT    R4,R7
          CFI FunCall wdt_reload
        BL       wdt_reload
        UXTH     R2,R4
        MOV      R1,R6
        MOV      R0,R5
          CFI FunCall w25_page_program
        BL       w25_page_program
        CMP      R0,#+0
        BNE.N    ??w25_write_data_2
??w25_write_data_0:
        MOVS     R0,#+0
        POP      {R1,R4-R7,PC}
??w25_write_data_3:
        MOVS     R0,#+1
        POP      {R1,R4-R7,PC}
          CFI EndBlock cfiBlock5

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock6 Using cfiCommon0
          CFI Function w25_erase_sector
        THUMB
w25_erase_sector:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R4,R0
        LDR.N    R0,??DataTable14
        LDRB     R0,[R0, #+16]
        CBNZ.N   R0,??w25_erase_sector_0
          CFI FunCall w25_busy
        BL       w25_busy
        LSLS     R0,R0,#+31
        BMI.N    ??w25_erase_sector_0
          CFI FunCall w25_WREN
        BL       w25_WREN
          CFI FunCall w25_read_status
        BL       w25_read_status
        LSLS     R0,R0,#+30
        BPL.N    ??w25_erase_sector_0
        BL       ?Subroutine4
??CrossCallReturnLabel_16:
        MOVS     R0,#+32
          CFI FunCall spi_send_byte
        BL       spi_send_byte
        BL       ?Subroutine2
??CrossCallReturnLabel_10:
        LSLS     R0,R4,#+16
        BL       ??Subroutine2_0
??CrossCallReturnLabel_5:
        UXTB     R0,R4
          CFI FunCall spi_send_byte
        BL       spi_send_byte
        BL       ?Subroutine6
??CrossCallReturnLabel_20:
          CFI FunCall w25_busy
        BL       w25_busy
        MOV      R5,R0
          CFI FunCall w25_WRDI
        BL       w25_WRDI
        LSLS     R0,R5,#+31
        BMI.N    ??w25_erase_sector_0
        MOV      R2,#+4096
        MOVS     R1,#+0
        LSRS     R4,R4,#+12
        LSLS     R0,R4,#+12
          CFI FunCall w25_verify
        BL       w25_verify
        CBZ.N    R0,??w25_erase_sector_1
        MOVS     R0,#+1
        POP      {R1,R4,R5,PC}
??w25_erase_sector_0:
        MOVS     R0,#+0
??w25_erase_sector_1:
        POP      {R1,R4,R5,PC}
          CFI EndBlock cfiBlock6

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond7 Using cfiCommon0
          CFI Function w25_read_data
          CFI Conditional ??CrossCallReturnLabel_11
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
          CFI Block cfiCond8 Using cfiCommon0
          CFI (cfiCond8) Function w25_erase_sector
          CFI (cfiCond8) Conditional ??CrossCallReturnLabel_10
          CFI (cfiCond8) R4 Frame(CFA, -12)
          CFI (cfiCond8) R5 Frame(CFA, -8)
          CFI (cfiCond8) R14 Frame(CFA, -4)
          CFI (cfiCond8) CFA R13+16
          CFI Block cfiCond9 Using cfiCommon0
          CFI (cfiCond9) Function w25_page_program
          CFI (cfiCond9) Conditional ??CrossCallReturnLabel_9
          CFI (cfiCond9) R4 Frame(CFA, -20)
          CFI (cfiCond9) R5 Frame(CFA, -16)
          CFI (cfiCond9) R6 Frame(CFA, -12)
          CFI (cfiCond9) R7 Frame(CFA, -8)
          CFI (cfiCond9) R14 Frame(CFA, -4)
          CFI (cfiCond9) CFA R13+24
          CFI Block cfiPicker10 Using cfiCommon1
          CFI (cfiPicker10) NoFunction
          CFI (cfiPicker10) Picker
        THUMB
?Subroutine2:
        LSLS     R0,R4,#+8
          CFI Block cfiCond11 Using cfiCommon0
          CFI (cfiCond11) Function w25_async_start
          CFI (cfiCond11) Conditional ??CrossCallReturnLabel_8
          CFI (cfiCond11) R4 Frame(CFA, -28)
          CFI (cfiCond11) R5 Frame(CFA, -24)
          CFI (cfiCond11) R6 Frame(CFA, -20)
          CFI (cfiCond11) R7 Frame(CFA, -16)
          CFI (cfiCond11) R8 Frame(CFA, -12)
          CFI (cfiCond11) R9 Frame(CFA, -8)
          CFI (cfiCond11) R14 Frame(CFA, -4)
          CFI (cfiCond11) CFA R13+32
          CFI Block cfiCond12 Using cfiCommon0
          CFI (cfiCond12) Function w25_async_start
          CFI (cfiCond12) Conditional ??CrossCallReturnLabel_7
          CFI (cfiCond12) R4 Frame(CFA, -28)
          CFI (cfiCond12) R5 Frame(CFA, -24)
          CFI (cfiCond12) R6 Frame(CFA, -20)
          CFI (cfiCond12) R7 Frame(CFA, -16)
          CFI (cfiCond12) R8 Frame(CFA, -12)
          CFI (cfiCond12) R9 Frame(CFA, -8)
          CFI (cfiCond12) R14 Frame(CFA, -4)
          CFI (cfiCond12) CFA R13+32
          CFI Block cfiCond13 Using cfiCommon0
          CFI (cfiCond13) Function w25_read_data
          CFI (cfiCond13) Conditional ??CrossCallReturnLabel_6
          CFI (cfiCond13) R4 Frame(CFA, -20)
          CFI (cfiCond13) R5 Frame(CFA, -16)
          CFI (cfiCond13) R6 Frame(CFA, -12)
          CFI (cfiCond13) R7 Frame(CFA, -8)
          CFI (cfiCond13) R14 Frame(CFA, -4)
          CFI (cfiCond13) CFA R13+24
          CFI Block cfiCond14 Using cfiCommon0
          CFI (cfiCond14) Function w25_erase_sector
          CFI (cfiCond14) Conditional ??CrossCallReturnLabel_5
          CFI (cfiCond14) R4 Frame(CFA, -12)
          CFI (cfiCond14) R5 Frame(CFA, -8)
          CFI (cfiCond14) R14 Frame(CFA, -4)
          CFI (cfiCond14) CFA R13+16
          CFI Block cfiCond15 Using cfiCommon0
          CFI (cfiCond15) Function w25_page_program
          CFI (cfiCond15) Conditional ??CrossCallReturnLabel_4
          CFI (cfiCond15) R4 Frame(CFA, -20)
          CFI (cfiCond15) R5 Frame(CFA, -16)
          CFI (cfiCond15) R6 Frame(CFA, -12)
          CFI (cfiCond15) R7 Frame(CFA, -8)
          CFI (cfiCond15) R14 Frame(CFA, -4)
          CFI (cfiCond15) CFA R13+24
??Subroutine2_0:
        LSRS     R0,R0,#+24
          CFI (cfiCond7) FunCall w25_read_data spi_send_byte
          CFI (cfiCond8) FunCall w25_erase_sector spi_send_byte
          CFI (cfiCond9) FunCall w25_page_program spi_send_byte
          CFI (cfiCond11) FunCall w25_async_start spi_send_byte
          CFI (cfiCond11) FunCall w25_async_start spi_send_byte
          CFI (cfiCond7) FunCall w25_read_data spi_send_byte
          CFI (cfiCond8) FunCall w25_erase_sector spi_send_byte
          CFI (cfiCond9) FunCall w25_page_program spi_send_byte
        B.W      spi_send_byte
          CFI EndBlock cfiCond7
          CFI EndBlock cfiCond8
          CFI EndBlock cfiCond9
          CFI EndBlock cfiPicker10
          CFI EndBlock cfiCond11
          CFI EndBlock cfiCond12
          CFI EndBlock cfiCond13
          CFI EndBlock cfiCond14
          CFI EndBlock cfiCond15

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock16 Using cfiCommon0
          CFI Function w25_read_devID
        THUMB
w25_read_devID:
        PUSH     {R2-R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+16
          CFI FunCall w25_busy
        BL       w25_busy
        LSLS     R0,R0,#+31
        BPL.N    ??w25_read_devID_0
        MOVW     R0,#+65535
        POP      {R1,R2,R4,PC}
??w25_read_devID_0:
        BL       ?Subroutine1
??CrossCallReturnLabel_3:
        MOVS     R0,#+144
          CFI FunCall spi_send_byte
        BL       spi_send_byte
        MOVS     R0,#+0
          CFI FunCall spi_send_byte
        BL       spi_send_byte
        MOVS     R0,#+0
          CFI FunCall spi_send_byte
        BL       spi_send_byte
        MOVS     R0,#+0
          CFI FunCall spi_send_byte
        BL       spi_send_byte
        MOVS     R1,#+2
        MOV      R0,SP
          CFI FunCall spi_read
        BL       spi_read
        BL       ?Subroutine3
??CrossCallReturnLabel_14:
        LDRH     R0,[SP, #+0]
        POP      {R1,R2,R4,PC}
          CFI EndBlock cfiBlock16

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock17 Using cfiCommon0
          CFI Function w25_read_status
        THUMB
w25_read_status:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        BL       ?Subroutine4
??CrossCallReturnLabel_15:
        MOVS     R0,#+5
          CFI FunCall spi_send_byte
        BL       spi_send_byte
          CFI FunCall spi_read_byte
        BL       spi_read_byte
        MOV      R4,R0
        BL       ?Subroutine6
??CrossCallReturnLabel_19:
        MOV      R0,R4
        POP      {R1,R4,R5,PC}
          CFI EndBlock cfiBlock17

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond18 Using cfiCommon0
          CFI Function w25_erase_sector
          CFI Conditional ??CrossCallReturnLabel_20
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
          CFI Block cfiCond19 Using cfiCommon0
          CFI (cfiCond19) Function w25_read_status
          CFI (cfiCond19) Conditional ??CrossCallReturnLabel_19
          CFI (cfiCond19) R4 Frame(CFA, -12)
          CFI (cfiCond19) R5 Frame(CFA, -8)
          CFI (cfiCond19) R14 Frame(CFA, -4)
          CFI (cfiCond19) CFA R13+16
          CFI Block cfiPicker20 Using cfiCommon1
          CFI (cfiPicker20) NoFunction
          CFI (cfiPicker20) Picker
        THUMB
?Subroutine6:
        LDR.N    R0,??DataTable14_2
        STR      R5,[R0, #+0]
          CFI (cfiCond18) FunCall w25_erase_sector w25_delay_OFF
          CFI (cfiCond19) FunCall w25_read_status w25_delay_OFF
        B.N      w25_delay_OFF
          CFI EndBlock cfiCond18
          CFI EndBlock cfiCond19
          CFI EndBlock cfiPicker20

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond21 Using cfiCommon0
          CFI Function w25_erase_sector
          CFI Conditional ??CrossCallReturnLabel_16
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
          CFI Block cfiCond22 Using cfiCommon0
          CFI (cfiCond22) Function w25_read_status
          CFI (cfiCond22) Conditional ??CrossCallReturnLabel_15
          CFI (cfiCond22) R4 Frame(CFA, -12)
          CFI (cfiCond22) R5 Frame(CFA, -8)
          CFI (cfiCond22) R14 Frame(CFA, -4)
          CFI (cfiCond22) CFA R13+16
          CFI Block cfiPicker23 Using cfiCommon1
          CFI (cfiPicker23) NoFunction
          CFI (cfiPicker23) Picker
        THUMB
?Subroutine4:
        MOV      R5,#+4096
        LDR.N    R0,??DataTable14_1
        STR      R5,[R0, #+0]
          CFI (cfiCond21) FunCall w25_erase_sector w25_delay_ON
          CFI (cfiCond22) FunCall w25_read_status w25_delay_ON
        B.N      w25_delay_ON
          CFI EndBlock cfiCond21
          CFI EndBlock cfiCond22
          CFI EndBlock cfiPicker23

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock24 Using cfiCommon0
          CFI Function w25_clear_status
        THUMB
w25_clear_status:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
          CFI FunCall w25_busy
        BL       w25_busy
        BL       ?Subroutine1
??CrossCallReturnLabel_2:
        MOVS     R0,#+48
          CFI FunCall spi_send_byte
        BL       spi_send_byte
        LDR.N    R0,??DataTable14_2
        STR      R4,[R0, #+0]
        POP      {R4,LR}
          CFI R4 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall w25_delay_OFF
        B.N      w25_delay_OFF
          CFI EndBlock cfiBlock24

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock25 Using cfiCommon0
          CFI Function w25_erase_chip
        THUMB
w25_erase_chip:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
          CFI FunCall w25_busy
        BL       w25_busy
          CFI FunCall w25_WREN
        BL       w25_WREN
        BL       ?Subroutine1
??CrossCallReturnLabel_1:
        MOVS     R0,#+96
          CFI FunCall spi_send_byte
        BL       spi_send_byte
        BL       ?Subroutine3
??CrossCallReturnLabel_13:
          CFI FunCall w25_clear_status
        BL       w25_clear_status
        POP      {R4,LR}
          CFI R4 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall w25_WRDI
        B.N      w25_WRDI
          CFI EndBlock cfiBlock25

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond26 Using cfiCommon0
          CFI Function w25_read_devID
          CFI Conditional ??CrossCallReturnLabel_14
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
          CFI Block cfiCond27 Using cfiCommon0
          CFI (cfiCond27) Function w25_erase_chip
          CFI (cfiCond27) Conditional ??CrossCallReturnLabel_13
          CFI (cfiCond27) R4 Frame(CFA, -8)
          CFI (cfiCond27) R14 Frame(CFA, -4)
          CFI (cfiCond27) CFA R13+8
          CFI Block cfiCond28 Using cfiCommon0
          CFI (cfiCond28) Function w25_busy
          CFI (cfiCond28) Conditional ??CrossCallReturnLabel_12
          CFI (cfiCond28) R4 Frame(CFA, -16)
          CFI (cfiCond28) R5 Frame(CFA, -12)
          CFI (cfiCond28) R6 Frame(CFA, -8)
          CFI (cfiCond28) R14 Frame(CFA, -4)
          CFI (cfiCond28) CFA R13+16
          CFI Block cfiPicker29 Using cfiCommon1
          CFI (cfiPicker29) NoFunction
          CFI (cfiPicker29) Picker
        THUMB
?Subroutine3:
        LDR.N    R0,??DataTable14_2
        STR      R4,[R0, #+0]
          CFI (cfiCond26) FunCall w25_read_devID w25_delay_OFF
          CFI (cfiCond27) FunCall w25_erase_chip w25_delay_OFF
          CFI (cfiCond28) FunCall w25_busy w25_delay_OFF
        B.N      w25_delay_OFF
          CFI EndBlock cfiCond26
          CFI EndBlock cfiCond27
          CFI EndBlock cfiCond28
          CFI EndBlock cfiPicker29

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond30 Using cfiCommon0
          CFI Function w25_read_devID
          CFI Conditional ??CrossCallReturnLabel_3
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
          CFI Block cfiCond31 Using cfiCommon0
          CFI (cfiCond31) Function w25_clear_status
          CFI (cfiCond31) Conditional ??CrossCallReturnLabel_2
          CFI (cfiCond31) R4 Frame(CFA, -8)
          CFI (cfiCond31) R14 Frame(CFA, -4)
          CFI (cfiCond31) CFA R13+8
          CFI Block cfiCond32 Using cfiCommon0
          CFI (cfiCond32) Function w25_erase_chip
          CFI (cfiCond32) Conditional ??CrossCallReturnLabel_1
          CFI (cfiCond32) R4 Frame(CFA, -8)
          CFI (cfiCond32) R14 Frame(CFA, -4)
          CFI (cfiCond32) CFA R13+8
          CFI Block cfiCond33 Using cfiCommon0
          CFI (cfiCond33) Function w25_busy
          CFI (cfiCond33) Conditional ??CrossCallReturnLabel_0
          CFI (cfiCond33) R4 Frame(CFA, -16)
          CFI (cfiCond33) R5 Frame(CFA, -12)
          CFI (cfiCond33) R6 Frame(CFA, -8)
          CFI (cfiCond33) R14 Frame(CFA, -4)
          CFI (cfiCond33) CFA R13+16
          CFI Block cfiPicker34 Using cfiCommon1
          CFI (cfiPicker34) NoFunction
          CFI (cfiPicker34) Picker
        THUMB
?Subroutine1:
        MOV      R4,#+4096
        LDR.N    R0,??DataTable14_1
        STR      R4,[R0, #+0]
          CFI (cfiCond30) FunCall w25_read_devID w25_delay_ON
          CFI (cfiCond31) FunCall w25_clear_status w25_delay_ON
          CFI (cfiCond32) FunCall w25_erase_chip w25_delay_ON
          CFI (cfiCond33) FunCall w25_busy w25_delay_ON
        B.N      w25_delay_ON
          CFI EndBlock cfiCond30
          CFI EndBlock cfiCond31
          CFI EndBlock cfiCond32
          CFI EndBlock cfiCond33
          CFI EndBlock cfiPicker34

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock35 Using cfiCommon0
          CFI Function w25_busy
        THUMB
w25_busy:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        LDR.N    R0,??DataTable14
        LDRB     R0,[R0, #+16]
        CBZ.N    R0,??w25_busy_0
        POP      {R4-R6,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall w25_read_status
        B.N      w25_read_status
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
??w25_busy_0:
        LDR.N    R6,??DataTable14_3
        BL       ?Subroutine1
??CrossCallReturnLabel_0:
        MOVS     R0,#+5
          CFI FunCall spi_send_byte
        BL       spi_send_byte
??w25_busy_1:
          CFI FunCall wdt_reload
        BL       wdt_reload
          CFI FunCall spi_read_byte
        BL       spi_read_byte
        MOV      R5,R0
          CFI FunCall spi_ready
        BL       spi_ready
        CBZ.N    R0,??w25_busy_2
        LSLS     R0,R5,#+31
        BPL.N    ??w25_busy_3
        MOV      R0,R6
        SUBS     R6,R0,#+1
        CMP      R0,#+0
        BNE.N    ??w25_busy_1
        B.N      ??w25_busy_3
??w25_busy_2:
        MOVS     R5,#+255
??w25_busy_3:
        BL       ?Subroutine3
??CrossCallReturnLabel_12:
        MOV      R0,R5
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock35

        SECTION `.text`:CODE:NOROOT(2)
          CFI Block cfiBlock36 Using cfiCommon0
          CFI Function w25_delay_ON
          CFI NoCalls
        THUMB
w25_delay_ON:
        nop
        BX       LR
          CFI EndBlock cfiBlock36

        SECTION `.text`:CODE:NOROOT(2)
          CFI Block cfiBlock37 Using cfiCommon0
          CFI Function w25_delay_OFF
          CFI NoCalls
        THUMB
w25_delay_OFF:
        nop
        nop
        nop
        nop
        nop
        BX       LR
          CFI EndBlock cfiBlock37

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock38 Using cfiCommon0
          CFI Function w25_WREN
          CFI NoCalls
        THUMB
w25_WREN:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        BL       ?Subroutine5
??CrossCallReturnLabel_18:
        MOVS     R0,#+6
        B.N      ?Subroutine0
          CFI EndBlock cfiBlock38

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock39 Using cfiCommon0
          CFI Function w25_WRDI
          CFI NoCalls
        THUMB
w25_WRDI:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        BL       ?Subroutine5
??CrossCallReturnLabel_17:
        MOVS     R0,#+4
          CFI EndBlock cfiBlock39
        REQUIRE ?Subroutine0

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock40 Using cfiCommon0
          CFI NoFunction
          CFI CFA R13+8
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI FunCall w25_WREN spi_send_byte
          CFI FunCall w25_WRDI spi_send_byte
        THUMB
?Subroutine0:
        BL       spi_send_byte
        LDR.N    R0,??DataTable14_2
        STR      R4,[R0, #+0]
        POP      {R4,LR}
          CFI CFA R13+0
          CFI R4 SameValue
          CFI R14 SameValue
          CFI FunCall w25_WREN w25_delay_OFF
          CFI FunCall w25_WRDI w25_delay_OFF
        B.N      w25_delay_OFF
          CFI EndBlock cfiBlock40

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond41 Using cfiCommon0
          CFI Function w25_WREN
          CFI Conditional ??CrossCallReturnLabel_18
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI Block cfiCond42 Using cfiCommon0
          CFI (cfiCond42) Function w25_WRDI
          CFI (cfiCond42) Conditional ??CrossCallReturnLabel_17
          CFI (cfiCond42) R4 Frame(CFA, -8)
          CFI (cfiCond42) R14 Frame(CFA, -4)
          CFI (cfiCond42) CFA R13+8
          CFI Block cfiPicker43 Using cfiCommon1
          CFI (cfiPicker43) NoFunction
          CFI (cfiPicker43) Picker
        THUMB
?Subroutine5:
        MOV      R4,#+4096
        LDR.N    R0,??DataTable14_1
        STR      R4,[R0, #+0]
          CFI (cfiCond41) FunCall w25_WREN w25_delay_ON
          CFI (cfiCond42) FunCall w25_WRDI w25_delay_ON
        B.N      w25_delay_ON
          CFI EndBlock cfiCond41
          CFI EndBlock cfiCond42
          CFI EndBlock cfiPicker43

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock44 Using cfiCommon0
          CFI Function w25_page_program
        THUMB
w25_page_program:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        MOV      R4,R0
        MOV      R5,R1
        MOV      R6,R2
          CFI FunCall w25_busy
        BL       w25_busy
        LSLS     R0,R0,#+31
        BMI.N    ??w25_page_program_0
          CFI FunCall w25_WREN
        BL       w25_WREN
          CFI FunCall w25_read_status
        BL       w25_read_status
        LSLS     R0,R0,#+30
        BPL.N    ??w25_page_program_0
        MOV      R7,#+4096
        LDR.N    R0,??DataTable14_1
        STR      R7,[R0, #+0]
          CFI FunCall w25_delay_ON
        BL       w25_delay_ON
        MOVS     R0,#+2
          CFI FunCall spi_send_byte
        BL       spi_send_byte
        BL       ?Subroutine2
??CrossCallReturnLabel_9:
        LSLS     R0,R4,#+16
        BL       ??Subroutine2_0
??CrossCallReturnLabel_4:
        UXTB     R0,R4
          CFI FunCall spi_send_byte
        BL       spi_send_byte
        MOV      R1,R6
        MOV      R0,R5
          CFI FunCall spi_send
        BL       spi_send
        LDR.N    R0,??DataTable14_2
        STR      R7,[R0, #+0]
          CFI FunCall w25_delay_OFF
        BL       w25_delay_OFF
          CFI FunCall w25_busy
        BL       w25_busy
        MOV      R7,R0
          CFI FunCall w25_WRDI
        BL       w25_WRDI
        LSLS     R0,R7,#+31
        BMI.N    ??w25_page_program_0
        MOV      R2,R6
        MOV      R1,R5
        MOV      R0,R4
          CFI FunCall w25_verify
        BL       w25_verify
        CBZ.N    R0,??w25_page_program_1
        MOVS     R0,#+1
        POP      {R1,R4-R7,PC}
??w25_page_program_0:
        MOVS     R0,#+0
??w25_page_program_1:
        POP      {R1,R4-R7,PC}
          CFI EndBlock cfiBlock44

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable14:
        DATA32
        DC32     w25_job

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable14_1:
        DATA32
        DC32     0x40010c14

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable14_2:
        DATA32
        DC32     0x40010c10

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable14_3:
        DATA32
        DC32     0xf4240

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock45 Using cfiCommon0
          CFI Function w25_verify
        THUMB
w25_verify:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        SUB      SP,SP,#+64
          CFI CFA R13+88
        MOV      R5,R0
        MOV      R4,R1
        MOV      R6,R2
        B.N      ??w25_verify_0
??w25_verify_1:
        ADDS     R5,R7,R5
        CBZ.N    R4,??w25_verify_2
        ADD      R4,R4,R7
??w25_verify_2:
        SUBS     R6,R6,R7
??w25_verify_0:
        UXTH     R0,R6
        CBZ.N    R0,??w25_verify_3
        CMP      R0,#+65
        ITE      CS
        MOVCS    R7,#+64
        UXTHCC   R7,R6
          CFI FunCall wdt_reload
        BL       wdt_reload
        MOV      R2,R7
        MOV      R1,SP
        MOV      R0,R5
          CFI FunCall w25_read_data
        BL       w25_read_data
        CBZ.N    R0,??w25_verify_4
        MOVS     R1,#+0
        B.N      ??w25_verify_5
??w25_verify_6:
        ADDS     R1,R1,#+1
??w25_verify_5:
        CMP      R1,R7
        BGE.N    ??w25_verify_1
        CMP      R4,#+0
        ITE      NE
        LDRBNE   R0,[R4, R1]
        MOVEQ    R0,#+255
        LDRB     R2,[SP, R1]
        CMP      R2,R0
        BEQ.N    ??w25_verify_6
??w25_verify_4:
        MOVS     R0,#+0
        B.N      ??w25_verify_7
??w25_verify_3:
        MOVS     R0,#+1
??w25_verify_7:
        ADD      SP,SP,#+68
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI EndBlock cfiBlock45

        SECTION `.iar_vfe_header`:DATA:NOALLOC:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
        DC32 0

        END
