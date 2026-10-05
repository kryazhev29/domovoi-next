; Cortex-M3 / IAR. Executable production module.
; Reference: Tests/asm/reference/User/photo.c
        PUBLIC photo
        PUBLIC photo_copy_off
        PUBLIC photo_erased_to
        PUBLIC photo_pending_since
        PUBLIC photo_prep_phase
        PUBLIC photo_restore_phase

        RTMODEL "__CPP_Runtime", "1"
        RTMODEL "__SystemLibrary", "DLib"
        RTMODEL "__dlib_file_descriptor", "0"
        RTMODEL "__dlib_full_locale_support", "0"
        RTMODEL "__dlib_version", "6"
        AAPCS BASE,INTERWORK
        PRESERVE8
        REQUIRE8

        #define SHT_PROGBITS 0x1

        EXTERN __aeabi_memcpy
        EXTERN __aeabi_memcpy4
        EXTERN calloc
        EXTERN comm_channel_ready
        EXTERN comm_tx_acquire
        EXTERN comm_tx_busy
        EXTERN comm_tx_commit
        EXTERN comm_tx_release
        EXTERN config
        EXTERN crc16_calc
        EXTERN crc32_update
        EXTERN device
        EXTERN fast_all_ff
        EXTERN free
        EXTERN log_addPhotoEvent
        EXTERN malloc
        EXTERN mem
        EXTERN mem_erase_sector
        EXTERN mem_read
        EXTERN mem_write
        EXTERN memcmp
        EXTERN port_buffer
        EXTERN timers_getMillis
        EXTERN uart_clear
        EXTERN uart_getstate
        EXTERN uart_init
        EXTERN uart_isReceiving
        EXTERN uart_received
        EXTERN uart_send
        EXTERN uart_setRecTimeout
        EXTERN usb_send
        EXTERN user_debug
        EXTERN user_getPhotoCount
        EXTERN wdt_reload

        PUBLIC photo_cleanup_active
        PUBLIC photo_confirmed_offset
        PUBLIC photo_connection_lost
        PUBLIC photo_poll
        PUBLIC photo_recover
        PUBLIC photo_request
        PUBLIC photo_start
        PUBLIC photo_storage_end
        PUBLIC rs485_baud
        PUBLIC rs485_init
        PUBLIC rs485_set_config

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
photo:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
photo_erased_to:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
photo_pending_since:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(1)
        DATA
photo_copy_off:
        DS8 2

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
photo_prep_phase:
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
photo_restore_phase:
        DS8 1

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock0 Using cfiCommon0
          CFI Function photo_alloc
        THUMB
