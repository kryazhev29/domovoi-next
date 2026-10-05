; Cortex-M3 / IAR. Executable production module.
; Reference: Tests/asm/reference/Modules/usb_stm32f10x.c
        PUBLIC usb_buffer_count
        PUBLIC usb_bus_errors
        PUBLIC usb_hung_armed
        PUBLIC usb_hung_ms
        PUBLIC usb_in_ago
        PUBLIC usb_in_taken
        PUBLIC usb_last_addr
        PUBLIC usb_last_bus_errors
        PUBLIC usb_last_recovery
        PUBLIC usb_last_regs
        PUBLIC usb_out_ago
        PUBLIC usb_recovery_ms
        PUBLIC usb_recovery_pending
        PUBLIC usb_rx_discard
        PUBLIC usb_rx_expired
        PUBLIC usb_rx_processing
        PUBLIC usb_rx_silence
        PUBLIC usb_sof_ago
        PUBLIC usb_unconf_armed
        PUBLIC usb_unconf_ms

        RTMODEL "__CPP_Runtime", "1"
        RTMODEL "__SystemLibrary", "DLib"
        RTMODEL "__dlib_file_descriptor", "0"
        RTMODEL "__dlib_full_locale_support", "0"
        RTMODEL "__dlib_version", "6"
        AAPCS BASE,INTERWORK
        PRESERVE8
        REQUIRE8

        #define SHT_PROGBITS 0x1

        EXTERN Set_System
        EXTERN Set_USBClock
        EXTERN USART_To_USB_Send_Data
        EXTERN USB_Init
        EXTERN USB_Interrupts_Config
        EXTERN USB_Send_Debug
        EXTERN USB_Tx_Pending
        EXTERN USB_Tx_Queued
        EXTERN USB_Tx_Reset
        EXTERN USB_Tx_State
        EXTERN __aeabi_memcpy
        EXTERN __aeabi_memmove
        EXTERN bDeviceState
        EXTERN crc16_calc
        EXTERN memcmp
        EXTERN strlen
        EXTERN usb_in_armed
        EXTERN user_storage_command_ready

        PUBLIC usb_buffer
        PUBLIC usb_bus_error_seen
        PUBLIC usb_bus_resets
        PUBLIC usb_check_data
        PUBLIC usb_clear_buffer
        PUBWEAK usb_data_reading
        PUBLIC usb_datareceived
        PUBLIC usb_debug_dropped
        PUBLIC usb_diag_fill
        PUBLIC usb_diag_fill_addr
        PUBLIC usb_diag_fill_errors
        PUBLIC usb_diag_fill_regs
        PUBLIC usb_enabled
        PUBLIC usb_esof_forced
        PUBLIC usb_esof_timeouts
        PUBLIC usb_get_state
        PUBLIC usb_in_done
        PUBLIC usb_init
        PUBLIC usb_out_unconfigured
        PUBLIC usb_recovery_count
        PUBLIC usb_recovery_reason
        PUBLIC usb_request_recovery
        PUBLIC usb_rx_errors
        PUBLIC usb_send
        PUBLIC usb_sendstr
        PUBLIC usb_sof_seen
        PUBLIC usb_timer1ms
        PUBLIC usb_tx_errors
        PUBLIC usb_watch_in_recoveries
        PUBLIC usb_watch_unconf_recoveries

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
usb_buffer:
        DS8 2'048

        SECTION `.bss`:DATA:REORDER:NOROOT(1)
        DATA
usb_buffer_count:
        DS8 2

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
usb_rx_processing:
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
usb_rx_discard:
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(1)
        DATA
usb_rx_silence:
        DS8 2

        SECTION `.bss`:DATA:REORDER:NOROOT(1)
        DATA
usb_rx_expired:
        DS8 2

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
usb_rx_errors:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
usb_enabled:
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
usb_tx_errors:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
usb_debug_dropped:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
usb_recovery_count:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
usb_recovery_pending:
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
usb_recovery_ms:
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
usb_esof_forced:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
usb_esof_timeouts:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
usb_bus_resets:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
usb_recovery_reason:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
usb_watch_in_recoveries:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
usb_watch_unconf_recoveries:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
usb_out_unconfigured:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
usb_last_recovery:
        DS8 28

        SECTION `.data`:DATA:REORDER:NOROOT(1)
        DATA
usb_in_ago:
        DATA16
        DC16 65'535

        SECTION `.data`:DATA:REORDER:NOROOT(1)
        DATA
usb_out_ago:
        DATA16
        DC16 65'535

        SECTION `.data`:DATA:REORDER:NOROOT(1)
        DATA
usb_sof_ago:
        DATA16
        DC16 65'535

        SECTION `.bss`:DATA:REORDER:NOROOT(1)
        DATA
usb_hung_ms:
        DS8 2

        SECTION `.bss`:DATA:REORDER:NOROOT(1)
        DATA
usb_unconf_ms:
        DS8 2

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
usb_hung_armed:
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
usb_unconf_armed:
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
usb_in_taken:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
usb_last_regs:
        DS8 36

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
usb_bus_errors:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
usb_last_bus_errors:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
usb_last_addr:
        DS8 8

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock0 Using cfiCommon0
          CFI Function usb_regs_read
          CFI NoCalls
        THUMB
