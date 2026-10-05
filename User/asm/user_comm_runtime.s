; Cortex-M3 / IAR. Executable production module.
; Reference: Tests/asm/reference/User/user_comm.c
        PUBLIC handlers

        RTMODEL "__CPP_Runtime", "1"
        RTMODEL "__SystemLibrary", "DLib"
        RTMODEL "__dlib_file_descriptor", "0"
        RTMODEL "__dlib_full_locale_support", "0"
        RTMODEL "__dlib_version", "6"
        AAPCS BASE,INTERWORK
        PRESERVE8
        REQUIRE8

        #define SHT_PROGBITS 0x1

        EXTERN comm_balance_init
        EXTERN comm_balance_poll
        EXTERN comm_call_init
        EXTERN comm_call_poll
        EXTERN comm_channel_receive
        EXTERN comm_channel_rx_capacity
        EXTERN comm_info_low_signal
        EXTERN comm_info_poll
        EXTERN comm_recover_init
        EXTERN comm_recover_no_internet
        EXTERN comm_recover_poll
        EXTERN comm_server_init
        EXTERN comm_server_link
        EXTERN comm_server_no_internet
        EXTERN comm_server_poll
        EXTERN comm_sms_init
        EXTERN comm_sms_poll
        EXTERN comm_sms_send
        EXTERN log_addSystemEvent
        EXTERN modem_slm336_start
        EXTERN server_rx_begin_pass
        EXTERN server_rx_process
        EXTERN strncpy
        EXTERN user_comm_balance
        EXTERN user_comm_busy
        EXTERN user_comm_link
        EXTERN user_comm_local_time
        EXTERN user_comm_low_signal
        EXTERN user_comm_rx
        EXTERN user_comm_rx_capacity
        EXTERN user_comm_rx_done
        EXTERN user_comm_session
        EXTERN user_comm_ussd
        EXTERN user_comm_volte
        EXTERN user_incomingcall
        EXTERN user_send_registrationData
        EXTERN user_send_statusPack
        EXTERN user_smsreceived
        EXTERN user_storage_flash_ready

        PUBLIC user_comm_poll
        PUBLIC user_comm_sms
        PUBLIC user_comm_start

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
          CFI Function user_comm_registration
        THUMB
user_comm_registration:
        MOVS     R0,#+1
          CFI FunCall user_send_registrationData
        B.W      user_send_registrationData
          CFI EndBlock cfiBlock0

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock1 Using cfiCommon0
          CFI Function user_comm_status
        THUMB
user_comm_status:
        MOVS     R0,#+1
          CFI FunCall user_send_statusPack
        B.W      user_send_statusPack
          CFI EndBlock cfiBlock1

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock2 Using cfiCommon0
          CFI Function user_comm_no_traffic
        THUMB
user_comm_no_traffic:
        MOVS     R0,#+13
          CFI FunCall log_addSystemEvent
        B.W      log_addSystemEvent
          CFI EndBlock cfiBlock2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
handlers:
        DATA32
        DC32 user_comm_registration, user_comm_status, user_comm_session
        DC32 server_rx_process, user_comm_no_traffic

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock3 Using cfiCommon0
          CFI Function user_comm_reset_session
        THUMB
user_comm_reset_session:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
        LDR.N    R0,??DataTable2
          CFI FunCall comm_server_init
        BL       comm_server_init
        LDR.N    R1,??DataTable2_1
        LDR.N    R0,??DataTable2_2
        POP      {R2,LR}
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall comm_channel_receive
        B.W      comm_channel_receive
          CFI EndBlock cfiBlock3

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock4 Using cfiCommon0
          CFI Function user_comm_call
        THUMB