photo_alloc:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        LDR.W    R4,??DataTable17
        LDR      R0,[R4, #+0]
        CBNZ.N   R0,??photo_alloc_0
        MOVS     R1,#+180
        MOVS     R0,#+1
          CFI FunCall calloc
        BL       calloc
        STR      R0,[R4, #+0]
??photo_alloc_0:
        LDR      R0,[R4, #+0]
        CBZ.N    R0,??photo_alloc_1
        MOVS     R0,#+1
??photo_alloc_1:
        POP      {R4,PC}
          CFI EndBlock cfiBlock0

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock1 Using cfiCommon0
          CFI Function photo_stop
        THUMB
photo_stop:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI FunCall rs485_baud
        BL       rs485_baud
        BL       ?Subroutine2
??CrossCallReturnLabel_10:
        LDR.W    R0,??DataTable17
        MOVS     R2,#+0
        LDR      R1,[R0, #+0]
        STRB     R2,[R1, #+34]
        LDR      R0,[R0, #+0]
        STRH     R2,[R0, #+32]
        LDR.W    R0,??DataTable17_1
        STRB     R2,[R0, #+116]
        POP      {R0,PC}
          CFI EndBlock cfiBlock1

        SECTION `.text`:CODE:NOROOT(2)
          CFI Block cfiBlock2 Using cfiCommon0
          CFI Function rs485_baud
          CFI NoCalls
        THUMB
rs485_baud:
        LDR.W    R0,??DataTable17_2
        LDRB     R0,[R0, #+174]
        SUBS     R0,R0,#+1
        CMP      R0,#+3
        BHI.N    ??rs485_baud_1
        TBB      [PC, R0]
        DATA
??rs485_baud_0:
        DC8      0x2,0x5,0x8,0xB
        THUMB
??rs485_baud_2:
        MOV      R0,#+57600
        BX       LR
??rs485_baud_3:
        MOV      R0,#+38400
        BX       LR
??rs485_baud_4:
        MOV      R0,#+19200
        BX       LR
??rs485_baud_5:
        MOV      R0,#+9600
        BX       LR
??rs485_baud_1:
        MOV      R0,#+115200
        BX       LR
          CFI EndBlock cfiBlock2

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock3 Using cfiCommon0
          CFI Function rs485_init
        THUMB
rs485_init:
        PUSH     {R2-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+24
        MOVS     R4,#+0
        MOVS     R5,#+0
        LDR.W    R6,??DataTable17_2
        LDRB     R0,[R6, #+172]
        CMP      R0,#+2
        IT       GE
        STRBGE   R4,[R6, #+172]
        LDRB     R0,[R6, #+174]
        CMP      R0,#+5
        BLT.N    ??rs485_init_0
        LDR.W    R0,??DataTable17_3
          CFI FunCall user_debug
        BL       user_debug
        STRB     R4,[R6, #+174]
??rs485_init_0:
        MOVS     R1,#+0
??rs485_init_1:
        ADD      R0,R6,R1, LSL #+1
        LDRB     R0,[R0, #+180]
        CMP      R0,#+3
        IT       EQ
        MOVEQ    R4,#+1
        BEQ.N    ??rs485_init_2
        ADD      R0,R6,R1, LSL #+1
        LDRB     R0,[R0, #+180]
        CBZ.N    R0,??rs485_init_2
        MOVS     R5,#+1
??rs485_init_2:
        ADDS     R1,R1,#+1
        CMP      R1,#+8
        BLT.N    ??rs485_init_1
        TST      R4,R5
          CFI FunCall user_debug
        ITT      NE
        LDRNE.W  R0,??DataTable17_4
        BLNE     user_debug
          CFI FunCall rs485_baud
        BL       rs485_baud
        BL       ?Subroutine2
??CrossCallReturnLabel_9:
        MOVS     R1,#+10
        MOVS     R0,#+0
        POP      {R2-R6,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall uart_setRecTimeout
        B.W      uart_setRecTimeout
          CFI EndBlock cfiBlock3

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond4 Using cfiCommon0
          CFI Function photo_stop
          CFI Conditional ??CrossCallReturnLabel_10
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI Block cfiCond5 Using cfiCommon0
          CFI (cfiCond5) Function rs485_init
          CFI (cfiCond5) Conditional ??CrossCallReturnLabel_9
          CFI (cfiCond5) R4 Frame(CFA, -16)
          CFI (cfiCond5) R5 Frame(CFA, -12)
          CFI (cfiCond5) R6 Frame(CFA, -8)
          CFI (cfiCond5) R14 Frame(CFA, -4)
          CFI (cfiCond5) CFA R13+24
          CFI Block cfiPicker6 Using cfiCommon1
          CFI (cfiPicker6) NoFunction
          CFI (cfiPicker6) Picker
        THUMB
?Subroutine2:
        MOVS     R1,#+32
        STR      R1,[SP, #+0]
        LDR.W    R3,??DataTable17_5
        MOVS     R2,#+0
        MOV      R1,R0
        MOVS     R0,#+0
          CFI (cfiCond4) FunCall photo_stop uart_init
          CFI (cfiCond5) FunCall rs485_init uart_init
        B.W      uart_init
          CFI EndBlock cfiCond4
          CFI EndBlock cfiCond5
          CFI EndBlock cfiPicker6

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock7 Using cfiCommon0
          CFI Function rs485_set_config
        THUMB
rs485_set_config:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R4,R0
        LDR.W    R5,??DataTable17_2
        LDRB     R1,[R5, #+174]
        LDRB     R0,[R4, #+174]
        CMP      R1,R0
        ITTT     EQ
        LDRBEQ   R1,[R5, #+172]
        LDRBEQ   R0,[R4, #+172]
        CMPEQ    R1,R0
        BNE.N    ??rs485_set_config_0
        MOVS     R2,#+16
        ADD      R1,R4,#+179
        ADD      R0,R5,#+179
          CFI FunCall memcmp
        BL       memcmp
        CBZ.N    R0,??rs485_set_config_1
??rs485_set_config_0:
        LDR.W    R1,??DataTable17
        LDR      R0,[R1, #+0]
        CMP      R0,#+0
        ITT      NE
        LDRBNE   R0,[R0, #+34]
        CMPNE    R0,#+0
        BEQ.N    ??rs485_set_config_1
        LDR.W    R0,??DataTable17_6
          CFI FunCall user_debug
        BL       user_debug
          CFI FunCall photo_stop
        BL       photo_stop
??rs485_set_config_1:
        MOV      R2,#+512
        MOV      R1,R4
        MOV      R0,R5
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        LDR.W    R1,??DataTable17
        LDR      R0,[R1, #+0]
        CMP      R0,#+0
        ITT      NE
        LDRBNE   R0,[R0, #+34]
        CMPNE    R0,#+0
        BNE.N    ??rs485_set_config_2
        POP      {R0,R4,R5,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall rs485_init
        B.N      rs485_init
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
??rs485_set_config_2:
        POP      {R0,R4,R5,PC}
          CFI EndBlock cfiBlock7

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock8 Using cfiCommon0
          CFI Function photo_fail
        THUMB
photo_fail:
        PUSH     {R4-R8,LR}
          CFI R14 Frame(CFA, -4)
          CFI R8 Frame(CFA, -8)
          CFI R7 Frame(CFA, -12)
          CFI R6 Frame(CFA, -16)
          CFI R5 Frame(CFA, -20)
          CFI R4 Frame(CFA, -24)
          CFI CFA R13+24
        SUB      SP,SP,#+96
          CFI CFA R13+120
        MOV      R7,R0
        MOV      R0,SP
        LDR.W    R1,??DataTable17_7
        MOVS     R2,#+96
          CFI FunCall __aeabi_memcpy4
        BL       __aeabi_memcpy4
        MOVS     R5,#+7
        LDR.W    R6,??DataTable17
        LDR      R0,[R6, #+0]
        LDRB     R2,[R0, #+34]
        LDRB     R1,[R0, #+35]
        LDRH     R8,[R0, #+40]
        B.N      ??photo_fail_0
??photo_fail_1:
        LDRB     R3,[R7], #+1
        STRB     R3,[SP, R0]
        ADDS     R5,R5,#+1
??photo_fail_0:
        LDRB     R0,[R7, #+0]
        CBZ.N    R0,??photo_fail_2
        UXTB     R0,R5
        CMP      R0,#+40
        BLT.N    ??photo_fail_1
??photo_fail_2:
        LDR.W    R7,??DataTable17_8
        UXTB     R3,R5
        ADD      R0,SP,R3
        LDR      R3,[R7, #+0]
        STR      R3,[R0, #+0]
        LDR      R3,[R7, #+4]
        STR      R3,[R0, #+4]
        ADDS     R5,R5,#+8
        ADDS     R2,R2,#+48
        UXTB     R0,R5
        STRB     R2,[SP, R0]
        ADDS     R5,R5,#+1
        LDR.W    R3,??DataTable18
        UXTB     R2,R5
        ADD      R0,SP,R2
        LDR      R2,[R3, #+0]
        STR      R2,[R0, #+0]
        LDR      R2,[R3, #+4]
        STR      R2,[R0, #+4]
        ADDS     R5,R5,#+8
        MOVS     R0,#+10
        SDIV     R0,R1,R0
        MOVS     R4,#+10
        MOVS     R2,#+10
        SDIV     R2,R0,R2
        MLS      R0,R4,R2,R0
        ADDS     R0,R0,#+48
        UXTB     R2,R5
        STRB     R0,[SP, R2]
        ADDS     R5,R5,#+1
        MOVS     R0,#+10
        SDIV     R0,R1,R0
        MLS      R1,R4,R0,R1
        ADDS     R1,R1,#+48
        UXTB     R0,R5
        STRB     R1,[SP, R0]
        ADDS     R5,R5,#+1
        MOVS     R2,#+5
        LDR.W    R1,??DataTable18_1
        UXTB     R0,R5
        ADD      R0,SP,R0
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        ADDS     R5,R5,#+5
        MOVW     R0,#+10000
        BL       ?Subroutine5
??CrossCallReturnLabel_15:
        MOV      R0,#+1000
        BL       ?Subroutine5
??CrossCallReturnLabel_16:
        MOVS     R0,#+100
        SDIV     R0,R8,R0
        BL       ?Subroutine4
??CrossCallReturnLabel_13:
        BL       ?Subroutine4
??CrossCallReturnLabel_14:
        MLS      R4,R4,R0,R8
        ADDS     R4,R4,#+48
        UXTB     R0,R5
        STRB     R4,[SP, R0]
        ADDS     R5,R5,#+1
        MOVS     R0,#+0
??photo_fail_3:
        CMP      R0,R8
        BGE.N    ??photo_fail_4
        MOVS     R2,#+32
        UXTB     R1,R5
        STRB     R2,[SP, R1]
        ADDS     R5,R5,#+1
        LDR.W    R1,??DataTable18_2
        LDR      R2,[R6, #+0]
        ADD      R2,R2,R0
        LDRB     R2,[R2, #+42]
        LSRS     R2,R2,#+4
        LDRB     R3,[R1, R2]
        UXTB     R2,R5
        STRB     R3,[SP, R2]
        ADDS     R5,R5,#+1
        LDR      R2,[R6, #+0]
        ADD      R2,R2,R0
        LDRB     R2,[R2, #+42]
        AND      R2,R2,#0xF
        LDRB     R2,[R1, R2]
        UXTB     R1,R5
        STRB     R2,[SP, R1]
        ADDS     R5,R5,#+1
        ADDS     R0,R0,#+1
        UXTB     R0,R0
        CMP      R0,#+8
        BCC.N    ??photo_fail_3
??photo_fail_4:
        MOVS     R2,#+3
        ADR.N    R1,??DataTable10
        UXTB     R5,R5
        ADD      R0,SP,R5
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        MOV      R0,SP
          CFI FunCall user_debug
        BL       user_debug
          CFI FunCall photo_stop
        BL       photo_stop
        ADD      SP,SP,#+96
          CFI CFA R13+24
        POP      {R4-R8,PC}
          CFI EndBlock cfiBlock8

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond9 Using cfiCommon0
          CFI Function photo_fail
          CFI Conditional ??CrossCallReturnLabel_15
          CFI R4 Frame(CFA, -24)
          CFI R5 Frame(CFA, -20)
          CFI R6 Frame(CFA, -16)
          CFI R7 Frame(CFA, -12)
          CFI R8 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+120
          CFI Block cfiCond10 Using cfiCommon0
          CFI (cfiCond10) Function photo_fail
          CFI (cfiCond10) Conditional ??CrossCallReturnLabel_16
          CFI (cfiCond10) R4 Frame(CFA, -24)
          CFI (cfiCond10) R5 Frame(CFA, -20)
          CFI (cfiCond10) R6 Frame(CFA, -16)
          CFI (cfiCond10) R7 Frame(CFA, -12)
          CFI (cfiCond10) R8 Frame(CFA, -8)
          CFI (cfiCond10) R14 Frame(CFA, -4)
          CFI (cfiCond10) CFA R13+120
          CFI Block cfiPicker11 Using cfiCommon1
          CFI (cfiPicker11) NoFunction
          CFI (cfiPicker11) Picker
        THUMB
?Subroutine5:
        SDIV     R0,R8,R0
        MOVS     R1,#+10
        SDIV     R1,R0,R1
        MLS      R0,R4,R1,R0
        ADDS     R0,R0,#+48
        UXTB     R1,R5
        STRB     R0,[SP, R1]
        ADDS     R5,R5,#+1
        BX       LR
          CFI EndBlock cfiCond9
          CFI EndBlock cfiCond10
          CFI EndBlock cfiPicker11

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond12 Using cfiCommon0
          CFI Function photo_fail
          CFI Conditional ??CrossCallReturnLabel_13
          CFI R4 Frame(CFA, -24)
          CFI R5 Frame(CFA, -20)
          CFI R6 Frame(CFA, -16)
          CFI R7 Frame(CFA, -12)
          CFI R8 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+120
          CFI Block cfiCond13 Using cfiCommon0
          CFI (cfiCond13) Function photo_fail
          CFI (cfiCond13) Conditional ??CrossCallReturnLabel_14
          CFI (cfiCond13) R4 Frame(CFA, -24)
          CFI (cfiCond13) R5 Frame(CFA, -20)
          CFI (cfiCond13) R6 Frame(CFA, -16)
          CFI (cfiCond13) R7 Frame(CFA, -12)
          CFI (cfiCond13) R8 Frame(CFA, -8)
          CFI (cfiCond13) R14 Frame(CFA, -4)
          CFI (cfiCond13) CFA R13+120
          CFI Block cfiPicker14 Using cfiCommon1
          CFI (cfiPicker14) NoFunction
          CFI (cfiPicker14) Picker
        THUMB
?Subroutine4:
        MOVS     R1,#+10
        SDIV     R1,R0,R1
        MLS      R0,R4,R1,R0
        ADDS     R0,R0,#+48
        UXTB     R1,R5
        STRB     R0,[SP, R1]
        ADDS     R5,R5,#+1
        MOVS     R0,#+10
        SDIV     R0,R8,R0
        BX       LR
          CFI EndBlock cfiCond12
          CFI EndBlock cfiCond13
          CFI EndBlock cfiPicker14

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_0:
        DATA8
        DC8 "RS485: invalid baud code, using 115200\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_1:
        DATA8
        DC8 "RS485: C328 exclusive, other devices ignored\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_2:
        DATA8
        DC8 "Photo: capture error, RS485 settings changed\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_3:
        DATA8
        DC8 ", stage "
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_4:
        DATA8
        DC8 ", retry "
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_5:
        DATA8
        DC8 ", RX "
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
        DATA8
        DC8 "\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_7:
        DATA8
        DC8 "Photo: clearing storage\015\012"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_8:
        DATA8
        DC8 "Photo: no space for next image\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_9:
        DATA8
        DC8 "Photo: invalid image size\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_10:
        DATA8
        DC8 "Photo: no space for image, queue retained\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_11:
        DATA8
        DC8 "Photo: camera sync OK\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_12:
        DATA8
        DC8 "Photo: invalid number or offset\015\012"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_13:
        DATA8
        DC8 "Photo: confirmation journal full\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_14:
        DATA8
        DC8 "SLDN"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_15:
        DATA8
        DC8 "60 s limit"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_16:
        DATA8
        DC8 "no answer"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_17:
        DATA8
        DC8 "Photo: upload stalled for 60 seconds\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
`photo_fail::hex`:
        DATA8
        DC8 "0123456789ABCDEF"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_18:
        DATA8
        DC8 "Photo: "
        DATA
        DS8 88

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock15 Using cfiCommon0
          CFI Function photo_command
        THUMB
photo_command:
        PUSH     {R1-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+32
        LDR      R4,[SP, #+32]
        LDR.W    R5,??DataTable19
        LDRD     R7,R6,[R5, #+0]
        STRD     R7,R6,[SP, #+0]
        STRB     R0,[SP, #+1]
        STRB     R1,[SP, #+2]
        STRB     R2,[SP, #+3]
        STRB     R3,[SP, #+4]
        STRB     R4,[SP, #+5]
        MOVS     R2,#+6
        MOV      R1,SP
        MOVS     R0,#+0
          CFI FunCall uart_send
        BL       uart_send
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        LDR.W    R2,??DataTable17
        LDR      R1,[R2, #+0]
        STR      R0,[R1, #+16]
        ADD      R0,R0,#+2000
        LDR      R1,[R2, #+0]
        STR      R0,[R1, #+8]
        POP      {R0-R2,R4-R7,PC}
          CFI EndBlock cfiBlock15

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_19:
        DATA8
        DC8 170, 0, 0, 0, 0, 0
        DATA16
        DS8 2

        SECTION `.text`:CODE:NOROOT(2)
          CFI Block cfiBlock16 Using cfiCommon0
          CFI Function photo_issue
        THUMB
photo_issue:
        PUSH     {R2-R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+16
        LDR.W    R4,??DataTable17
        BL       ?Subroutine1
??CrossCallReturnLabel_3:
        SUBS     R0,R0,#+1
        CMP      R0,#+5
        BHI.N    ??photo_issue_1
        TBB      [PC, R0]
        DATA
??photo_issue_0:
        DC8      0x3,0x7,0xE,0x15
        DC8      0x19,0x20
        THUMB
??photo_issue_2:
        BL       ?Subroutine3
??CrossCallReturnLabel_11:
        MOVS     R0,#+13
        B.N      ??photo_issue_3
??photo_issue_4:
        MOVS     R0,#+7
        STR      R0,[SP, #+0]
        MOVS     R3,#+0
        MOVS     R2,#+7
        MOVS     R1,#+0
        MOVS     R0,#+1
        B.N      ??photo_issue_3
??photo_issue_5:
        MOVS     R0,#+0
        STR      R0,[SP, #+0]
        MOVS     R3,#+0
        MOVS     R2,#+64
        MOVS     R1,#+8
        MOVS     R0,#+6
        B.N      ??photo_issue_3
??photo_issue_6:
        BL       ?Subroutine3
??CrossCallReturnLabel_12:
        MOVS     R0,#+5
        B.N      ??photo_issue_3
??photo_issue_7:
        MOVS     R0,#+0
        STR      R0,[SP, #+0]
        MOVS     R3,#+0
        MOVS     R2,#+0
        MOVS     R1,#+1
        MOVS     R0,#+4
        B.N      ??photo_issue_3
??photo_issue_8:
        LDR      R0,[R4, #+0]
        LDRH     R0,[R0, #+30]
        LSRS     R0,R0,#+8
        STR      R0,[SP, #+0]
        LDR      R0,[R4, #+0]
        LDRH     R3,[R0, #+30]
        UXTB     R3,R3
        MOVS     R2,#+0
        MOVS     R1,#+0
        MOVS     R0,#+14
??photo_issue_3:
          CFI FunCall photo_command
        BL       photo_command
??photo_issue_1:
        BL       ?Subroutine1
??CrossCallReturnLabel_4:
        CMP      R0,#+1
        BNE.N    ??photo_issue_9
        LDR      R0,[R4, #+0]
        LDR      R0,[R0, #+16]
        ADDS     R0,R0,#+50
        LDR      R1,[R4, #+0]
        STR      R0,[R1, #+8]
??photo_issue_9:
        POP      {R0,R1,R4,PC}
          CFI EndBlock cfiBlock16

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond17 Using cfiCommon0
          CFI Function photo_issue
          CFI Conditional ??CrossCallReturnLabel_11
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
          CFI Block cfiCond18 Using cfiCommon0
          CFI (cfiCond18) Function photo_issue
          CFI (cfiCond18) Conditional ??CrossCallReturnLabel_12
          CFI (cfiCond18) R4 Frame(CFA, -8)
          CFI (cfiCond18) R14 Frame(CFA, -4)
          CFI (cfiCond18) CFA R13+16
          CFI Block cfiPicker19 Using cfiCommon1
          CFI (cfiPicker19) NoFunction
          CFI (cfiPicker19) Picker
        THUMB
?Subroutine3:
        MOVS     R0,#+0
        STR      R0,[SP, #+0]
        MOVS     R3,#+0
        MOVS     R2,#+0
        MOVS     R1,#+0
        BX       LR
          CFI EndBlock cfiCond17
          CFI EndBlock cfiCond18
          CFI EndBlock cfiPicker19

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock20 Using cfiCommon0
          CFI Function photo_start
        THUMB
photo_start:
        PUSH     {R1-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+32
        MOV      R6,R0
        MOVS     R1,#+0
        MOVS     R3,#+0
        LDR.W    R2,??DataTable17_2
??photo_start_0:
        ADD      R0,R2,R3, LSL #+1
        LDRB     R0,[R0, #+180]
        CMP      R0,#+3
        ITTTT    EQ
        ADDEQ    R0,R2,R3, LSL #+1
        LDRBEQ   R0,[R0, #+179]
        CMPEQ    R0,R6
        MOVEQ    R1,#+1
        ADDS     R3,R3,#+1
        CMP      R3,#+8
        BLT.N    ??photo_start_0
        CBZ.N    R1,??photo_start_1
        LDRB     R0,[R2, #+172]
        CMP      R0,#+1
        BEQ.N    ??photo_start_2
??photo_start_1:
        MOVS     R0,#+1
        POP      {R1-R7,PC}
??photo_start_2:
        LDR.W    R4,??DataTable17_1
        LDRH     R0,[R4, #+129]
        UBFX     R0,R0,#+11,#+1
        CMP      R0,#+0
        BNE.N    ??photo_start_3
          CFI FunCall photo_alloc
        BL       photo_alloc
        CMP      R0,#+0
        BEQ.N    ??photo_start_3
        LDR.W    R5,??DataTable17
        LDR      R0,[R5, #+0]
        LDRB     R0,[R0, #+34]
        CMP      R0,#+0
        BNE.N    ??photo_start_3
          CFI FunCall user_getPhotoCount
        BL       user_getPhotoCount
        STRH     R0,[R4, #+155]
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        LDR      R1,[R4, #+157]
        CMP      R0,R1
        BCC.N    ??photo_start_3
          CFI FunCall photo_cleanup_active
        BL       photo_cleanup_active
        CBNZ.N   R0,??photo_start_4
          CFI FunCall photo_recover
        BL       photo_recover
        CMP      R0,#+0
        BEQ.N    ??photo_start_3
        LDRH     R0,[R4, #+155]
        CBNZ.N   R0,??photo_start_5
        LDR      R0,[R4, #+157]
        LDR.W    R1,??DataTable21
        LDR      R1,[R1, #+72]
        CMP      R0,R1
        BEQ.N    ??photo_start_5
          CFI FunCall photo_clear_confirmed
        BL       photo_clear_confirmed
        CMP      R0,#+0
        BEQ.N    ??photo_start_3
??photo_start_4:
        LDR.W    R0,??DataTable21_1
        B.N      ??photo_start_6
??photo_start_5:
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        LDR      R1,[R4, #+157]
        ADDW     R1,R1,#+1033
        CMP      R1,R0
        BCC.N    ??photo_start_7
        LDR.W    R0,??DataTable21_2
??photo_start_6:
          CFI FunCall user_debug
        BL       user_debug
        B.N      ??photo_start_3
??photo_start_7:
        LDR      R1,[R4, #+157]
        LDR      R0,[R5, #+0]
        STR      R1,[R0, #+4]
        LDR      R0,[R4, #+47]
        CMP      R0,#+0
        ITE      NE
        MOVNE    R1,R0
        MOVEQ    R1,#+1
        LDR      R0,[R5, #+0]
        STR      R1,[R0, #+0]
        LDR.W    R0,??DataTable21
        LDR      R7,[R0, #+72]
        B.N      ??photo_start_8
??photo_start_9:
        LDR      R0,[SP, #+4]
        LDR      R1,[R5, #+0]
        LDR      R1,[R1, #+0]
        CMP      R0,R1
        BCC.N    ??photo_start_10
        ADDS     R0,R0,#+1
        LDR      R1,[R5, #+0]
        STR      R0,[R1, #+0]
??photo_start_10:
          CFI FunCall wdt_reload
        BL       wdt_reload
        LDR      R0,[SP, #+8]
        ADDS     R7,R0,R7
        ADDW     R7,R7,#+1033
??photo_start_8:
        LDR      R0,[R4, #+157]
        CMP      R7,R0
        BCS.N    ??photo_start_11
        MOVS     R2,#+8
        ADD      R1,SP,#+4
        MOV      R0,R7
          CFI FunCall mem_read
        BL       mem_read
        CMP      R0,#+0
        BNE.N    ??photo_start_9
        B.N      ??photo_start_3
??photo_start_11:
        LDR      R0,[R5, #+0]
        LDR      R0,[R0, #+0]
        CMN      R0,#+1
        BNE.N    ??photo_start_12
??photo_start_3:
        MOVS     R0,#+2
        POP      {R1-R7,PC}
??photo_start_12:
        MOVS     R1,#+0
        LDR      R0,[R5, #+0]
        STRH     R1,[R0, #+32]
        LDR      R0,[R5, #+0]
        STRH     R1,[R0, #+30]
        LDR      R0,[R5, #+0]
        STRH     R1,[R0, #+26]
        LDR      R0,[R5, #+0]
        STRH     R1,[R0, #+24]
        LDR.W    R0,??DataTable21_3
        STR      R1,[R0, #+0]
        LDR.W    R0,??DataTable22
        STRB     R1,[R0, #+0]
        LDR      R0,[R5, #+0]
        STRB     R1,[R0, #+37]
        LDR      R0,[R5, #+0]
        STRB     R1,[R0, #+36]
        MOVS     R0,#+0
        LDR      R1,[R5, #+0]
        STRB     R0,[R1, #+35]
        MOVS     R1,#+0
        LDR      R0,[R5, #+0]
        STRH     R1,[R0, #+40]
        MOVS     R0,#+1
        LDR      R1,[R5, #+0]
        STRB     R0,[R1, #+34]
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        LDR      R1,[R5, #+0]
        STR      R0,[R1, #+12]
        SUBS     R0,R0,#+50
        LDR      R1,[R5, #+0]
        STR      R0,[R1, #+16]
        LDR      R0,[R5, #+0]
        LDR      R1,[R0, #+12]
        STR      R1,[R0, #+8]
          CFI FunCall rs485_baud
        BL       rs485_baud
        MOV      R1,R0
        MOVS     R0,#+64
        STR      R0,[SP, #+0]
        LDR      R0,[R5, #+0]
        ADD      R3,R0,#+51
        MOVS     R2,#+0
        MOVS     R0,#+0
          CFI FunCall uart_init
        BL       uart_init
        STRB     R6,[R4, #+116]
        MOVS     R0,#+0
        POP      {R1-R7,PC}
          CFI EndBlock cfiBlock20

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock21 Using cfiCommon0
          CFI Function photo_write
        THUMB
photo_write:
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
          CFI FunCall mem_write
        BL       mem_write
        CBNZ.N   R0,??photo_write_0
        B.N      ??photo_write_1
??photo_write_2:
        ADDS     R4,R6,R4
        ADD      R5,R5,R6
        SUBS     R7,R7,R6
??photo_write_0:
        UXTH     R0,R7
        CBZ.N    R0,??photo_write_3
          CFI FunCall wdt_reload
        BL       wdt_reload
        UXTH     R0,R7
        CMP      R0,#+65
        ITE      CS
        MOVCS    R6,#+64
        UXTHCC   R6,R7
        MOV      R2,R6
        BL       ??Subroutine7_0
??CrossCallReturnLabel_19:
        CBZ.N    R0,??photo_write_1
        MOV      R2,R6
        MOV      R1,R5
        MOV      R0,SP
          CFI FunCall memcmp
        BL       memcmp
        CMP      R0,#+0
        BEQ.N    ??photo_write_2
??photo_write_1:
        MOVS     R0,#+0
        B.N      ??photo_write_4
??photo_write_3:
        MOVS     R0,#+1
??photo_write_4:
        ADD      SP,SP,#+68
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI EndBlock cfiBlock21

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock22 Using cfiCommon0
          CFI Function photo_storage_end
          CFI NoCalls
        THUMB
photo_storage_end:
        LDR.W    R1,??DataTable21
        LDR      R0,[R1, #+76]
        CMP      R0,#+8192
        ITE      CS
        SUBCS    R0,R0,#+8192
        MOVCC    R0,#+0
        LDR      R1,[R1, #+72]
        ADDS     R0,R0,R1
        BX       LR
          CFI EndBlock cfiBlock22

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable10:
        DATA8
        DC8      0x0D, 0x0A, 0x00, 0x00

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock23 Using cfiCommon0
          CFI Function photo_backup_crc
        THUMB
photo_backup_crc:
        PUSH     {R4-R8,LR}
          CFI R14 Frame(CFA, -4)
          CFI R8 Frame(CFA, -8)
          CFI R7 Frame(CFA, -12)
          CFI R6 Frame(CFA, -16)
          CFI R5 Frame(CFA, -20)
          CFI R4 Frame(CFA, -24)
          CFI CFA R13+24
        SUB      SP,SP,#+64
          CFI CFA R13+88
        MOV      R8,R0
        MOV      R5,R1
        MOV      R6,R2
        MOV      R4,#+4294967295
        B.N      ??photo_backup_crc_0
??photo_backup_crc_1:
        UXTH     R2,R7
        MOV      R1,SP
        MOV      R0,R4
          CFI FunCall crc32_update
        BL       crc32_update
        MOV      R4,R0
          CFI FunCall wdt_reload
        BL       wdt_reload
        UXTH     R0,R7
        ADD      R8,R0,R8
        SUBS     R5,R5,R7
??photo_backup_crc_0:
        UXTH     R0,R5
        CBZ.N    R0,??photo_backup_crc_2
        CMP      R0,#+65
        ITE      CS
        MOVCS    R7,#+64
        MOVCC    R7,R5
        UXTH     R2,R7
        MOV      R1,SP
        MOV      R0,R8
          CFI FunCall mem_read
        BL       mem_read
        CMP      R0,#+0
        BNE.N    ??photo_backup_crc_1
??photo_backup_crc_3:
        ADD      SP,SP,#+64
          CFI CFA R13+24
        POP      {R4-R8,PC}
          CFI CFA R13+88
??photo_backup_crc_2:
        STR      R4,[R6, #+0]
        MOVS     R0,#+1
        B.N      ??photo_backup_crc_3
          CFI EndBlock cfiBlock23

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock24 Using cfiCommon0
          CFI Function photo_cleanup_active
        THUMB
photo_cleanup_active:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        LDR.W    R1,??DataTable21
        LDR      R1,[R1, #+76]
        CMP      R1,#+8192
        BCS.N    ??photo_cleanup_active_0
        MOVS     R0,#+0
        POP      {R1,PC}
??photo_cleanup_active_0:
        MOVS     R2,#+4
        MOV      R1,SP
        ADD      R0,R0,#+4128
          CFI FunCall mem_read
        BL       mem_read
        CBNZ.N   R0,??photo_cleanup_active_1
        MOVS     R0,#+2
        POP      {R1,PC}
??photo_cleanup_active_1:
        LDR      R0,[SP, #+0]
        SUBS     R0,R0,#+1
        SBCS     R0,R0,R0
        LSRS     R0,R0,#+31
        POP      {R1,PC}
          CFI EndBlock cfiBlock24

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock25 Using cfiCommon0
          CFI Function photo_recover
        THUMB
photo_recover:
        PUSH     {R4,R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+12
        SUB      SP,SP,#+20
          CFI CFA R13+32
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        MOV      R4,R0
        LDR.W    R5,??DataTable21
        LDR      R0,[R5, #+76]
        CMP      R0,#+8192
        BCC.N    ??photo_recover_0
          CFI FunCall photo_cleanup_active
        BL       photo_cleanup_active
        CBNZ.N   R0,??photo_recover_0
        MOVS     R2,#+16
        ADD      R1,SP,#+4
        ADD      R0,R4,#+4096
          CFI FunCall mem_read
        BL       mem_read
        CBZ.N    R0,??photo_recover_0
        LDR      R1,[SP, #+16]
        LDR.W    R0,??DataTable23
        CMP      R1,R0
        IT       NE
        MOVNE    R0,#+1
        BNE.N    ??photo_recover_1
        LDR      R0,[SP, #+4]
        LDR      R1,[R5, #+72]
        CMP      R0,R1
        BCC.N    ??photo_recover_0
        CMP      R0,R4
        BCS.N    ??photo_recover_0
        LSLS     R0,R0,#+20
        BNE.N    ??photo_recover_0
        ADD      R1,SP,#+4
        LDR      R0,[R1, #+4]
        CBZ.N    R0,??photo_recover_0
        CMP      R0,#+4096
        BCS.N    ??photo_recover_0
        LDR.W    R0,??DataTable23_1
        LDRB     R0,[R0, #+0]
        CBNZ.N   R0,??photo_recover_0
        MOV      R2,SP
        LDR      R1,[R1, #+4]
        UXTH     R1,R1
        MOV      R0,R4
          CFI FunCall photo_backup_crc
        BL       photo_backup_crc
??photo_recover_0:
        MOVS     R0,#+0
??photo_recover_1:
        ADD      SP,SP,#+20
          CFI CFA R13+12
        POP      {R4,R5,PC}
          CFI EndBlock cfiBlock25

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock26 Using cfiCommon0
          CFI Function photo_restore_step
        THUMB
photo_restore_step:
        PUSH     {R3-R8,LR}
          CFI R14 Frame(CFA, -4)
          CFI R8 Frame(CFA, -8)
          CFI R7 Frame(CFA, -12)
          CFI R6 Frame(CFA, -16)
          CFI R5 Frame(CFA, -20)
          CFI R4 Frame(CFA, -24)
          CFI CFA R13+28
        SUB      SP,SP,#+84
          CFI CFA R13+112
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        MOV      R8,R0
        MOVS     R2,#+16
        ADD      R1,SP,#+4
        ADD      R0,R8,#+4096
          CFI FunCall mem_read
        BL       mem_read
        CMP      R0,#+0
        BEQ.N    ??photo_restore_step_0
        LDR      R1,[SP, #+16]
        LDR.W    R0,??DataTable23
        CMP      R1,R0
        BEQ.N    ??photo_restore_step_1
        MOVS     R1,#+0
        LDR.W    R0,??DataTable23_1
        STRB     R1,[R0, #+0]
        B.N      ??photo_restore_step_2
??photo_restore_step_1:
        LDR      R0,[SP, #+4]
        LDR.W    R1,??DataTable21
        LDR      R1,[R1, #+72]
        CMP      R0,R1
        BCC.N    ??photo_restore_step_2
        CMP      R0,R8
        BCS.N    ??photo_restore_step_2
        LSLS     R0,R0,#+20
        BNE.N    ??photo_restore_step_2
        ADD      R5,SP,#+4
        LDR      R0,[R5, #+4]
        CMP      R0,#+0
        BEQ.N    ??photo_restore_step_2
        CMP      R0,#+4096
        BCS.N    ??photo_restore_step_2
        LDR.W    R4,??DataTable23_1
        LDRB     R0,[R4, #+0]
        CBNZ.N   R0,??photo_restore_step_3
        MOV      R2,SP
        LDR      R1,[R5, #+4]
        UXTH     R1,R1
        MOV      R0,R8
          CFI FunCall photo_backup_crc
        BL       photo_backup_crc
        CMP      R0,#+0
        BEQ.N    ??photo_restore_step_2
        LDR      R0,[SP, #+0]
        LDR      R1,[SP, #+12]
        CMP      R0,R1
        BNE.N    ??photo_restore_step_2
??photo_restore_step_3:
        LDRB     R0,[R4, #+0]
        CBNZ.N   R0,??photo_restore_step_4
          CFI FunCall wdt_reload
        BL       wdt_reload
        LDR      R0,[SP, #+4]
          CFI FunCall mem_erase_sector
        BL       mem_erase_sector
        MOVS     R1,R0
        BEQ.N    ??photo_restore_step_5
        MOVS     R0,#+1
        STRB     R0,[R4, #+0]
        MOVS     R2,#+0
        LDR.W    R0,??DataTable25
        STRH     R2,[R0, #+0]
??photo_restore_step_5:
        MOVS     R0,R1
        BEQ.N    ??photo_restore_step_0
        B.N      ??photo_restore_step_6
??photo_restore_step_4:
        CMP      R0,#+1
        BNE.N    ??photo_restore_step_7
        LDR.W    R7,??DataTable25
        LDR      R1,[R5, #+4]
        LDRH     R0,[R7, #+0]
        SUBS     R1,R1,R0
        CMP      R1,#+65
        ITEE     CS
        MOVCS    R6,#+64
        LDRCC    R6,[R5, #+4]
        SUBCC    R6,R6,R0
        UXTH     R2,R6
        ADD      R1,SP,#+20
        ADD      R0,R0,R8
          CFI FunCall mem_read
        BL       mem_read
        CBZ.N    R0,??photo_restore_step_0
        UXTH     R2,R6
        ADD      R1,SP,#+20
        LDR      R0,[SP, #+4]
        LDRH     R3,[R7, #+0]
        ADDS     R0,R3,R0
          CFI FunCall photo_write
        BL       photo_write
        CBZ.N    R0,??photo_restore_step_0
        LDRH     R0,[R7, #+0]
        ADDS     R6,R6,R0
        UXTH     R0,R6
        LDR      R1,[R5, #+4]
        CMP      R0,R1
        ITT      EQ
        MOVEQ    R0,#+2
        STRBEQ   R0,[R4, #+0]
        STRH     R6,[R7, #+0]
??photo_restore_step_2:
        MOVS     R0,#+0
        B.N      ??photo_restore_step_0
??photo_restore_step_7:
          CFI FunCall wdt_reload
        BL       wdt_reload
        ADD      R0,R8,#+4096
          CFI FunCall mem_erase_sector
        BL       mem_erase_sector
        CBZ.N    R0,??photo_restore_step_0
        MOVS     R0,#+0
        STRB     R0,[R4, #+0]
          CFI FunCall user_getPhotoCount
        BL       user_getPhotoCount
??photo_restore_step_6:
        MOVS     R0,#+1
??photo_restore_step_0:
        ADD      SP,SP,#+88
          CFI CFA R13+24
        POP      {R4-R8,PC}
          CFI EndBlock cfiBlock26

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock27 Using cfiCommon0
          CFI Function photo_prepare
        THUMB
photo_prepare:
        PUSH     {R3-R9,LR}
          CFI R14 Frame(CFA, -4)
          CFI R9 Frame(CFA, -8)
          CFI R8 Frame(CFA, -12)
          CFI R7 Frame(CFA, -16)
          CFI R6 Frame(CFA, -20)
          CFI R5 Frame(CFA, -24)
          CFI R4 Frame(CFA, -28)
          CFI CFA R13+32
        SUB      SP,SP,#+64
          CFI CFA R13+96
        MOV      R9,R0
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        LDR.W    R6,??DataTable17
        LDR      R1,[R6, #+0]
        LDR      R4,[R1, #+4]
        UBFX     R4,R4,#+0,#+12
        MOVS     R7,#+0
        CMP      R9,#+0
        BEQ.N    ??photo_prepare_0
        CMP      R9,#+65536
        BCC.N    ??photo_prepare_1
??photo_prepare_0:
        LDR.W    R0,??DataTable25_1
        B.N      ??photo_prepare_2
??photo_prepare_1:
        LDR      R1,[R1, #+4]
        ADDW     R1,R1,#+1033
        CMP      R0,R1
        BCC.N    ??photo_prepare_3
        LDR      R1,[R6, #+0]
        LDR      R1,[R1, #+4]
        SUBS     R0,R0,R1
        SUBW     R0,R0,#+1033
        CMP      R0,R9
        BCS.N    ??photo_prepare_4
??photo_prepare_3:
        LDR.W    R0,??DataTable25_2
??photo_prepare_2:
          CFI FunCall user_debug
        BL       user_debug
        MOVS     R0,#+0
        B.N      ??photo_prepare_5
??photo_prepare_4:
        CBZ.N    R4,??photo_prepare_6
        MOV      R8,R1
        B.N      ??photo_prepare_7
??photo_prepare_8:
        UXTH     R1,R5
        MOV      R0,SP
          CFI FunCall fast_all_ff
        BL       fast_all_ff
        CBNZ.N   R0,??photo_prepare_9
        MOVS     R7,#+1
??photo_prepare_9:
          CFI FunCall wdt_reload
        BL       wdt_reload
        UXTH     R5,R5
        ADD      R8,R5,R8
??photo_prepare_7:
        BL       ?Subroutine6
??CrossCallReturnLabel_17:
        CMP      R8,R0
        BCS.N    ??photo_prepare_6
        BL       ?Subroutine6
??CrossCallReturnLabel_18:
        SUB      R0,R0,R8
        CMP      R0,#+65
        IT       CS
        MOVCS    R5,#+64
        BCS.N    ??photo_prepare_10
        LDR      R0,[R6, #+0]
        LDR      R5,[R0, #+4]
        SUBS     R5,R5,R4
        ADD      R5,R5,#+4096
        SUB      R5,R5,R8
??photo_prepare_10:
        UXTH     R2,R5
        MOV      R1,SP
        MOV      R0,R8
          CFI FunCall mem_read
        BL       mem_read
        CMP      R0,#+0
        BNE.N    ??photo_prepare_8
        B.N      ??photo_prepare_5
??photo_prepare_6:
        LDR      R0,[R6, #+0]
        STRH     R9,[R0, #+24]
        MOVS     R1,#+0
        LDR.W    R0,??DataTable25
        STRH     R1,[R0, #+0]
        CBZ.N    R7,??photo_prepare_11
        MOVS     R1,#+1
        LDR.W    R0,??DataTable22
        STRB     R1,[R0, #+0]
        MOVS     R0,#+7
        B.N      ??photo_prepare_12
??photo_prepare_11:
        CMP      R4,#+0
        LDR      R0,[R6, #+0]
        ITTTE    NE
        LDRNE    R0,[R0, #+4]
        SUBNE    R4,R0,R4
        ADDNE    R4,R4,#+4096
        LDREQ    R4,[R0, #+4]
        LDR.W    R0,??DataTable21_3
        STR      R4,[R0, #+0]
        MOVS     R0,#+6
??photo_prepare_12:
        LDR      R1,[R6, #+0]
        STRB     R0,[R1, #+34]
        MOVS     R0,#+1
??photo_prepare_5:
        ADD      SP,SP,#+68
          CFI CFA R13+28
        POP      {R4-R9,PC}
          CFI EndBlock cfiBlock27

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond28 Using cfiCommon0
          CFI Function photo_prepare
          CFI Conditional ??CrossCallReturnLabel_17
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+96
          CFI Block cfiCond29 Using cfiCommon0
          CFI (cfiCond29) Function photo_prepare
          CFI (cfiCond29) Conditional ??CrossCallReturnLabel_18
          CFI (cfiCond29) R4 Frame(CFA, -28)
          CFI (cfiCond29) R5 Frame(CFA, -24)
          CFI (cfiCond29) R6 Frame(CFA, -20)
          CFI (cfiCond29) R7 Frame(CFA, -16)
          CFI (cfiCond29) R8 Frame(CFA, -12)
          CFI (cfiCond29) R9 Frame(CFA, -8)
          CFI (cfiCond29) R14 Frame(CFA, -4)
          CFI (cfiCond29) CFA R13+96
          CFI Block cfiPicker30 Using cfiCommon1
          CFI (cfiPicker30) NoFunction
          CFI (cfiPicker30) Picker
        THUMB
?Subroutine6:
        LDR      R0,[R6, #+0]
        LDR      R0,[R0, #+4]
        SUBS     R0,R0,R4
        ADD      R0,R0,#+4096
        BX       LR
          CFI EndBlock cfiCond28
          CFI EndBlock cfiCond29
          CFI EndBlock cfiPicker30

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock31 Using cfiCommon0
          CFI Function photo_prepare_step
        THUMB
photo_prepare_step:
        PUSH     {R3-R9,LR}
          CFI R14 Frame(CFA, -4)
          CFI R9 Frame(CFA, -8)
          CFI R8 Frame(CFA, -12)
          CFI R7 Frame(CFA, -16)
          CFI R6 Frame(CFA, -20)
          CFI R5 Frame(CFA, -24)
          CFI R4 Frame(CFA, -28)
          CFI CFA R13+32
        SUB      SP,SP,#+80
          CFI CFA R13+112
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        MOV      R8,R0
        LDR.W    R6,??DataTable17
        LDR      R0,[R6, #+0]
        LDR      R5,[R0, #+4]
        UBFX     R5,R5,#+0,#+12
        LDR.W    R9,??DataTable22
        LDRB     R0,[R9, #+0]
        CMP      R0,#+1
        BNE.N    ??photo_prepare_step_0
          CFI FunCall wdt_reload
        BL       wdt_reload
        ADD      R0,R8,#+4096
          CFI FunCall mem_erase_sector
        BL       mem_erase_sector
        CMP      R0,#+0
        BEQ.N    ??photo_prepare_step_1
        MOVS     R0,#+2
        B.N      ??photo_prepare_step_2
??photo_prepare_step_0:
        CMP      R0,#+2
        BNE.N    ??photo_prepare_step_3
          CFI FunCall wdt_reload
        BL       wdt_reload
        MOV      R0,R8
          CFI FunCall mem_erase_sector
        BL       mem_erase_sector
        CMP      R0,#+0
        BEQ.N    ??photo_prepare_step_1
        MOVS     R0,#+3
??photo_prepare_step_2:
        STRB     R0,[R9, #+0]
        MOVS     R0,#+1
        B.N      ??photo_prepare_step_4
??photo_prepare_step_3:
        CMP      R0,#+3
        BNE.N    ??photo_prepare_step_5
        LDR.W    R7,??DataTable25
        LDRH     R0,[R7, #+0]
        SUBS     R0,R5,R0
        CMP      R0,#+65
        ITEE     CS
        MOVCS    R4,#+64
        LDRHCC   R4,[R7, #+0]
        SUBCC    R4,R5,R4
        UXTH     R2,R4
        ADD      R1,SP,#+16
        LDR      R0,[R6, #+0]
        LDR      R0,[R0, #+4]
        SUBS     R0,R0,R5
        LDRH     R3,[R7, #+0]
        ADDS     R0,R3,R0
          CFI FunCall mem_read
        BL       mem_read
        CMP      R0,#+0
        BEQ.N    ??photo_prepare_step_1
        UXTH     R2,R4
        ADD      R1,SP,#+16
        LDRH     R0,[R7, #+0]
        ADD      R0,R0,R8
          CFI FunCall photo_write
        BL       photo_write
        CBZ.N    R0,??photo_prepare_step_1
        LDRH     R0,[R7, #+0]
        ADDS     R4,R4,R0
        UXTH     R0,R4
        CMP      R0,R5
        ITT      EQ
        MOVEQ    R0,#+4
        STRBEQ   R0,[R9, #+0]
        STRH     R4,[R7, #+0]
??photo_prepare_step_6:
        MOVS     R0,#+0
??photo_prepare_step_4:
        ADD      SP,SP,#+84
          CFI CFA R13+28
        POP      {R4-R9,PC}
          CFI CFA R13+112
??photo_prepare_step_5:
        CMP      R0,#+4
        BNE.N    ??photo_prepare_step_7
        LDR      R0,[R6, #+0]
        LDR      R0,[R0, #+4]
        SUBS     R0,R0,R5
        STR      R0,[SP, #+0]
        STR      R5,[SP, #+4]
        ADD      R2,SP,#+8
        MOV      R1,R5
        MOV      R0,R8
          CFI FunCall photo_backup_crc
        BL       photo_backup_crc
        CBZ.N    R0,??photo_prepare_step_1
        LDR.W    R0,??DataTable23
        STR      R0,[SP, #+12]
        MOVS     R2,#+12
        MOV      R1,SP
        ADD      R0,R8,#+4096
          CFI FunCall photo_write
        BL       photo_write
        CBZ.N    R0,??photo_prepare_step_1
        MOVS     R2,#+4
        ADD      R1,SP,#+12
        ADD      R3,R8,#+4096
        ADD      R0,R3,#+12
          CFI FunCall photo_write
        BL       photo_write
        CBNZ.N   R0,??photo_prepare_step_8
??photo_prepare_step_1:
          CFI FunCall photo_stop
        BL       photo_stop
        B.N      ??photo_prepare_step_6
??photo_prepare_step_8:
        MOVS     R0,#+5
        STRB     R0,[R9, #+0]
        B.N      ??photo_prepare_step_6
??photo_prepare_step_7:
        CMP      R0,#+5
        BNE.N    ??photo_prepare_step_6
          CFI FunCall photo_recover
        BL       photo_recover
        CMP      R0,#+0
        BEQ.N    ??photo_prepare_step_6
        LDR      R0,[R6, #+0]
        LDR      R0,[R0, #+4]
        SUBS     R5,R0,R5
        ADD      R5,R5,#+4096
        LDR.W    R0,??DataTable21_3
        STR      R5,[R0, #+0]
        MOVS     R1,#+6
        LDR      R0,[R6, #+0]
        STRB     R1,[R0, #+34]
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        LDR      R1,[R6, #+0]
        STR      R0,[R1, #+8]
        B.N      ??photo_prepare_step_6
          CFI EndBlock cfiBlock31

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock32 Using cfiCommon0
          CFI Function photo_data_ready
        THUMB
photo_data_ready:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        LDR.N    R0,??DataTable17
        LDR      R1,[R0, #+0]
        LDRH     R2,[R1, #+26]
        ADDS     R2,R2,#+58
        LDR      R1,[R0, #+0]
        LDRH     R1,[R1, #+24]
        CMP      R1,R2
        IT       HI
        MOVHI    R1,R2
        LDR      R2,[R0, #+0]
        LDR      R4,[R2, #+4]
        ADDS     R4,R1,R4
        ADDW     R4,R4,#+1033
        LDRH     R0,[R2, #+24]
        CMP      R1,R0
        BNE.N    ??photo_data_ready_0
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        SUBS     R0,R0,R4
        CMP      R0,#+8
        IT       CS
        ADDCS    R4,R4,#+8
??photo_data_ready_0:
        LDR.W    R5,??DataTable21_3
        LDR      R0,[R5, #+0]
        CMP      R0,R4
        BCC.N    ??photo_data_ready_1
        MOVS     R0,#+1
        POP      {R1,R4,R5,PC}
??photo_data_ready_1:
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        LDR      R1,[R5, #+0]
        CMP      R1,R0
        BCS.N    ??photo_data_ready_2
          CFI FunCall wdt_reload
        BL       wdt_reload
        LDR      R0,[R5, #+0]
          CFI FunCall mem_erase_sector
        BL       mem_erase_sector
        CBNZ.N   R0,??photo_data_ready_3
??photo_data_ready_2:
          CFI FunCall photo_stop
        BL       photo_stop
        B.N      ??photo_data_ready_4
??photo_data_ready_3:
        LDR      R0,[R5, #+0]
        ADD      R0,R0,#+4096
        STR      R0,[R5, #+0]
??photo_data_ready_4:
        MOVS     R0,#+0
        POP      {R1,R4,R5,PC}
          CFI EndBlock cfiBlock32

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock33 Using cfiCommon0
          CFI Function photo_frame
        THUMB
photo_frame:
        PUSH     {R1-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+32
        MOVS     R3,#+0
        LDR.N    R5,??DataTable17
        LDR      R2,[R5, #+0]
        LDRB     R2,[R2, #+34]
        CMP      R2,#+6
        BNE.W    ??photo_frame_0
        LDRB     R2,[R0, #+0]
        LDRB     R4,[R0, #+1]
        ORR      R2,R2,R4, LSL #+8
        LDRB     R4,[R0, #+2]
        LDRB     R6,[R0, #+3]
        ORR      R4,R4,R6, LSL #+8
        MOVS     R7,#+0
        B.N      ??photo_frame_1
??photo_frame_2:
        UXTH     R6,R7
        LDRB     R6,[R0, R6]
        ADDS     R3,R6,R3
        ADDS     R7,R7,#+1
??photo_frame_1:
        UXTH     R12,R7
        SUBS     R6,R1,#+2
        CMP      R12,R6
        BCC.N    ??photo_frame_2
        LDR      R6,[R5, #+0]
        LDRH     R6,[R6, #+30]
        CMP      R2,R6
        BNE.N    ??photo_frame_3
        CBZ.N    R4,??photo_frame_3
        CMP      R4,#+59
        BCS.N    ??photo_frame_3
        SUBS     R2,R1,#+6
        CMP      R2,R4
        ITTTT    EQ
        ADDEQ    R2,R0,R1
        LDRBEQ   R2,[R2, #-2]
        UXTBEQ   R3,R3
        CMPEQ    R2,R3
        BNE.N    ??photo_frame_3
        ADD      R1,R0,R1
        LDRB     R1,[R1, #-1]
        CBNZ.N   R1,??photo_frame_3
        LDR      R1,[R5, #+0]
        LDRH     R2,[R1, #+24]
        LDRH     R1,[R1, #+26]
        SUBS     R2,R2,R1
        CMP      R2,R4
        BGE.N    ??photo_frame_4
??photo_frame_3:
        MOVS     R0,#+0
        LDR      R1,[R5, #+0]
        STRH     R0,[R1, #+32]
        POP      {R0-R2,R4-R7,PC}
??photo_frame_4:
        MOV      R2,R4
        ADDS     R1,R0,#+4
        LDR      R0,[R5, #+0]
        LDR      R0,[R0, #+4]
        LDR      R3,[R5, #+0]
        LDRH     R3,[R3, #+26]
        ADDS     R0,R3,R0
        ADDW     R0,R0,#+1033
          CFI FunCall photo_write
        BL       photo_write
        CMP      R0,#+0
        BEQ.N    ??photo_frame_5
        LDR      R0,[R5, #+0]
        LDRH     R0,[R0, #+26]
        ADDS     R4,R4,R0
        LDR      R0,[R5, #+0]
        STRH     R4,[R0, #+26]
        LDR      R0,[R5, #+0]
        LDRH     R1,[R0, #+30]
        ADDS     R1,R1,#+1
        STRH     R1,[R0, #+30]
        LDR      R0,[R5, #+0]
        LDRH     R0,[R0, #+26]
        LDR      R1,[R5, #+0]
        LDRH     R1,[R1, #+24]
        CMP      R0,R1
        BNE.N    ??photo_frame_6
        LDR      R0,[R5, #+0]
        LDR      R0,[R0, #+0]
        STR      R0,[SP, #+4]
        LDR      R0,[R5, #+0]
        LDRH     R0,[R0, #+24]
        STR      R0,[SP, #+8]
        MOVS     R2,#+4
        ADD      R1,SP,#+8
        LDR      R0,[R5, #+0]
        LDR      R0,[R0, #+4]
        ADDS     R0,R0,#+4
          CFI FunCall photo_write
        BL       photo_write
        CBZ.N    R0,??photo_frame_7
        MOVS     R2,#+4
        ADD      R1,SP,#+4
        LDR      R0,[R5, #+0]
        LDR      R0,[R0, #+4]
          CFI FunCall photo_write
        BL       photo_write
        CBZ.N    R0,??photo_frame_7
          CFI FunCall user_getPhotoCount
        BL       user_getPhotoCount
        MOVS     R1,R0
        ITE      NE
        MOVNE    R1,#+8192
        MOVEQ    R1,#+0
        LDR.N    R2,??DataTable17_1
        STRH     R0,[R2, #+155]
        LDRH     R3,[R2, #+129]
        MOVW     R0,#+57343
        ANDS     R3,R0,R3
        ORRS     R1,R1,R3
        STRH     R1,[R2, #+129]
        LDR      R0,[R5, #+0]
        LDR      R0,[R0, #+0]
          CFI FunCall log_addPhotoEvent
        BL       log_addPhotoEvent
??photo_frame_7:
        MOVS     R0,#+240
        STR      R0,[SP, #+0]
        MOVS     R3,#+240
        MOVS     R2,#+0
        MOVS     R1,#+0
        MOVS     R0,#+14
          CFI FunCall photo_command
        BL       photo_command
??photo_frame_5:
        B.N      ??photo_frame_8
??photo_frame_0:
        LDRB     R1,[R0, #+0]
        CMP      R1,#+170
        BNE.N    ??photo_frame_9
        LDRB     R1,[R0, #+1]
        CMP      R1,#+15
        BEQ.N    ??photo_frame_8
        CMP      R2,#+1
        BNE.N    ??photo_frame_10
        CMP      R1,#+14
        ITT      EQ
        LDRBEQ   R1,[R0, #+2]
        CMPEQ    R1,#+13
        BNE.N    ??photo_frame_11
        MOVS     R1,#+1
        LDR      R2,[R5, #+0]
        STRB     R1,[R2, #+36]
??photo_frame_11:
        LDRB     R0,[R0, #+1]
        CMP      R0,#+13
        BNE.N    ??photo_frame_12
        MOVS     R1,#+1
        LDR      R0,[R5, #+0]
        STRB     R1,[R0, #+37]
??photo_frame_12:
        LDR      R0,[R5, #+0]
        LDRB     R0,[R0, #+36]
        CMP      R0,#+0
        ITTT     NE
        LDRNE    R0,[R5, #+0]
        LDRBNE   R0,[R0, #+37]
        CMPNE    R0,#+0
        BEQ.N    ??photo_frame_9
        STR      R3,[SP, #+0]
        MOVS     R2,#+0
        MOVS     R1,#+13
        MOVS     R0,#+14
          CFI FunCall photo_command
        BL       photo_command
        MOVS     R1,#+2
        LDR      R0,[R5, #+0]
        STRB     R1,[R0, #+34]
        LDR.W    R0,??DataTable26
          CFI FunCall user_debug
        BL       user_debug
??photo_frame_6:
        MOVS     R1,#+0
        LDR      R0,[R5, #+0]
        STRB     R1,[R0, #+35]
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        LDR      R1,[R5, #+0]
        STR      R0,[R1, #+8]
??photo_frame_9:
        POP      {R0-R2,R4-R7,PC}
??photo_frame_10:
        CMP      R2,#+5
        ITTT     EQ
        CMPEQ    R1,#+10
        LDRBEQ   R1,[R0, #+2]
        CMPEQ    R1,#+1
        BNE.N    ??photo_frame_13
        LDRB     R1,[R0, #+3]
        LDRB     R2,[R0, #+4]
        ORR      R1,R1,R2, LSL #+8
        LDRB     R0,[R0, #+5]
        ORR      R0,R1,R0, LSL #+16
          CFI FunCall photo_prepare
        BL       photo_prepare
        CMP      R0,#+0
        BNE.N    ??photo_frame_6
??photo_frame_8:
          CFI FunCall photo_stop
        BL       photo_stop
        POP      {R0-R2,R4-R7,PC}
??photo_frame_13:
        LDRB     R1,[R0, #+1]
        CMP      R1,#+14
        BNE.N    ??photo_frame_9
        CMP      R2,#+2
        BNE.N    ??photo_frame_14
        LDRB     R0,[R0, #+2]
        CMP      R0,#+1
        BEQ.N    ??photo_frame_15
        POP      {R0-R2,R4-R7,PC}
??photo_frame_14:
        CMP      R2,#+3
        BNE.N    ??photo_frame_16
        LDRB     R0,[R0, #+2]
        CMP      R0,#+6
        BNE.N    ??photo_frame_9
        B.N      ??photo_frame_15
??photo_frame_16:
        CMP      R2,#+4
        ITT      EQ
        LDRBEQ   R0,[R0, #+2]
        CMPEQ    R0,#+5
        BNE.N    ??photo_frame_9
??photo_frame_15:
        LDR      R0,[R5, #+0]
        ADDS     R1,R2,#+1
        STRB     R1,[R0, #+34]
        B.N      ??photo_frame_6
          CFI EndBlock cfiBlock33

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable17:
        DATA32
        DC32     photo

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable17_1:
        DATA32
        DC32     device

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable17_2:
        DATA32
        DC32     config

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable17_3:
        DATA32
        DC32     ?_0

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable17_4:
        DATA32
        DC32     ?_1

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable17_5:
        DATA32
        DC32     port_buffer

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable17_6:
        DATA32
        DC32     ?_2

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable17_7:
        DATA32
        DC32     ?_18

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable17_8:
        DATA32
        DC32     ?_3

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock34 Using cfiCommon0
          CFI Function photo_confirmed_offset
        THUMB
photo_confirmed_offset:
        PUSH     {R1-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+32
        MOV      R4,R0
        MOV      R6,R1
        MOVS     R5,#+0
        MOVS     R2,#+1
        MOV      R1,SP
        ADD      R0,R4,#+1032
          CFI FunCall mem_read
        BL       mem_read
        CBZ.N    R0,??photo_confirmed_offset_0
        LDRB     R0,[SP, #+0]
        CBNZ.N   R0,??photo_confirmed_offset_1
        MOV      R0,R6
        POP      {R1-R7,PC}
??photo_confirmed_offset_1:
        MOVS     R7,#+0
??photo_confirmed_offset_2:
        MOVS     R2,#+8
        ADD      R1,SP,#+4
        UXTH     R0,R7
        ADD      R0,R4,R0, LSL #+3
        ADDS     R0,R0,#+8
          CFI FunCall mem_read
        BL       mem_read
        CBZ.N    R0,??photo_confirmed_offset_0
        MOVW     R1,#+65535
        LDRH     R0,[SP, #+4]
        CMP      R0,R1
        ITTT     EQ
        ADDEQ    R0,SP,#+4
        LDRHEQ   R2,[R0, #+2]
        CMPEQ    R2,R1
        ITT      EQ
        LDRHEQ   R2,[R0, #+4]
        CMPEQ    R2,R1
        BNE.N    ??photo_confirmed_offset_3
        LDRH     R0,[R0, #+6]
        CMP      R0,R1
        BNE.N    ??photo_confirmed_offset_4
        B.N      ??photo_confirmed_offset_5
??photo_confirmed_offset_0:
        MOVS     R0,#+0
        POP      {R1-R7,PC}
??photo_confirmed_offset_5:
        MOV      R0,R5
        POP      {R1-R7,PC}
??photo_confirmed_offset_3:
        ADD      R2,SP,#+4
        LDRH     R0,[R2, #+4]
        CMP      R0,#+0
        ITT      EQ
        LDRHEQ   R0,[R2, #+6]
        CMPEQ    R0,#+0
        ITTTT    EQ
        LDRHEQ   R3,[SP, #+4]
        LDRHEQ   R0,[R2, #+2]
        EOREQ    R3,R0,R3
        CMPEQ    R3,R1
        BNE.N    ??photo_confirmed_offset_4
        LDRH     R0,[SP, #+4]
        CMP      R6,R0
        ITT      CS
        CMPCS    R0,R5
        MOVCS    R5,R0
??photo_confirmed_offset_4:
        ADDS     R7,R7,#+1
        UXTH     R0,R7
        CMP      R0,#+128
        BCC.N    ??photo_confirmed_offset_2
        B.N      ??photo_confirmed_offset_5
          CFI EndBlock cfiBlock34

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock35 Using cfiCommon0
          CFI Function photo_ack_slot
        THUMB
photo_ack_slot:
        PUSH     {R2-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+24
        MOV      R4,R0
        MOVS     R6,#+0
??photo_ack_slot_0:
        UXTH     R0,R6
        ADD      R5,R4,R0, LSL #+3
        ADDS     R5,R5,#+8
        MOVS     R2,#+8
        MOV      R1,SP
        MOV      R0,R5
          CFI FunCall mem_read
        BL       mem_read
        CBZ.N    R0,??photo_ack_slot_1
        MOVW     R0,#+65535
        LDRH     R1,[SP, #+0]
        CMP      R1,R0
        ITT      EQ
        LDRHEQ   R1,[SP, #+2]
        CMPEQ    R1,R0
        BNE.N    ??photo_ack_slot_2
        LDRH     R1,[SP, #+4]
        CMP      R1,R0
        ITT      EQ
        LDRHEQ   R1,[SP, #+6]
        CMPEQ    R1,R0
        BEQ.N    ??photo_ack_slot_3
??photo_ack_slot_2:
        ADDS     R6,R6,#+1
        UXTH     R0,R6
        CMP      R0,#+128
        BCC.N    ??photo_ack_slot_0
        B.N      ??photo_ack_slot_1
??photo_ack_slot_3:
        MOV      R0,R5
        POP      {R1,R2,R4-R6,PC}
??photo_ack_slot_1:
        MOVS     R0,#+0
        POP      {R1,R2,R4-R6,PC}
          CFI EndBlock cfiBlock35

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock36 Using cfiCommon0
          CFI Function photo_ack_write
        THUMB
photo_ack_write:
        PUSH     {R0-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+32
        MOV      R4,R0
        MOV      R6,R1
        MOV      R5,R2
        MOVS     R0,#+0
        STR      R0,[SP, #+8]
        MOV      R0,R4
          CFI FunCall photo_confirmed_offset
        BL       photo_confirmed_offset
        CMP      R0,R5
        BCS.N    ??photo_ack_write_0
        CMP      R5,R6
        BNE.N    ??photo_ack_write_1
        MOVS     R0,#+0
        STRB     R0,[SP, #+0]
        MOVS     R2,#+1
        MOV      R1,SP
        ADD      R0,R4,#+1032
          CFI FunCall photo_write
        BL       photo_write
        B.N      ??photo_ack_write_2
??photo_ack_write_1:
        MOV      R0,R4
          CFI FunCall photo_ack_slot
        BL       photo_ack_slot
        MOVS     R4,R0
        IT       EQ
        MOVEQ    R0,#+0
        BEQ.N    ??photo_ack_write_2
        STRH     R5,[SP, #+4]
        MVNS     R5,R5
        STRH     R5,[SP, #+6]
        MOVS     R2,#+4
        ADD      R1,SP,#+4
          CFI FunCall photo_write
        BL       photo_write
        CBZ.N    R0,??photo_ack_write_2
        MOVS     R2,#+4
        ADD      R1,SP,#+8
        ADDS     R0,R4,#+4
          CFI FunCall photo_write
        BL       photo_write
        CBZ.N    R0,??photo_ack_write_2
??photo_ack_write_0:
        MOVS     R0,#+1
??photo_ack_write_2:
        ADD      SP,SP,#+16
          CFI CFA R13+16
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock36

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock37 Using cfiCommon0
          CFI Function photo_clear_confirmed
        THUMB
photo_clear_confirmed:
        PUSH     {R1-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+32
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        ADD      R6,R0,#+4128
        LDR.N    R0,??DataTable21
        LDR      R4,[R0, #+72]
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        MOV      R7,R0
          CFI FunCall photo_cleanup_active
        BL       photo_cleanup_active
        LDR.N    R5,??DataTable21_4
        LDR      R1,[R5, #+157]
        CMP      R7,R1
        BCC.N    ??photo_clear_confirmed_0
        CMP      R0,#+2
        BEQ.N    ??photo_clear_confirmed_0
        CMP      R0,#+0
        BNE.N    ??photo_clear_confirmed_1
        CMP      R4,R1
        BCS.N    ??photo_clear_confirmed_1
        SUBS     R0,R7,R1
        CMP      R0,#+8
        ITEE     CC
        STRCC    R7,[SP, #+4]
        ADDCS    R0,R1,#+8
        STRCS    R0,[SP, #+4]
        LDR      R1,[SP, #+4]
        ADDW     R1,R1,#+4095
        LSRS     R1,R1,#+12
        LSLS     R1,R1,#+12
        STR      R1,[SP, #+4]
        CMP      R7,R1
        BCC.N    ??photo_clear_confirmed_0
        CMP      R4,R1
        BCS.N    ??photo_clear_confirmed_0
        MOVS     R1,#+0
        STR      R1,[SP, #+0]
        LDR      R1,[SP, #+4]
        MVNS     R1,R1
        STR      R1,[SP, #+8]
        MOVS     R2,#+8
        ADD      R1,SP,#+4
        ADDS     R0,R6,#+4
          CFI FunCall photo_write
        BL       photo_write
        CBZ.N    R0,??photo_clear_confirmed_0
        MOVS     R2,#+4
        MOV      R1,SP
        MOV      R0,R6
          CFI FunCall photo_write
        BL       photo_write
        CBNZ.N   R0,??photo_clear_confirmed_2
??photo_clear_confirmed_0:
        MOVS     R0,#+0
        POP      {R1-R7,PC}
??photo_clear_confirmed_2:
        STR      R4,[R5, #+157]
        MOVS     R0,#+0
        STRH     R0,[R5, #+155]
        LDRH     R1,[R5, #+129]
        MOVW     R0,#+57343
        ANDS     R1,R0,R1
        STRH     R1,[R5, #+129]
        MOVS     R0,#+0
        STRH     R0,[R5, #+167]
        LDR.W    R1,??DataTable26_1
        LDR      R0,[R1, #+0]
        CBZ.N    R0,??photo_clear_confirmed_1
        STR      R4,[R0, #+20]
        MOVS     R0,#+0
        LDR      R1,[R1, #+0]
        STRH     R0,[R1, #+28]
??photo_clear_confirmed_1:
        MOVS     R0,#+1
        POP      {R1-R7,PC}
          CFI EndBlock cfiBlock37

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable18:
        DATA32
        DC32     ?_4

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable18_1:
        DATA32
        DC32     ?_5

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable18_2:
        DATA32
        DC32     `photo_fail::hex`

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock38 Using cfiCommon0
          CFI Function photo_cleanup_step
        THUMB
photo_cleanup_step:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        SUB      SP,SP,#+272
          CFI CFA R13+296
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        ADD      R4,R0,#+4128
        LDR.N    R0,??DataTable21
        LDR      R5,[R0, #+72]
        MOVS     R0,#+0
        STRB     R0,[SP, #+0]
        MOVS     R2,#+12
        ADD      R1,SP,#+4
        MOV      R0,R4
          CFI FunCall mem_read
        BL       mem_read
        CMP      R0,#+0
        BEQ.N    ??photo_cleanup_step_0
        LDR      R0,[SP, #+4]
        CMP      R0,#+0
        BNE.N    ??photo_cleanup_step_0
        ADD      R6,SP,#+4
        LDR      R1,[R6, #+4]
        LDR      R0,[SP, #+12]
        MVNS     R0,R0
        CMP      R1,R0
        BNE.N    ??photo_cleanup_step_0
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        LDR      R1,[R6, #+4]
        CMP      R0,R1
        BCC.N    ??photo_cleanup_step_0
        CMP      R5,R1
        BCS.N    ??photo_cleanup_step_0
        SUBS     R0,R1,R5
        LSLS     R0,R0,#+20
        BNE.N    ??photo_cleanup_step_0
        SUBS     R6,R1,R5
        LSLS     R6,R6,#+4
        LSRS     R6,R6,#+16
        MOVW     R0,#+257
        CMP      R6,R0
        BCS.N    ??photo_cleanup_step_0
        MOV      R2,R6
        ADD      R1,SP,#+16
        ADD      R0,R4,#+12
          CFI FunCall mem_read
        BL       mem_read
        CBZ.N    R0,??photo_cleanup_step_0
        MOVS     R7,#+0
        B.N      ??photo_cleanup_step_1
??photo_cleanup_step_2:
        ADDS     R7,R7,#+1
??photo_cleanup_step_1:
        CMP      R7,R6
        BGE.N    ??photo_cleanup_step_3
        ADD      R0,SP,#+16
        LDRB     R0,[R0, R7]
        CMP      R0,#+0
        BEQ.N    ??photo_cleanup_step_2
          CFI FunCall wdt_reload
        BL       wdt_reload
        ADD      R0,R5,R7, LSL #+12
          CFI FunCall mem_erase_sector
        BL       mem_erase_sector
        CBZ.N    R0,??photo_cleanup_step_0
        MOVS     R2,#+1
        MOV      R1,SP
        ADDS     R4,R7,R4
        ADD      R0,R4,#+12
          CFI FunCall photo_write
        BL       photo_write
        B.N      ??photo_cleanup_step_0
??photo_cleanup_step_3:
          CFI FunCall wdt_reload
        BL       wdt_reload
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        ADD      R0,R0,#+4096
          CFI FunCall mem_erase_sector
        BL       mem_erase_sector
        CBZ.N    R0,??photo_cleanup_step_0
        LDR.N    R1,??DataTable21_4
        STR      R5,[R1, #+157]
        MOVS     R0,#+0
        STRH     R0,[R1, #+155]
        LDRH     R2,[R1, #+129]
        MOVW     R0,#+57343
        ANDS     R2,R0,R2
        STRH     R2,[R1, #+129]
??photo_cleanup_step_0:
        ADD      SP,SP,#+276
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI EndBlock cfiBlock38

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19:
        DATA32
        DC32     ?_19

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock39 Using cfiCommon0
          CFI Function photo_first_pending
        THUMB
photo_first_pending:
        PUSH     {R1-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+32
        MOV      R4,R0
        MOV      R5,R1
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        LDR.N    R6,??DataTable21_4
        LDR      R1,[R6, #+157]
        CMP      R0,R1
        BCC.N    ??photo_first_pending_0
        LDR.W    R0,??DataTable26_2
        LDR      R7,[R0, #+72]
        B.N      ??photo_first_pending_1
??photo_first_pending_2:
        BL       ?Subroutine10
??CrossCallReturnLabel_30:
        LDR      R1,[SP, #+4]
        CMP      R0,R1
        BCC.N    ??photo_first_pending_3
        ADDS     R7,R1,R7
        ADDW     R7,R7,#+1033
??photo_first_pending_1:
        LDR      R0,[R6, #+157]
        CMP      R7,R0
        BCS.N    ??photo_first_pending_4
        BL       ?Subroutine8
??CrossCallReturnLabel_23:
        CMP      R0,#+0
        BNE.N    ??photo_first_pending_2
??photo_first_pending_0:
        MOVS     R0,#+2
        POP      {R1-R7,PC}
??photo_first_pending_3:
        STR      R7,[R4, #+0]
        STR      R0,[R5, #+0]
        MOVS     R0,#+1
        POP      {R1-R7,PC}
??photo_first_pending_4:
        STR      R0,[R4, #+0]
        MOVS     R0,#+0
        STR      R0,[R5, #+0]
        POP      {R1-R7,PC}
          CFI EndBlock cfiBlock39

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock40 Using cfiCommon0
          CFI Function photo_ack_request
        THUMB
photo_ack_request:
        PUSH     {R1-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+32
        MOV      R4,R0
        MOV      R5,R1
        LDR.N    R6,??DataTable21
        LDR      R7,[R6, #+72]
        B.N      ??photo_ack_request_0
??photo_ack_request_1:
        BL       ?Subroutine10
??CrossCallReturnLabel_29:
        LDR      R1,[SP, #+4]
        CMP      R0,R1
        BCC.N    ??photo_ack_request_2
??photo_ack_request_3:
        LDR      R0,[SP, #+4]
        ADDS     R7,R0,R7
        ADDW     R7,R7,#+1033
??photo_ack_request_0:
        LDR.N    R0,??DataTable21_4
        LDR      R0,[R0, #+157]
        CMP      R7,R0
        BCS.N    ??photo_ack_request_4
        BL       ?Subroutine8
??CrossCallReturnLabel_22:
        CMP      R0,#+0
        BEQ.N    ??photo_ack_request_5
        CMP      R4,#+0
        BEQ.N    ??photo_ack_request_1
        LDR      R0,[SP, #+0]
        CMP      R0,R4
        BNE.N    ??photo_ack_request_3
??photo_ack_request_2:
        LDR      R1,[SP, #+4]
        CMP      R1,R5
        IT       CS
        LDRCS    R4,[R6, #+72]
        BCS.N    ??photo_ack_request_6
??photo_ack_request_4:
        LDR.W    R0,??DataTable26_3
          CFI FunCall user_debug
        BL       user_debug
        MOVS     R0,#+1
        POP      {R1-R7,PC}
??photo_ack_request_7:
        LDR      R0,[SP, #+4]
        ADDS     R4,R0,R4
        ADDW     R4,R4,#+1033
??photo_ack_request_6:
        CMP      R7,R4
        BCC.N    ??photo_ack_request_8
        BL       ?Subroutine7
??CrossCallReturnLabel_21:
        CBZ.N    R0,??photo_ack_request_5
        LDR      R1,[SP, #+4]
        UXTH     R1,R1
        MOV      R0,R4
          CFI FunCall photo_confirmed_offset
        BL       photo_confirmed_offset
        CMP      R4,R7
        ITE      EQ
        MOVEQ    R1,R5
        LDRNE    R1,[SP, #+4]
        UXTH     R2,R1
        CMP      R0,R2
        BCS.N    ??photo_ack_request_7
        UXTH     R1,R1
        LDR      R0,[SP, #+4]
        CMP      R1,R0
        BEQ.N    ??photo_ack_request_7
        MOV      R0,R4
          CFI FunCall photo_ack_slot
        BL       photo_ack_slot
        CMP      R0,#+0
        BNE.N    ??photo_ack_request_7
        LDR.W    R0,??DataTable26_4
          CFI FunCall user_debug
        BL       user_debug
        B.N      ??photo_ack_request_5
??photo_ack_request_8:
        LDR      R4,[R6, #+72]
        B.N      ??photo_ack_request_9
??photo_ack_request_10:
        LDR      R0,[SP, #+4]
        ADDS     R4,R0,R4
        ADDW     R4,R4,#+1033
??photo_ack_request_9:
        CMP      R7,R4
        BCC.N    ??photo_ack_request_11
        BL       ?Subroutine7
??CrossCallReturnLabel_20:
        CBZ.N    R0,??photo_ack_request_5
        CMP      R4,R7
        ITE      EQ
        MOVEQ    R2,R5
        LDRNE    R2,[SP, #+4]
        UXTH     R2,R2
        LDR      R1,[SP, #+4]
        UXTH     R1,R1
        MOV      R0,R4
          CFI FunCall photo_ack_write
        BL       photo_ack_write
        CMP      R0,#+0
        BNE.N    ??photo_ack_request_10
??photo_ack_request_5:
        MOVS     R0,#+2
        POP      {R1-R7,PC}
??photo_ack_request_11:
        MOVS     R0,#+0
        POP      {R1-R7,PC}
          CFI EndBlock cfiBlock40

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable21:
        DATA32
        DC32     mem

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable21_1:
        DATA32
        DC32     ?_7

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable21_2:
        DATA32
        DC32     ?_8

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable21_3:
        DATA32
        DC32     photo_erased_to

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable21_4:
        DATA32
        DC32     device

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond41 Using cfiCommon0
          CFI Function photo_first_pending
          CFI Conditional ??CrossCallReturnLabel_30
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond42 Using cfiCommon0
          CFI (cfiCond42) Function photo_ack_request
          CFI (cfiCond42) Conditional ??CrossCallReturnLabel_29
          CFI (cfiCond42) R4 Frame(CFA, -20)
          CFI (cfiCond42) R5 Frame(CFA, -16)
          CFI (cfiCond42) R6 Frame(CFA, -12)
          CFI (cfiCond42) R7 Frame(CFA, -8)
          CFI (cfiCond42) R14 Frame(CFA, -4)
          CFI (cfiCond42) CFA R13+32
          CFI Block cfiPicker43 Using cfiCommon1
          CFI (cfiPicker43) NoFunction
          CFI (cfiPicker43) Picker
        THUMB
?Subroutine10:
        LDR      R1,[SP, #+4]
        UXTH     R1,R1
        MOV      R0,R7
          CFI (cfiCond41) FunCall photo_first_pending photo_confirmed_offset
          CFI (cfiCond42) FunCall photo_ack_request photo_confirmed_offset
        B.N      photo_confirmed_offset
          CFI EndBlock cfiCond41
          CFI EndBlock cfiCond42
          CFI EndBlock cfiPicker43

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond44 Using cfiCommon0
          CFI Function photo_first_pending
          CFI Conditional ??CrossCallReturnLabel_23
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond45 Using cfiCommon0
          CFI (cfiCond45) Function photo_ack_request
          CFI (cfiCond45) Conditional ??CrossCallReturnLabel_22
          CFI (cfiCond45) R4 Frame(CFA, -20)
          CFI (cfiCond45) R5 Frame(CFA, -16)
          CFI (cfiCond45) R6 Frame(CFA, -12)
          CFI (cfiCond45) R7 Frame(CFA, -8)
          CFI (cfiCond45) R14 Frame(CFA, -4)
          CFI (cfiCond45) CFA R13+32
          CFI Block cfiPicker46 Using cfiCommon1
          CFI (cfiPicker46) NoFunction
          CFI (cfiPicker46) Picker
        THUMB
?Subroutine8:
        MOVS     R2,#+8
        MOV      R1,SP
        MOV      R0,R7
          CFI (cfiCond44) FunCall photo_first_pending mem_read
          CFI (cfiCond45) FunCall photo_ack_request mem_read
        B.W      mem_read
          CFI EndBlock cfiCond44
          CFI EndBlock cfiCond45
          CFI EndBlock cfiPicker46

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond47 Using cfiCommon0
          CFI Function photo_ack_request
          CFI Conditional ??CrossCallReturnLabel_21
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond48 Using cfiCommon0
          CFI (cfiCond48) Function photo_ack_request
          CFI (cfiCond48) Conditional ??CrossCallReturnLabel_20
          CFI (cfiCond48) R4 Frame(CFA, -20)
          CFI (cfiCond48) R5 Frame(CFA, -16)
          CFI (cfiCond48) R6 Frame(CFA, -12)
          CFI (cfiCond48) R7 Frame(CFA, -8)
          CFI (cfiCond48) R14 Frame(CFA, -4)
          CFI (cfiCond48) CFA R13+32
          CFI Block cfiPicker49 Using cfiCommon1
          CFI (cfiPicker49) NoFunction
          CFI (cfiPicker49) Picker
        THUMB
?Subroutine7:
        MOVS     R2,#+8
          CFI Block cfiCond50 Using cfiCommon0
          CFI (cfiCond50) Function photo_write
          CFI (cfiCond50) Conditional ??CrossCallReturnLabel_19
          CFI (cfiCond50) R4 Frame(CFA, -20)
          CFI (cfiCond50) R5 Frame(CFA, -16)
          CFI (cfiCond50) R6 Frame(CFA, -12)
          CFI (cfiCond50) R7 Frame(CFA, -8)
          CFI (cfiCond50) R14 Frame(CFA, -4)
          CFI (cfiCond50) CFA R13+88
??Subroutine7_0:
        MOV      R1,SP
        MOV      R0,R4
          CFI (cfiCond47) FunCall photo_ack_request mem_read
          CFI (cfiCond47) FunCall photo_ack_request mem_read
          CFI (cfiCond50) FunCall photo_write mem_read
        B.W      mem_read
          CFI EndBlock cfiCond47
          CFI EndBlock cfiCond48
          CFI EndBlock cfiPicker49
          CFI EndBlock cfiCond50

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock51 Using cfiCommon0
          CFI Function photo_request
        THUMB
photo_request:
        PUSH     {R3-R9,LR}
          CFI R14 Frame(CFA, -4)
          CFI R9 Frame(CFA, -8)
          CFI R8 Frame(CFA, -12)
          CFI R7 Frame(CFA, -16)
          CFI R6 Frame(CFA, -20)
          CFI R5 Frame(CFA, -24)
          CFI R4 Frame(CFA, -28)
          CFI CFA R13+32
        SUB      SP,SP,#+16
          CFI CFA R13+48
        MOV      R5,R0
        MOV      R8,R1
        MOV      R4,R2
        MOVS     R7,R3
        ITT      NE
        CMPNE    R7,#+6
        MOVNE    R0,#+1
        BNE.N    ??photo_request_0
        LDR.W    R9,??DataTable26_5
        LDRH     R0,[R9, #+129]
        UBFX     R0,R0,#+11,#+1
        CMP      R0,#+0
        BNE.N    ??photo_request_1
        CMP      R5,#+2
        BEQ.N    ??photo_request_1
          CFI FunCall photo_alloc
        BL       photo_alloc
        CMP      R0,#+0
        BEQ.N    ??photo_request_1
        LDR.W    R6,??DataTable26_1
        LDR      R0,[R6, #+0]
        LDRB     R0,[R0, #+34]
        CBNZ.N   R0,??photo_request_1
          CFI FunCall user_getPhotoCount
        BL       user_getPhotoCount
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        LDR      R1,[R9, #+157]
        CMP      R0,R1
        BCC.N    ??photo_request_1
          CFI FunCall photo_cleanup_active
        BL       photo_cleanup_active
        CBNZ.N   R0,??photo_request_2
          CFI FunCall photo_recover
        BL       photo_recover
        CBZ.N    R0,??photo_request_1
??photo_request_2:
        CMP      R7,#+0
        ITT      NE
        LDRHNE   R0,[R9, #+155]
        CMPNE    R0,#+0
        BEQ.N    ??photo_request_3
        ADD      R2,SP,#+12
        LDR      R0,[R4, #+0]
        STR      R0,[R2, #+0]
        ADDS     R3,R4,#+4
        LDRB     R2,[R3, #+0]
        STRB     R2,[SP, #+0]
        LDRB     R2,[R3, #+1]
        STRB     R2,[SP, #+1]
        LDRH     R1,[SP, #+0]
        LDR      R0,[SP, #+12]
          CFI FunCall photo_ack_request
        BL       photo_ack_request
        MOVS     R1,R0
        BNE.N    ??photo_request_0
          CFI FunCall user_getPhotoCount
        BL       user_getPhotoCount
        LDRH     R0,[R9, #+155]
        CBNZ.N   R0,??photo_request_3
          CFI FunCall photo_clear_confirmed
        BL       photo_clear_confirmed
        CBNZ.N   R0,??photo_request_3
??photo_request_1:
        MOVS     R0,#+2
        B.N      ??photo_request_0
??photo_request_3:
        ADD      R1,SP,#+4
        ADD      R0,SP,#+8
          CFI FunCall photo_first_pending
        BL       photo_first_pending
        CMP      R0,#+2
        BEQ.N    ??photo_request_0
        LDR      R1,[SP, #+8]
        LDR      R0,[R6, #+0]
        STR      R1,[R0, #+20]
        LDR      R1,[SP, #+4]
        LDR      R0,[R6, #+0]
        STRH     R1,[R0, #+28]
        LDR      R0,[R6, #+0]
        STRB     R5,[R0, #+50]
        LDR      R0,[R6, #+0]
        STRB     R8,[R0, #+39]
        MOVS     R0,#+1
        LDR      R1,[R6, #+0]
        STRB     R0,[R1, #+38]
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        LDR.W    R1,??DataTable26_6
        STR      R0,[R1, #+0]
        MOVS     R0,#+0
??photo_request_0:
        ADD      SP,SP,#+20
          CFI CFA R13+28
        POP      {R4-R9,PC}
          CFI EndBlock cfiBlock51

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable22:
        DATA32
        DC32     photo_prep_phase

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock52 Using cfiCommon0
          CFI Function photo_next
        THUMB
photo_next:
        PUSH     {R2-R10,LR}
          CFI R14 Frame(CFA, -4)
          CFI R10 Frame(CFA, -8)
          CFI R9 Frame(CFA, -12)
          CFI R8 Frame(CFA, -16)
          CFI R7 Frame(CFA, -20)
          CFI R6 Frame(CFA, -24)
          CFI R5 Frame(CFA, -28)
          CFI R4 Frame(CFA, -32)
          CFI CFA R13+40
        MOV      R4,R0
        MOV      R5,R1
        MOV      R9,R2
        LDR      R7,[R4, #+0]
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        LDR.W    R8,??DataTable26_5
        LDR      R1,[R8, #+157]
        CMP      R0,R1
        BCC.N    ??photo_next_0
        MOV      R10,R7
        B.N      ??photo_next_1
??photo_next_2:
        LDR      R1,[SP, #+4]
        UXTH     R1,R1
        MOV      R0,R10
          CFI FunCall photo_confirmed_offset
        BL       photo_confirmed_offset
        MOV      R6,R0
        LDR      R0,[SP, #+4]
        CMP      R6,R0
        BNE.N    ??photo_next_3
        ADD      R10,R0,R10
        ADDW     R10,R10,#+1033
??photo_next_1:
        LDR      R0,[R8, #+157]
        CMP      R10,R0
        BCS.N    ??photo_next_4
        MOVS     R2,#+8
        MOV      R1,SP
        MOV      R0,R10
          CFI FunCall mem_read
        BL       mem_read
        CMP      R0,#+0
        BNE.N    ??photo_next_2
??photo_next_0:
        MOVS     R0,#+2
        B.N      ??photo_next_5
??photo_next_3:
        STR      R10,[R4, #+0]
        MOV      R0,R9
        MOV      R1,SP
        MOVS     R2,#+8
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        CMP      R10,R7
        BNE.N    ??photo_next_6
        LDR      R0,[R5, #+0]
        CMP      R6,R0
        IT       CC
        MOVCC    R6,R0
??photo_next_6:
        STR      R6,[R5, #+0]
        MOVS     R0,#+1
        B.N      ??photo_next_5
??photo_next_4:
        MOVS     R0,#+0
??photo_next_5:
        POP      {R1,R2,R4-R10,PC}
          CFI EndBlock cfiBlock52

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable23:
        DATA32
        DC32     0x50484f54

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable23_1:
        DATA32
        DC32     photo_restore_phase

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock53 Using cfiCommon0
          CFI Function photo_send
        THUMB
photo_send:
        PUSH     {R4-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+20
        SUB      SP,SP,#+20
          CFI CFA R13+40
        LDR.W    R7,??DataTable26_1
        LDR      R0,[R7, #+0]
        LDR      R0,[R0, #+20]
        STR      R0,[SP, #+4]
        LDR      R0,[R7, #+0]
        LDRH     R0,[R0, #+28]
        STR      R0,[SP, #+0]
        MOVS     R4,#+0
        ADD      R2,SP,#+12
        MOV      R1,SP
        ADD      R0,SP,#+4
          CFI FunCall photo_next
        BL       photo_next
        MOV      R6,R0
        CMP      R6,#+2
        BEQ.N    ??photo_send_0
        BL       ?Subroutine9
??CrossCallReturnLabel_24:
        CMP      R0,#+1
        BNE.N    ??photo_send_1
        ADD      R0,SP,#+8
          CFI FunCall comm_tx_acquire
        BL       comm_tx_acquire
        B.N      ??photo_send_2
??photo_send_1:
        MOVW     R0,#+534
          CFI FunCall malloc
        BL       malloc
??photo_send_2:
        MOVS     R5,R0
        BEQ.N    ??photo_send_0
        BL       ?Subroutine9
??CrossCallReturnLabel_25:
        CMP      R0,#+1
        BNE.N    ??photo_send_3
        LDRH     R1,[SP, #+8]
        MOVW     R0,#+534
        CMP      R1,R0
        BCC.N    ??photo_send_4
??photo_send_3:
        CBNZ.N   R6,??photo_send_5
??photo_send_6:
        MOVS     R6,#+4
??photo_send_7:
        LDR.W    R1,??DataTable26_7
        LDR      R0,[R1, #+0]
        STR      R0,[R5, #+0]
        STRB     R6,[R5, #+4]
        LSRS     R0,R6,#+8
        STRB     R0,[R5, #+5]
        MOVS     R0,#+60
        STRB     R0,[R5, #+6]
        LDR      R0,[R7, #+0]
        LDRB     R0,[R0, #+39]
        STRB     R0,[R5, #+7]
        MOVS     R0,#+14
        STRB     R0,[R5, #+8]
        CBZ.N    R4,??photo_send_8
        LDR.W    R0,??DataTable26_5
        LDRH     R1,[R0, #+155]
        CMP      R1,#+256
        ITE      CC
        MOVCC    R0,R1
        MOVCS    R0,#+255
        B.N      ??photo_send_9
??photo_send_5:
        LDR      R1,[SP, #+16]
        LDR      R0,[SP, #+0]
        SUBS     R1,R1,R0
        MOVW     R0,#+513
        CMP      R1,R0
        IT       CS
        MOVCS    R4,#+512
        BCS.N    ??photo_send_10
        LDR      R4,[SP, #+16]
        LDR      R0,[SP, #+0]
        SUBS     R4,R4,R0
        UXTH     R4,R4
        CMP      R4,#+0
        BEQ.N    ??photo_send_6
??photo_send_10:
        ADD      R6,R4,#+14
        UXTH     R6,R6
        B.N      ??photo_send_7
??photo_send_8:
        MOVS     R0,#+0
??photo_send_9:
        STRB     R0,[R5, #+9]
        CBZ.N    R4,??photo_send_11
        ADD      R2,SP,#+12
        ADD      R0,R5,#+10
        LDR      R1,[R2, #+0]
        STR      R1,[R0, #+0]
        ADD      R3,SP,#+16
        ADD      R0,R5,#+14
        LDR      R2,[R3, #+0]
        STR      R2,[R0, #+0]
        LDR      R0,[SP, #+0]
        STRB     R0,[R5, #+18]
        LDR      R0,[SP, #+0]
        LSRS     R0,R0,#+8
        STRB     R0,[R5, #+19]
        MOV      R2,R4
        ADD      R1,R5,#+20
        LDR      R0,[SP, #+4]
        LDR      R3,[SP, #+0]
        ADDS     R0,R3,R0
        ADDW     R0,R0,#+1033
          CFI FunCall mem_read
        BL       mem_read
        CBNZ.N   R0,??photo_send_11
        BL       ?Subroutine9
??CrossCallReturnLabel_26:
        CBNZ.N   R0,??photo_send_4
        MOV      R0,R5
          CFI FunCall free
        BL       free
??photo_send_0:
        B.N      ??photo_send_12
??photo_send_4:
          CFI FunCall comm_tx_release
        BL       comm_tx_release
        B.N      ??photo_send_12
??photo_send_11:
        ADDS     R1,R6,#+2
        UXTH     R1,R1
        ADDS     R0,R5,#+4
          CFI FunCall crc16_calc
        BL       crc16_calc
        ADDS     R1,R5,R6
        STRB     R0,[R1, #+6]
        LSRS     R0,R0,#+8
        STRB     R0,[R1, #+7]
        BL       ?Subroutine9
??CrossCallReturnLabel_27:
        CBNZ.N   R0,??photo_send_13
        ADDS     R6,R6,#+8
        UXTH     R1,R6
        MOV      R0,R5
          CFI FunCall usb_send
        BL       usb_send
        B.N      ??photo_send_14
??photo_send_13:
        ADDS     R6,R6,#+8
        UXTH     R0,R6
          CFI FunCall comm_tx_commit
        BL       comm_tx_commit
??photo_send_14:
        MOV      R6,R0
        BL       ?Subroutine9
??CrossCallReturnLabel_28:
        CBNZ.N   R0,??photo_send_15
        MOV      R0,R5
          CFI FunCall free
        BL       free
        B.N      ??photo_send_16
??photo_send_15:
          CFI FunCall comm_tx_release
        BL       comm_tx_release
??photo_send_16:
        CBZ.N    R6,??photo_send_12
        CBZ.N    R4,??photo_send_17
        LDR      R0,[SP, #+0]
        ADDS     R4,R4,R0
        STR      R4,[SP, #+0]
        LDR      R0,[SP, #+16]
        CMP      R4,R0
        BNE.N    ??photo_send_18
        LDR      R1,[SP, #+4]
        LDR      R0,[SP, #+16]
        ADDS     R1,R0,R1
        ADDW     R1,R1,#+1033
        STR      R1,[SP, #+4]
        MOVS     R0,#+0
        STR      R0,[SP, #+0]
??photo_send_18:
        LDR      R1,[SP, #+4]
        LDR      R0,[R7, #+0]
        STR      R1,[R0, #+20]
        LDR      R0,[SP, #+0]
        LDR      R1,[R7, #+0]
        STRH     R0,[R1, #+28]
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        LDR.N    R1,??DataTable26_6
        STR      R0,[R1, #+0]
        B.N      ??photo_send_12
??photo_send_17:
        MOVS     R0,#+0
        LDR      R1,[R7, #+0]
        STRB     R0,[R1, #+38]
??photo_send_12:
        ADD      SP,SP,#+20
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI EndBlock cfiBlock53

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond54 Using cfiCommon0
          CFI Function photo_send
          CFI Conditional ??CrossCallReturnLabel_24
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+40
          CFI Block cfiCond55 Using cfiCommon0
          CFI (cfiCond55) Function photo_send
          CFI (cfiCond55) Conditional ??CrossCallReturnLabel_25
          CFI (cfiCond55) R4 Frame(CFA, -20)
          CFI (cfiCond55) R5 Frame(CFA, -16)
          CFI (cfiCond55) R6 Frame(CFA, -12)
          CFI (cfiCond55) R7 Frame(CFA, -8)
          CFI (cfiCond55) R14 Frame(CFA, -4)
          CFI (cfiCond55) CFA R13+40
          CFI Block cfiCond56 Using cfiCommon0
          CFI (cfiCond56) Function photo_send
          CFI (cfiCond56) Conditional ??CrossCallReturnLabel_26
          CFI (cfiCond56) R4 Frame(CFA, -20)
          CFI (cfiCond56) R5 Frame(CFA, -16)
          CFI (cfiCond56) R6 Frame(CFA, -12)
          CFI (cfiCond56) R7 Frame(CFA, -8)
          CFI (cfiCond56) R14 Frame(CFA, -4)
          CFI (cfiCond56) CFA R13+40
          CFI Block cfiCond57 Using cfiCommon0
          CFI (cfiCond57) Function photo_send
          CFI (cfiCond57) Conditional ??CrossCallReturnLabel_27
          CFI (cfiCond57) R4 Frame(CFA, -20)
          CFI (cfiCond57) R5 Frame(CFA, -16)
          CFI (cfiCond57) R6 Frame(CFA, -12)
          CFI (cfiCond57) R7 Frame(CFA, -8)
          CFI (cfiCond57) R14 Frame(CFA, -4)
          CFI (cfiCond57) CFA R13+40
          CFI Block cfiCond58 Using cfiCommon0
          CFI (cfiCond58) Function photo_send
          CFI (cfiCond58) Conditional ??CrossCallReturnLabel_28
          CFI (cfiCond58) R4 Frame(CFA, -20)
          CFI (cfiCond58) R5 Frame(CFA, -16)
          CFI (cfiCond58) R6 Frame(CFA, -12)
          CFI (cfiCond58) R7 Frame(CFA, -8)
          CFI (cfiCond58) R14 Frame(CFA, -4)
          CFI (cfiCond58) CFA R13+40
          CFI Block cfiPicker59 Using cfiCommon1
          CFI (cfiPicker59) NoFunction
          CFI (cfiPicker59) Picker
        THUMB
?Subroutine9:
        LDR      R0,[R7, #+0]
        LDRB     R0,[R0, #+50]
        BX       LR
          CFI EndBlock cfiCond54
          CFI EndBlock cfiCond55
          CFI EndBlock cfiCond56
          CFI EndBlock cfiCond57
          CFI EndBlock cfiCond58
          CFI EndBlock cfiPicker59

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock60 Using cfiCommon0
          CFI Function photo_connection_lost
          CFI NoCalls
        THUMB
photo_connection_lost:
        LDR.N    R1,??DataTable26_1
        LDR      R0,[R1, #+0]
        CBZ.N    R0,??photo_connection_lost_0
        LDRB     R0,[R0, #+50]
        CMP      R0,#+1
        BNE.N    ??photo_connection_lost_0
        MOVS     R0,#+0
        LDR      R1,[R1, #+0]
        STRB     R0,[R1, #+38]
??photo_connection_lost_0:
        BX       LR
          CFI EndBlock cfiBlock60

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable25:
        DATA32
        DC32     photo_copy_off

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable25_1:
        DATA32
        DC32     ?_9

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable25_2:
        DATA32
        DC32     ?_10

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock61 Using cfiCommon0
          CFI Function photo_poll
        THUMB
photo_poll:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        MOVS     R5,#+0
          CFI FunCall photo_cleanup_active
        BL       photo_cleanup_active
        CBNZ.N   R0,??photo_poll_0
          CFI FunCall photo_recover
        BL       photo_recover
        CBZ.N    R0,??photo_poll_0
        LDR.N    R1,??DataTable26_5
        LDRH     R0,[R1, #+155]
        CBNZ.N   R0,??photo_poll_0
        LDR.N    R0,??DataTable26_2
        LDR      R0,[R0, #+72]
        LDR      R1,[R1, #+157]
        CMP      R0,R1
        BCS.N    ??photo_poll_0
        LDR.N    R1,??DataTable26_1
        LDR      R0,[R1, #+0]
        CMP      R0,#+0
          CFI FunCall photo_clear_confirmed
        ITTE     NE
        LDRBNE   R0,[R0, #+34]
        CMPNE    R0,#+0
        BLEQ     photo_clear_confirmed
??photo_poll_0:
          CFI FunCall photo_cleanup_active
        BL       photo_cleanup_active
        CBNZ.N   R0,??photo_poll_1
          CFI FunCall photo_recover
        BL       photo_recover
        CBNZ.N   R0,??photo_poll_1
          CFI FunCall photo_restore_step
        BL       photo_restore_step
        MOV      R5,R0
??photo_poll_1:
        LDR.N    R4,??DataTable26_1
        LDR      R0,[R4, #+0]
        CMP      R0,#+0
        BEQ.W    ??photo_poll_2
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        LDR.N    R0,??DataTable26_5
        LDRH     R0,[R0, #+129]
        UBFX     R0,R0,#+11,#+1
        CMP      R0,#+0
        BL       ?Subroutine1
??CrossCallReturnLabel_5:
        BEQ.N    ??photo_poll_3
        CMP      R0,#+0
        BEQ.N    ??photo_poll_4
          CFI FunCall photo_stop
        BL       photo_stop
??photo_poll_4:
        B.N      ??photo_poll_5
??photo_poll_3:
        CMP      R0,#+0
        BEQ.N    ??photo_poll_6
        MOVS     R0,#+0
          CFI FunCall uart_getstate
        BL       uart_getstate
        CMP      R0,#+0
        BEQ.N    ??photo_poll_6
        MOVS     R0,#+0
          CFI FunCall uart_isReceiving
        BL       uart_isReceiving
        CMP      R0,#+0
        BNE.N    ??photo_poll_6
          CFI FunCall uart_received
        BL       uart_received
        MOV      R6,R0
        MOVS     R7,#+0
        B.N      ??photo_poll_7
??photo_poll_8:
        LDR      R0,[R4, #+0]
        LDRH     R0,[R0, #+32]
        UXTH     R2,R1
        CMP      R0,R2
        BNE.N    ??photo_poll_9
        UXTH     R1,R1
        LDR      R0,[R4, #+0]
        ADDS     R0,R0,#+115
          CFI FunCall photo_frame
        BL       photo_frame
        MOVS     R1,#+0
        LDR      R0,[R4, #+0]
        STRH     R1,[R0, #+32]
??photo_poll_9:
        ADDS     R7,R7,#+1
??photo_poll_7:
        CMP      R7,R6
        BGE.N    ??photo_poll_10
        BL       ?Subroutine0
??CrossCallReturnLabel_0:
        BEQ.N    ??photo_poll_10
        LDR      R0,[R4, #+0]
        LDRH     R0,[R0, #+32]
        CMP      R0,#+64
        BCS.N    ??photo_poll_11
        LDR      R0,[R4, #+0]
        LDRH     R0,[R0, #+40]
        CMP      R0,#+8
        BCS.N    ??photo_poll_12
        LDR      R0,[R4, #+0]
        LDRH     R1,[R0, #+40]
        ADD      R0,R0,R1
        LDR      R1,[R4, #+0]
        ADD      R1,R1,R7
        LDRB     R1,[R1, #+51]
        STRB     R1,[R0, #+42]
??photo_poll_12:
        LDR      R0,[R4, #+0]
        LDRH     R1,[R0, #+40]
        MOVW     R0,#+65535
        CMP      R1,R0
        BEQ.N    ??photo_poll_13
        LDR      R0,[R4, #+0]
        ADDS     R1,R1,#+1
        STRH     R1,[R0, #+40]
??photo_poll_13:
        LDR      R0,[R4, #+0]
        LDRH     R0,[R0, #+32]
        ADDS     R1,R0,#+1
        LDR      R2,[R4, #+0]
        STRH     R1,[R2, #+32]
        LDR      R1,[R4, #+0]
        ADD      R1,R1,R7
        LDRB     R1,[R1, #+51]
        LDR      R2,[R4, #+0]
        ADD      R0,R2,R0
        STRB     R1,[R0, #+115]
        MOVS     R1,#+6
        BL       ?Subroutine1
??CrossCallReturnLabel_6:
        CMP      R0,#+6
        BNE.N    ??photo_poll_8
        LDR      R0,[R4, #+0]
        LDRH     R0,[R0, #+32]
        CMP      R0,#+4
        BLT.N    ??photo_poll_8
        LDR      R0,[R4, #+0]
        LDRB     R1,[R0, #+117]
        LDRB     R0,[R0, #+118]
        ADD      R1,R1,R0, LSL #+8
        ADDS     R1,R1,#+6
        UXTH     R0,R1
        CMP      R0,#+65
        BCC.N    ??photo_poll_8
??photo_poll_11:
          CFI FunCall photo_stop
        BL       photo_stop
??photo_poll_10:
        MOVS     R0,#+0
          CFI FunCall uart_clear
        BL       uart_clear
??photo_poll_6:
        BL       ?Subroutine1
??CrossCallReturnLabel_7:
        CMP      R0,#+7
        BNE.N    ??photo_poll_14
          CFI FunCall photo_recover
        BL       photo_recover
        CBZ.N    R0,??photo_poll_14
        MOVS     R0,R5
        BNE.N    ??photo_poll_14
          CFI FunCall photo_prepare_step
        BL       photo_prepare_step
        MOV      R5,R0
??photo_poll_14:
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        MOV      R6,R0
        LDR      R0,[R4, #+0]
        LDRB     R0,[R0, #+34]
        CBZ.N    R0,??photo_poll_15
        LDR      R0,[R4, #+0]
        LDR      R0,[R0, #+12]
        SUBS     R0,R6,R0
        MOVW     R1,#+60000
        CMP      R0,R1
          CFI FunCall photo_fail
        ITT      CS
        LDRCS.N  R0,??DataTable26_8
        BLCS     photo_fail
??photo_poll_15:
        BL       ?Subroutine0
??CrossCallReturnLabel_1:
        ITTT     NE
        LDRNE    R0,[R4, #+0]
        LDRBNE   R0,[R0, #+34]
        CMPNE    R0,#+7
        BEQ.N    ??photo_poll_16
        LDR      R0,[R4, #+0]
        LDR      R0,[R0, #+8]
        SUBS     R0,R6,R0
        BMI.N    ??photo_poll_16
        BL       ?Subroutine1
??CrossCallReturnLabel_8:
        CMP      R0,#+6
        BEQ.N    ??photo_poll_17
        LDR      R0,[R4, #+0]
        LDR      R0,[R0, #+16]
        SUBS     R0,R6,R0
        CMP      R0,#+50
        BCC.N    ??photo_poll_16
        B.N      ??photo_poll_18
??photo_poll_17:
        MOVS     R0,R5
        BNE.N    ??photo_poll_18
          CFI FunCall photo_data_ready
        BL       photo_data_ready
        CBZ.N    R0,??photo_poll_19
??photo_poll_18:
        LDR      R0,[R4, #+0]
        LDRB     R0,[R0, #+35]
        ADDS     R2,R0,#+1
        LDR      R1,[R4, #+0]
        STRB     R2,[R1, #+35]
        LDR      R1,[R4, #+0]
        LDRB     R1,[R1, #+34]
        CMP      R1,#+1
        ITE      EQ
        MOVEQ    R1,#+30
        MOVNE    R1,#+3
        CMP      R0,R1
        BLT.N    ??photo_poll_20
        LDR.N    R0,??DataTable26_9
          CFI FunCall photo_fail
        BL       photo_fail
        B.N      ??photo_poll_16
??photo_poll_20:
        MOVS     R0,#+0
        LDR      R1,[R4, #+0]
        STRH     R0,[R1, #+32]
          CFI FunCall photo_issue
        BL       photo_issue
        B.N      ??photo_poll_16
??photo_poll_19:
        MOVS     R5,#+1
??photo_poll_16:
        LDR      R0,[R4, #+0]
        LDRB     R0,[R0, #+38]
        CBZ.N    R0,??photo_poll_21
        LDR.N    R0,??DataTable26_6
        LDR      R0,[R0, #+0]
        SUBS     R6,R6,R0
        MOVW     R0,#+60000
        CMP      R6,R0
        BCC.N    ??photo_poll_22
        MOVS     R1,#+0
        LDR      R0,[R4, #+0]
        STRB     R1,[R0, #+38]
        LDR.N    R0,??DataTable26_10
          CFI FunCall user_debug
        BL       user_debug
        B.N      ??photo_poll_21
??photo_poll_22:
        LDR      R0,[R4, #+0]
        LDRB     R0,[R0, #+50]
        CMP      R0,#+1
        BNE.N    ??photo_poll_23
          CFI FunCall comm_channel_ready
        BL       comm_channel_ready
        CBNZ.N   R0,??photo_poll_23
??photo_poll_5:
        MOVS     R1,#+0
        LDR      R0,[R4, #+0]
        STRB     R1,[R0, #+38]
        B.N      ??photo_poll_21
??photo_poll_23:
          CFI FunCall photo_cleanup_active
        BL       photo_cleanup_active
        CMP      R0,#+1
        BEQ.N    ??photo_poll_24
          CFI FunCall photo_recover
        BL       photo_recover
        CBZ.N    R0,??photo_poll_21
??photo_poll_24:
        LDR      R0,[R4, #+0]
        LDRB     R0,[R0, #+50]
        CMP      R0,#+1
        BNE.N    ??photo_poll_25
          CFI FunCall comm_tx_busy
        BL       comm_tx_busy
        CBNZ.N   R0,??photo_poll_21
??photo_poll_25:
          CFI FunCall photo_send
        BL       photo_send
??photo_poll_21:
        BL       ?Subroutine0
??CrossCallReturnLabel_2:
        ITTT     EQ
        LDREQ    R0,[R4, #+0]
        LDRBEQ   R0,[R0, #+38]
        CMPEQ    R0,#+0
        BNE.N    ??photo_poll_2
        LDR      R0,[R4, #+0]
          CFI FunCall free
        BL       free
        MOVS     R0,#+0
        STR      R0,[R4, #+0]
??photo_poll_2:
        CBNZ.N   R5,??photo_poll_26
          CFI FunCall photo_cleanup_active
        BL       photo_cleanup_active
        CBZ.N    R0,??photo_poll_26
        POP      {R0,R4-R7,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R7 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall photo_cleanup_step
        B.N      photo_cleanup_step
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
??photo_poll_26:
        POP      {R0,R4-R7,PC}
          CFI EndBlock cfiBlock61

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable26:
        DATA32
        DC32     ?_11

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable26_1:
        DATA32
        DC32     photo

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable26_2:
        DATA32
        DC32     mem

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable26_3:
        DATA32
        DC32     ?_12

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable26_4:
        DATA32
        DC32     ?_13

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable26_5:
        DATA32
        DC32     device

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable26_6:
        DATA32
        DC32     photo_pending_since

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable26_7:
        DATA32
        DC32     ?_14

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable26_8:
        DATA32
        DC32     ?_15

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable26_9:
        DATA32
        DC32     ?_16

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable26_10:
        DATA32
        DC32     ?_17

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond62 Using cfiCommon0
          CFI Function photo_issue
          CFI Conditional ??CrossCallReturnLabel_3
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
          CFI Block cfiCond63 Using cfiCommon0
          CFI (cfiCond63) Function photo_issue
          CFI (cfiCond63) Conditional ??CrossCallReturnLabel_4
          CFI (cfiCond63) R4 Frame(CFA, -8)
          CFI (cfiCond63) R14 Frame(CFA, -4)
          CFI (cfiCond63) CFA R13+16
          CFI Block cfiCond64 Using cfiCommon0
          CFI (cfiCond64) Function photo_poll
          CFI (cfiCond64) Conditional ??CrossCallReturnLabel_5
          CFI (cfiCond64) R4 Frame(CFA, -20)
          CFI (cfiCond64) R5 Frame(CFA, -16)
          CFI (cfiCond64) R6 Frame(CFA, -12)
          CFI (cfiCond64) R7 Frame(CFA, -8)
          CFI (cfiCond64) R14 Frame(CFA, -4)
          CFI (cfiCond64) CFA R13+24
          CFI Block cfiCond65 Using cfiCommon0
          CFI (cfiCond65) Function photo_poll
          CFI (cfiCond65) Conditional ??CrossCallReturnLabel_6
          CFI (cfiCond65) R4 Frame(CFA, -20)
          CFI (cfiCond65) R5 Frame(CFA, -16)
          CFI (cfiCond65) R6 Frame(CFA, -12)
          CFI (cfiCond65) R7 Frame(CFA, -8)
          CFI (cfiCond65) R14 Frame(CFA, -4)
          CFI (cfiCond65) CFA R13+24
          CFI Block cfiCond66 Using cfiCommon0
          CFI (cfiCond66) Function photo_poll
          CFI (cfiCond66) Conditional ??CrossCallReturnLabel_7
          CFI (cfiCond66) R4 Frame(CFA, -20)
          CFI (cfiCond66) R5 Frame(CFA, -16)
          CFI (cfiCond66) R6 Frame(CFA, -12)
          CFI (cfiCond66) R7 Frame(CFA, -8)
          CFI (cfiCond66) R14 Frame(CFA, -4)
          CFI (cfiCond66) CFA R13+24
          CFI Block cfiCond67 Using cfiCommon0
          CFI (cfiCond67) Function photo_poll
          CFI (cfiCond67) Conditional ??CrossCallReturnLabel_8
          CFI (cfiCond67) R4 Frame(CFA, -20)
          CFI (cfiCond67) R5 Frame(CFA, -16)
          CFI (cfiCond67) R6 Frame(CFA, -12)
          CFI (cfiCond67) R7 Frame(CFA, -8)
          CFI (cfiCond67) R14 Frame(CFA, -4)
          CFI (cfiCond67) CFA R13+24
          CFI Block cfiPicker68 Using cfiCommon1
          CFI (cfiPicker68) NoFunction
          CFI (cfiPicker68) Picker
        THUMB
?Subroutine1:
        LDR      R0,[R4, #+0]
        LDRB     R0,[R0, #+34]
        BX       LR
          CFI EndBlock cfiCond62
          CFI EndBlock cfiCond63
          CFI EndBlock cfiCond64
          CFI EndBlock cfiCond65
          CFI EndBlock cfiCond66
          CFI EndBlock cfiCond67
          CFI EndBlock cfiPicker68

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond69 Using cfiCommon0
          CFI Function photo_poll
          CFI Conditional ??CrossCallReturnLabel_0
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
          CFI Block cfiCond70 Using cfiCommon0
          CFI (cfiCond70) Function photo_poll
          CFI (cfiCond70) Conditional ??CrossCallReturnLabel_1
          CFI (cfiCond70) R4 Frame(CFA, -20)
          CFI (cfiCond70) R5 Frame(CFA, -16)
          CFI (cfiCond70) R6 Frame(CFA, -12)
          CFI (cfiCond70) R7 Frame(CFA, -8)
          CFI (cfiCond70) R14 Frame(CFA, -4)
          CFI (cfiCond70) CFA R13+24
          CFI Block cfiCond71 Using cfiCommon0
          CFI (cfiCond71) Function photo_poll
          CFI (cfiCond71) Conditional ??CrossCallReturnLabel_2
          CFI (cfiCond71) R4 Frame(CFA, -20)
          CFI (cfiCond71) R5 Frame(CFA, -16)
          CFI (cfiCond71) R6 Frame(CFA, -12)
          CFI (cfiCond71) R7 Frame(CFA, -8)
          CFI (cfiCond71) R14 Frame(CFA, -4)
          CFI (cfiCond71) CFA R13+24
          CFI Block cfiPicker72 Using cfiCommon1
          CFI (cfiPicker72) NoFunction
          CFI (cfiPicker72) Picker
        THUMB
?Subroutine0:
        LDR      R0,[R4, #+0]
        LDRB     R0,[R0, #+34]
        CMP      R0,#+0
        BX       LR
          CFI EndBlock cfiCond69
          CFI EndBlock cfiCond70
          CFI EndBlock cfiCond71
          CFI EndBlock cfiPicker72

        SECTION `.iar_vfe_header`:DATA:NOALLOC:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
        DC32 0

        END