usb_regs_read:
        LDR.W    R1,??DataTable19
        LDR      R1,[R1, #+0]
        UXTH     R1,R1
        STR      R1,[R0, #+0]
        LDR.W    R1,??DataTable19_1
        LDR      R1,[R1, #+0]
        UXTH     R1,R1
        STR      R1,[R0, #+4]
        LDR.W    R1,??DataTable19_2
        LDR      R2,[R1, #+0]
        UXTH     R2,R2
        LDR.W    R1,??DataTable19_3
        ADD      R1,R1,R2, LSL #+1
        LDR      R1,[R1, #+0]
        UBFX     R1,R1,#+0,#+10
        STR      R1,[R0, #+8]
        LDR.W    R1,??DataTable19_4
        LDR      R1,[R1, #+0]
        UXTH     R1,R1
        STR      R1,[R0, #+12]
        LDR.W    R1,??DataTable19_5
        LDR      R1,[R1, #+0]
        UXTH     R1,R1
        STR      R1,[R0, #+16]
        LDR.W    R1,??DataTable19_6
        LDR      R1,[R1, #+0]
        UXTH     R1,R1
        STR      R1,[R0, #+20]
        LDR.W    R1,??DataTable19_7
        LDRB     R1,[R1, #+0]
        STR      R1,[R0, #+24]
        LDR.W    R1,??DataTable19_8
        LDR      R1,[R1, #+0]
        STR      R1,[R0, #+28]
        LDR.W    R1,??DataTable19_9
        LDR      R1,[R1, #+0]
        STR      R1,[R0, #+32]
        BX       LR
          CFI EndBlock cfiBlock0

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock1 Using cfiCommon0
          CFI Function usb_in_done
          CFI NoCalls
        THUMB
usb_in_done:
        MOVS     R1,#+0
        LDR.W    R0,??DataTable19_10
        STRH     R1,[R0, #+0]
        LDR.W    R0,??DataTable19_11
        STRH     R1,[R0, #+0]
        LDR.W    R1,??DataTable19_9
        B.N      ?Subroutine0
          CFI EndBlock cfiBlock1

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock2 Using cfiCommon0
          CFI Function usb_bus_error_seen
          CFI NoCalls
        THUMB
usb_bus_error_seen:
        LDR.W    R1,??DataTable19_12
          CFI EndBlock cfiBlock2
        REQUIRE ?Subroutine0

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock3 Using cfiCommon0
          CFI NoFunction
        THUMB
?Subroutine0:
        LDR      R0,[R1, #+0]
        ADDS     R0,R0,#+1
        STR      R0,[R1, #+0]
        BX       LR
          CFI EndBlock cfiBlock3

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock4 Using cfiCommon0
          CFI Function usb_sof_seen
          CFI NoCalls
        THUMB
usb_sof_seen:
        MOVS     R1,#+0
        LDR.W    R0,??DataTable19_13
        STRH     R1,[R0, #+0]
        BX       LR
          CFI EndBlock cfiBlock4

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock5 Using cfiCommon0
          CFI Function usb_request_recovery
        THUMB
usb_request_recovery:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        BL       ?Subroutine3
??CrossCallReturnLabel_0:
        LDR.W    R7,??DataTable19_14
        LDR.W    R0,??DataTable19_15
        LDR      R0,[R0, #+0]
        STR      R0,[R7, #+0]
        LDR.W    R5,??DataTable19_16
        LDR      R0,[R5, #+0]
        STR      R0,[R7, #+4]
          CFI FunCall USB_Tx_Pending
        BL       USB_Tx_Pending
        STR      R0,[R7, #+8]
          CFI FunCall USB_Tx_Queued
        BL       USB_Tx_Queued
        STR      R0,[R7, #+12]
        LDR.W    R0,??DataTable19_10
        LDRH     R0,[R0, #+0]
        STR      R0,[R7, #+16]
        LDR.W    R6,??DataTable19_17
        LDRH     R0,[R6, #+0]
        STR      R0,[R7, #+20]
        LDR.W    R0,??DataTable19_13
        LDRH     R0,[R0, #+0]
        STR      R0,[R7, #+24]
        LDR.W    R0,??DataTable19_18
          CFI FunCall usb_regs_read
        BL       usb_regs_read
        LDR.W    R0,??DataTable19_12
        LDR      R1,[R0, #+0]
        LDR.W    R0,??DataTable19_19
        STR      R1,[R0, #+0]
        LDR.W    R0,??DataTable19_20
        LDR.W    R1,??DataTable19_21
        LDR      R1,[R1, #+0]
        UXTH     R1,R1
        STR      R1,[R0, #+0]
        LDR.W    R1,??DataTable19_22
        LDR      R1,[R1, #+0]
        UXTH     R1,R1
        STR      R1,[R0, #+4]
        MOVS     R1,#+0
        LDR.W    R0,??DataTable19_23
        STRB     R1,[R0, #+0]
        LDR.W    R0,??DataTable19_24
        STRB     R1,[R0, #+0]
        LDR.W    R0,??DataTable19_25
        STRH     R1,[R0, #+0]
        LDR.W    R0,??DataTable19_11
        STRH     R1,[R0, #+0]
        MOVW     R0,#+65535
        STRH     R0,[R6, #+0]
        MOVS     R1,#+1
        LDR.W    R0,??DataTable19_5
        STR      R1,[R0, #+0]
        MOV      R1,#+256
        LDR.W    R0,??DataTable19_26
        STR      R1,[R0, #+0]
        MOVS     R1,#+0
        LDR.W    R0,??DataTable19_27
        STRB     R1,[R0, #+0]
          CFI FunCall USB_Tx_Reset
        BL       USB_Tx_Reset
          CFI FunCall usb_clear_buffer
        BL       usb_clear_buffer
        MOVS     R0,#+0
        STR      R0,[R5, #+0]
        MOVS     R1,#+20
        LDR.W    R0,??DataTable19_28
        STRB     R1,[R0, #+0]
        MOVS     R1,#+1
        LDR.W    R0,??DataTable19_29
        STRB     R1,[R0, #+0]
        LDR.W    R0,??DataTable19_30
        LDR      R1,[R0, #+0]
        ADDS     R1,R1,#+1
        STR      R1,[R0, #+0]
        MSR      PRIMASK,R4
        POP      {R0,R4-R7,PC}
          CFI EndBlock cfiBlock5

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock6 Using cfiCommon0
          CFI Function usb_init
        THUMB
usb_init:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        MOV      R4,#+256
        LDR.W    R0,??DataTable19_26
        STR      R4,[R0, #+0]
          CFI FunCall Set_System
        BL       Set_System
          CFI FunCall Set_USBClock
        BL       Set_USBClock
          CFI FunCall USB_Interrupts_Config
        BL       USB_Interrupts_Config
          CFI FunCall USB_Init
        BL       USB_Init
        MOVS     R1,#+1
        LDR.W    R0,??DataTable19_27
        STRB     R1,[R0, #+0]
        LDR.W    R0,??DataTable19_31
        STR      R4,[R0, #+0]
        POP      {R4,PC}
          CFI EndBlock cfiBlock6

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock7 Using cfiCommon0
          CFI Function usb_datareceived
        THUMB
usb_datareceived:
        PUSH     {R3-R11,LR}
          CFI R14 Frame(CFA, -4)
          CFI R11 Frame(CFA, -8)
          CFI R10 Frame(CFA, -12)
          CFI R9 Frame(CFA, -16)
          CFI R8 Frame(CFA, -20)
          CFI R7 Frame(CFA, -24)
          CFI R6 Frame(CFA, -28)
          CFI R5 Frame(CFA, -32)
          CFI R4 Frame(CFA, -36)
          CFI CFA R13+40
        MOV      R9,R0
        MOVS     R5,R1
        MRS      R6,PRIMASK
        CPSID    I
        LDR.W    R7,??DataTable19_32
        LDRH     R8,[R7, #+0]
        BNE.N    ??usb_datareceived_0
        LDR.W    R0,??DataTable19_33
        LDRB     R0,[R0, #+0]
        B.N      ??CrossCallReturnLabel_5
??usb_datareceived_0:
        MOVS     R1,#+0
        LDR.W    R0,??DataTable19_17
        STRH     R1,[R0, #+0]
        LDR.W    R0,??DataTable19_16
        LDR      R0,[R0, #+0]
        CMP      R0,#+5
        BEQ.N    ??usb_datareceived_1
        LDR.W    R1,??DataTable19_34
        LDR      R0,[R1, #+0]
        ADDS     R0,R0,#+1
        STR      R0,[R1, #+0]
        MOVS     R1,#+1
        LDR.W    R0,??DataTable19_23
        B.N      ??usb_datareceived_2
??usb_datareceived_1:
          CFI FunCall USB_Tx_Pending
        BL       USB_Tx_Pending
        CBZ.N    R0,??usb_datareceived_3
        MOVS     R1,#+1
        LDR.W    R0,??DataTable19_24
??usb_datareceived_2:
        STRB     R1,[R0, #+0]
??usb_datareceived_3:
        LDR.W    R0,??DataTable19_33
        LDRB     R0,[R0, #+0]
        CMP      R0,#+0
        BNE.N    ??CrossCallReturnLabel_5
        LDR.W    R0,??DataTable19_35
        LDRB     R0,[R0, #+0]
        CBNZ.N   R0,??usb_datareceived_4
        LDR.W    R0,??DataTable19_36
        LDRH     R4,[R0, #+0]
        MOV      R11,#+0
        CMP      R4,#+6
        BLT.N    ??usb_datareceived_5
        LDR.W    R10,??DataTable19_37
        MOVS     R2,#+4
        LDR.W    R1,??DataTable19_38
        MOV      R0,R10
          CFI FunCall memcmp
        BL       memcmp
        CBNZ.N   R0,??usb_datareceived_6
        LDRB     R1,[R10, #+4]
        LDRB     R0,[R10, #+5]
        ORR      R0,R1,R0, LSL #+8
        ADD      R11,R0,#+8
??usb_datareceived_6:
        CMP      R4,R11
        ITT      CC
        MOVWCC   R0,#+2049
        CMPCC    R11,R0
        BCS.N    ??usb_datareceived_4
??usb_datareceived_7:
        MOV      R0,R4
          CFI FunCall usb_rx_consume
        BL       usb_rx_consume
        LDRH     R8,[R7, #+0]
??usb_datareceived_4:
        RSB      R1,R8,#+2048
        CMP      R1,R5
        BGE.N    ??usb_datareceived_8
        MOVS     R1,#+1
        LDR.W    R0,??DataTable19_33
        STRB     R1,[R0, #+0]
        BL       ?Subroutine6
??CrossCallReturnLabel_12:
        B.N      ??CrossCallReturnLabel_5
??usb_datareceived_5:
        CMP      R4,#+0
        BNE.N    ??usb_datareceived_7
        B.N      ??usb_datareceived_4
??usb_datareceived_8:
        MOV      R2,R5
        MOV      R1,R9
        LDR.W    R0,??DataTable19_37
        ADD      R0,R0,R8
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        DMB      SY
        ADD      R5,R5,R8
        STRH     R5,[R7, #+0]
        BL       ?Subroutine4
??CrossCallReturnLabel_5:
        MSR      PRIMASK,R6
        POP      {R0,R4-R11,PC}
          CFI EndBlock cfiBlock7

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock8 Using cfiCommon0
          CFI Function usb_send
        THUMB
usb_send:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
        LDR.W    R2,??DataTable19_27
        LDRB     R2,[R2, #+0]
        CBZ.N    R2,??usb_send_0
        LDR.W    R2,??DataTable19_39
        LDR      R2,[R2, #+0]
        LSLS     R2,R2,#+16
        BPL.N    ??usb_send_0
          CFI FunCall USART_To_USB_Send_Data
        BL       USART_To_USB_Send_Data
        CBZ.N    R0,??usb_send_0
        MOVS     R0,#+1
        POP      {R1,PC}
??usb_send_0:
        LDR.W    R1,??DataTable19_40
        LDR      R0,[R1, #+0]
        ADDS     R0,R0,#+1
        STR      R0,[R1, #+0]
        MOVS     R0,#+0
        POP      {R1,PC}
          CFI EndBlock cfiBlock8

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock9 Using cfiCommon0
          CFI Function usb_sendstr
        THUMB
usb_sendstr:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R4,R0
          CFI FunCall strlen
        BL       strlen
        MOV      R5,R0
        MOVS     R1,#+0
        LDR.W    R0,??DataTable19_27
        LDRB     R0,[R0, #+0]
        CBZ.N    R0,??usb_sendstr_0
        LDR.W    R0,??DataTable19_39
        LDR      R0,[R0, #+0]
        LSLS     R0,R0,#+16
        BPL.N    ??usb_sendstr_0
        UXTH     R1,R5
        MOV      R0,R4
          CFI FunCall USB_Send_Debug
        BL       USB_Send_Debug
        MOV      R1,R0
??usb_sendstr_0:
        LDR.W    R2,??DataTable19_41
        LDR      R0,[R2, #+0]
        UXTH     R5,R5
        SUBS     R5,R5,R1
        ADDS     R5,R5,R0
        STR      R5,[R2, #+0]
        POP      {R0,R4,R5,PC}
          CFI EndBlock cfiBlock9

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock10 Using cfiCommon0
          CFI Function usb_timer1ms
        THUMB
usb_timer1ms:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        BL       ?Subroutine3
??CrossCallReturnLabel_1:
        LDR.W    R1,??DataTable19_28
        LDRB     R0,[R1, #+0]
        CBZ.N    R0,??usb_timer1ms_0
        LDRB     R0,[R1, #+0]
        SUBS     R0,R0,#+1
        STRB     R0,[R1, #+0]
??usb_timer1ms_0:
        MOVW     R5,#+65535
        LDR.W    R0,??DataTable19_10
        BL       ?Subroutine5
??CrossCallReturnLabel_8:
        LDR.W    R0,??DataTable19_17
        BL       ?Subroutine5
??CrossCallReturnLabel_9:
        LDR.W    R0,??DataTable19_13
        BL       ?Subroutine5
??CrossCallReturnLabel_10:
          CFI FunCall USB_Tx_Pending
        BL       USB_Tx_Pending
        CBNZ.N   R0,??usb_timer1ms_1
        MOVS     R1,#+0
        LDR.W    R0,??DataTable19_24
        STRB     R1,[R0, #+0]
        LDR.W    R0,??DataTable19_11
        B.N      ??usb_timer1ms_2
??usb_timer1ms_1:
        LDR.W    R0,??DataTable19_24
        LDRB     R0,[R0, #+0]
        CMP      R0,#+0
        ITTT     NE
        LDRNE.W  R0,??DataTable19_11
        LDRHNE   R1,[R0, #+0]
        CMPNE    R1,R5
        BEQ.N    ??usb_timer1ms_3
        LDRH     R1,[R0, #+0]
        ADDS     R1,R1,#+1
??usb_timer1ms_2:
        STRH     R1,[R0, #+0]
??usb_timer1ms_3:
        LDR.W    R0,??DataTable19_16
        LDR      R0,[R0, #+0]
        CMP      R0,#+5
        BNE.N    ??usb_timer1ms_4
        MOVS     R0,#+0
        LDR.W    R1,??DataTable19_23
        STRB     R0,[R1, #+0]
        LDR.W    R1,??DataTable19_25
        STRH     R0,[R1, #+0]
        B.N      ??CrossCallReturnLabel_11
??usb_timer1ms_4:
        LDR.W    R0,??DataTable19_23
        LDRB     R0,[R0, #+0]
        CMP      R0,#+0
        ITTT     NE
        LDRNE.W  R0,??DataTable19_25
        LDRHNE   R1,[R0, #+0]
        CMPNE    R1,R5
        BL       ??Subroutine5_0
??CrossCallReturnLabel_11:
        LDR.W    R0,??DataTable19_42
        LDRH     R2,[R0, #+0]
        LDR.W    R1,??DataTable19_32
        LDRH     R1,[R1, #+0]
        MOVS     R3,R1
        BEQ.N    ??usb_timer1ms_5
        CMP      R2,#+200
        BGE.N    ??usb_timer1ms_5
        ADDS     R2,R2,#+1
        STRH     R2,[R0, #+0]
        UXTH     R2,R2
        CMP      R2,#+200
        ITT      EQ
        LDREQ.W  R0,??DataTable19_36
        STRHEQ   R1,[R0, #+0]
??usb_timer1ms_5:
        B.N      ?Subroutine2
          CFI EndBlock cfiBlock10

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond11 Using cfiCommon0
          CFI Function usb_timer1ms
          CFI Conditional ??CrossCallReturnLabel_8
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
          CFI Block cfiCond12 Using cfiCommon0
          CFI (cfiCond12) Function usb_timer1ms
          CFI (cfiCond12) Conditional ??CrossCallReturnLabel_9
          CFI (cfiCond12) R4 Frame(CFA, -12)
          CFI (cfiCond12) R5 Frame(CFA, -8)
          CFI (cfiCond12) R14 Frame(CFA, -4)
          CFI (cfiCond12) CFA R13+16
          CFI Block cfiCond13 Using cfiCommon0
          CFI (cfiCond13) Function usb_timer1ms
          CFI (cfiCond13) Conditional ??CrossCallReturnLabel_10
          CFI (cfiCond13) R4 Frame(CFA, -12)
          CFI (cfiCond13) R5 Frame(CFA, -8)
          CFI (cfiCond13) R14 Frame(CFA, -4)
          CFI (cfiCond13) CFA R13+16
          CFI Block cfiPicker14 Using cfiCommon1
          CFI (cfiPicker14) NoFunction
          CFI (cfiPicker14) Picker
        THUMB
?Subroutine5:
        LDRH     R1,[R0, #+0]
        CMP      R1,R5
          CFI Block cfiCond15 Using cfiCommon0
          CFI (cfiCond15) Function usb_timer1ms
          CFI (cfiCond15) Conditional ??CrossCallReturnLabel_11
          CFI (cfiCond15) R4 Frame(CFA, -12)
          CFI (cfiCond15) R5 Frame(CFA, -8)
          CFI (cfiCond15) R14 Frame(CFA, -4)
          CFI (cfiCond15) CFA R13+16
??Subroutine5_0:
        BEQ.N    ??Subroutine5_1
        LDRH     R1,[R0, #+0]
        ADDS     R1,R1,#+1
        STRH     R1,[R0, #+0]
??Subroutine5_1:
        BX       LR
          CFI EndBlock cfiCond11
          CFI EndBlock cfiCond12
          CFI EndBlock cfiCond13
          CFI EndBlock cfiPicker14
          CFI EndBlock cfiCond15

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock16 Using cfiCommon0
          CFI Function usb_rx_consume
        THUMB
usb_rx_consume:
        PUSH     {R4-R8,LR}
          CFI R14 Frame(CFA, -4)
          CFI R8 Frame(CFA, -8)
          CFI R7 Frame(CFA, -12)
          CFI R6 Frame(CFA, -16)
          CFI R5 Frame(CFA, -20)
          CFI R4 Frame(CFA, -24)
          CFI CFA R13+24
        MOV      R7,R0
        LDR.W    R5,??DataTable19_32
        LDRH     R4,[R5, #+0]
        LDR.W    R6,??DataTable19_36
        LDRH     R8,[R6, #+0]
        SUBS     R4,R4,R7
        UXTH     R4,R4
        LDR.W    R0,??DataTable19_37
        MOV      R2,R4
        ADDS     R1,R0,R7
          CFI FunCall __aeabi_memmove
        BL       __aeabi_memmove
        STRH     R4,[R5, #+0]
        CMP      R7,R8
        ITE      LT
        SUBLT    R7,R8,R7
        MOVGE    R7,#+0
        STRH     R7,[R6, #+0]
        CBNZ.N   R4,??CrossCallReturnLabel_6
        BL       ?Subroutine4
??CrossCallReturnLabel_6:
        POP      {R4-R8,PC}
          CFI EndBlock cfiBlock16

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond17 Using cfiCommon0
          CFI Function usb_datareceived
          CFI Conditional ??CrossCallReturnLabel_5
          CFI R4 Frame(CFA, -36)
          CFI R5 Frame(CFA, -32)
          CFI R6 Frame(CFA, -28)
          CFI R7 Frame(CFA, -24)
          CFI R8 Frame(CFA, -20)
          CFI R9 Frame(CFA, -16)
          CFI R10 Frame(CFA, -12)
          CFI R11 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+40
          CFI Block cfiCond18 Using cfiCommon0
          CFI (cfiCond18) Function usb_rx_consume
          CFI (cfiCond18) Conditional ??CrossCallReturnLabel_6
          CFI (cfiCond18) R4 Frame(CFA, -24)
          CFI (cfiCond18) R5 Frame(CFA, -20)
          CFI (cfiCond18) R6 Frame(CFA, -16)
          CFI (cfiCond18) R7 Frame(CFA, -12)
          CFI (cfiCond18) R8 Frame(CFA, -8)
          CFI (cfiCond18) R14 Frame(CFA, -4)
          CFI (cfiCond18) CFA R13+24
          CFI Block cfiCond19 Using cfiCommon0
          CFI (cfiCond19) Function usb_check_data
          CFI (cfiCond19) Conditional ??CrossCallReturnLabel_7
          CFI (cfiCond19) R4 Frame(CFA, -36)
          CFI (cfiCond19) R5 Frame(CFA, -32)
          CFI (cfiCond19) R6 Frame(CFA, -28)
          CFI (cfiCond19) R7 Frame(CFA, -24)
          CFI (cfiCond19) R8 Frame(CFA, -20)
          CFI (cfiCond19) R9 Frame(CFA, -16)
          CFI (cfiCond19) R10 Frame(CFA, -12)
          CFI (cfiCond19) R11 Frame(CFA, -8)
          CFI (cfiCond19) R14 Frame(CFA, -4)
          CFI (cfiCond19) CFA R13+40
          CFI Block cfiPicker20 Using cfiCommon1
          CFI (cfiPicker20) NoFunction
          CFI (cfiPicker20) Picker
        THUMB
?Subroutine4:
        MOVS     R1,#+0
        LDR.W    R0,??DataTable19_42
        STRH     R1,[R0, #+0]
        BX       LR
          CFI EndBlock cfiCond17
          CFI EndBlock cfiCond18
          CFI EndBlock cfiCond19
          CFI EndBlock cfiPicker20

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock21 Using cfiCommon0
          CFI Function usb_check_data
        THUMB
usb_check_data:
        PUSH     {R3-R11,LR}
          CFI R14 Frame(CFA, -4)
          CFI R11 Frame(CFA, -8)
          CFI R10 Frame(CFA, -12)
          CFI R9 Frame(CFA, -16)
          CFI R8 Frame(CFA, -20)
          CFI R7 Frame(CFA, -24)
          CFI R6 Frame(CFA, -28)
          CFI R5 Frame(CFA, -32)
          CFI R4 Frame(CFA, -36)
          CFI CFA R13+40
          CFI FunCall user_storage_command_ready
        BL       user_storage_command_ready
        MRS      R9,PRIMASK
        MOVS     R4,#+0
        CPSID    I
        LDR.W    R5,??DataTable19_35
        LDRB     R1,[R5, #+0]
        CMP      R1,#+0
        BNE.N    ??usb_check_data_0
        LDR.W    R2,??DataTable19_29
        LDRB     R1,[R2, #+0]
        CBZ.N    R1,??usb_check_data_1
        LDR.N    R0,??DataTable19_28
        LDRB     R0,[R0, #+0]
        CMP      R0,#+0
??usb_check_data_0:
        BNE.N    ??usb_check_data_2
        LDR.W    R0,??DataTable19_39
        LDR      R0,[R0, #+0]
        LSLS     R0,R0,#+16
        BPL.N    ??usb_check_data_2
        MOVS     R0,#+0
        STRB     R0,[R2, #+0]
          CFI FunCall usb_init
        BL       usb_init
        B.N      ??usb_check_data_2
??usb_check_data_1:
        LDR.N    R1,??DataTable19_11
        LDRH     R2,[R1, #+0]
        CMP      R2,#+2000
        ITTT     CC
        LDRCC.N  R2,??DataTable19_25
        LDRHCC   R2,[R2, #+0]
        CMPCC    R2,#+2000
        BCC.N    ??usb_check_data_3
        LDRH     R0,[R1, #+0]
        CMP      R0,#+2000
        BCC.N    ??usb_check_data_4
        MOVS     R1,#+2
        LDR.N    R0,??DataTable19_15
        STR      R1,[R0, #+0]
        LDR.N    R1,??DataTable19_43
        B.N      ??usb_check_data_5
??usb_check_data_4:
        MOVS     R1,#+3
        LDR.N    R0,??DataTable19_15
        STR      R1,[R0, #+0]
        LDR.N    R1,??DataTable19_44
??usb_check_data_5:
        LDR      R0,[R1, #+0]
        ADDS     R0,R0,#+1
        STR      R0,[R1, #+0]
          CFI FunCall usb_request_recovery
        BL       usb_request_recovery
        B.N      ??usb_check_data_2
??usb_check_data_3:
        CMP      R0,#+0
        BEQ.N    ??usb_check_data_2
        LDR.W    R8,??DataTable19_33
        LDRB     R0,[R8, #+0]
        CBZ.N    R0,??usb_check_data_6
        MOVS     R0,#+0
        LDR.N    R1,??DataTable19_32
        STRH     R0,[R1, #+0]
        STRB     R0,[R8, #+0]
        LDR.N    R1,??DataTable19_42
        STRH     R0,[R1, #+0]
        MOVS     R1,#+0
        LDR.N    R0,??DataTable19_36
        STRH     R1,[R0, #+0]
??usb_check_data_6:
        LDR.N    R6,??DataTable19_32
        LDRH     R11,[R6, #+0]
        B.N      ??usb_check_data_7
??usb_check_data_8:
        ADDS     R4,R4,#+1
??usb_check_data_7:
        UXTH     R0,R4
        SUB      R0,R11,R0
        CMP      R0,#+4
        BLT.N    ??usb_check_data_9
        MOVS     R2,#+4
        LDR.N    R1,??DataTable19_38
        LDR.N    R3,??DataTable19_37
        UXTH     R0,R4
        ADD      R0,R3,R0
          CFI FunCall memcmp
        BL       memcmp
        CMP      R0,#+0
        BNE.N    ??usb_check_data_8
??usb_check_data_9:
        UXTH     R0,R4
        CBZ.N    R0,??usb_check_data_10
        SUB      R1,R11,R4
        UXTH     R11,R1
          CFI FunCall usb_rx_consume
        BL       usb_rx_consume
??usb_check_data_10:
        CMP      R11,#+6
        BGE.N    ??usb_check_data_11
        LDR.N    R0,??DataTable19_36
        LDRH     R0,[R0, #+0]
        MOVS     R1,R0
        BEQ.N    ??usb_check_data_2
          CFI FunCall usb_rx_consume
        BL       usb_rx_consume
        B.N      ??usb_check_data_2
??usb_check_data_11:
        LDR.W    R10,??DataTable19_37
        LDRB     R1,[R10, #+4]
        LDRB     R0,[R10, #+5]
        ORR      R1,R1,R0, LSL #+8
        SUBS     R2,R1,#+3
        MOVW     R0,#+2038
        CMP      R2,R0
        BCC.N    ??usb_check_data_12
        MOVS     R0,#+1
        B.N      ??usb_check_data_13
??usb_check_data_12:
        ADD      R4,R1,#+8
        UXTH     R4,R4
        LDR.N    R7,??DataTable19_36
        LDRH     R0,[R7, #+0]
        MOVS     R2,R0
        BEQ.N    ??usb_check_data_14
        CMP      R0,R4
        BGE.N    ??usb_check_data_14
??usb_check_data_13:
          CFI FunCall usb_rx_consume
        BL       usb_rx_consume
        BL       ?Subroutine6
??CrossCallReturnLabel_13:
        B.N      ??usb_check_data_2
??usb_check_data_14:
        CMP      R11,R4
        BGE.N    ??usb_check_data_15
??usb_check_data_2:
        MSR      PRIMASK,R9
        B.N      ??usb_check_data_16
??usb_check_data_15:
        MOVS     R0,#+1
        STRB     R0,[R5, #+0]
        MSR      PRIMASK,R9
        ADD      R0,R10,R4
        LDRB     R2,[R0, #-2]
        LDRB     R0,[R0, #-1]
        ORR      R9,R2,R0, LSL #+8
        ADDS     R1,R1,#+2
        UXTH     R1,R1
        ADD      R0,R10,#+4
          CFI FunCall crc16_calc
        BL       crc16_calc
        CMP      R9,R0
        BNE.N    ??usb_check_data_17
        LDRB     R0,[R8, #+0]
        CBNZ.N   R0,??usb_check_data_18
        MOV      R1,R4
        MOV      R0,R10
          CFI FunCall usb_data_reading
        BL       usb_data_reading
??usb_check_data_18:
        MOV      R1,R4
        B.N      ??usb_check_data_19
??usb_check_data_17:
        BL       ?Subroutine6
??CrossCallReturnLabel_14:
        MOVS     R1,#+4
??usb_check_data_19:
        BL       ?Subroutine3
??CrossCallReturnLabel_2:
        LDRB     R0,[R8, #+0]
        CBZ.N    R0,??usb_check_data_20
        MOVS     R0,#+0
        STRH     R0,[R6, #+0]
        STRB     R0,[R8, #+0]
        BL       ?Subroutine4
??CrossCallReturnLabel_7:
        MOVS     R0,#+0
        STRH     R0,[R7, #+0]
        B.N      ??usb_check_data_21
??usb_check_data_20:
        MOV      R0,R1
          CFI FunCall usb_rx_consume
        BL       usb_rx_consume
??usb_check_data_21:
        MOVS     R0,#+0
        STRB     R0,[R5, #+0]
        MSR      PRIMASK,R4
??usb_check_data_16:
        POP      {R0,R4-R11,PC}
          CFI EndBlock cfiBlock21

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond22 Using cfiCommon0
          CFI Function usb_datareceived
          CFI Conditional ??CrossCallReturnLabel_12
          CFI R4 Frame(CFA, -36)
          CFI R5 Frame(CFA, -32)
          CFI R6 Frame(CFA, -28)
          CFI R7 Frame(CFA, -24)
          CFI R8 Frame(CFA, -20)
          CFI R9 Frame(CFA, -16)
          CFI R10 Frame(CFA, -12)
          CFI R11 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+40
          CFI Block cfiCond23 Using cfiCommon0
          CFI (cfiCond23) Function usb_check_data
          CFI (cfiCond23) Conditional ??CrossCallReturnLabel_13
          CFI (cfiCond23) R4 Frame(CFA, -36)
          CFI (cfiCond23) R5 Frame(CFA, -32)
          CFI (cfiCond23) R6 Frame(CFA, -28)
          CFI (cfiCond23) R7 Frame(CFA, -24)
          CFI (cfiCond23) R8 Frame(CFA, -20)
          CFI (cfiCond23) R9 Frame(CFA, -16)
          CFI (cfiCond23) R10 Frame(CFA, -12)
          CFI (cfiCond23) R11 Frame(CFA, -8)
          CFI (cfiCond23) R14 Frame(CFA, -4)
          CFI (cfiCond23) CFA R13+40
          CFI Block cfiCond24 Using cfiCommon0
          CFI (cfiCond24) Function usb_check_data
          CFI (cfiCond24) Conditional ??CrossCallReturnLabel_14
          CFI (cfiCond24) R4 Frame(CFA, -36)
          CFI (cfiCond24) R5 Frame(CFA, -32)
          CFI (cfiCond24) R6 Frame(CFA, -28)
          CFI (cfiCond24) R7 Frame(CFA, -24)
          CFI (cfiCond24) R8 Frame(CFA, -20)
          CFI (cfiCond24) R9 Frame(CFA, -16)
          CFI (cfiCond24) R10 Frame(CFA, -12)
          CFI (cfiCond24) R11 Frame(CFA, -8)
          CFI (cfiCond24) R14 Frame(CFA, -4)
          CFI (cfiCond24) CFA R13+40
          CFI Block cfiPicker25 Using cfiCommon1
          CFI (cfiPicker25) NoFunction
          CFI (cfiPicker25) Picker
        THUMB
?Subroutine6:
        LDR.N    R1,??DataTable19_45
        LDR      R0,[R1, #+0]
        ADDS     R0,R0,#+1
        STR      R0,[R1, #+0]
        BX       LR
          CFI EndBlock cfiCond22
          CFI EndBlock cfiCond23
          CFI EndBlock cfiCond24
          CFI EndBlock cfiPicker25

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond26 Using cfiCommon0
          CFI Function usb_request_recovery
          CFI Conditional ??CrossCallReturnLabel_0
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
          CFI Block cfiCond27 Using cfiCommon0
          CFI (cfiCond27) Function usb_timer1ms
          CFI (cfiCond27) Conditional ??CrossCallReturnLabel_1
          CFI (cfiCond27) R4 Frame(CFA, -12)
          CFI (cfiCond27) R5 Frame(CFA, -8)
          CFI (cfiCond27) R14 Frame(CFA, -4)
          CFI (cfiCond27) CFA R13+16
          CFI Block cfiCond28 Using cfiCommon0
          CFI (cfiCond28) Function usb_check_data
          CFI (cfiCond28) Conditional ??CrossCallReturnLabel_2
          CFI (cfiCond28) R4 Frame(CFA, -36)
          CFI (cfiCond28) R5 Frame(CFA, -32)
          CFI (cfiCond28) R6 Frame(CFA, -28)
          CFI (cfiCond28) R7 Frame(CFA, -24)
          CFI (cfiCond28) R8 Frame(CFA, -20)
          CFI (cfiCond28) R9 Frame(CFA, -16)
          CFI (cfiCond28) R10 Frame(CFA, -12)
          CFI (cfiCond28) R11 Frame(CFA, -8)
          CFI (cfiCond28) R14 Frame(CFA, -4)
          CFI (cfiCond28) CFA R13+40
          CFI Block cfiCond29 Using cfiCommon0
          CFI (cfiCond29) Function usb_diag_fill
          CFI (cfiCond29) Conditional ??CrossCallReturnLabel_3
          CFI (cfiCond29) R4 Frame(CFA, -12)
          CFI (cfiCond29) R5 Frame(CFA, -8)
          CFI (cfiCond29) R14 Frame(CFA, -4)
          CFI (cfiCond29) CFA R13+16
          CFI Block cfiCond30 Using cfiCommon0
          CFI (cfiCond30) Function usb_diag_fill_regs
          CFI (cfiCond30) Conditional ??CrossCallReturnLabel_4
          CFI (cfiCond30) R4 Frame(CFA, -8)
          CFI (cfiCond30) R14 Frame(CFA, -4)
          CFI (cfiCond30) CFA R13+8
          CFI Block cfiPicker31 Using cfiCommon1
          CFI (cfiPicker31) NoFunction
          CFI (cfiPicker31) Picker
        THUMB
?Subroutine3:
        MRS      R4,PRIMASK
        CPSID    I
        BX       LR
          CFI EndBlock cfiCond26
          CFI EndBlock cfiCond27
          CFI EndBlock cfiCond28
          CFI EndBlock cfiCond29
          CFI EndBlock cfiCond30
          CFI EndBlock cfiPicker31

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock32 Using cfiCommon0
          CFI Function usb_get_state
          CFI NoCalls
        THUMB
usb_get_state:
        LDR.N    R0,??DataTable19_27
        LDRB     R0,[R0, #+0]
        BX       LR
          CFI EndBlock cfiBlock32

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock33 Using cfiCommon0
          CFI Function usb_clear_buffer
          CFI NoCalls
        THUMB
usb_clear_buffer:
        MRS      R1,PRIMASK
        CPSID    I
        LDR.N    R0,??DataTable19_35
        LDRB     R0,[R0, #+0]
        CBZ.N    R0,??usb_clear_buffer_0
        MOVS     R2,#+1
        LDR.N    R0,??DataTable19_33
        STRB     R2,[R0, #+0]
        B.N      ??usb_clear_buffer_1
??usb_clear_buffer_0:
        MOVS     R2,#+0
        LDR.N    R0,??DataTable19_32
        STRH     R2,[R0, #+0]
        LDR.N    R0,??DataTable19_33
        STRB     R2,[R0, #+0]
        LDR.N    R0,??DataTable19_42
        STRH     R2,[R0, #+0]
        LDR.N    R0,??DataTable19_36
        STRH     R2,[R0, #+0]
??usb_clear_buffer_1:
        B.N      ?Subroutine1
          CFI EndBlock cfiBlock33

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock34 Using cfiCommon0
          CFI Function usb_diag_fill
        THUMB
usb_diag_fill:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R5,R0
        BL       ?Subroutine3
??CrossCallReturnLabel_3:
        LDR.N    R0,??DataTable19_46
        LDR      R0,[R0, #+0]
        STR      R0,[R5, #+0]
        LDR.N    R0,??DataTable19_47
        LDR      R0,[R0, #+0]
        STR      R0,[R5, #+4]
        LDR.N    R0,??DataTable19_48
        LDR      R0,[R0, #+0]
        STR      R0,[R5, #+8]
        LDR.N    R0,??DataTable19_30
        LDR      R0,[R0, #+0]
        STR      R0,[R5, #+12]
        LDR.N    R0,??DataTable19_43
        LDR      R0,[R0, #+0]
        STR      R0,[R5, #+16]
        LDR.N    R0,??DataTable19_44
        LDR      R0,[R0, #+0]
        STR      R0,[R5, #+20]
        MOVS     R1,#+0
??usb_diag_fill_0:
        LDR.N    R0,??DataTable19_14
        LDR      R2,[R0, R1, LSL #+2]
        ADD      R0,R5,R1, LSL #+2
        STR      R2,[R0, #+24]
        ADDS     R1,R1,#+1
        CMP      R1,#+7
        BLT.N    ??usb_diag_fill_0
        LDR.N    R0,??DataTable19_16
        LDR      R0,[R0, #+0]
        STR      R0,[R5, #+52]
          CFI FunCall USB_Tx_Pending
        BL       USB_Tx_Pending
        STR      R0,[R5, #+56]
          CFI FunCall USB_Tx_Queued
        BL       USB_Tx_Queued
        STR      R0,[R5, #+60]
        LDR.N    R0,??DataTable19_10
        LDRH     R0,[R0, #+0]
        STR      R0,[R5, #+64]
        LDR.N    R0,??DataTable19_17
        LDRH     R0,[R0, #+0]
        STR      R0,[R5, #+68]
        LDR.N    R0,??DataTable19_13
        LDRH     R0,[R0, #+0]
        STR      R0,[R5, #+72]
        LDR.N    R0,??DataTable19_40
        LDR      R0,[R0, #+0]
        STR      R0,[R5, #+76]
        LDR.N    R0,??DataTable19_45
        LDR      R0,[R0, #+0]
        STR      R0,[R5, #+80]
        LDR.N    R0,??DataTable19_41
        LDR      R0,[R0, #+0]
        STR      R0,[R5, #+84]
        LDR.N    R0,??DataTable19_34
        LDR      R0,[R0, #+0]
        STR      R0,[R5, #+88]
          CFI EndBlock cfiBlock34
        REQUIRE ?Subroutine2

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock35 Using cfiCommon0
          CFI NoFunction
          CFI CFA R13+16
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
        THUMB
?Subroutine2:
        MSR      PRIMASK,R4
        POP      {R0,R4,R5,PC}
          CFI EndBlock cfiBlock35

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock36 Using cfiCommon0
          CFI Function usb_diag_fill_regs
        THUMB
usb_diag_fill_regs:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        BL       ?Subroutine3
??CrossCallReturnLabel_4:
        MOVS     R1,#+0
??usb_diag_fill_regs_0:
        LDR.N    R2,??DataTable19_18
        LDR      R2,[R2, R1, LSL #+2]
        STR      R2,[R0, R1, LSL #+2]
        ADDS     R1,R1,#+1
        CMP      R1,#+9
        BCC.N    ??usb_diag_fill_regs_0
        ADDS     R0,R0,#+36
          CFI FunCall usb_regs_read
        BL       usb_regs_read
        MSR      PRIMASK,R4
        POP      {R4,PC}
          CFI EndBlock cfiBlock36

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock37 Using cfiCommon0
          CFI Function usb_diag_fill_errors
          CFI NoCalls
        THUMB
usb_diag_fill_errors:
        MRS      R1,PRIMASK
        CPSID    I
        LDR.N    R2,??DataTable19_12
        LDR      R2,[R2, #+0]
        STR      R2,[R0, #+0]
        LDR.N    R2,??DataTable19_19
        LDR      R2,[R2, #+0]
        STR      R2,[R0, #+4]
          CFI EndBlock cfiBlock37
        REQUIRE ?Subroutine1

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock38 Using cfiCommon0
          CFI NoFunction
        THUMB
?Subroutine1:
        MSR      PRIMASK,R1
        BX       LR
          CFI EndBlock cfiBlock38

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock39 Using cfiCommon0
          CFI Function usb_diag_fill_addr
          CFI NoCalls
        THUMB
usb_diag_fill_addr:
        MRS      R1,PRIMASK
        CPSID    I
        LDR.N    R3,??DataTable19_20
        LDR      R2,[R3, #+0]
        STR      R2,[R0, #+0]
        LDR      R2,[R3, #+4]
        STR      R2,[R0, #+4]
        LDR.N    R2,??DataTable19_21
        LDR      R2,[R2, #+0]
        UXTH     R2,R2
        STR      R2,[R0, #+8]
        LDR.N    R2,??DataTable19_22
        LDR      R2,[R2, #+0]
        UXTH     R2,R2
        STR      R2,[R0, #+12]
        B.N      ?Subroutine1
          CFI EndBlock cfiBlock39

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19:
        DATA32
        DC32     0x40005c04

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_1:
        DATA32
        DC32     0x40005c0c

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_2:
        DATA32
        DC32     0x40005c50

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_3:
        DATA32
        DC32     0x40006014

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_4:
        DATA32
        DC32     0x40005c44

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_5:
        DATA32
        DC32     0x40005c40

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_6:
        DATA32
        DC32     0x40005c48

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_7:
        DATA32
        DC32     USB_Tx_State

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_8:
        DATA32
        DC32     usb_in_armed

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_9:
        DATA32
        DC32     usb_in_taken

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_10:
        DATA32
        DC32     usb_in_ago

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_11:
        DATA32
        DC32     usb_hung_ms

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_12:
        DATA32
        DC32     usb_bus_errors

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_13:
        DATA32
        DC32     usb_sof_ago

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_14:
        DATA32
        DC32     usb_last_recovery

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_15:
        DATA32
        DC32     usb_recovery_reason

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_16:
        DATA32
        DC32     bDeviceState

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_17:
        DATA32
        DC32     usb_out_ago

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_18:
        DATA32
        DC32     usb_last_regs

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_19:
        DATA32
        DC32     usb_last_bus_errors

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_20:
        DATA32
        DC32     usb_last_addr

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_21:
        DATA32
        DC32     0x40005c4c

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_22:
        DATA32
        DC32     0x40005c00

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_23:
        DATA32
        DC32     usb_unconf_armed

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_24:
        DATA32
        DC32     usb_hung_armed

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_25:
        DATA32
        DC32     usb_unconf_ms

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_26:
        DATA32
        DC32     0x40010814

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_27:
        DATA32
        DC32     usb_enabled

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_28:
        DATA32
        DC32     usb_recovery_ms

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_29:
        DATA32
        DC32     usb_recovery_pending

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_30:
        DATA32
        DC32     usb_recovery_count

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_31:
        DATA32
        DC32     0x40010810

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_32:
        DATA32
        DC32     usb_buffer_count

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_33:
        DATA32
        DC32     usb_rx_discard

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_34:
        DATA32
        DC32     usb_out_unconfigured

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_35:
        DATA32
        DC32     usb_rx_processing

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_36:
        DATA32
        DC32     usb_rx_expired

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_37:
        DATA32
        DC32     usb_buffer

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_38:
        DATA32
        DC32     ?_0

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_39:
        DATA32
        DC32     0x40010c08

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_40:
        DATA32
        DC32     usb_tx_errors

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_41:
        DATA32
        DC32     usb_debug_dropped

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_42:
        DATA32
        DC32     usb_rx_silence

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_43:
        DATA32
        DC32     usb_watch_in_recoveries

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_44:
        DATA32
        DC32     usb_watch_unconf_recoveries

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_45:
        DATA32
        DC32     usb_rx_errors

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_46:
        DATA32
        DC32     usb_esof_forced

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_47:
        DATA32
        DC32     usb_esof_timeouts

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_48:
        DATA32
        DC32     usb_bus_resets

        SECTION `.text`:CODE:REORDER:NOROOT(1)
          CFI Block cfiBlock40 Using cfiCommon0
          CFI Function usb_data_reading
          CFI NoCalls
        THUMB
usb_data_reading:
        BX       LR
          CFI EndBlock cfiBlock40

        SECTION `.iar_vfe_header`:DATA:NOALLOC:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
        DC32 0

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_0:
        DATA8
        DC8 "SLDN"
        DATA
        DS8 3

        END
