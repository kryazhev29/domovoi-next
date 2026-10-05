; Cortex-M3 / IAR. Executable production module.
; Reference: Tests/asm/reference/User/sector_runtime.c

        RTMODEL "__CPP_Runtime", "1"
        RTMODEL "__SystemLibrary", "DLib"
        RTMODEL "__dlib_file_descriptor", "0"
        RTMODEL "__dlib_full_locale_support", "0"
        RTMODEL "__dlib_version", "6"
        AAPCS BASE,INTERWORK
        PRESERVE8
        REQUIRE8

        #define SHT_PROGBITS 0x1

        EXTERN crc32_update
        EXTERN mem_config_result
        EXTERN memcmp
        EXTERN sj_destination
        EXTERN w25_async_active
        EXTERN w25_async_poll
        EXTERN w25_async_start
        EXTERN w25_read_data

        PUBLIC sj_runtime_begin
        PUBLIC sj_runtime_blocked
        PUBLIC sj_runtime_cancel
        PUBLIC sj_runtime_poll
        PUBLIC sj_runtime_reset
        PUBLIC sjr

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
; data[64] remains persistent for W25; check[64] is poll-local.
sjr:
        DS8 128

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock0 Using cfiCommon0
          CFI Function sj_runtime_blocked
          CFI NoCalls
        THUMB