user_comm_call:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
        SUB      SP,SP,#+24
          CFI CFA R13+32
        MOV      R1,R0
        MOVS     R2,#+23
        MOV      R0,SP
          CFI FunCall strncpy
        BL       strncpy
        MOVS     R0,#+0
        STRB     R0,[SP, #+23]
        MOVS     R1,#+1
        MOV      R0,SP
          CFI FunCall user_incomingcall
        BL       user_incomingcall
        ADD      SP,SP,#+28
          CFI CFA R13+4
        POP      {PC}
          CFI EndBlock cfiBlock4

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock5 Using cfiCommon0
          CFI Function user_comm_start
        THUMB
user_comm_start:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        MOV      R4,R2
          CFI FunCall modem_slm336_start
        BL       modem_slm336_start
        CBZ.N    R0,??user_comm_start_0
          CFI FunCall user_comm_reset_session
        BL       user_comm_reset_session
        LDR.N    R0,??DataTable2_3
          CFI FunCall comm_server_no_internet
        BL       comm_server_no_internet
        LDR.N    R0,??DataTable2_4
          CFI FunCall comm_server_link
        BL       comm_server_link
        LDR.N    R0,??DataTable2_5
          CFI FunCall comm_info_low_signal
        BL       comm_info_low_signal
        MOV      R1,R4
        LDR.N    R0,??DataTable2_6
          CFI FunCall comm_recover_init
        BL       comm_recover_init
        LDR.N    R1,??DataTable2_7
        LDR.N    R0,??DataTable2_8
          CFI FunCall comm_call_init
        BL       comm_call_init
        LDR.N    R0,??DataTable2_9
          CFI FunCall comm_sms_init
        BL       comm_sms_init
        LDR.N    R1,??DataTable2_10
        LDR.N    R0,??DataTable2_11
          CFI FunCall comm_balance_init
        BL       comm_balance_init
        MOVS     R0,#+1
??user_comm_start_0:
        POP      {R4,PC}
          CFI EndBlock cfiBlock5

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable2:
        DATA32
        DC32     handlers

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable2_1:
        DATA32
        DC32     user_comm_rx_done

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable2_2:
        DATA32
        DC32     user_comm_rx

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable2_3:
        DATA32
        DC32     comm_recover_no_internet

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable2_4:
        DATA32
        DC32     user_comm_link

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable2_5:
        DATA32
        DC32     user_comm_low_signal

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable2_6:
        DATA32
        DC32     `user_comm_start::hooks`

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable2_7:
        DATA32
        DC32     user_comm_volte

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable2_8:
        DATA32
        DC32     user_comm_call

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable2_9:
        DATA32
        DC32     user_smsreceived

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable2_10:
        DATA32
        DC32     user_comm_balance

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable2_11:
        DATA32
        DC32     user_comm_ussd

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
`user_comm_start::hooks`:
        DATA32
        DC32 user_comm_reset_session, user_comm_local_time, user_comm_busy

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock6 Using cfiCommon0
          CFI Function user_comm_sms
          CFI FunCall comm_sms_send
        THUMB
user_comm_sms:
        B.W      comm_sms_send
          CFI EndBlock cfiBlock6

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock7 Using cfiCommon0
          CFI Function user_comm_poll
        THUMB
user_comm_poll:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        MOV      R4,R0
          CFI FunCall server_rx_begin_pass
        BL       server_rx_begin_pass
          CFI FunCall user_comm_rx_capacity
        BL       user_comm_rx_capacity
          CFI FunCall comm_channel_rx_capacity
        BL       comm_channel_rx_capacity
        MOV      R0,R4
          CFI FunCall comm_server_poll
        BL       comm_server_poll
        MOV      R0,R4
          CFI FunCall comm_recover_poll
        BL       comm_recover_poll
        MOV      R0,R4
          CFI FunCall comm_info_poll
        BL       comm_info_poll
          CFI FunCall user_storage_flash_ready
        BL       user_storage_flash_ready
        CBZ.N    R0,??user_comm_poll_0
        MOV      R0,R4
          CFI FunCall comm_call_poll
        BL       comm_call_poll
??user_comm_poll_0:
          CFI FunCall user_storage_flash_ready
        BL       user_storage_flash_ready
        CBZ.N    R0,??user_comm_poll_1
        MOV      R0,R4
          CFI FunCall comm_sms_poll
        BL       comm_sms_poll
??user_comm_poll_1:
        MOV      R0,R4
        POP      {R4,LR}
          CFI R4 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall comm_balance_poll
        B.W      comm_balance_poll
          CFI EndBlock cfiBlock7

        SECTION `.iar_vfe_header`:DATA:NOALLOC:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
        DC32 0

        END