sj_runtime_blocked:
        LDR.W    R1,??DataTable6
        LDRB     R0,[R1, #+59]
        CBNZ.N   R0,??sj_runtime_blocked_0
        LDRB     R0,[R1, #+54]
        CBZ.N    R0,??sj_runtime_blocked_1
        CMP      R0,#+30
        BGE.N    ??sj_runtime_blocked_1
??sj_runtime_blocked_0:
        MOVS     R0,#+1
        BX       LR
??sj_runtime_blocked_1:
        MOVS     R0,#+0
        BX       LR
          CFI EndBlock cfiBlock0

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock1 Using cfiCommon0
          CFI Function sj_runtime_cancel
          CFI NoCalls
        THUMB
sj_runtime_cancel:
        MOVS     R1,#+1
        LDR.W    R0,??DataTable6
        STRB     R1,[R0, #+58]
        BX       LR
          CFI EndBlock cfiBlock1

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock2 Using cfiCommon0
          CFI Function sj_runtime_reset
          CFI NoCalls
        THUMB
sj_runtime_reset:
        LDR.W    R1,??DataTable6
        LDRB     R0,[R1, #+56]
        CBNZ.N   R0,??sj_runtime_reset_0
        STRB     R0,[R1, #+54]
??sj_runtime_reset_0:
        BX       LR
          CFI EndBlock cfiBlock2

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock3 Using cfiCommon0
          CFI Function sj_runtime_begin
        THUMB
sj_runtime_begin:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        MOV      R4,R0
        MOV      R7,R1
        MOV      R5,R2
          CFI FunCall mem_config_result
        BL       mem_config_result
        CMP      R0,#+0
        BNE.N    ??sj_runtime_begin_0
        LDR.N    R6,??DataTable6
        LDRB     R0,[R6, #+54]
        CMP      R0,#+0
        ITT      EQ
        LDRBEQ   R0,[R6, #+59]
        CMPEQ    R0,#+0
        BNE.N    ??sj_runtime_begin_0
        MOVS     R0,R7
        BEQ.N    ??sj_runtime_begin_0
        MOV      R0,R4
          CFI FunCall sj_destination
        BL       sj_destination
        CBZ.N    R0,??sj_runtime_begin_0
          CFI FunCall w25_async_active
        BL       w25_async_active
        CBZ.N    R0,??sj_runtime_begin_1
??sj_runtime_begin_0:
        MOVS     R0,#+0
        POP      {R1,R4-R7,PC}
??sj_runtime_begin_1:
        STR      R4,[R6, #+8]
        STR      R7,[R6, #+0]
        STR      R5,[R6, #+4]
        STRH     R0,[R6, #+52]
        MOV      R0,#+4294967295
        STR      R0,[R6, #+12]
        MOVS     R0,#+0
        STRB     R0,[R6, #+58]
        STRB     R0,[R6, #+57]
        STRB     R0,[R6, #+60]
        MOVS     R0,#+16
        STRB     R0,[R6, #+54]
        MOVS     R0,#+1
        POP      {R1,R4-R7,PC}
          CFI EndBlock cfiBlock3

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock4 Using cfiCommon0
          CFI Function sjr_fail
          CFI NoCalls
        THUMB
sjr_fail:
        LDR.N    R1,??DataTable6
        LDRB     R0,[R1, #+57]
        CBZ.N    R0,??sjr_fail_0
        MOVS     R0,#+1
        STRB     R0,[R1, #+59]
??sjr_fail_0:
        MOVS     R0,#+31
        STRB     R0,[R1, #+54]
        BX       LR
          CFI EndBlock cfiBlock4

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock5 Using cfiCommon0
          CFI Function sjr_start
        THUMB
sjr_start:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI FunCall w25_async_start
        BL       w25_async_start
        CBNZ.N   R0,??sjr_start_0
        POP      {R0,LR}
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall sjr_fail
        B.N      sjr_fail
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
??sjr_start_0:
        LDR      R0,[SP, #+8]
        LDR.N    R2,??DataTable6
        MOVS     R1,#+1
        STRB     R1,[R2, #+56]
        STRB     R0,[R2, #+55]
        POP      {R0,PC}
          CFI EndBlock cfiBlock5

        SECTION `.text`:CODE:NOROOT(2)
          CFI Block cfiBlock6 Using cfiCommon0
          CFI Function sj_runtime_poll
        THUMB
sj_runtime_poll:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
; Scratch is consumed synchronously; the outgoing fifth argument reuses it
; only in phases where no comparison bytes remain live. No DMA owns it.
        SUB      SP,SP,#+64
          CFI CFA R13+80
        MOV      R5,R0
        LDR.N    R4,??DataTable6
        LDRB     R0,[R4, #+54]
        CMP      R0,#+0
        BEQ.N    ??sj_runtime_poll_1
        LDRB     R0,[R4, #+56]
        CBZ.N    R0,??sj_runtime_poll_2
        MOV      R0,R5
          CFI FunCall w25_async_poll
        BL       w25_async_poll
        CMP      R0,#+1
        BEQ.N    ??sj_runtime_poll_1
        MOVS     R1,#+0
        STRB     R1,[R4, #+56]
        CMP      R0,#+2
        BEQ.N    ??sj_runtime_poll_3
          CFI FunCall sjr_fail
        BL       sjr_fail
        B.N      ??sj_runtime_poll_4
??sj_runtime_poll_3:
        LDRB     R0,[R4, #+55]
        STRB     R0,[R4, #+54]
??sj_runtime_poll_4:
        LDRB     R0,[R4, #+54]
        CMP      R0,#+31
        BNE.N    ??sj_runtime_poll_5
        B.N      ??sj_runtime_poll_6
??sj_runtime_poll_2:
        LDRB     R0,[R4, #+58]
        CBZ.N    R0,??sj_runtime_poll_7
        LDRB     R0,[R4, #+57]
        CBNZ.N   R0,??sj_runtime_poll_7
        LDRB     R0,[R4, #+54]
        CMP      R0,#+30
        BGE.N    ??sj_runtime_poll_7
        MOVS     R0,#+31
        STRB     R0,[R4, #+54]
        B.N      ??sj_runtime_poll_6
??sj_runtime_poll_7:
        LDRB     R0,[R4, #+54]
        SUBS     R0,R0,#+1
        CMP      R0,#+30
        BHI.N    ??sj_runtime_poll_8
        TBH      [PC, R0, LSL #+1]
        DATA
??sj_runtime_poll_0:
        DC16 (??sj_runtime_poll_15-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_18-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_19-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_20-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_24-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_28-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_29-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_30-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_31-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_32-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_33-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_34-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_35-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_37-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_40-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_9-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_8-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_8-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_8-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_8-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_8-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_8-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_8-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_8-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_8-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_8-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_8-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_8-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_8-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_41-??sj_runtime_poll_0)/2
        DC16 (??sj_runtime_poll_6-??sj_runtime_poll_0)/2
        THUMB
??sj_runtime_poll_9:
        BL       ?Subroutine3
??CrossCallReturnLabel_7:
        CBZ.N    R0,??sj_runtime_poll_8
        BL       ?Subroutine1
??CrossCallReturnLabel_3:
        CBNZ.N   R0,??sj_runtime_poll_10
??sj_runtime_poll_8:
          CFI FunCall sjr_fail
        BL       sjr_fail
??sj_runtime_poll_11:
        LDRB     R0,[R4, #+54]
        CMP      R0,#+31
??sj_runtime_poll_5:
        BNE.N    ??sj_runtime_poll_12
??sj_runtime_poll_6:
        MOVS     R0,#+3
??sj_runtime_poll_1:
        ADD      SP,SP,#+64
          CFI CFA R13+16
        POP      {R1,R4,R5,PC}
          CFI CFA R13+80
??sj_runtime_poll_10:
        BL       ?Subroutine2
??CrossCallReturnLabel_5:
        CBZ.N    R0,??sj_runtime_poll_13
        MOVS     R0,#+1
        STRB     R0,[R4, #+60]
??sj_runtime_poll_13:
        LDRH     R1,[R4, #+52]
        ADDS     R1,R1,#+64
        UXTH     R0,R1
        CMP      R0,#+4096
        BNE.N    ??sj_runtime_poll_14
        MOVS     R1,#+0
        LDRB     R0,[R4, #+60]
        CMP      R0,#+0
        ITE      NE
        MOVNE    R0,#+1
        MOVEQ    R0,#+30
        STRB     R0,[R4, #+54]
??sj_runtime_poll_14:
        STRH     R1,[R4, #+52]
        B.N      ??sj_runtime_poll_11
??sj_runtime_poll_15:
        MOVS     R0,#+2
??sj_runtime_poll_16:
        BL       ?Subroutine7
??CrossCallReturnLabel_14:
        MOV      R0,#+2072576
??sj_runtime_poll_17:
          CFI FunCall sjr_start
        BL       sjr_start
        B.N      ??sj_runtime_poll_11
??sj_runtime_poll_18:
        MOVS     R0,#+3
        BL       ?Subroutine7
??CrossCallReturnLabel_15:
        LDR.N    R0,??DataTable6_1
        B.N      ??sj_runtime_poll_17
??sj_runtime_poll_19:
        BL       ?Subroutine3
??CrossCallReturnLabel_6:
        CMP      R0,#+0
        BEQ.N    ??sj_runtime_poll_8
        MOVS     R2,#+64
        ADD      R1,R4,#+61
        LDR      R0,[R4, #+12]
          CFI FunCall crc32_update
        BL       crc32_update
        STR      R0,[R4, #+12]
        MOVS     R0,#+4
        BL       ?Subroutine8
??CrossCallReturnLabel_17:
        BL       ?Subroutine10
??CrossCallReturnLabel_21:
        B.N      ??sj_runtime_poll_17
??sj_runtime_poll_20:
        BL       ?Subroutine11
??CrossCallReturnLabel_23:
        BNE.N    ??sj_runtime_poll_21
        MOVS     R0,#+0
        MOV      R1,#+4294967295
        STR      R1,[R4, #+16]
        MOVS     R1,#+5
??sj_runtime_poll_22:
        STRB     R1,[R4, #+54]
        STRH     R0,[R4, #+52]
??sj_runtime_poll_12:
        MOVS     R0,#+1
??sj_runtime_poll_23:
        ADD      SP,SP,#+64
          CFI CFA R13+16
        POP      {R1,R4,R5,PC}
          CFI CFA R13+80
??sj_runtime_poll_21:
        MOVS     R1,#+3
        B.N      ??sj_runtime_poll_22
??sj_runtime_poll_24:
        MOVS     R2,#+64
        MOV      R1,SP
        BL       ?Subroutine10
??CrossCallReturnLabel_22:
          CFI FunCall w25_read_data
        BL       w25_read_data
        CMP      R0,#+0
??sj_runtime_poll_25:
        BEQ.N    ??sj_runtime_poll_8
        BL       ?Subroutine4
??CrossCallReturnLabel_9:
        BL       ?Subroutine9
??CrossCallReturnLabel_19:
        BNE.N    ??sj_runtime_poll_11
        LDR      R0,[R4, #+16]
        LDR      R1,[R4, #+12]
        CMP      R0,R1
??sj_runtime_poll_26:
        BNE.N    ??sj_runtime_poll_8
        LDR.N    R0,??DataTable6_2
        STR      R0,[R4, #+20]
        MOVS     R0,#+1
        STR      R0,[R4, #+24]
        LDR      R0,[R4, #+8]
        STR      R0,[R4, #+28]
        MOV      R0,#+4096
        STR      R0,[R4, #+32]
        MVNS     R0,R1
        STR      R0,[R4, #+36]
        STR      R1,[R4, #+40]
        MOVS     R2,#+24
        ADD      R1,R4,#+20
        MOV      R0,#+4294967295
          CFI FunCall crc32_update
        BL       crc32_update
        MVNS     R0,R0
        STR      R0,[R4, #+44]
        LDR.N    R0,??DataTable6_3
        STR      R0,[R4, #+48]
        MOVS     R0,#+6
??sj_runtime_poll_27:
        STRB     R0,[R4, #+54]
        B.N      ??sj_runtime_poll_12
??sj_runtime_poll_28:
        MOVS     R0,#+7
        STR      R0,[SP, #+0]
        MOV      R3,R5
        MOVS     R2,#+28
        ADD      R1,R4,#+20
        B.N      ??CrossCallReturnLabel_14
??sj_runtime_poll_29:
        BL       ?Subroutine5
??CrossCallReturnLabel_11:
        CMP      R0,#+0
        BEQ.N    ??sj_runtime_poll_25
        MOVS     R2,#+28
        BL       ?Subroutine6
??CrossCallReturnLabel_13:
        CMP      R0,#+0
        ITT      EQ
        LDRBEQ   R0,[SP, #+28]
        CMPEQ    R0,#+255
        BNE.N    ??sj_runtime_poll_26
        LDRB     R0,[SP, #+29]
        CMP      R0,#+255
        ITTTT    EQ
        LDRBEQ   R0,[SP, #+30]
        CMPEQ    R0,#+255
        LDRBEQ   R0,[SP, #+31]
        CMPEQ    R0,#+255
        BNE.N    ??sj_runtime_poll_26
        MOVS     R0,#+8
        B.N      ??sj_runtime_poll_27
??sj_runtime_poll_30:
        MOVS     R0,#+1
        STRB     R0,[R4, #+57]
        MOVS     R0,#+9
        STR      R0,[SP, #+0]
        MOV      R3,R5
        MOVS     R2,#+4
        ADD      R1,R4,#+48
        LDR.N    R0,??DataTable6_4
        B.N      ??sj_runtime_poll_17
??sj_runtime_poll_31:
        BL       ?Subroutine5
??CrossCallReturnLabel_10:
        CMP      R0,#+0
        BEQ.N    ??sj_runtime_poll_25
        MOVS     R2,#+32
        BL       ?Subroutine6
??CrossCallReturnLabel_12:
        CMP      R0,#+0
        BNE.N    ??sj_runtime_poll_26
        MOVS     R0,#+10
        B.N      ??sj_runtime_poll_27
??sj_runtime_poll_32:
        MOVS     R0,#+11
        BL       ?Subroutine7
??CrossCallReturnLabel_16:
        LDR      R0,[R4, #+8]
        B.N      ??sj_runtime_poll_17
??sj_runtime_poll_33:
        MOVS     R0,#+0
        STRH     R0,[R4, #+52]
        MOVS     R0,#+12
        B.N      ??sj_runtime_poll_27
??sj_runtime_poll_34:
        BL       ?Subroutine0
??CrossCallReturnLabel_1:
        CMP      R0,#+0
        BEQ.N    ??sj_runtime_poll_25
        MOVS     R0,#+13
        BL       ?Subroutine8
??CrossCallReturnLabel_18:
        LDR      R0,[R4, #+8]
        LDRH     R5,[R4, #+52]
        ADDS     R0,R5,R0
        B.N      ??sj_runtime_poll_17
??sj_runtime_poll_35:
        BL       ?Subroutine11
??CrossCallReturnLabel_24:
        BNE.N    ??sj_runtime_poll_36
        MOVS     R0,#+0
        MOV      R1,#+4294967295
        STR      R1,[R4, #+16]
        MOVS     R1,#+14
        B.N      ??sj_runtime_poll_22
??sj_runtime_poll_36:
        MOVS     R1,#+12
        B.N      ??sj_runtime_poll_22
??sj_runtime_poll_37:
        BL       ?Subroutine1
??CrossCallReturnLabel_2:
        CMP      R0,#+0
        BEQ.N    ??sj_runtime_poll_38
        BL       ?Subroutine0
??CrossCallReturnLabel_0:
        CMP      R0,#+0
??sj_runtime_poll_38:
        BEQ.W    ??sj_runtime_poll_8
        BL       ?Subroutine2
??CrossCallReturnLabel_4:
        CMP      R0,#+0
        BNE.N    ??sj_runtime_poll_39
        BL       ?Subroutine4
??CrossCallReturnLabel_8:
        BL       ?Subroutine9
??CrossCallReturnLabel_20:
        BNE.W    ??sj_runtime_poll_11
        LDR      R1,[R4, #+16]
        LDR      R0,[R4, #+12]
        CMP      R1,R0
??sj_runtime_poll_39:
        BNE.W    ??sj_runtime_poll_8
        MOVS     R0,#+15
        B.N      ??sj_runtime_poll_27
??sj_runtime_poll_40:
        MOVS     R0,#+30
        B.N      ??sj_runtime_poll_16
??sj_runtime_poll_41:
        MOVS     R0,#+2
        ADD      SP,SP,#+64
          CFI CFA R13+16
        POP      {R1,R4,R5,PC}
          CFI CFA R13+80
          CFI EndBlock cfiBlock6

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable6:
        DATA32
        DC32     sjr

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable6_1:
        DATA32
        DC32     0x1f9000

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable6_2:
        DATA32
        DC32     0x314a4353

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable6_3:
        DATA32
        DC32     0x314b4f53

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable6_4:
        DATA32
        DC32     0x1fa01c

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond7 Using cfiCommon0
          CFI Function sj_runtime_poll
          CFI Conditional ??CrossCallReturnLabel_23
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+80
          CFI Block cfiCond8 Using cfiCommon0
          CFI (cfiCond8) Function sj_runtime_poll
          CFI (cfiCond8) Conditional ??CrossCallReturnLabel_24
          CFI (cfiCond8) R4 Frame(CFA, -12)
          CFI (cfiCond8) R5 Frame(CFA, -8)
          CFI (cfiCond8) R14 Frame(CFA, -4)
          CFI (cfiCond8) CFA R13+80
          CFI Block cfiPicker9 Using cfiCommon1
          CFI (cfiPicker9) NoFunction
          CFI (cfiPicker9) Picker
        THUMB
?Subroutine11:
        LDRH     R0,[R4, #+52]
        ADDS     R0,R0,#+64
        UXTH     R1,R0
        CMP      R1,#+4096
        BX       LR
          CFI EndBlock cfiCond7
          CFI EndBlock cfiCond8
          CFI EndBlock cfiPicker9

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond10 Using cfiCommon0
          CFI Function sj_runtime_poll
          CFI Conditional ??CrossCallReturnLabel_21
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+80
          CFI Block cfiCond11 Using cfiCommon0
          CFI (cfiCond11) Function sj_runtime_poll
          CFI (cfiCond11) Conditional ??CrossCallReturnLabel_22
          CFI (cfiCond11) R4 Frame(CFA, -12)
          CFI (cfiCond11) R5 Frame(CFA, -8)
          CFI (cfiCond11) R14 Frame(CFA, -4)
          CFI (cfiCond11) CFA R13+80
          CFI Block cfiPicker12 Using cfiCommon1
          CFI (cfiPicker12) NoFunction
          CFI (cfiPicker12) Picker
        THUMB
?Subroutine10:
        LDRH     R0,[R4, #+52]
        ADD      R0,R0,#+1048576
        ADD      R0,R0,#+1019904
        BX       LR
          CFI EndBlock cfiCond10
          CFI EndBlock cfiCond11
          CFI EndBlock cfiPicker12

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond13 Using cfiCommon0
          CFI Function sj_runtime_poll
          CFI Conditional ??CrossCallReturnLabel_19
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+80
          CFI Block cfiCond14 Using cfiCommon0
          CFI (cfiCond14) Function sj_runtime_poll
          CFI (cfiCond14) Conditional ??CrossCallReturnLabel_20
          CFI (cfiCond14) R4 Frame(CFA, -12)
          CFI (cfiCond14) R5 Frame(CFA, -8)
          CFI (cfiCond14) R14 Frame(CFA, -4)
          CFI (cfiCond14) CFA R13+80
          CFI Block cfiPicker15 Using cfiCommon1
          CFI (cfiPicker15) NoFunction
          CFI (cfiPicker15) Picker
        THUMB
?Subroutine9:
        STR      R0,[R4, #+16]
        LDRH     R0,[R4, #+52]
        ADDS     R0,R0,#+64
        STRH     R0,[R4, #+52]
        LDRH     R0,[R4, #+52]
        CMP      R0,#+4096
        BX       LR
          CFI EndBlock cfiCond13
          CFI EndBlock cfiCond14
          CFI EndBlock cfiPicker15

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond16 Using cfiCommon0
          CFI Function sj_runtime_poll
          CFI Conditional ??CrossCallReturnLabel_17
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+80
          CFI Block cfiCond17 Using cfiCommon0
          CFI (cfiCond17) Function sj_runtime_poll
          CFI (cfiCond17) Conditional ??CrossCallReturnLabel_18
          CFI (cfiCond17) R4 Frame(CFA, -12)
          CFI (cfiCond17) R5 Frame(CFA, -8)
          CFI (cfiCond17) R14 Frame(CFA, -4)
          CFI (cfiCond17) CFA R13+80
          CFI Block cfiPicker18 Using cfiCommon1
          CFI (cfiPicker18) NoFunction
          CFI (cfiPicker18) Picker
        THUMB
?Subroutine8:
        STR      R0,[SP, #+0]
        MOV      R3,R5
        MOVS     R2,#+64
        ADD      R1,R4,#+61
        BX       LR
          CFI EndBlock cfiCond16
          CFI EndBlock cfiCond17
          CFI EndBlock cfiPicker18

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond19 Using cfiCommon0
          CFI Function sj_runtime_poll
          CFI Conditional ??CrossCallReturnLabel_14
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+80
          CFI Block cfiCond20 Using cfiCommon0
          CFI (cfiCond20) Function sj_runtime_poll
          CFI (cfiCond20) Conditional ??CrossCallReturnLabel_15
          CFI (cfiCond20) R4 Frame(CFA, -12)
          CFI (cfiCond20) R5 Frame(CFA, -8)
          CFI (cfiCond20) R14 Frame(CFA, -4)
          CFI (cfiCond20) CFA R13+80
          CFI Block cfiCond21 Using cfiCommon0
          CFI (cfiCond21) Function sj_runtime_poll
          CFI (cfiCond21) Conditional ??CrossCallReturnLabel_16
          CFI (cfiCond21) R4 Frame(CFA, -12)
          CFI (cfiCond21) R5 Frame(CFA, -8)
          CFI (cfiCond21) R14 Frame(CFA, -4)
          CFI (cfiCond21) CFA R13+80
          CFI Block cfiPicker22 Using cfiCommon1
          CFI (cfiPicker22) NoFunction
          CFI (cfiPicker22) Picker
        THUMB
?Subroutine7:
        STR      R0,[SP, #+0]
        MOV      R3,R5
        MOVS     R2,#+0
        MOVS     R1,#+0
        BX       LR
          CFI EndBlock cfiCond19
          CFI EndBlock cfiCond20
          CFI EndBlock cfiCond21
          CFI EndBlock cfiPicker22

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond23 Using cfiCommon0
          CFI Function sj_runtime_poll
          CFI Conditional ??CrossCallReturnLabel_13
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+80
          CFI Block cfiCond24 Using cfiCommon0
          CFI (cfiCond24) Function sj_runtime_poll
          CFI (cfiCond24) Conditional ??CrossCallReturnLabel_12
          CFI (cfiCond24) R4 Frame(CFA, -12)
          CFI (cfiCond24) R5 Frame(CFA, -8)
          CFI (cfiCond24) R14 Frame(CFA, -4)
          CFI (cfiCond24) CFA R13+80
          CFI Block cfiPicker25 Using cfiCommon1
          CFI (cfiPicker25) NoFunction
          CFI (cfiPicker25) Picker
        THUMB
?Subroutine6:
        MOV      R1,SP
        ADD      R0,R4,#+20
          CFI (cfiCond23) FunCall sj_runtime_poll memcmp
          CFI (cfiCond23) FunCall sj_runtime_poll memcmp
        B.W      memcmp
          CFI EndBlock cfiCond23
          CFI EndBlock cfiCond24
          CFI EndBlock cfiPicker25

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond26 Using cfiCommon0
          CFI Function sj_runtime_poll
          CFI Conditional ??CrossCallReturnLabel_11
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+80
          CFI Block cfiCond27 Using cfiCommon0
          CFI (cfiCond27) Function sj_runtime_poll
          CFI (cfiCond27) Conditional ??CrossCallReturnLabel_10
          CFI (cfiCond27) R4 Frame(CFA, -12)
          CFI (cfiCond27) R5 Frame(CFA, -8)
          CFI (cfiCond27) R14 Frame(CFA, -4)
          CFI (cfiCond27) CFA R13+80
          CFI Block cfiPicker28 Using cfiCommon1
          CFI (cfiPicker28) NoFunction
          CFI (cfiPicker28) Picker
        THUMB
?Subroutine5:
        MOVS     R2,#+32
        MOV      R1,SP
        MOV      R0,#+2072576
          CFI (cfiCond26) FunCall sj_runtime_poll w25_read_data
          CFI (cfiCond26) FunCall sj_runtime_poll w25_read_data
        B.W      w25_read_data
          CFI EndBlock cfiCond26
          CFI EndBlock cfiCond27
          CFI EndBlock cfiPicker28

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond29 Using cfiCommon0
          CFI Function sj_runtime_poll
          CFI Conditional ??CrossCallReturnLabel_9
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+80
          CFI Block cfiCond30 Using cfiCommon0
          CFI (cfiCond30) Function sj_runtime_poll
          CFI (cfiCond30) Conditional ??CrossCallReturnLabel_8
          CFI (cfiCond30) R4 Frame(CFA, -12)
          CFI (cfiCond30) R5 Frame(CFA, -8)
          CFI (cfiCond30) R14 Frame(CFA, -4)
          CFI (cfiCond30) CFA R13+80
          CFI Block cfiPicker31 Using cfiCommon1
          CFI (cfiPicker31) NoFunction
          CFI (cfiPicker31) Picker
        THUMB
?Subroutine4:
        MOVS     R2,#+64
        MOV      R1,SP
        LDR      R0,[R4, #+16]
          CFI (cfiCond29) FunCall sj_runtime_poll crc32_update
          CFI (cfiCond29) FunCall sj_runtime_poll crc32_update
        B.W      crc32_update
          CFI EndBlock cfiCond29
          CFI EndBlock cfiCond30
          CFI EndBlock cfiPicker31

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond32 Using cfiCommon0
          CFI Function sj_runtime_poll
          CFI Conditional ??CrossCallReturnLabel_7
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+80
          CFI Block cfiCond33 Using cfiCommon0
          CFI (cfiCond33) Function sj_runtime_poll
          CFI (cfiCond33) Conditional ??CrossCallReturnLabel_6
          CFI (cfiCond33) R4 Frame(CFA, -12)
          CFI (cfiCond33) R5 Frame(CFA, -8)
          CFI (cfiCond33) R14 Frame(CFA, -4)
          CFI (cfiCond33) CFA R13+80
          CFI Block cfiPicker34 Using cfiCommon1
          CFI (cfiPicker34) NoFunction
          CFI (cfiPicker34) Picker
        THUMB
?Subroutine3:
        ADD      R2,R4,#+61
        LDRH     R1,[R4, #+52]
        LDR      R0,[R4, #+4]
        LDR      R3,[R4, #+0]
          CFI (cfiCond32) IndirectCall
          CFI (cfiCond32) IndirectCall
        ANOTE "tailcall"
        BX       R3
          CFI EndBlock cfiCond32
          CFI EndBlock cfiCond33
          CFI EndBlock cfiPicker34

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond35 Using cfiCommon0
          CFI Function sj_runtime_poll
          CFI Conditional ??CrossCallReturnLabel_5
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+80
          CFI Block cfiCond36 Using cfiCommon0
          CFI (cfiCond36) Function sj_runtime_poll
          CFI (cfiCond36) Conditional ??CrossCallReturnLabel_4
          CFI (cfiCond36) R4 Frame(CFA, -12)
          CFI (cfiCond36) R5 Frame(CFA, -8)
          CFI (cfiCond36) R14 Frame(CFA, -4)
          CFI (cfiCond36) CFA R13+80
          CFI Block cfiPicker37 Using cfiCommon1
          CFI (cfiPicker37) NoFunction
          CFI (cfiPicker37) Picker
        THUMB
?Subroutine2:
        MOVS     R2,#+64
        MOV      R1,SP
        ADD      R0,R4,#+61
          CFI (cfiCond35) FunCall sj_runtime_poll memcmp
          CFI (cfiCond35) FunCall sj_runtime_poll memcmp
        B.W      memcmp
          CFI EndBlock cfiCond35
          CFI EndBlock cfiCond36
          CFI EndBlock cfiPicker37

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond38 Using cfiCommon0
          CFI Function sj_runtime_poll
          CFI Conditional ??CrossCallReturnLabel_3
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+80
          CFI Block cfiCond39 Using cfiCommon0
          CFI (cfiCond39) Function sj_runtime_poll
          CFI (cfiCond39) Conditional ??CrossCallReturnLabel_2
          CFI (cfiCond39) R4 Frame(CFA, -12)
          CFI (cfiCond39) R5 Frame(CFA, -8)
          CFI (cfiCond39) R14 Frame(CFA, -4)
          CFI (cfiCond39) CFA R13+80
          CFI Block cfiPicker40 Using cfiCommon1
          CFI (cfiPicker40) NoFunction
          CFI (cfiPicker40) Picker
        THUMB
?Subroutine1:
        MOVS     R2,#+64
        MOV      R1,SP
        LDR      R0,[R4, #+8]
        LDRH     R3,[R4, #+52]
        ADDS     R0,R3,R0
          CFI (cfiCond38) FunCall sj_runtime_poll w25_read_data
          CFI (cfiCond38) FunCall sj_runtime_poll w25_read_data
        B.W      w25_read_data
          CFI EndBlock cfiCond38
          CFI EndBlock cfiCond39
          CFI EndBlock cfiPicker40

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond41 Using cfiCommon0
          CFI Function sj_runtime_poll
          CFI Conditional ??CrossCallReturnLabel_1
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+80
          CFI Block cfiCond42 Using cfiCommon0
          CFI (cfiCond42) Function sj_runtime_poll
          CFI (cfiCond42) Conditional ??CrossCallReturnLabel_0
          CFI (cfiCond42) R4 Frame(CFA, -12)
          CFI (cfiCond42) R5 Frame(CFA, -8)
          CFI (cfiCond42) R14 Frame(CFA, -4)
          CFI (cfiCond42) CFA R13+80
          CFI Block cfiPicker43 Using cfiCommon1
          CFI (cfiPicker43) NoFunction
          CFI (cfiPicker43) Picker
        THUMB
?Subroutine0:
        MOVS     R2,#+64
        ADD      R1,R4,#+61
        LDRH     R0,[R4, #+52]
        ADD      R0,R0,#+1048576
        ADD      R0,R0,#+1019904
          CFI (cfiCond41) FunCall sj_runtime_poll w25_read_data
          CFI (cfiCond41) FunCall sj_runtime_poll w25_read_data
        B.W      w25_read_data
          CFI EndBlock cfiCond41
          CFI EndBlock cfiCond42
          CFI EndBlock cfiPicker43

        SECTION `.iar_vfe_header`:DATA:NOALLOC:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
        DC32 0

        END
