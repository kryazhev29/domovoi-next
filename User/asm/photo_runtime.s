#include "../work_slots.h"
        EXTERN work_acquire
        EXTERN work_release
; Cortex-M3 / IAR. Executable production module.
; Reference: Tests/asm/reference/User/photo.c
        PUBLIC photo
        PUBLIC photo_erased_to
        PUBLIC photo_pending_since
        PUBLIC pr

        RTMODEL "__CPP_Runtime", "1"
        RTMODEL "__SystemLibrary", "DLib"
        RTMODEL "__dlib_file_descriptor", "0"
        RTMODEL "__dlib_full_locale_support", "0"
        RTMODEL "__dlib_version", "6"
        AAPCS BASE,INTERWORK
        #include "flash_owner.h"
        EXTERN flash_claim
        EXTERN flash_release
        EXTERN reply_send
        PRESERVE8
        REQUIRE8

        #define SHT_PROGBITS 0x1

        EXTERN __aeabi_memclr
        EXTERN __aeabi_memcpy
        EXTERN __aeabi_memcpy4
        EXTERN comm_channel_ready
        EXTERN comm_tx_acquire
        EXTERN comm_tx_busy
        EXTERN comm_tx_commit
        EXTERN comm_tx_release
        EXTERN config
        EXTERN crc16_calcInit
        EXTERN crc32_update
        EXTERN device
        EXTERN fast_all_ff
        EXTERN log_addPhotoEvent
        EXTERN mem
        EXTERN mem_read
        EXTERN memcmp
        EXTERN port_buffer
        EXTERN sjr
        EXTERN timers_getMillis
        EXTERN uart_clear
        EXTERN uart_getstate
        EXTERN uart_init
        EXTERN uart_isReceiving
        EXTERN uart_received
        EXTERN uart_send
        EXTERN uart_setRecTimeout
        EXTERN usb_bus_resets
        EXTERN usb_try_send
        EXTERN user_debug
        EXTERN user_send_answer_comand
        EXTERN user_storage_background_ready
        EXTERN user_storage_command_ready
        EXTERN idcrc_io_poll
        EXTERN idcrc_io_start
        EXTERN w25_read_data
        EXTERN wdt_reload

        PUBLIC photo_cleanup_active
        PUBLIC photo_confirmed_offset
        PUBLIC photo_connection_lost
        PUBLIC photo_poll
        PUBLIC photo_recover
        PUBLIC photo_request
        PUBLIC photo_request_answer
        PUBLIC photo_start
        PUBLIC photo_storage_busy
        PUBLIC photo_storage_cancel
        PUBLIC photo_storage_end
        PUBLIC photo_storage_poll
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

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
pr:
        DS8 120

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock0 Using cfiCommon0
          CFI Function photo_storage_busy
          CFI NoCalls
        THUMB
photo_storage_busy:
        LDR.W    R0,??DataTable14
        LDRB     R0,[R0, #+106]
        CBZ.N    R0,??photo_storage_busy_0
        MOVS     R0,#+1
??photo_storage_busy_0:
        BX       LR
          CFI EndBlock cfiBlock0

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock1 Using cfiCommon0
          CFI Function photo_storage_cancel
        THUMB
photo_storage_cancel:
        LDR.W    R0,??DataTable14
        MOVS     R1,#+1
        STRB     R1,[R0, #+111]
        STRB     R1,[R0, #+112]
        LDRB     R1,[R0, #+110]
        CMP      R1,#+5
        ITT      EQ
        MOVEQ    R1,#+0
        STRBEQ   R1,[R0, #+114]
        LDR.W    R1,??DataTable14_1
        LDR      R0,[R1, #+0]
        CBZ.N    R0,??photo_storage_cancel_0
        MOVS     R2,#+0
        STRB     R2,[R0, #+38]
        LDR      R0,[R1, #+0]
        LDRB     R0,[R0, #+34]
        CMP      R0,#+0
          CFI FunCall photo_stop
        IT       NE
        BNE.W    photo_stop
??photo_storage_cancel_0:
        BX       LR
          CFI EndBlock cfiBlock1

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock2 Using cfiCommon0
          CFI Function pr_release
        THUMB
pr_release:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        LDR.W    R4,??DataTable14
        LDR      R0,[R4, #+0]
        MOVS R1,#WORK_PHOTO_PACKET
          CFI FunCall work_release
        BL       work_release
        MOVS     R0,#+0
        STR      R0,[R4, #+0]
        STRB     R0,[R4, #+113]
        STRB     R0,[R4, #+108]
        STRB     R0,[R4, #+106]
        POP      {R4,LR}
          CFI R4 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
        MOVS     R0,#+FO_PHOTO
          CFI FunCall flash_release
        B.W      flash_release
          CFI EndBlock cfiBlock2

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiPrOwner Using cfiCommon0
          CFI Function pr_owner
        THUMB
; R0 = the state just stored in pr[106]: nonzero holds the Flash for the
; photo, zero frees it. Every 0 -> busy transition runs with no other owner
; (pr_poll, pr_request and pr_start check user_storage_command_ready).
pr_owner:
        CMP      R0,#+0
        MOV      R0,#+FO_PHOTO
          CFI FunCall flash_release
        BEQ.W    flash_release
          CFI FunCall flash_claim
        B.W      flash_claim
          CFI EndBlock cfiPrOwner

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock3 Using cfiCommon0
          CFI Function pr_fail
        THUMB
pr_fail:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        LDR.W    R4,??DataTable14
        MOVS     R0,#+1
        STRB     R0,[R4, #+116]
        LDR.W    R5,??DataTable14_1
        LDR      R0,[R5, #+0]
        CBZ.N    R0,??CrossCallReturnLabel_21
        LDRB     R0,[R0, #+34]
        CBZ.N    R0,??pr_fail_0
          CFI FunCall photo_stop
        BL       photo_stop
??pr_fail_0:
        MOVS     R1,#+0
        LDR      R0,[R5, #+0]
        STRB     R1,[R0, #+38]
        LDRB     R0,[R4, #+110]
        CMP      R0,#+4
        IT       NE
        CMPNE    R0,#+5
        BNE.N    ??CrossCallReturnLabel_21
        LDRB     R0,[R4, #+111]
        CBNZ.N   R0,??CrossCallReturnLabel_21
        LDR      R0,[R5, #+0]
        LDRB     R3,[R0, #+39]
        LDRB     R2,[R0, #+50]
        MOVS     R1,#+2
        BL       ??Subroutine7_0
??CrossCallReturnLabel_21:
        MOVS     R0,#+0
        STRB     R0,[R4, #+114]
        POP      {R0,R4,R5,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall pr_release
        B.N      pr_release
          CFI EndBlock cfiBlock3

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock4 Using cfiCommon0
          CFI Function pr_write
        THUMB
pr_write:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        MOV      R4,R0
        MOV      R5,R2
        MOV      R6,R3
        MOVS     R0,R5
        BEQ.N    ??pr_write_0
        CMP      R5,#+65
        BLT.N    ??pr_write_1
??pr_write_0:
        POP      {R4-R6,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall pr_fail
        B.N      pr_fail
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
??pr_write_1:
        LDR.W    R0,??DataTable14_2
        ADD      R2,R0,#+61
        CMP      R1,R2
        BEQ.N    ??pr_write_2
        MOV      R2,R5
        ADDS     R0,R0,#+61
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
??pr_write_2:
        LDR.W    R0,??DataTable14
        STR      R4,[R0, #+4]
        STRH     R5,[R0, #+74]
        MOVS     R1,#+0
        STRH     R1,[R0, #+76]
        STRB     R1,[R0, #+109]
        STRB     R6,[R0, #+107]
        MOVS     R1,#+1
        STRB     R1,[R0, #+106]
        POP      {R4-R6,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
        MOVS     R0,#+FO_PHOTO
          CFI FunCall flash_claim
        B.W      flash_claim
          CFI EndBlock cfiBlock4

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock5 Using cfiCommon0
          CFI Function pr_erase
        THUMB
pr_erase:
        LDR.W    R2,??DataTable14
        STR      R0,[R2, #+4]
        MOVS     R0,#+0
        STRH     R0,[R2, #+76]
        STRH     R0,[R2, #+74]
        MOVS     R0,#+1
        STRB     R0,[R2, #+109]
        STRB     R1,[R2, #+107]
        STRB     R0,[R2, #+106]
        MOVS     R0,#+FO_PHOTO
          CFI FunCall flash_claim
        B.W      flash_claim
          CFI EndBlock cfiBlock5

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock6 Using cfiCommon0
          CFI Function pr_scan_begin
        THUMB
pr_scan_begin:
        LDR.W    R1,??DataTable14
        STRB     R0,[R1, #+110]
        LDR.W    R0,??DataTable14_3
        LDR      R0,[R0, #+72]
        STR      R0,[R1, #+8]
        MOVS     R0,#+0
        STRH     R0,[R1, #+88]
        STR      R0,[R1, #+56]
        STR      R0,[R1, #+48]
        STRH     R0,[R1, #+92]
        MOVS     R0,#+3
        STRB     R0,[R1, #+106]
        MOVS     R0,#+FO_PHOTO
          CFI FunCall flash_claim
        B.W      flash_claim
          CFI EndBlock cfiBlock6

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock7 Using cfiCommon0
          CFI Function pr_scan_finish
        THUMB
pr_scan_finish:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        LDR.W    R1,??DataTable14
        LDRB     R0,[R1, #+110]
        CMP      R0,#+6
        BNE.N    ??pr_scan_finish_0
        MOVS     R0,#+0
        STRH     R0,[R1, #+98]
        MOVS     R0,#+39
        B.N      ??pr_scan_finish_1
??pr_scan_finish_0:
        LDR.W    R2,??DataTable14_4
        LDR      R0,[R1, #+8]
        STR      R0,[R2, #+157]
        LDRH     R0,[R1, #+88]
        STRH     R0,[R2, #+155]
        CMP      R0,#+0
        ITE      NE
        MOVNE    R4,#+8192
        MOVEQ    R4,#+0
        LDRH     R3,[R2, #+129]
        MOVW     R0,#+57343
        ANDS     R3,R0,R3
        ORRS     R4,R4,R3
        STRH     R4,[R2, #+129]
        LDR      R0,[R1, #+56]
        STR      R0,[R1, #+52]
        MOVS     R0,#+1
        STRB     R0,[R1, #+114]
        LDRB     R0,[R1, #+110]
        CMP      R0,#+1
        IT       EQ
        MOVEQ    R0,#+7
        BEQ.N    ??pr_scan_finish_1
        CMP      R0,#+4
        BNE.N    ??pr_scan_finish_2
        LDR      R0,[R1, #+60]
        CMN      R0,#+1
        BEQ.N    ??pr_scan_finish_3
        LDR.W    R2,??DataTable14_1
        LDR      R0,[R2, #+0]
        CBZ.N    R0,??CrossCallReturnLabel_23
        LDRB     R0,[R1, #+111]
        CBNZ.N   R0,??CrossCallReturnLabel_23
        LDR      R0,[R2, #+0]
        LDRB     R3,[R0, #+39]
        LDRB     R2,[R0, #+50]
        BL       ?Subroutine7
??CrossCallReturnLabel_23:
        POP      {R4,LR}
          CFI R4 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall pr_release
        B.N      pr_release
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
??pr_scan_finish_2:
        CMP      R0,#+5
        BNE.N    ??pr_scan_finish_4
??pr_scan_finish_3:
        MOVS     R0,#+38
        B.N      ??pr_scan_finish_1
??pr_scan_finish_4:
        MOVS     R0,#+0
??pr_scan_finish_1:
        STRB     R0,[R1, #+106]
        POP      {R4,LR}
          CFI R4 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall pr_owner
        B.W      pr_owner
          CFI EndBlock cfiBlock7

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock8 Using cfiCommon0
          CFI Function pr_start
        THUMB
pr_start:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        MOV      R4,R0
        MOVS     R1,#+0
        MOVS     R3,#+0
        LDR.W    R2,??DataTable14_5
??pr_start_0:
        ADD      R0,R2,R3, LSL #+1
        LDRB     R0,[R0, #+180]
        CMP      R0,#+3
        ITTTT    EQ
        ADDEQ    R0,R2,R3, LSL #+1
        LDRBEQ   R5,[R0, #+179]
        CMPEQ    R5,R4
        MOVEQ    R1,#+1
        ADDS     R3,R3,#+1
        CMP      R3,#+8
        BLT.N    ??pr_start_0
        CBZ.N    R1,??pr_start_1
        LDRB     R0,[R2, #+172]
        CMP      R0,#+1
        BEQ.N    ??pr_start_2
??pr_start_1:
        MOVS     R0,#+1
        POP      {R4-R6,PC}
??pr_start_2:
          CFI FunCall user_storage_command_ready
        BL       user_storage_command_ready
        CMP      R0,#+0
        ITTT     NE
        LDRNE.W  R5,??DataTable14
        LDRBNE   R0,[R5, #+114]
        CMPNE    R0,#+0
        BEQ.N    ??pr_start_3
        LDRB     R0,[R5, #+106]
        CMP      R0,#+0
        ITT      EQ
        LDRBEQ   R0,[R5, #+116]
        CMPEQ    R0,#+0
        BNE.N    ??pr_start_3
        LDRB     R0,[R5, #+115]
        CMP      R0,#+0
        ITTTT    EQ
        LDREQ.W  R6,??DataTable14_4
        LDRHEQ   R0,[R6, #+129]
        UBFXEQ   R0,R0,#+11,#+1
        CMPEQ    R0,#+0
        BNE.N    ??pr_start_3
          CFI FunCall photo_alloc
        BL       photo_alloc
        CBZ.N    R0,??pr_start_3
        LDR.W    R0,??DataTable14_1
        LDR      R0,[R0, #+0]
        LDRB     R0,[R0, #+34]
        CBNZ.N   R0,??pr_start_3
        LDRH     R0,[R6, #+155]
        CBNZ.N   R0,??pr_start_4
        LDR.W    R0,??DataTable14_3
        LDR      R1,[R0, #+72]
        LDR      R0,[R6, #+157]
        CMP      R1,R0
        BCC.N    ??pr_start_3
??pr_start_4:
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        LDR      R1,[R6, #+157]
        CMP      R0,R1
        BCC.N    ??pr_start_3
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        LDR      R1,[R6, #+157]
        SUBS     R0,R0,R1
        MOVW     R1,#+1034
        CMP      R0,R1
        BCC.N    ??pr_start_3
        LDR      R0,[R5, #+52]
        CMN      R0,#+2
        ITT      NE
        LDRNE    R0,[R6, #+47]
        CMNNE    R0,#+1
        BNE.N    ??pr_start_5
??pr_start_3:
        MOVS     R0,#+2
        POP      {R4-R6,PC}
??pr_start_5:
        STRB     R4,[R5, #+117]
        MOVS     R0,#+0
        STRB     R0,[R5, #+113]
        STRB     R0,[R5, #+111]
        STRB     R0,[R5, #+112]
        MOVS     R0,#+1
          CFI FunCall pr_scan_begin
        BL       pr_scan_begin
        MOVS     R0,#+0
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock8

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock9 Using cfiCommon0
          CFI Function pr_prepare
        THUMB
pr_prepare:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        MOV      R4,R0
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        CBZ.N    R4,??pr_prepare_0
        CMP      R4,#+65536
        BCS.N    ??pr_prepare_0
        LDR.N    R2,??DataTable14_1
        LDR      R1,[R2, #+0]
        LDR      R1,[R1, #+4]
        CMP      R0,R1
        BCC.N    ??pr_prepare_0
        LDR      R1,[R2, #+0]
        LDR      R3,[R1, #+4]
        SUBS     R3,R0,R3
        MOVW     R1,#+1033
        CMP      R3,R1
        BCC.N    ??pr_prepare_0
        LDR      R1,[R2, #+0]
        LDR      R1,[R1, #+4]
        SUBS     R0,R0,R1
        SUBW     R0,R0,#+1033
        CMP      R0,R4
        BCS.N    ??pr_prepare_1
??pr_prepare_0:
        MOVS     R0,#+0
        POP      {R4,PC}
??pr_prepare_1:
        LDR      R0,[R2, #+0]
        STRH     R4,[R0, #+24]
        MOVS     R1,#+7
        LDR      R0,[R2, #+0]
        STRB     R1,[R0, #+34]
        LDR.N    R1,??DataTable14
        MOVS     R0,#+2
        STRB     R0,[R1, #+110]
        LDR      R0,[R2, #+0]
        LDR      R0,[R0, #+4]
        UBFX     R0,R0,#+0,#+12
        STRH     R0,[R1, #+80]
        MOVS     R0,#+0
        STRH     R0,[R1, #+72]
        LDR      R0,[R2, #+0]
        LDR      R0,[R0, #+4]
        STR      R0,[R1, #+8]
        LDR      R0,[R2, #+0]
        LDR      R0,[R0, #+4]
        LSRS     R0,R0,#+12
        LSLS     R0,R0,#+12
        ADD      R0,R0,#+4096
        STR      R0,[R1, #+12]
        MOVS     R0,#+8
        STRB     R0,[R1, #+106]
        LDRH     R0,[R1, #+80]
        CBNZ.N   R0,??pr_prepare_2
        LDR      R0,[R2, #+0]
        LDR      R2,[R0, #+4]
        LDR.N    R0,??DataTable14_6
        STR      R2,[R0, #+0]
        MOVS     R0,#+22
        STRB     R0,[R1, #+106]
??pr_prepare_2:
        MOVS     R0,#+FO_PHOTO
          CFI FunCall flash_claim
        BL       flash_claim
        POP      {R4,PC}
          CFI EndBlock cfiBlock9

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock10 Using cfiCommon0
          CFI Function pr_data_ready
        THUMB
pr_data_ready:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        LDR.N    R0,??DataTable14_1
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
        BNE.N    ??pr_data_ready_0
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        SUBS     R0,R0,R4
        CMP      R0,#+8
        IT       CS
        ADDCS    R4,R4,#+8
??pr_data_ready_0:
        LDR.N    R5,??DataTable14_6
        LDR      R0,[R5, #+0]
        CMP      R0,R4
        BCC.N    ??pr_data_ready_1
        MOVS     R0,#+1
        POP      {R1,R4,R5,PC}
??pr_data_ready_1:
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        LDR      R1,[R5, #+0]
        CMP      R1,R0
        BCC.N    ??pr_data_ready_2
          CFI FunCall photo_stop
        BL       photo_stop
        B.N      ??pr_data_ready_3
??pr_data_ready_2:
        MOVS     R1,#+3
        LDR.N    R0,??DataTable14
        STRB     R1,[R0, #+110]
        MOVS     R1,#+23
        LDR      R0,[R5, #+0]
          CFI FunCall pr_erase
        BL       pr_erase
??pr_data_ready_3:
        MOVS     R0,#+0
        POP      {R1,R4,R5,PC}
          CFI EndBlock cfiBlock10

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock11 Using cfiCommon0
          CFI Function pr_frame
        THUMB
pr_frame:
        MOV      R2,R1
        SUBS     R1,R1,#+6
        CMP      R1,#+59
        BCC.N    ??pr_frame_0
        BX       LR
??pr_frame_0:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        LDR.N    R3,??DataTable14
        LDRB     R1,[R3, #+106]
        CMP      R1,#+0
        BNE.N    ??pr_frame_1
        LDRB     R5,[R0, #+0]
        LDRB     R1,[R0, #+1]
        ORR      R5,R5,R1, LSL #+8
        LDRB     R1,[R0, #+2]
        LDRB     R4,[R0, #+3]
        ORR      R1,R1,R4, LSL #+8
        MOVS     R6,#+0
        MOVS     R7,#+0
        B.N      ??pr_frame_2
??pr_frame_3:
        UXTH     R4,R7
        LDRB     R4,[R0, R4]
        ADDS     R6,R4,R6
        ADDS     R7,R7,#+1
??pr_frame_2:
        UXTH     R12,R7
        SUBS     R4,R2,#+2
        CMP      R12,R4
        BLT.N    ??pr_frame_3
        LDR.N    R4,??DataTable14_1
        LDR      R7,[R4, #+0]
        LDRH     R7,[R7, #+30]
        CMP      R5,R7
        BNE.N    ??pr_frame_4
        CBZ.N    R1,??pr_frame_4
        CMP      R1,#+59
        BGE.N    ??pr_frame_4
        SUBS     R5,R2,#+6
        CMP      R5,R1
        ITTTT    EQ
        ADDEQ    R5,R0,R2
        LDRBEQ   R5,[R5, #-2]
        UXTBEQ   R6,R6
        CMPEQ    R5,R6
        BNE.N    ??pr_frame_4
        ADD      R2,R0,R2
        LDRB     R2,[R2, #-1]
        CBNZ.N   R2,??pr_frame_4
        LDR      R2,[R4, #+0]
        LDRH     R5,[R2, #+24]
        LDRH     R2,[R2, #+26]
        SUBS     R5,R5,R2
        CMP      R5,R1
        BGE.N    ??pr_frame_5
??pr_frame_4:
        MOVS     R0,#+0
        LDR      R1,[R4, #+0]
        STRH     R0,[R1, #+32]
        POP      {R0,R4-R7,PC}
??pr_frame_5:
        MOVS     R2,#+3
        STRB     R2,[R3, #+110]
        STRH     R1,[R3, #+104]
        MOVS     R3,#+24
        MOV      R2,R1
        ADDS     R1,R0,#+4
        LDR      R0,[R4, #+0]
        LDR      R0,[R0, #+4]
        LDR      R4,[R4, #+0]
        LDRH     R4,[R4, #+26]
        ADDS     R0,R4,R0
        ADDW     R0,R0,#+1033
        ADD      SP,SP,#+4
          CFI CFA R13+20
        POP      {R4-R7,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R7 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall pr_write
        B.N      pr_write
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
??pr_frame_1:
        POP      {R0,R4-R7,PC}
          CFI EndBlock cfiBlock11

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock12 Using cfiCommon0
          CFI Function pr_request
        THUMB
pr_request:
        PUSH     {R4-R8,LR}
          CFI R14 Frame(CFA, -4)
          CFI R8 Frame(CFA, -8)
          CFI R7 Frame(CFA, -12)
          CFI R6 Frame(CFA, -16)
          CFI R5 Frame(CFA, -20)
          CFI R4 Frame(CFA, -24)
          CFI CFA R13+24
        MOV      R4,R0
        MOV      R8,R1
        MOV      R5,R2
        MOVS     R6,R3
        ITT      NE
        CMPNE    R6,#+6
        MOVNE    R0,#+1
        BNE.N    ??pr_request_0
          CFI FunCall user_storage_command_ready
        BL       user_storage_command_ready
        CMP      R0,#+0
        ITTT     NE
        LDRNE.N  R7,??DataTable14
        LDRBNE   R0,[R7, #+114]
        CMPNE    R0,#+0
        BEQ.N    ??pr_request_1
        LDRB     R0,[R7, #+106]
        CMP      R0,#+0
        ITT      EQ
        LDRBEQ   R0,[R7, #+116]
        CMPEQ    R0,#+0
        BNE.N    ??pr_request_1
        LDRB     R0,[R7, #+115]
        CMP      R0,#+0
        ITTTT    EQ
        LDREQ.N  R0,??DataTable14_4
        LDRHEQ   R0,[R0, #+129]
        UBFXEQ   R0,R0,#+11,#+1
        CMPEQ    R0,#+0
        BNE.N    ??pr_request_1
        CMP      R4,#+2
        BEQ.N    ??pr_request_1
          CFI FunCall photo_alloc
        BL       photo_alloc
        CBZ.N    R0,??pr_request_1
        LDR.N    R1,??DataTable14_1
        LDR      R0,[R1, #+0]
        LDRB     R0,[R0, #+34]
        CBZ.N    R0,??pr_request_2
??pr_request_1:
        MOVS     R0,#+2
        B.N      ??pr_request_0
??pr_request_2:
        MOV      R0,#+4294967295
        STR      R0,[R7, #+60]
        MOVS     R0,#+0
        STRH     R0,[R7, #+90]
        CBZ.N    R6,??pr_request_3
        ADD      R3,R7,#+60
        LDR      R0,[R5], #+4
        STR      R0,[R3, #+0]
        ADD      R0,R7,#+90
        LDRB     R3,[R5, #+0]
        STRB     R3,[R0, #+0]
        LDRB     R3,[R5, #+1]
        STRB     R3,[R0, #+1]
??pr_request_3:
        MOVS     R0,#+0
        STRB     R0,[R7, #+118]
        STRB     R0,[R7, #+113]
        STRB     R0,[R7, #+112]
        STRB     R0,[R7, #+111]
        LDR.N    R0,??DataTable14_7
        LDR      R0,[R0, #+0]
        STR      R0,[R7, #+68]
        LDR      R0,[R1, #+0]
        STRB     R4,[R0, #+50]
        LDR      R0,[R1, #+0]
        STRB     R8,[R0, #+39]
        MOVS     R0,#+0
        LDR      R1,[R1, #+0]
        STRB     R0,[R1, #+38]
        MOVS     R0,#+4
          CFI FunCall pr_scan_begin
        BL       pr_scan_begin
        MOVS     R0,#+255
??pr_request_0:
        POP      {R4-R8,PC}
          CFI EndBlock cfiBlock12

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock13 Using cfiCommon0
          CFI Function photo_request_answer
        THUMB
photo_request_answer:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R4,R0
        MOV      R5,R1
          CFI FunCall pr_request
        BL       pr_request
        MOV      R1,R0
        CMP      R0,#+255
        BEQ.N    ??photo_request_answer_0
        MOV      R3,R5
        MOV      R2,R4
        MOVS     R0,#+21
        ADD      SP,SP,#+4
          CFI CFA R13+12
        POP      {R4,R5,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall user_send_answer_comand
        B.W      user_send_answer_comand
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
??photo_request_answer_0:
        POP      {R0,R4,R5,PC}
          CFI EndBlock cfiBlock13

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock14 Using cfiCommon0
          CFI Function pr_send_begin
        THUMB
pr_send_begin:
        LDR.N    R0,??DataTable14
        MOVS     R1,#+6
        STRB     R1,[R0, #+110]
        MOVS     R1,#+0
        STRB     R1,[R0, #+112]
        STRB     R1,[R0, #+111]
        LDR.N    R1,??DataTable14_7
        LDR      R1,[R1, #+0]
        STR      R1,[R0, #+68]
        LDR.N    R1,??DataTable14_1
        LDR      R1,[R1, #+0]
        LDR      R1,[R1, #+20]
        STR      R1,[R0, #+8]
        MOVS     R1,#+3
        STRB     R1,[R0, #+106]
        MOVS     R1,#+0
        STRB     R1,[R0, #+118]
        MOVS     R0,#+FO_PHOTO
          CFI FunCall flash_claim
        B.W      flash_claim
          CFI EndBlock cfiBlock14

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock15 Using cfiCommon0
          CFI Function pr_cleanup_begin
        THUMB
pr_cleanup_begin:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        LDR.N    R1,??DataTable14_4
        LDR      R1,[R1, #+157]
        LDR.N    R2,??DataTable14_3
        LDR      R2,[R2, #+72]
        CMP      R2,R1
        BCS.N    ??pr_cleanup_begin_0
        CMP      R0,R1
        BCC.N    ??pr_cleanup_begin_0
        SUBS     R2,R0,R1
        CMP      R2,#+8
        IT       CS
        ADDCS    R1,R1,#+8
        ADDW     R1,R1,#+4095
        LSRS     R1,R1,#+12
        LSLS     R1,R1,#+12
        CMP      R0,R1
        BCC.N    ??pr_cleanup_begin_0
        LDR.N    R0,??DataTable14
        MOVS     R2,#+0
        STRB     R2,[R0, #+113]
        STRB     R2,[R0, #+111]
        STRB     R2,[R0, #+112]
        MOVS     R2,#+7
        STRB     R2,[R0, #+110]
        MOVS     R2,#+0
        STR      R2,[R0, #+24]
        STR      R1,[R0, #+28]
        MVNS     R1,R1
        STR      R1,[R0, #+32]
        MOVS     R1,#+27
        STRB     R1,[R0, #+106]
        MOVS     R0,#+FO_PHOTO
          CFI FunCall flash_claim
        BL       flash_claim
??pr_cleanup_begin_0:
        POP      {R0,PC}
          CFI EndBlock cfiBlock15

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable14:
        DATA32
        DC32     pr

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable14_1:
        DATA32
        DC32     photo

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable14_2:
        DATA32
        DC32     sjr

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable14_3:
        DATA32
        DC32     mem

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable14_4:
        DATA32
        DC32     device

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable14_5:
        DATA32
        DC32     config

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable14_6:
        DATA32
        DC32     photo_erased_to

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable14_7:
        DATA32
        DC32     usb_bus_resets

        SECTION `.text`:CODE:NOROOT(2)
          CFI Block cfiBlock16 Using cfiCommon0
          CFI Function photo_storage_poll
        THUMB
photo_storage_poll:
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
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        MOV      R6,R0
        LDR.W    R7,??DataTable15
        LDRB     R0,[R7, #+106]
        CMP      R0,#+0
        BEQ.N    ??photo_storage_poll_1
        LDR.W    R4,??DataTable15_1
        LDR      R0,[R4, #+0]
        CBZ.N    R0,??photo_storage_poll_2
        LDRB     R0,[R0, #+50]
        CBNZ.N   R0,??photo_storage_poll_2
        LDR      R0,[R7, #+68]
        LDR.W    R1,??DataTable15_2
        LDR      R1,[R1, #+0]
        CMP      R0,R1
        BEQ.N    ??photo_storage_poll_2
        LDRB     R0,[R7, #+110]
        CMP      R0,#+4
        ITT      NE
        CMPNE    R0,#+5
        CMPNE    R0,#+6
        BNE.N    ??photo_storage_poll_2
        MOVS     R0,#+1
        STRB     R0,[R7, #+111]
        MOVS     R0,#+0
        LDR      R1,[R4, #+0]
        STRB     R0,[R1, #+38]
??photo_storage_poll_2:
        LDR.W    R9,??DataTable16
        LDRH     R0,[R9, #+129]
        UBFX     R0,R0,#+11,#+1
        CBZ.N    R0,??photo_storage_poll_3
          CFI FunCall photo_storage_cancel
        BL       photo_storage_cancel
??photo_storage_poll_3:
        LDRB     R0,[R7, #+108]
        CBZ.N    R0,??photo_storage_poll_4
        MOV      R0,R5
          CFI FunCall idcrc_io_poll
        BL       idcrc_io_poll
        CMP      R0,#+1
        BEQ.N    ??photo_storage_poll_1
        MOVS     R1,#+0
        STRB     R1,[R7, #+108]
        CMP      R0,#+2
??photo_storage_poll_5:
        BNE.W    ??photo_storage_poll_6
        LDRB     R0,[R7, #+109]
        CBNZ.N   R0,??photo_storage_poll_7
        LDRH     R1,[R7, #+76]
        LDRH     R0,[R7, #+78]
        ADDS     R1,R0,R1
        STRH     R1,[R7, #+76]
        LDRH     R1,[R7, #+76]
        LDRH     R0,[R7, #+74]
        CMP      R1,R0
        BNE.N    ??photo_storage_poll_1
??photo_storage_poll_7:
        LDRB     R0,[R7, #+107]
        B.N      ??photo_storage_poll_8
??photo_storage_poll_4:
        LDRB     R0,[R7, #+112]
        CBZ.N    R0,??photo_storage_poll_9
        LDRB     R0,[R7, #+113]
        CBNZ.N   R0,??photo_storage_poll_9
??photo_storage_poll_10:
          CFI FunCall pr_release
        BL       pr_release
??photo_storage_poll_1:
        B.N      ??photo_storage_poll_11
??photo_storage_poll_9:
        LDRB     R0,[R7, #+106]
        SUBS     R0,R0,#+1
        CMP      R0,#+42
        BHI.N    ??photo_storage_poll_5
        TBH      [PC, R0, LSL #+1]
        DATA
??photo_storage_poll_0:
        DC16     (??jt0_0-??photo_storage_poll_0)/2,(??jt0_1-??photo_storage_poll_0)/2,(??jt0_2-??photo_storage_poll_0)/2,(??jt0_3-??photo_storage_poll_0)/2
        DC16     (??jt0_4-??photo_storage_poll_0)/2,(??jt0_5-??photo_storage_poll_0)/2,(??jt0_6-??photo_storage_poll_0)/2,(??jt0_7-??photo_storage_poll_0)/2
        DC16     (??jt0_8-??photo_storage_poll_0)/2,(??jt0_9-??photo_storage_poll_0)/2,(??jt0_10-??photo_storage_poll_0)/2,(??jt0_11-??photo_storage_poll_0)/2
        DC16     (??jt0_12-??photo_storage_poll_0)/2,(??jt0_13-??photo_storage_poll_0)/2,(??jt0_14-??photo_storage_poll_0)/2,(??jt0_15-??photo_storage_poll_0)/2
        DC16     (??jt0_16-??photo_storage_poll_0)/2,(??jt0_17-??photo_storage_poll_0)/2,(??jt0_18-??photo_storage_poll_0)/2,(??jt0_19-??photo_storage_poll_0)/2
        DC16     (??jt0_20-??photo_storage_poll_0)/2,(??jt0_21-??photo_storage_poll_0)/2,(??jt0_22-??photo_storage_poll_0)/2,(??jt0_23-??photo_storage_poll_0)/2
        DC16     (??jt0_24-??photo_storage_poll_0)/2,(??jt0_25-??photo_storage_poll_0)/2,(??jt0_26-??photo_storage_poll_0)/2,(??jt0_27-??photo_storage_poll_0)/2
        DC16     (??jt0_28-??photo_storage_poll_0)/2,(??jt0_29-??photo_storage_poll_0)/2,(??jt0_30-??photo_storage_poll_0)/2,(??jt0_31-??photo_storage_poll_0)/2
        DC16     (??jt0_32-??photo_storage_poll_0)/2,(??jt0_33-??photo_storage_poll_0)/2,(??jt0_34-??photo_storage_poll_0)/2,(??jt0_35-??photo_storage_poll_0)/2
        DC16     (??jt0_36-??photo_storage_poll_0)/2,(??jt0_37-??photo_storage_poll_0)/2,(??jt0_38-??photo_storage_poll_0)/2,(??jt0_39-??photo_storage_poll_0)/2
        DC16     (??jt0_40-??photo_storage_poll_0)/2,(??jt0_41-??photo_storage_poll_0)/2,(??jt0_42-??photo_storage_poll_0)/2
        THUMB
??photo_storage_poll_12:
??jt0_0:
        LDRH     R2,[R7, #+74]
        LDRH     R0,[R7, #+76]
        SUBS     R2,R2,R0
        LDRB     R0,[R7, #+109]
        CBNZ.N   R0,??photo_storage_poll_13
        LDR      R1,[R7, #+4]
        LDRH     R0,[R7, #+76]
        ADDS     R1,R0,R1
        AND      R1,R1,#0xFF
        RSB      R1,R1,#+256
        UXTH     R0,R2
        CMP      R1,R0
        BCS.N    ??photo_storage_poll_13
        LDR      R0,[R7, #+4]
        LDRH     R2,[R7, #+76]
        ADDS     R2,R2,R0
        AND      R2,R2,#0xFF
        RSB      R2,R2,#+256
??photo_storage_poll_13:
        STRH     R2,[R7, #+78]
        LDRB     R0,[R7, #+109]
        CBZ.N    R0,??photo_storage_poll_14
        MOVS     R1,#+0
        B.N      ??photo_storage_poll_15
??photo_storage_poll_14:
        LDR.W    R1,??DataTable20
        LDRH     R0,[R7, #+76]
        ADD      R0,R1,R0
        ADD      R1,R0,#+61
??photo_storage_poll_15:
        MOV      R3,R5
        UXTH     R2,R2
        LDR      R0,[R7, #+4]
        LDRH     R4,[R7, #+76]
        ADDS     R0,R4,R0
          CFI FunCall idcrc_io_start
        BL       idcrc_io_start
        CMP      R0,#+0
        BEQ.N    ??photo_storage_poll_6
        MOVS     R0,#+1
        STRB     R0,[R7, #+108]
        B.N      ??photo_storage_poll_11
??photo_storage_poll_16:
??jt0_1:
        MOVS     R2,#+12
        ADD      R1,R7,#+24
        ADD      R0,R6,#+4128
          CFI FunCall w25_read_data
        BL       w25_read_data
        CMP      R0,#+0
        BEQ.N    ??photo_storage_poll_6
        LDR      R0,[R7, #+24]
        CBNZ.N   R0,??photo_storage_poll_17
        LDR      R1,[R7, #+28]
        LDR      R0,[R7, #+32]
        MVNS     R0,R0
        CMP      R1,R0
        BNE.N    ??photo_storage_poll_18
        CMP      R6,R1
        BCC.N    ??photo_storage_poll_18
        LDR.W    R0,??DataTable20_1
        LDR      R1,[R0, #+72]
        LDR      R0,[R7, #+28]
        CMP      R1,R0
        BCS.N    ??photo_storage_poll_18
        LSLS     R0,R0,#+20
        BNE.N    ??photo_storage_poll_18
        MOVS     R0,#+1
        STRB     R0,[R7, #+115]
        MOVS     R0,#+7
        STRB     R0,[R7, #+110]
        LDR      R0,[R7, #+28]
        STR      R0,[R7, #+12]
        BL       ?Subroutine4
??CrossCallReturnLabel_11:
        MOVS     R0,#+29
        B.N      ??photo_storage_poll_8
??photo_storage_poll_17:
        MOVS     R2,#+16
        ADD      R1,R7,#+24
        ADD      R0,R6,#+4096
          CFI FunCall w25_read_data
        BL       w25_read_data
        CMP      R0,#+0
        BEQ.N    ??photo_storage_poll_6
        LDR      R1,[R7, #+36]
        LDR.W    R0,??DataTable20_2
        CMP      R1,R0
        BNE.N    ??photo_storage_poll_19
        LDR      R0,[R7, #+24]
        LDR.W    R1,??DataTable20_1
        LDR      R1,[R1, #+72]
        CMP      R0,R1
        BCC.N    ??photo_storage_poll_18
        CMP      R0,R6
        BCS.N    ??photo_storage_poll_18
        LSLS     R0,R0,#+20
        BNE.N    ??photo_storage_poll_18
        LDR      R0,[R7, #+28]
        CBZ.N    R0,??photo_storage_poll_18
        CMP      R0,#+4096
        BCS.N    ??photo_storage_poll_18
        BL       ?Subroutine2
??CrossCallReturnLabel_4:
        BL       ?Subroutine1
??CrossCallReturnLabel_2:
        MOVS     R0,#+16
        B.N      ??photo_storage_poll_8
??photo_storage_poll_19:
        BL       ?Subroutine4
??CrossCallReturnLabel_12:
        MOVS     R0,#+42
??photo_storage_poll_8:
        STRB     R0,[R7, #+106]
          CFI FunCall pr_owner
        BL       pr_owner
        B.N      ??photo_storage_poll_11
??photo_storage_poll_20:
??jt0_41:
        BL       ?Subroutine16
??CrossCallReturnLabel_46:
        BL       ?Subroutine17
??CrossCallReturnLabel_48:
          CFI FunCall w25_read_data
        BL       w25_read_data
        CBNZ.N   R0,??photo_storage_poll_21
??photo_storage_poll_6:
          CFI FunCall pr_fail
        BL       pr_fail
        B.N      ??photo_storage_poll_11
??photo_storage_poll_21:
        MOVS     R1,#+64
        ADD      R0,R4,#+61
          CFI FunCall fast_all_ff
        BL       fast_all_ff
        CBNZ.N   R0,??photo_storage_poll_22
??photo_storage_poll_18:
        MOVS     R1,#+43
??photo_storage_poll_23:
        ADD      R0,R6,#+4096
??photo_storage_poll_24:
          CFI FunCall pr_erase
        BL       pr_erase
        B.N      ??photo_storage_poll_11
??photo_storage_poll_22:
        LDRH     R4,[R7, #+72]
        ADDS     R4,R4,#+64
        UXTH     R0,R4
        CMP      R0,#+4096
        BNE.N    ??photo_storage_poll_25
        MOVS     R0,#+43
??photo_storage_poll_26:
        STRB     R0,[R7, #+106]
??photo_storage_poll_25:
        STRH     R4,[R7, #+72]
??photo_storage_poll_11:
        POP      {R0,R4-R9,PC}
??photo_storage_poll_27:
??jt0_42:
        MOVS     R0,#+0
??photo_storage_poll_28:
          CFI FunCall pr_scan_begin
        BL       pr_scan_begin
        B.N      ??photo_storage_poll_11
??photo_storage_poll_29:
??jt0_2:
        LDR      R0,[R7, #+8]
        ADDS     R0,R0,#+8
        CMP      R6,R0
        BCC.N    ??photo_storage_poll_30
        LDRB     R0,[R7, #+110]
        CMP      R0,#+6
        BNE.N    ??photo_storage_poll_31
        LDR      R0,[R7, #+8]
        LDR      R1,[R9, #+157]
        CMP      R0,R1
        BCS.N    ??photo_storage_poll_30
??photo_storage_poll_31:
        BL       ?Subroutine11
??CrossCallReturnLabel_33:
        CMP      R0,#+0
        BEQ.N    ??photo_storage_poll_6
        LDR      R0,[R7, #+24]
        CMN      R0,#+1
        BNE.N    ??photo_storage_poll_32
??photo_storage_poll_30:
          CFI FunCall pr_scan_finish
        BL       pr_scan_finish
        B.N      ??photo_storage_poll_11
??photo_storage_poll_32:
        LDR      R0,[R7, #+28]
        CBZ.N    R0,??photo_storage_poll_33
        CMP      R0,#+65536
        BCS.N    ??photo_storage_poll_33
        LDR      R1,[R7, #+8]
        SUBS     R1,R6,R1
        MOVW     R0,#+1033
        CMP      R1,R0
        BCC.N    ??photo_storage_poll_33
        LDR      R1,[R7, #+8]
        SUBS     R1,R6,R1
        SUBW     R1,R1,#+1033
        LDR      R0,[R7, #+28]
        CMP      R1,R0
        BCS.N    ??photo_storage_poll_34
??photo_storage_poll_33:
        STR      R6,[R7, #+8]
        B.N      ??photo_storage_poll_30
??photo_storage_poll_34:
        LDR      R0,[R7, #+24]
        LDR      R1,[R7, #+56]
        CMP      R0,R1
        IT       CC
        MOVCC    R0,R1
        STR      R0,[R7, #+56]
        MOVS     R0,#+0
        STRH     R0,[R7, #+82]
        STRH     R0,[R7, #+84]
        MOVW     R0,#+65535
        STRH     R0,[R7, #+86]
        MOVS     R0,#+4
        B.N      ??photo_storage_poll_8
??photo_storage_poll_35:
??jt0_3:
        LDR.W    R4,??DataTable20
        MOVS     R2,#+1
        ADD      R1,R4,#+61
        LDR      R0,[R7, #+8]
        ADD      R0,R0,#+1032
          CFI FunCall w25_read_data
        BL       w25_read_data
        CMP      R0,#+0
??photo_storage_poll_36:
        BEQ.N    ??photo_storage_poll_6
        LDRB     R0,[R4, #+61]
        CMP      R0,#+0
        ITEEE    NE
        MOVNE    R0,#+5
        LDREQ    R0,[R7, #+28]
        STRHEQ   R0,[R7, #+82]
        MOVEQ    R0,#+6
        B.N      ??photo_storage_poll_8
??photo_storage_poll_37:
??jt0_4:
        BL       ?Subroutine16
??CrossCallReturnLabel_47:
        LDR      R0,[R7, #+8]
        LDRH     R3,[R7, #+84]
        ADDS     R0,R3,R0
        ADDS     R0,R0,#+8
          CFI FunCall w25_read_data
        BL       w25_read_data
        CMP      R0,#+0
        BEQ.N    ??photo_storage_poll_36
        MOV      R9,#+0
??photo_storage_poll_38:
        ADD      R0,R4,R9
        ADD      R6,R0,#+61
        LDRB     R5,[R6, #+0]
        LDRB     R0,[R6, #+1]
        ORR      R5,R5,R0, LSL #+8
        LDRB     R1,[R6, #+2]
        LDRB     R0,[R6, #+3]
        ORR      R8,R1,R0, LSL #+8
        MOVS     R1,#+8
        MOV      R0,R6
          CFI FunCall fast_all_ff
        BL       fast_all_ff
        CBNZ.N   R0,??photo_storage_poll_39
        LDRB     R0,[R6, #+4]
        CMP      R0,#+0
        ITT      EQ
        LDRBEQ   R0,[R6, #+5]
        CMPEQ    R0,#+0
        BNE.N    ??photo_storage_poll_40
        LDRB     R0,[R6, #+6]
        CMP      R0,#+0
        ITT      EQ
        LDRBEQ   R0,[R6, #+7]
        CMPEQ    R0,#+0
        ITTT     EQ
        EOREQ    R1,R8,R5
        MOVWEQ   R0,#+65535
        CMPEQ    R1,R0
        BNE.N    ??photo_storage_poll_40
        LDR      R0,[R7, #+28]
        CMP      R0,R5
        BCC.N    ??photo_storage_poll_40
        LDRH     R0,[R7, #+82]
        CMP      R0,R5
        IT       LT
        STRHLT   R5,[R7, #+82]
??photo_storage_poll_40:
        ADD      R9,R9,#+8
        CMP      R9,#+64
        BLT.N    ??photo_storage_poll_38
        B.N      ??photo_storage_poll_41
??photo_storage_poll_39:
        LDRH     R0,[R7, #+84]
        ADD      R0,R9,R0
        STRH     R0,[R7, #+86]
        MOVS     R0,#+6
        STRB     R0,[R7, #+106]
??photo_storage_poll_41:
        LDRH     R0,[R7, #+84]
        ADDS     R0,R0,#+64
        UXTH     R1,R0
        CMP      R1,#+1024
        ITT      EQ
        MOVEQ    R1,#+6
        STRBEQ   R1,[R7, #+106]
        STRH     R0,[R7, #+84]
        B.N      ??photo_storage_poll_11
??photo_storage_poll_42:
??jt0_5:
        LDRH     R1,[R7, #+82]
        LDR      R0,[R7, #+28]
        CMP      R1,R0
        BCS.N    ??photo_storage_poll_43
        LDRH     R0,[R7, #+88]
        ADDS     R0,R0,#+1
        STRH     R0,[R7, #+88]
        LDR      R0,[R7, #+48]
        CBNZ.N   R0,??photo_storage_poll_43
        LDR      R0,[R7, #+8]
        STR      R0,[R7, #+48]
        STRH     R1,[R7, #+92]
??photo_storage_poll_43:
        LDRB     R0,[R7, #+110]
        CMP      R0,#+4
        BNE.N    ??photo_storage_poll_44
        LDR      R0,[R7, #+60]
        CMN      R0,#+1
        BEQ.N    ??photo_storage_poll_45
        CBZ.N    R0,??photo_storage_poll_46
        MOV      R1,R0
        LDR      R0,[R7, #+24]
        CMP      R1,R0
        BEQ.N    ??photo_storage_poll_47
        B.N      ??photo_storage_poll_45
??photo_storage_poll_46:
        LDR      R0,[R7, #+28]
        CMP      R1,R0
        BCS.N    ??photo_storage_poll_45
??photo_storage_poll_47:
        LDR      R1,[R7, #+28]
        LDRH     R0,[R7, #+90]
        CMP      R1,R0
        BCC.N    ??photo_storage_poll_48
        LDRH     R1,[R7, #+82]
        CMP      R1,R0
        BCS.N    ??photo_storage_poll_49
        MOV      R1,R0
        LDR      R0,[R7, #+28]
        CMP      R1,R0
        BEQ.N    ??photo_storage_poll_49
        LDRH     R1,[R7, #+86]
        MOVW     R0,#+65535
        CMP      R1,R0
        BNE.N    ??photo_storage_poll_49
??photo_storage_poll_48:
        LDRB     R0,[R7, #+111]
        CMP      R0,#+0
        BNE.N    ??CrossCallReturnLabel_22
        BL       ?Subroutine0
??CrossCallReturnLabel_0:
        BL       ?Subroutine7
??CrossCallReturnLabel_22:
        B.N      ??photo_storage_poll_10
??photo_storage_poll_49:
        LDR      R0,[R7, #+8]
        STR      R0,[R7, #+40]
        MOV      R1,R0
        LDRH     R0,[R7, #+86]
        ADDS     R1,R0,R1
        ADDS     R1,R1,#+8
        STR      R1,[R7, #+44]
        LDRH     R0,[R7, #+82]
        STRH     R0,[R7, #+96]
        LDR      R0,[R7, #+28]
        STRH     R0,[R7, #+94]
        BL       ?Subroutine9
??CrossCallReturnLabel_26:
        STR      R0,[R7, #+8]
        MOVS     R0,#+5
        STRB     R0,[R7, #+110]
??photo_storage_poll_50:
        MOVS     R0,#+34
        B.N      ??photo_storage_poll_8
??photo_storage_poll_44:
        CMP      R0,#+6
        BNE.N    ??photo_storage_poll_45
        LDR      R0,[R7, #+28]
        CMP      R1,R0
        BCS.N    ??photo_storage_poll_45
        LDR      R0,[R7, #+8]
        LDR      R1,[R4, #+0]
        LDR      R1,[R1, #+20]
        CMP      R0,R1
        ITTE     EQ
        LDREQ    R0,[R4, #+0]
        LDRHEQ   R1,[R0, #+28]
        MOVNE    R1,#+0
        LDRH     R0,[R7, #+82]
        CMP      R0,R1
        IT       CC
        MOVCC    R0,R1
        LDR      R1,[R7, #+28]
        CMP      R0,R1
        BCC.N    ??photo_storage_poll_51
??photo_storage_poll_45:
        LDR      R1,[R7, #+8]
        BL       ?Subroutine12
??CrossCallReturnLabel_34:
        STR      R1,[R7, #+8]
        MOVS     R0,#+3
        B.N      ??photo_storage_poll_8
??photo_storage_poll_51:
        STRH     R0,[R7, #+100]
        SUBS     R0,R1,R0
        UXTH     R2,R0
        MOVW     R1,#+513
        CMP      R2,R1
        IT       GE
        MOVGE    R0,#+512
        STRH     R0,[R7, #+98]
        MOVS     R0,#+39
        B.N      ??photo_storage_poll_8
??photo_storage_poll_52:
??jt0_6:
        LDRB     R0,[R7, #+115]
        CMP      R0,#+0
        BNE.W    ??photo_storage_poll_6
        LDRB     R0,[R7, #+114]
        CMP      R0,#+0
        BEQ.N    ??photo_storage_poll_53
        LDRH     R0,[R9, #+155]
        CBNZ.N   R0,??photo_storage_poll_54
        LDR.W    R0,??DataTable20_1
        LDR      R1,[R0, #+72]
        LDR      R0,[R9, #+157]
        CMP      R1,R0
        BCC.N    ??photo_storage_poll_55
??photo_storage_poll_54:
        LDR      R0,[R9, #+157]
        CMP      R6,R0
        ITTT     CS
        SUBCS    R6,R6,R0
        MOVWCS   R0,#+1034
        CMPCS    R6,R0
??photo_storage_poll_55:
        BCC.W    ??photo_storage_poll_6
        LDR      R1,[R9, #+157]
        LDR      R0,[R4, #+0]
        STR      R1,[R0, #+4]
        LDR      R0,[R9, #+47]
        CMP      R0,#+0
        ITE      NE
        MOVNE    R1,R0
        MOVEQ    R1,#+1
        LDR      R0,[R4, #+0]
        STR      R1,[R0, #+0]
        LDR      R0,[R7, #+52]
        LDR      R1,[R4, #+0]
        LDR      R1,[R1, #+0]
        CMP      R0,R1
        BCC.N    ??photo_storage_poll_56
        ADDS     R0,R0,#+1
        LDR      R1,[R4, #+0]
        STR      R0,[R1, #+0]
??photo_storage_poll_56:
        LDR      R0,[R4, #+0]
        LDR      R0,[R0, #+0]
        CMN      R0,#+1
        ITTT     NE
        LDRNE    R0,[R4, #+0]
        LDRNE    R0,[R0, #+0]
        CMPNE    R0,#+0
        BEQ.N    ??photo_storage_poll_53
        MOVS     R0,#+0
        LDR      R1,[R4, #+0]
        STRH     R0,[R1, #+32]
        LDR      R1,[R4, #+0]
        STRH     R0,[R1, #+30]
        LDR      R1,[R4, #+0]
        STRH     R0,[R1, #+26]
        LDR      R1,[R4, #+0]
        STRH     R0,[R1, #+24]
        LDR.W    R1,??DataTable27
        STR      R0,[R1, #+0]
        LDR      R1,[R4, #+0]
        STRB     R0,[R1, #+37]
        LDR      R1,[R4, #+0]
        STRB     R0,[R1, #+36]
        LDR      R1,[R4, #+0]
        STRB     R0,[R1, #+35]
        LDR      R1,[R4, #+0]
        STRH     R0,[R1, #+40]
        MOVS     R1,#+1
        LDR      R0,[R4, #+0]
        STRB     R1,[R0, #+34]
        LDR      R0,[R4, #+0]
        STR      R5,[R0, #+12]
        SUB      R0,R5,#+50
        LDR      R1,[R4, #+0]
        STR      R0,[R1, #+16]
        LDR      R0,[R4, #+0]
        STR      R5,[R0, #+8]
          CFI FunCall rs485_baud
        BL       rs485_baud
        MOV      R1,R0
        MOVS     R0,#+64
        STR      R0,[SP, #+0]
        LDR      R0,[R4, #+0]
        ADD      R3,R0,#+51
        MOVS     R2,#+0
        MOVS     R0,#+0
          CFI FunCall uart_init
        BL       uart_init
        LDRB     R0,[R7, #+117]
        STRB     R0,[R9, #+116]
??photo_storage_poll_57:
        MOVS     R0,#+0
        B.N      ??photo_storage_poll_8
??photo_storage_poll_58:
??jt0_7:
        LDR      R4,[R7, #+12]
        LDR      R0,[R7, #+8]
        SUBS     R4,R4,R0
        UXTH     R0,R4
        CMP      R0,#+65
        IT       GE
        MOVGE    R4,#+64
        BL       ?Subroutine23
??CrossCallReturnLabel_60:
        LDR      R0,[R7, #+8]
          CFI FunCall w25_read_data
        BL       w25_read_data
        CMP      R0,#+0
??photo_storage_poll_53:
        BEQ.W    ??photo_storage_poll_6
        UXTH     R1,R4
        ADD      R0,R5,#+61
          CFI FunCall fast_all_ff
        BL       fast_all_ff
        CBNZ.N   R0,??photo_storage_poll_59
        MOVS     R0,#+9
        B.N      ??photo_storage_poll_8
??photo_storage_poll_59:
        LDR      R0,[R7, #+8]
        UXTH     R4,R4
        ADDS     R4,R4,R0
        LDR      R0,[R7, #+12]
        CMP      R4,R0
        BNE.N    ??photo_storage_poll_60
        MOV      R1,R0
        LDR.W    R0,??DataTable27
        STR      R1,[R0, #+0]
        MOVS     R0,#+22
        STRB     R0,[R7, #+106]
??photo_storage_poll_60:
        STR      R4,[R7, #+8]
        B.N      ??photo_storage_poll_11
??photo_storage_poll_61:
??jt0_8:
        MOVS     R1,#+10
        B.N      ??photo_storage_poll_23
??photo_storage_poll_62:
??jt0_9:
        BL       ?Subroutine1
??CrossCallReturnLabel_3:
        MOVS     R1,#+11
        MOV      R0,R6
        B.N      ??photo_storage_poll_24
??photo_storage_poll_63:
??jt0_10:
        BL       ?Subroutine18
??CrossCallReturnLabel_50:
        IT       GE
        MOVGE    R5,#+64
        BL       ?Subroutine24
??CrossCallReturnLabel_62:
        LDR      R0,[R4, #+0]
        LDR      R0,[R0, #+4]
        LSRS     R0,R0,#+12
        LDRH     R3,[R7, #+72]
        ADD      R0,R3,R0, LSL #+12
          CFI FunCall w25_read_data
        BL       w25_read_data
        CMP      R0,#+0
        BEQ.N    ??photo_storage_poll_64
        BL       ?Subroutine25
??CrossCallReturnLabel_66:
        STR      R0,[R7, #+16]
        STRH     R5,[R7, #+104]
        MOVS     R3,#+12
        UXTH     R2,R5
        ADD      R1,R9,#+61
        LDRH     R0,[R7, #+72]
        ADDS     R0,R0,R6
??photo_storage_poll_65:
          CFI FunCall pr_write
        BL       pr_write
        B.N      ??photo_storage_poll_11
??photo_storage_poll_66:
??jt0_11:
        BL       ?Subroutine26
??CrossCallReturnLabel_67:
        LDRH     R1,[R7, #+80]
        UXTH     R2,R0
        CMP      R2,R1
        BNE.N    ??photo_storage_poll_67
        LDR      R0,[R7, #+16]
        STR      R0,[R7, #+20]
        MOV      R0,#+4294967295
        STR      R0,[R7, #+16]
        MOVS     R0,#+0
        MOVS     R1,#+13
??photo_storage_poll_68:
        STRH     R0,[R7, #+72]
        STRB     R1,[R7, #+106]
??photo_storage_poll_69:
        B.N      ??photo_storage_poll_11
??photo_storage_poll_67:
        MOVS     R1,#+11
        B.N      ??photo_storage_poll_68
??photo_storage_poll_70:
??jt0_12:
        BL       ?Subroutine18
??CrossCallReturnLabel_51:
        IT       GE
        MOVGE    R5,#+64
        BL       ?Subroutine24
??CrossCallReturnLabel_63:
        BL       ??Subroutine30_0
??CrossCallReturnLabel_79:
        CMP      R0,#+0
        BEQ.N    ??photo_storage_poll_64
        BL       ?Subroutine25
??CrossCallReturnLabel_65:
        STR      R0,[R7, #+16]
        BL       ?Subroutine19
??CrossCallReturnLabel_52:
        LDRH     R1,[R7, #+72]
        LDRH     R0,[R7, #+80]
        CMP      R1,R0
        BNE.N    ??photo_storage_poll_69
        LDR      R1,[R7, #+16]
        LDR      R0,[R7, #+20]
        CMP      R1,R0
        BNE.N    ??photo_storage_poll_71
        LDR      R0,[R4, #+0]
        LDR      R0,[R0, #+4]
        LSRS     R0,R0,#+12
        LSLS     R0,R0,#+12
        STR      R0,[R7, #+24]
        LDRH     R0,[R7, #+80]
        STR      R0,[R7, #+28]
        STR      R1,[R7, #+32]
        LDR.W    R0,??DataTable20_2
        STR      R0,[R7, #+36]
        MOVS     R0,#+14
        B.N      ??photo_storage_poll_8
??photo_storage_poll_72:
??jt0_13:
        MOVS     R3,#+15
        MOVS     R2,#+12
        ADD      R1,R7,#+24
        ADD      R0,R6,#+4096
        B.N      ??photo_storage_poll_65
??photo_storage_poll_73:
??jt0_14:
        BL       ?Subroutine2
??CrossCallReturnLabel_5:
        MOVS     R3,#+17
        MOVS     R2,#+4
        ADD      R1,R7,#+36
        ADD      R6,R6,#+4096
        ADD      R0,R6,#+12
        B.N      ??photo_storage_poll_65
??photo_storage_poll_74:
??jt0_15:
        BL       ?Subroutine14
??CrossCallReturnLabel_38:
        IT       GE
        MOVGE    R4,#+64
        BL       ?Subroutine13
??CrossCallReturnLabel_81:
        CMP      R0,#+0
??photo_storage_poll_64:
        BEQ.W    ??photo_storage_poll_6
        BL       ?Subroutine22
??CrossCallReturnLabel_59:
        BL       ?Subroutine15
??CrossCallReturnLabel_43:
        LDRH     R0,[R7, #+72]
        LDR      R1,[R7, #+28]
        CMP      R0,R1
        BNE.N    ??photo_storage_poll_69
        LDR      R1,[R7, #+16]
        LDR      R0,[R7, #+32]
        CMP      R1,R0
??photo_storage_poll_71:
        BNE.W    ??photo_storage_poll_6
        MOVS     R0,#+17
        B.N      ??photo_storage_poll_8
??photo_storage_poll_75:
??jt0_16:
        BL       ?Subroutine4
??CrossCallReturnLabel_13:
        MOVS     R1,#+18
        LDR      R0,[R7, #+24]
        B.N      ??photo_storage_poll_24
??photo_storage_poll_76:
??jt0_17:
        BL       ?Subroutine14
??CrossCallReturnLabel_39:
        IT       GE
        MOVGE    R4,#+64
        BL       ?Subroutine13
??CrossCallReturnLabel_80:
        CMP      R0,#+0
        BEQ.N    ??photo_storage_poll_64
        STRH     R4,[R7, #+104]
        MOVS     R3,#+19
        UXTH     R2,R4
        ADD      R1,R5,#+61
        LDR      R0,[R7, #+24]
        LDRH     R4,[R7, #+72]
        ADDS     R0,R4,R0
        B.N      ??photo_storage_poll_65
??photo_storage_poll_77:
??jt0_18:
        BL       ?Subroutine26
??CrossCallReturnLabel_68:
        STRH     R0,[R7, #+72]
        UXTH     R2,R0
        LDR      R1,[R7, #+28]
        CMP      R2,R1
        BNE.N    ??photo_storage_poll_78
        MOVS     R0,#+0
        MOV      R1,#+4294967295
        STR      R1,[R7, #+16]
        MOVS     R1,#+20
        B.N      ??photo_storage_poll_68
??photo_storage_poll_78:
        MOVS     R1,#+18
        B.N      ??photo_storage_poll_68
??photo_storage_poll_79:
??jt0_19:
        BL       ?Subroutine14
??CrossCallReturnLabel_40:
        IT       GE
        MOVGE    R4,#+64
        BL       ?Subroutine23
??CrossCallReturnLabel_61:
        LDR      R0,[R7, #+24]
        LDRH     R3,[R7, #+72]
        ADDS     R0,R3,R0
          CFI FunCall w25_read_data
        BL       w25_read_data
        CMP      R0,#+0
        BEQ.N    ??photo_storage_poll_64
        BL       ?Subroutine22
??CrossCallReturnLabel_58:
        BL       ?Subroutine15
??CrossCallReturnLabel_44:
        LDRH     R1,[R7, #+72]
        LDR      R0,[R7, #+28]
        CMP      R1,R0
        BNE.W    ??photo_storage_poll_11
        LDR      R0,[R7, #+16]
        LDR      R1,[R7, #+32]
        CMP      R0,R1
        BNE.N    ??photo_storage_poll_71
        MOVS     R0,#+21
??photo_storage_poll_80:
        B.N      ??photo_storage_poll_8
??photo_storage_poll_81:
??jt0_20:
        MOVS     R1,#+22
        B.N      ??photo_storage_poll_23
??photo_storage_poll_82:
??jt0_21:
        LDRB     R0,[R7, #+113]
        CBZ.N    R0,??photo_storage_poll_83
        LDR      R0,[R7, #+24]
        ADD      R0,R0,#+4096
        LDR.W    R1,??DataTable27
        STR      R0,[R1, #+0]
??photo_storage_poll_83:
        MOVS     R0,#+0
        STRB     R0,[R7, #+113]
        LDRB     R0,[R7, #+110]
        CMP      R0,#+2
        ITT      EQ
        LDRBEQ   R0,[R7, #+112]
        CMPEQ    R0,#+0
        BNE.N    ??photo_storage_poll_84
        MOVS     R0,#+6
        LDR      R1,[R4, #+0]
        STRB     R0,[R1, #+34]
        LDR      R0,[R4, #+0]
        STR      R5,[R0, #+8]
        B.N      ??photo_storage_poll_57
??photo_storage_poll_84:
        LDRB     R0,[R7, #+112]
        CMP      R0,#+0
        BEQ.W    ??photo_storage_poll_28
??photo_storage_poll_85:
        B.N      ??photo_storage_poll_10
??photo_storage_poll_86:
??jt0_22:
        LDR.W    R1,??DataTable27
        LDR      R0,[R1, #+0]
        ADD      R0,R0,#+4096
        STR      R0,[R1, #+0]
        B.N      ??photo_storage_poll_57
??photo_storage_poll_87:
??jt0_23:
        LDR      R0,[R4, #+0]
        LDRH     R1,[R0, #+26]
        LDRH     R0,[R7, #+104]
        ADDS     R1,R0,R1
        LDR      R0,[R4, #+0]
        STRH     R1,[R0, #+26]
        LDR      R0,[R4, #+0]
        LDRH     R1,[R0, #+30]
        ADDS     R1,R1,#+1
        STRH     R1,[R0, #+30]
        MOVS     R1,#+0
        LDR      R0,[R4, #+0]
        STRB     R1,[R0, #+35]
        LDR      R0,[R4, #+0]
        STR      R5,[R0, #+8]
        LDR      R0,[R4, #+0]
        LDRH     R0,[R0, #+26]
        LDR      R1,[R4, #+0]
        LDRH     R1,[R1, #+24]
        CMP      R0,R1
        IT       NE
        MOVNE    R0,#+0
        BNE.N    ??photo_storage_poll_80
        LDR      R0,[R4, #+0]
        LDR      R0,[R0, #+0]
        STR      R0,[R7, #+24]
        LDR      R0,[R4, #+0]
        LDRH     R0,[R0, #+24]
        STR      R0,[R7, #+28]
        MOVS     R3,#+25
        MOVS     R2,#+4
        ADD      R1,R7,#+28
        LDR      R0,[R4, #+0]
        LDR      R0,[R0, #+4]
??photo_storage_poll_88:
        ADDS     R0,R0,#+4
        B.N      ??photo_storage_poll_65
??photo_storage_poll_89:
??jt0_24:
        BL       ?Subroutine2
??CrossCallReturnLabel_6:
        MOVS     R3,#+26
        MOVS     R2,#+4
        ADD      R1,R7,#+24
        LDR      R0,[R4, #+0]
        LDR      R0,[R0, #+4]
        B.N      ??photo_storage_poll_65
??photo_storage_poll_90:
??jt0_25:
        LDRH     R0,[R9, #+155]
        ADDS     R0,R0,#+1
        STRH     R0,[R9, #+155]
        LDRH     R0,[R9, #+129]
        ORR      R0,R0,#0x2000
        STRH     R0,[R9, #+129]
        LDR      R0,[R4, #+0]
        LDR      R1,[R0, #+4]
        LDRH     R0,[R0, #+24]
        BL       ??Subroutine12_0
??CrossCallReturnLabel_35:
        STR      R1,[R9, #+157]
        LDR      R0,[R4, #+0]
        LDR      R0,[R0, #+0]
        STR      R0,[R7, #+52]
        LDR      R0,[R4, #+0]
        LDR      R0,[R0, #+0]
          CFI FunCall log_addPhotoEvent
        BL       log_addPhotoEvent
        MOVS     R0,#+240
        STR      R0,[SP, #+0]
        MOVS     R3,#+240
        MOVS     R2,#+0
        MOVS     R1,#+0
        MOVS     R0,#+14
          CFI FunCall photo_command
        BL       photo_command
          CFI FunCall photo_stop
        BL       photo_stop
        B.N      ??photo_storage_poll_85
??photo_storage_poll_91:
??jt0_26:
        MOVS     R3,#+28
        MOVS     R2,#+8
        ADD      R1,R7,#+28
        ADD      R6,R6,#+4096
        ADD      R0,R6,#+36
        B.N      ??photo_storage_poll_65
??photo_storage_poll_92:
??jt0_27:
        BL       ?Subroutine2
??CrossCallReturnLabel_7:
        MOVS     R3,#+32
        MOVS     R2,#+4
        ADD      R1,R7,#+24
        ADD      R0,R6,#+4128
        B.N      ??photo_storage_poll_65
??photo_storage_poll_93:
??jt0_31:
        MOVS     R0,#+1
        STRB     R0,[R7, #+115]
        MOVS     R0,#+0
        STRB     R0,[R7, #+114]
        LDR.W    R1,??DataTable20_1
        LDR      R0,[R1, #+72]
        BL       ?Subroutine27
??CrossCallReturnLabel_69:
        LDRH     R2,[R9, #+129]
        MOVW     R0,#+57343
        ANDS     R2,R0,R2
        STRH     R2,[R9, #+129]
        MOVS     R0,#+0
        STRH     R0,[R9, #+167]
        LDR      R0,[R4, #+0]
        CMP      R0,#+0
        BEQ.W    ??photo_storage_poll_10
        LDR      R0,[R1, #+72]
        LDR      R1,[R4, #+0]
        STR      R0,[R1, #+20]
        MOVS     R0,#+0
        LDR      R1,[R4, #+0]
        STRH     R0,[R1, #+28]
        B.N      ??photo_storage_poll_85
??photo_storage_poll_94:
??jt0_28:
        LDR.W    R4,??DataTable20_1
        LDR      R1,[R7, #+12]
        LDR      R0,[R4, #+72]
        SUBS     R1,R1,R0
        LDRH     R5,[R7, #+72]
        RSB      R5,R5,R1, LSR #+12
        UXTH     R0,R5
        CMP      R0,#+65
        IT       GE
        MOVGE    R5,#+64
        BGE.N    ??photo_storage_poll_95
        CMP      R0,#+0
        BEQ.N    ??photo_storage_poll_96
??photo_storage_poll_95:
        LDRH     R1,[R7, #+72]
        UXTH     R0,R5
        ADDS     R1,R0,R1
        MOVW     R0,#+257
        CMP      R1,R0
        BGE.W    ??photo_storage_poll_6
        BL       ?Subroutine24
??CrossCallReturnLabel_64:
        BL       ?Subroutine17
??CrossCallReturnLabel_49:
        ADDS     R0,R0,#+44
          CFI FunCall w25_read_data
        BL       w25_read_data
        CMP      R0,#+0
        BEQ.N    ??photo_storage_poll_96
        MOVS     R1,#+0
        B.N      ??photo_storage_poll_97
??photo_storage_poll_98:
        ADDS     R1,R1,#+1
??photo_storage_poll_97:
        UXTH     R0,R1
        UXTH     R2,R5
        CMP      R0,R2
        BCS.N    ??photo_storage_poll_99
        ADD      R0,R9,R0
        LDRB     R0,[R0, #+61]
        CMP      R0,#+0
        BEQ.N    ??photo_storage_poll_98
        LDRH     R0,[R7, #+72]
        ADDS     R1,R1,R0
        STRH     R1,[R7, #+72]
        BL       ?Subroutine2
??CrossCallReturnLabel_8:
        MOVS     R0,#+30
??photo_storage_poll_100:
        B.N      ??photo_storage_poll_8
??photo_storage_poll_99:
        BL       ?Subroutine19
??CrossCallReturnLabel_53:
        LDR      R0,[R4, #+72]
        LDRH     R1,[R7, #+72]
        ADD      R0,R0,R1, LSL #+12
        LDR      R1,[R7, #+12]
        CMP      R0,R1
        BNE.W    ??photo_storage_poll_11
        MOVS     R1,#+33
        B.N      ??photo_storage_poll_23
??photo_storage_poll_101:
??jt0_29:
        MOVS     R1,#+31
        BL       ?Subroutine9
??CrossCallReturnLabel_27:
        LDRH     R2,[R7, #+72]
        ADD      R0,R0,R2, LSL #+12
        B.N      ??photo_storage_poll_24
??photo_storage_poll_102:
??jt0_30:
        BL       ?Subroutine8
??CrossCallReturnLabel_24:
        MOVS     R3,#+32
        MOVS     R2,#+1
        ADD      R1,R0,#+61
        LDRH     R0,[R7, #+72]
        ADDS     R6,R0,R6
        ADD      R6,R6,#+4096
        ADD      R0,R6,#+44
        B.N      ??photo_storage_poll_65
??photo_storage_poll_103:
??jt0_32:
        MOVS     R0,#+0
        STRB     R0,[R7, #+115]
        MOVS     R0,#+1
        STRB     R0,[R7, #+114]
        MOVS     R0,#+0
        STR      R0,[R7, #+52]
        BL       ?Subroutine9
??CrossCallReturnLabel_28:
        BL       ?Subroutine27
??CrossCallReturnLabel_70:
        LDRH     R1,[R9, #+129]
        MOVW     R0,#+57343
        ANDS     R1,R0,R1
        STRH     R1,[R9, #+129]
        B.N      ??photo_storage_poll_85
??photo_storage_poll_104:
??jt0_33:
        BL       ?Subroutine11
??CrossCallReturnLabel_32:
        CMP      R0,#+0
        ITT      NE
        LDRNE    R0,[R7, #+28]
        CMPNE    R0,#+0
??photo_storage_poll_96:
        BEQ.W    ??photo_storage_poll_6
        CMP      R0,#+65536
        BCS.W    ??photo_storage_poll_6
        LDR      R1,[R7, #+40]
        LDR      R0,[R7, #+8]
        CMP      R1,R0
        ITTTT    CS
        CMPCS    R6,R0
        SUBCS    R1,R6,R0
        MOVWCS   R0,#+1033
        CMPCS    R1,R0
        BCC.N    ??photo_storage_poll_105
        LDR      R0,[R7, #+8]
        SUBS     R6,R6,R0
        SUBW     R6,R6,#+1033
        LDR      R0,[R7, #+28]
        CMP      R6,R0
??photo_storage_poll_105:
        BCC.W    ??photo_storage_poll_6
        LDR      R0,[R7, #+8]
        LDR      R1,[R7, #+40]
        CMP      R0,R1
        BNE.N    ??photo_storage_poll_106
        LDRH     R0,[R7, #+96]
        LDRH     R1,[R7, #+90]
        CMP      R0,R1
        IT       CS
        MOVCS    R0,#+5
        BCS.W    ??photo_storage_poll_28
??photo_storage_poll_106:
        MOVS     R0,#+35
        B.N      ??photo_storage_poll_100
??photo_storage_poll_107:
??jt0_34:
        LDR      R0,[R7, #+8]
        LDR      R1,[R7, #+40]
        CMP      R0,R1
        BCC.N    ??photo_storage_poll_108
        LDRH     R0,[R7, #+90]
        LDRH     R1,[R7, #+94]
        CMP      R0,R1
        BNE.N    ??photo_storage_poll_109
??photo_storage_poll_108:
        BL       ?Subroutine8
??CrossCallReturnLabel_25:
        MOVS     R3,#+37
        MOVS     R2,#+1
        ADD      R1,R0,#+61
        LDR      R0,[R7, #+8]
        ADD      R0,R0,#+1032
        B.N      ??photo_storage_poll_65
??photo_storage_poll_109:
        STRH     R0,[SP, #+0]
        LDRH     R0,[R7, #+90]
        MVNS     R0,R0
        STRH     R0,[SP, #+2]
        MOVS     R3,#+36
        MOVS     R2,#+4
        MOV      R1,SP
        LDR      R0,[R7, #+44]
          CFI FunCall pr_write
        BL       pr_write
        B.N      ??photo_storage_poll_69
??photo_storage_poll_110:
??jt0_35:
        LDR.W    R4,??DataTable20
        MOVS     R1,#+4
        ADD      R0,R4,#+61
          CFI FunCall __aeabi_memclr
        BL       __aeabi_memclr
        MOVS     R3,#+37
        MOVS     R2,#+4
        ADD      R1,R4,#+61
        LDR      R0,[R7, #+44]
        B.N      ??photo_storage_poll_88
??photo_storage_poll_111:
??jt0_36:
        LDR      R0,[R7, #+8]
        LDR      R1,[R7, #+40]
        CMP      R0,R1
        IT       EQ
        MOVEQ    R0,#+5
        BEQ.W    ??photo_storage_poll_28
        MOV      R1,R0
        BL       ?Subroutine12
??CrossCallReturnLabel_36:
        STR      R1,[R7, #+8]
        B.N      ??photo_storage_poll_50
??photo_storage_poll_112:
??jt0_37:
        LDRB     R0,[R7, #+111]
        CBNZ.N   R0,??photo_storage_poll_113
        LDR      R0,[R4, #+0]
        LDRB     R0,[R0, #+50]
        CMP      R0,#+1
        BNE.N    ??photo_storage_poll_114
          CFI FunCall comm_tx_busy
        BL       comm_tx_busy
        CBZ.N    R0,??photo_storage_poll_113
        LDRB     R0,[R7, #+118]
        CBNZ.N   R0,??CrossCallReturnLabel_9
        BL       ?Subroutine3
??CrossCallReturnLabel_9:
        LDR      R0,[R7, #+64]
        SUBS     R5,R5,R0
        BMI.W    ??photo_storage_poll_11
        MOVS     R0,#+1
        STRB     R0,[R7, #+111]
        B.N      ??photo_storage_poll_85
??photo_storage_poll_113:
        LDRB     R0,[R7, #+111]
        CMP      R0,#+0
        BNE.W    ??photo_storage_poll_10
??photo_storage_poll_114:
        BL       ?Subroutine0
??CrossCallReturnLabel_1:
        MOVS     R1,#+0
        BL       ??Subroutine7_0
??CrossCallReturnLabel_20:
        LDR      R0,[R7, #+48]
        CBNZ.N   R0,??photo_storage_poll_115
        LDR      R0,[R9, #+157]
??photo_storage_poll_115:
        LDR      R1,[R4, #+0]
        STR      R0,[R1, #+20]
        LDR      R0,[R7, #+48]
        CBZ.N    R0,??photo_storage_poll_116
        LDRH     R0,[R7, #+92]
??photo_storage_poll_116:
        LDR      R1,[R4, #+0]
        STRH     R0,[R1, #+28]
        MOVS     R0,#+1
        LDR      R1,[R4, #+0]
        STRB     R0,[R1, #+38]
??photo_storage_poll_117:
        LDR.W    R0,??DataTable36
        STR      R5,[R0, #+0]
        B.N      ??photo_storage_poll_85
??photo_storage_poll_118:
??jt0_38:
        LDR      R0,[R7, #+0]
        CMP      R0,#+0
        BNE.N    ??CrossCallReturnLabel_14
        MOVW     R0,#+534
        MOVS R1,#WORK_PHOTO_PACKET
          CFI FunCall work_acquire
        BL       work_acquire
        STR      R0,[R7, #+0]
        CMP      R0,#+0
        BEQ.W    ??photo_storage_poll_10
        BL       ?Subroutine28
??CrossCallReturnLabel_71:
        ITE      NE
        ADDNE    R0,R0,#+14
        MOVEQ    R0,#+4
        LDR.W    R5,??DataTable36_1
        LDR      R1,[R7, #+0]
        LDR      R3,[R5, #+0]
        STR      R3,[R1, #+0]
        LDR      R1,[R7, #+0]
        STRB     R0,[R1, #+4]
        LSLS     R0,R0,#+16
        LSRS     R0,R0,#+24
        LDR      R1,[R7, #+0]
        STRB     R0,[R1, #+5]
        MOVS     R0,#+60
        LDR      R1,[R7, #+0]
        STRB     R0,[R1, #+6]
        LDR      R0,[R4, #+0]
        LDRB     R1,[R0, #+39]
        LDR      R0,[R7, #+0]
        STRB     R1,[R0, #+7]
        MOVS     R1,#+14
        LDR      R0,[R7, #+0]
        STRB     R1,[R0, #+8]
        LDRH     R0,[R7, #+98]
        CBZ.N    R0,??photo_storage_poll_119
        LDRH     R0,[R9, #+155]
        CMP      R0,#+256
        ITE      LT
        MOVLT    R1,R0
        MOVGE    R1,#+255
        B.N      ??photo_storage_poll_120
??photo_storage_poll_119:
        MOVS     R1,#+0
??photo_storage_poll_120:
        LDR      R0,[R7, #+0]
        STRB     R1,[R0, #+9]
        LDRH     R0,[R7, #+98]
        CBZ.N    R0,??photo_storage_poll_121
        ADD      R2,R7,#+24
        LDR      R0,[R7, #+0]
        LDR      R1,[R2, #+0]
        STR      R1,[R0, #+10]!
        LDR      R1,[R2, #+4]
        STR      R1,[R0, #+4]
        ADD      R2,R7,#+100
        LDR      R0,[R7, #+0]
        LDRB     R1,[R2, #+0]
        STRB     R1,[R0, #+18]!
        LDRB     R1,[R2, #+1]
        STRB     R1,[R0, #+1]
??photo_storage_poll_121:
        BL       ?Subroutine4
??CrossCallReturnLabel_14:
        LDRH     R4,[R7, #+98]
        BL       ??Subroutine14_0
??CrossCallReturnLabel_41:
        IT       GE
        MOVGE    R4,#+64
        BGE.N    ??photo_storage_poll_122
        CBZ.N    R0,??photo_storage_poll_123
??photo_storage_poll_122:
        UXTH     R2,R4
        LDR      R0,[R7, #+0]
        LDRH     R1,[R7, #+72]
        ADD      R0,R0,R1
        ADD      R1,R0,#+20
        LDR      R0,[R7, #+8]
        LDRH     R3,[R7, #+100]
        ADDS     R0,R3,R0
        LDRH     R3,[R7, #+72]
        ADDS     R0,R3,R0
        ADDW     R0,R0,#+1033
          CFI FunCall w25_read_data
        BL       w25_read_data
        CMP      R0,#+0
        BEQ.W    ??photo_storage_poll_6
??photo_storage_poll_123:
        LDRH     R0,[R7, #+72]
        ADDS     R4,R4,R0
        LDRH     R0,[R7, #+98]
        UXTH     R1,R4
        CMP      R1,R0
        BNE.W    ??photo_storage_poll_25
        MOVS     R4,#+0
        MOVW     R0,#+65535
        STRH     R0,[R7, #+102]
        MOVS     R0,#+40
        B.W      ??photo_storage_poll_26
??photo_storage_poll_124:
??jt0_39:
        BL       ?Subroutine28
??CrossCallReturnLabel_72:
        ITE      NE
        ADDNE    R4,R0,#+16
        MOVEQ    R4,#+6
        BL       ??Subroutine14_0
??CrossCallReturnLabel_42:
        IT       GE
        MOVGE    R4,#+64
        LDRH     R2,[R7, #+102]
        UXTH     R1,R4
        LDR      R3,[R7, #+0]
        LDRH     R0,[R7, #+72]
        ADD      R0,R3,R0
        ADDS     R0,R0,#+4
          CFI FunCall crc16_calcInit
        BL       crc16_calcInit
        STRH     R0,[R7, #+102]
        BL       ??Subroutine15_0
??CrossCallReturnLabel_45:
        BL       ?Subroutine28
??CrossCallReturnLabel_73:
        ITE      NE
        ADDNE    R1,R0,#+16
        MOVEQ    R1,#+6
        LDRH     R0,[R7, #+72]
        CMP      R0,R1
        BNE.W    ??photo_storage_poll_11
        ADD      R2,R7,#+102
        LDR      R1,[R7, #+0]
        ADD      R0,R1,R0
        LDRB     R3,[R2, #+0]
        STRB     R3,[R0, #+4]!
        LDRB     R3,[R2, #+1]
        STRB     R3,[R0, #+1]
        MOVS     R0,#+41
        B.N      ??photo_storage_poll_100
??photo_storage_poll_125:
??jt0_40:
        LDRB     R0,[R7, #+111]
        CMP      R0,#+0
        BNE.W    ??photo_storage_poll_10
        LDR      R0,[R4, #+0]
        LDRB     R0,[R0, #+38]
        CMP      R0,#+0
        BEQ.W    ??photo_storage_poll_10
        BL       ?Subroutine28
??CrossCallReturnLabel_74:
        ITE      NE
        ADDNE    R9,R0,#+22
        MOVEQ    R9,#+12
        MOVS     R6,#+0
        LDR      R0,[R4, #+0]
        LDRB     R2,[R0, #+50]
        UXTH     R1,R9
        LDR      R0,[R7, #+0]
          CFI FunCall reply_send
        BL       reply_send
        MOV      R6,R0
        CBNZ.N   R6,??photo_storage_poll_130
??photo_storage_poll_128:
        LDRB     R0,[R7, #+118]
        CBNZ.N   R0,??CrossCallReturnLabel_10
        BL       ?Subroutine3
??CrossCallReturnLabel_10:
        LDR      R0,[R7, #+64]
        SUBS     R5,R5,R0
        BMI.W    ??photo_storage_poll_11
        B.N      ??photo_storage_poll_85
??photo_storage_poll_130:
        LDRH     R0,[R7, #+98]
        CBZ.N    R0,??photo_storage_poll_131
        LDRH     R1,[R7, #+100]
        ADDS     R1,R0,R1
        LDR      R0,[R4, #+0]
        STRH     R1,[R0, #+28]
        LDR      R0,[R7, #+8]
        LDR      R1,[R4, #+0]
        STR      R0,[R1, #+20]
        LDR      R0,[R4, #+0]
        LDRH     R0,[R0, #+28]
        LDR      R1,[R7, #+28]
        CMP      R0,R1
        BNE.N    ??photo_storage_poll_132
        LDR      R0,[R4, #+0]
        LDR      R1,[R0, #+20]
        BL       ?Subroutine12
??CrossCallReturnLabel_37:
        LDR      R0,[R4, #+0]
        STR      R1,[R0, #+20]
        MOVS     R0,#+0
        LDR      R1,[R4, #+0]
        STRH     R0,[R1, #+28]
??photo_storage_poll_132:
        B.N      ??photo_storage_poll_117
??photo_storage_poll_131:
        LDR      R1,[R4, #+0]
        STRB     R0,[R1, #+38]
        B.N      ??photo_storage_poll_85
          CFI EndBlock cfiBlock16

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable15:
        DATA32
        DC32     pr

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable15_1:
        DATA32
        DC32     photo

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable15_2:
        DATA32
        DC32     usb_bus_resets

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond17 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_71
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond18 Using cfiCommon0
          CFI (cfiCond18) Function photo_storage_poll
          CFI (cfiCond18) Conditional ??CrossCallReturnLabel_72
          CFI (cfiCond18) R4 Frame(CFA, -28)
          CFI (cfiCond18) R5 Frame(CFA, -24)
          CFI (cfiCond18) R6 Frame(CFA, -20)
          CFI (cfiCond18) R7 Frame(CFA, -16)
          CFI (cfiCond18) R8 Frame(CFA, -12)
          CFI (cfiCond18) R9 Frame(CFA, -8)
          CFI (cfiCond18) R14 Frame(CFA, -4)
          CFI (cfiCond18) CFA R13+32
          CFI Block cfiCond19 Using cfiCommon0
          CFI (cfiCond19) Function photo_storage_poll
          CFI (cfiCond19) Conditional ??CrossCallReturnLabel_73
          CFI (cfiCond19) R4 Frame(CFA, -28)
          CFI (cfiCond19) R5 Frame(CFA, -24)
          CFI (cfiCond19) R6 Frame(CFA, -20)
          CFI (cfiCond19) R7 Frame(CFA, -16)
          CFI (cfiCond19) R8 Frame(CFA, -12)
          CFI (cfiCond19) R9 Frame(CFA, -8)
          CFI (cfiCond19) R14 Frame(CFA, -4)
          CFI (cfiCond19) CFA R13+32
          CFI Block cfiCond20 Using cfiCommon0
          CFI (cfiCond20) Function photo_storage_poll
          CFI (cfiCond20) Conditional ??CrossCallReturnLabel_74
          CFI (cfiCond20) R4 Frame(CFA, -28)
          CFI (cfiCond20) R5 Frame(CFA, -24)
          CFI (cfiCond20) R6 Frame(CFA, -20)
          CFI (cfiCond20) R7 Frame(CFA, -16)
          CFI (cfiCond20) R8 Frame(CFA, -12)
          CFI (cfiCond20) R9 Frame(CFA, -8)
          CFI (cfiCond20) R14 Frame(CFA, -4)
          CFI (cfiCond20) CFA R13+32
          CFI Block cfiPicker21 Using cfiCommon1
          CFI (cfiPicker21) NoFunction
          CFI (cfiPicker21) Picker
        THUMB
?Subroutine28:
        LDRH     R0,[R7, #+98]
        CMP      R0,#+0
        BX       LR
          CFI EndBlock cfiCond17
          CFI EndBlock cfiCond18
          CFI EndBlock cfiCond19
          CFI EndBlock cfiCond20
          CFI EndBlock cfiPicker21

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond22 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_69
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond23 Using cfiCommon0
          CFI (cfiCond23) Function photo_storage_poll
          CFI (cfiCond23) Conditional ??CrossCallReturnLabel_70
          CFI (cfiCond23) R4 Frame(CFA, -28)
          CFI (cfiCond23) R5 Frame(CFA, -24)
          CFI (cfiCond23) R6 Frame(CFA, -20)
          CFI (cfiCond23) R7 Frame(CFA, -16)
          CFI (cfiCond23) R8 Frame(CFA, -12)
          CFI (cfiCond23) R9 Frame(CFA, -8)
          CFI (cfiCond23) R14 Frame(CFA, -4)
          CFI (cfiCond23) CFA R13+32
          CFI Block cfiPicker24 Using cfiCommon1
          CFI (cfiPicker24) NoFunction
          CFI (cfiPicker24) Picker
        THUMB
?Subroutine27:
        STR      R0,[R9, #+157]
        MOVS     R0,#+0
        STRH     R0,[R9, #+155]
        BX       LR
          CFI EndBlock cfiCond22
          CFI EndBlock cfiCond23
          CFI EndBlock cfiPicker24

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond25 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_67
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond26 Using cfiCommon0
          CFI (cfiCond26) Function photo_storage_poll
          CFI (cfiCond26) Conditional ??CrossCallReturnLabel_68
          CFI (cfiCond26) R4 Frame(CFA, -28)
          CFI (cfiCond26) R5 Frame(CFA, -24)
          CFI (cfiCond26) R6 Frame(CFA, -20)
          CFI (cfiCond26) R7 Frame(CFA, -16)
          CFI (cfiCond26) R8 Frame(CFA, -12)
          CFI (cfiCond26) R9 Frame(CFA, -8)
          CFI (cfiCond26) R14 Frame(CFA, -4)
          CFI (cfiCond26) CFA R13+32
          CFI Block cfiPicker27 Using cfiCommon1
          CFI (cfiPicker27) NoFunction
          CFI (cfiPicker27) Picker
        THUMB
?Subroutine26:
        LDRH     R1,[R7, #+72]
        LDRH     R0,[R7, #+104]
        ADDS     R0,R0,R1
        BX       LR
          CFI EndBlock cfiCond25
          CFI EndBlock cfiCond26
          CFI EndBlock cfiPicker27

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond28 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_66
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond29 Using cfiCommon0
          CFI (cfiCond29) Function photo_storage_poll
          CFI (cfiCond29) Conditional ??CrossCallReturnLabel_65
          CFI (cfiCond29) R4 Frame(CFA, -28)
          CFI (cfiCond29) R5 Frame(CFA, -24)
          CFI (cfiCond29) R6 Frame(CFA, -20)
          CFI (cfiCond29) R7 Frame(CFA, -16)
          CFI (cfiCond29) R8 Frame(CFA, -12)
          CFI (cfiCond29) R9 Frame(CFA, -8)
          CFI (cfiCond29) R14 Frame(CFA, -4)
          CFI (cfiCond29) CFA R13+32
          CFI Block cfiPicker30 Using cfiCommon1
          CFI (cfiPicker30) NoFunction
          CFI (cfiPicker30) Picker
        THUMB
?Subroutine25:
        UXTH     R2,R5
        ADD      R1,R9,#+61
        LDR      R0,[R7, #+16]
          CFI (cfiCond28) FunCall photo_storage_poll crc32_update
          CFI (cfiCond28) FunCall photo_storage_poll crc32_update
        B.W      crc32_update
          CFI EndBlock cfiCond28
          CFI EndBlock cfiCond29
          CFI EndBlock cfiPicker30

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond31 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_62
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond32 Using cfiCommon0
          CFI (cfiCond32) Function photo_storage_poll
          CFI (cfiCond32) Conditional ??CrossCallReturnLabel_63
          CFI (cfiCond32) R4 Frame(CFA, -28)
          CFI (cfiCond32) R5 Frame(CFA, -24)
          CFI (cfiCond32) R6 Frame(CFA, -20)
          CFI (cfiCond32) R7 Frame(CFA, -16)
          CFI (cfiCond32) R8 Frame(CFA, -12)
          CFI (cfiCond32) R9 Frame(CFA, -8)
          CFI (cfiCond32) R14 Frame(CFA, -4)
          CFI (cfiCond32) CFA R13+32
          CFI Block cfiCond33 Using cfiCommon0
          CFI (cfiCond33) Function photo_storage_poll
          CFI (cfiCond33) Conditional ??CrossCallReturnLabel_64
          CFI (cfiCond33) R4 Frame(CFA, -28)
          CFI (cfiCond33) R5 Frame(CFA, -24)
          CFI (cfiCond33) R6 Frame(CFA, -20)
          CFI (cfiCond33) R7 Frame(CFA, -16)
          CFI (cfiCond33) R8 Frame(CFA, -12)
          CFI (cfiCond33) R9 Frame(CFA, -8)
          CFI (cfiCond33) R14 Frame(CFA, -4)
          CFI (cfiCond33) CFA R13+32
          CFI Block cfiPicker34 Using cfiCommon1
          CFI (cfiPicker34) NoFunction
          CFI (cfiPicker34) Picker
        THUMB
?Subroutine24:
        LDR.W    R9,??DataTable20
        UXTH     R2,R5
        ADD      R1,R9,#+61
        BX       LR
          CFI EndBlock cfiCond31
          CFI EndBlock cfiCond32
          CFI EndBlock cfiCond33
          CFI EndBlock cfiPicker34

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable16:
        DATA32
        DC32     device

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond35 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_60
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond36 Using cfiCommon0
          CFI (cfiCond36) Function photo_storage_poll
          CFI (cfiCond36) Conditional ??CrossCallReturnLabel_61
          CFI (cfiCond36) R4 Frame(CFA, -28)
          CFI (cfiCond36) R5 Frame(CFA, -24)
          CFI (cfiCond36) R6 Frame(CFA, -20)
          CFI (cfiCond36) R7 Frame(CFA, -16)
          CFI (cfiCond36) R8 Frame(CFA, -12)
          CFI (cfiCond36) R9 Frame(CFA, -8)
          CFI (cfiCond36) R14 Frame(CFA, -4)
          CFI (cfiCond36) CFA R13+32
          CFI Block cfiPicker37 Using cfiCommon1
          CFI (cfiPicker37) NoFunction
          CFI (cfiPicker37) Picker
        THUMB
?Subroutine23:
        LDR.N    R5,??DataTable20
        UXTH     R2,R4
        ADD      R1,R5,#+61
        BX       LR
          CFI EndBlock cfiCond35
          CFI EndBlock cfiCond36
          CFI EndBlock cfiPicker37

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond38 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_59
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond39 Using cfiCommon0
          CFI (cfiCond39) Function photo_storage_poll
          CFI (cfiCond39) Conditional ??CrossCallReturnLabel_58
          CFI (cfiCond39) R4 Frame(CFA, -28)
          CFI (cfiCond39) R5 Frame(CFA, -24)
          CFI (cfiCond39) R6 Frame(CFA, -20)
          CFI (cfiCond39) R7 Frame(CFA, -16)
          CFI (cfiCond39) R8 Frame(CFA, -12)
          CFI (cfiCond39) R9 Frame(CFA, -8)
          CFI (cfiCond39) R14 Frame(CFA, -4)
          CFI (cfiCond39) CFA R13+32
          CFI Block cfiPicker40 Using cfiCommon1
          CFI (cfiPicker40) NoFunction
          CFI (cfiPicker40) Picker
        THUMB
?Subroutine22:
        UXTH     R2,R4
        ADD      R1,R5,#+61
        LDR      R0,[R7, #+16]
          CFI (cfiCond38) FunCall photo_storage_poll crc32_update
          CFI (cfiCond38) FunCall photo_storage_poll crc32_update
        B.W      crc32_update
          CFI EndBlock cfiCond38
          CFI EndBlock cfiCond39
          CFI EndBlock cfiPicker40

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond41 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_52
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond42 Using cfiCommon0
          CFI (cfiCond42) Function photo_storage_poll
          CFI (cfiCond42) Conditional ??CrossCallReturnLabel_53
          CFI (cfiCond42) R4 Frame(CFA, -28)
          CFI (cfiCond42) R5 Frame(CFA, -24)
          CFI (cfiCond42) R6 Frame(CFA, -20)
          CFI (cfiCond42) R7 Frame(CFA, -16)
          CFI (cfiCond42) R8 Frame(CFA, -12)
          CFI (cfiCond42) R9 Frame(CFA, -8)
          CFI (cfiCond42) R14 Frame(CFA, -4)
          CFI (cfiCond42) CFA R13+32
          CFI Block cfiPicker43 Using cfiCommon1
          CFI (cfiPicker43) NoFunction
          CFI (cfiPicker43) Picker
        THUMB
?Subroutine19:
        LDRH     R0,[R7, #+72]
        ADDS     R5,R5,R0
        STRH     R5,[R7, #+72]
        BX       LR
          CFI EndBlock cfiCond41
          CFI EndBlock cfiCond42
          CFI EndBlock cfiPicker43

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond44 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_50
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond45 Using cfiCommon0
          CFI (cfiCond45) Function photo_storage_poll
          CFI (cfiCond45) Conditional ??CrossCallReturnLabel_51
          CFI (cfiCond45) R4 Frame(CFA, -28)
          CFI (cfiCond45) R5 Frame(CFA, -24)
          CFI (cfiCond45) R6 Frame(CFA, -20)
          CFI (cfiCond45) R7 Frame(CFA, -16)
          CFI (cfiCond45) R8 Frame(CFA, -12)
          CFI (cfiCond45) R9 Frame(CFA, -8)
          CFI (cfiCond45) R14 Frame(CFA, -4)
          CFI (cfiCond45) CFA R13+32
          CFI Block cfiPicker46 Using cfiCommon1
          CFI (cfiPicker46) NoFunction
          CFI (cfiPicker46) Picker
        THUMB
?Subroutine18:
        LDRH     R5,[R7, #+80]
        LDRH     R0,[R7, #+72]
        SUBS     R5,R5,R0
        UXTH     R0,R5
        CMP      R0,#+65
        BX       LR
          CFI EndBlock cfiCond44
          CFI EndBlock cfiCond45
          CFI EndBlock cfiPicker46

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond47 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_48
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond48 Using cfiCommon0
          CFI (cfiCond48) Function photo_storage_poll
          CFI (cfiCond48) Conditional ??CrossCallReturnLabel_49
          CFI (cfiCond48) R4 Frame(CFA, -28)
          CFI (cfiCond48) R5 Frame(CFA, -24)
          CFI (cfiCond48) R6 Frame(CFA, -20)
          CFI (cfiCond48) R7 Frame(CFA, -16)
          CFI (cfiCond48) R8 Frame(CFA, -12)
          CFI (cfiCond48) R9 Frame(CFA, -8)
          CFI (cfiCond48) R14 Frame(CFA, -4)
          CFI (cfiCond48) CFA R13+32
          CFI Block cfiPicker49 Using cfiCommon1
          CFI (cfiPicker49) NoFunction
          CFI (cfiPicker49) Picker
        THUMB
?Subroutine17:
        LDRH     R0,[R7, #+72]
        ADDS     R0,R0,R6
        ADD      R0,R0,#+4096
        BX       LR
          CFI EndBlock cfiCond47
          CFI EndBlock cfiCond48
          CFI EndBlock cfiPicker49

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond50 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_46
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond51 Using cfiCommon0
          CFI (cfiCond51) Function photo_storage_poll
          CFI (cfiCond51) Conditional ??CrossCallReturnLabel_47
          CFI (cfiCond51) R4 Frame(CFA, -28)
          CFI (cfiCond51) R5 Frame(CFA, -24)
          CFI (cfiCond51) R6 Frame(CFA, -20)
          CFI (cfiCond51) R7 Frame(CFA, -16)
          CFI (cfiCond51) R8 Frame(CFA, -12)
          CFI (cfiCond51) R9 Frame(CFA, -8)
          CFI (cfiCond51) R14 Frame(CFA, -4)
          CFI (cfiCond51) CFA R13+32
          CFI Block cfiPicker52 Using cfiCommon1
          CFI (cfiPicker52) NoFunction
          CFI (cfiPicker52) Picker
        THUMB
?Subroutine16:
        LDR.N    R4,??DataTable20
        MOVS     R2,#+64
        ADD      R1,R4,#+61
        BX       LR
          CFI EndBlock cfiCond50
          CFI EndBlock cfiCond51
          CFI EndBlock cfiPicker52

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond53 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_43
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond54 Using cfiCommon0
          CFI (cfiCond54) Function photo_storage_poll
          CFI (cfiCond54) Conditional ??CrossCallReturnLabel_44
          CFI (cfiCond54) R4 Frame(CFA, -28)
          CFI (cfiCond54) R5 Frame(CFA, -24)
          CFI (cfiCond54) R6 Frame(CFA, -20)
          CFI (cfiCond54) R7 Frame(CFA, -16)
          CFI (cfiCond54) R8 Frame(CFA, -12)
          CFI (cfiCond54) R9 Frame(CFA, -8)
          CFI (cfiCond54) R14 Frame(CFA, -4)
          CFI (cfiCond54) CFA R13+32
          CFI Block cfiPicker55 Using cfiCommon1
          CFI (cfiPicker55) NoFunction
          CFI (cfiPicker55) Picker
        THUMB
?Subroutine15:
        STR      R0,[R7, #+16]
          CFI Block cfiCond56 Using cfiCommon0
          CFI (cfiCond56) Function photo_storage_poll
          CFI (cfiCond56) Conditional ??CrossCallReturnLabel_45
          CFI (cfiCond56) R4 Frame(CFA, -28)
          CFI (cfiCond56) R5 Frame(CFA, -24)
          CFI (cfiCond56) R6 Frame(CFA, -20)
          CFI (cfiCond56) R7 Frame(CFA, -16)
          CFI (cfiCond56) R8 Frame(CFA, -12)
          CFI (cfiCond56) R9 Frame(CFA, -8)
          CFI (cfiCond56) R14 Frame(CFA, -4)
          CFI (cfiCond56) CFA R13+32
??Subroutine15_0:
        LDRH     R0,[R7, #+72]
        ADDS     R4,R4,R0
        STRH     R4,[R7, #+72]
        BX       LR
          CFI EndBlock cfiCond53
          CFI EndBlock cfiCond54
          CFI EndBlock cfiPicker55
          CFI EndBlock cfiCond56

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond57 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_38
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond58 Using cfiCommon0
          CFI (cfiCond58) Function photo_storage_poll
          CFI (cfiCond58) Conditional ??CrossCallReturnLabel_39
          CFI (cfiCond58) R4 Frame(CFA, -28)
          CFI (cfiCond58) R5 Frame(CFA, -24)
          CFI (cfiCond58) R6 Frame(CFA, -20)
          CFI (cfiCond58) R7 Frame(CFA, -16)
          CFI (cfiCond58) R8 Frame(CFA, -12)
          CFI (cfiCond58) R9 Frame(CFA, -8)
          CFI (cfiCond58) R14 Frame(CFA, -4)
          CFI (cfiCond58) CFA R13+32
          CFI Block cfiCond59 Using cfiCommon0
          CFI (cfiCond59) Function photo_storage_poll
          CFI (cfiCond59) Conditional ??CrossCallReturnLabel_40
          CFI (cfiCond59) R4 Frame(CFA, -28)
          CFI (cfiCond59) R5 Frame(CFA, -24)
          CFI (cfiCond59) R6 Frame(CFA, -20)
          CFI (cfiCond59) R7 Frame(CFA, -16)
          CFI (cfiCond59) R8 Frame(CFA, -12)
          CFI (cfiCond59) R9 Frame(CFA, -8)
          CFI (cfiCond59) R14 Frame(CFA, -4)
          CFI (cfiCond59) CFA R13+32
          CFI Block cfiPicker60 Using cfiCommon1
          CFI (cfiPicker60) NoFunction
          CFI (cfiPicker60) Picker
        THUMB
?Subroutine14:
        LDR      R4,[R7, #+28]
          CFI Block cfiCond61 Using cfiCommon0
          CFI (cfiCond61) Function photo_storage_poll
          CFI (cfiCond61) Conditional ??CrossCallReturnLabel_41
          CFI (cfiCond61) R4 Frame(CFA, -28)
          CFI (cfiCond61) R5 Frame(CFA, -24)
          CFI (cfiCond61) R6 Frame(CFA, -20)
          CFI (cfiCond61) R7 Frame(CFA, -16)
          CFI (cfiCond61) R8 Frame(CFA, -12)
          CFI (cfiCond61) R9 Frame(CFA, -8)
          CFI (cfiCond61) R14 Frame(CFA, -4)
          CFI (cfiCond61) CFA R13+32
          CFI Block cfiCond62 Using cfiCommon0
          CFI (cfiCond62) Function photo_storage_poll
          CFI (cfiCond62) Conditional ??CrossCallReturnLabel_42
          CFI (cfiCond62) R4 Frame(CFA, -28)
          CFI (cfiCond62) R5 Frame(CFA, -24)
          CFI (cfiCond62) R6 Frame(CFA, -20)
          CFI (cfiCond62) R7 Frame(CFA, -16)
          CFI (cfiCond62) R8 Frame(CFA, -12)
          CFI (cfiCond62) R9 Frame(CFA, -8)
          CFI (cfiCond62) R14 Frame(CFA, -4)
          CFI (cfiCond62) CFA R13+32
??Subroutine14_0:
        LDRH     R0,[R7, #+72]
        SUBS     R4,R4,R0
        UXTH     R0,R4
        CMP      R0,#+65
        BX       LR
          CFI EndBlock cfiCond57
          CFI EndBlock cfiCond58
          CFI EndBlock cfiCond59
          CFI EndBlock cfiPicker60
          CFI EndBlock cfiCond61
          CFI EndBlock cfiCond62

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond63 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_81
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond64 Using cfiCommon0
          CFI (cfiCond64) Function photo_storage_poll
          CFI (cfiCond64) Conditional ??CrossCallReturnLabel_80
          CFI (cfiCond64) R4 Frame(CFA, -28)
          CFI (cfiCond64) R5 Frame(CFA, -24)
          CFI (cfiCond64) R6 Frame(CFA, -20)
          CFI (cfiCond64) R7 Frame(CFA, -16)
          CFI (cfiCond64) R8 Frame(CFA, -12)
          CFI (cfiCond64) R9 Frame(CFA, -8)
          CFI (cfiCond64) R14 Frame(CFA, -4)
          CFI (cfiCond64) CFA R13+32
          CFI Block cfiPicker65 Using cfiCommon1
          CFI (cfiPicker65) NoFunction
          CFI (cfiPicker65) Picker
        THUMB
?Subroutine13:
        LDR.N    R5,??DataTable20
        UXTH     R2,R4
        ADD      R1,R5,#+61
          CFI EndBlock cfiCond63
          CFI EndBlock cfiCond64
          CFI EndBlock cfiPicker65
        REQUIRE ??Subroutine30_0

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond66 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_79
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond67 Using cfiCommon0
          CFI (cfiCond67) Function photo_storage_poll
          CFI (cfiCond67) Conditional ??CrossCallReturnLabel_81
          CFI (cfiCond67) R4 Frame(CFA, -28)
          CFI (cfiCond67) R5 Frame(CFA, -24)
          CFI (cfiCond67) R6 Frame(CFA, -20)
          CFI (cfiCond67) R7 Frame(CFA, -16)
          CFI (cfiCond67) R8 Frame(CFA, -12)
          CFI (cfiCond67) R9 Frame(CFA, -8)
          CFI (cfiCond67) R14 Frame(CFA, -4)
          CFI (cfiCond67) CFA R13+32
          CFI Block cfiCond68 Using cfiCommon0
          CFI (cfiCond68) Function photo_storage_poll
          CFI (cfiCond68) Conditional ??CrossCallReturnLabel_80
          CFI (cfiCond68) R4 Frame(CFA, -28)
          CFI (cfiCond68) R5 Frame(CFA, -24)
          CFI (cfiCond68) R6 Frame(CFA, -20)
          CFI (cfiCond68) R7 Frame(CFA, -16)
          CFI (cfiCond68) R8 Frame(CFA, -12)
          CFI (cfiCond68) R9 Frame(CFA, -8)
          CFI (cfiCond68) R14 Frame(CFA, -4)
          CFI (cfiCond68) CFA R13+32
          CFI Block cfiPicker69 Using cfiCommon1
          CFI (cfiPicker69) NoFunction
          CFI (cfiPicker69) Picker
        THUMB
??Subroutine30_0:
        LDRH     R0,[R7, #+72]
        ADDS     R0,R0,R6
          CFI (cfiCond66) FunCall photo_storage_poll w25_read_data
          CFI (cfiCond66) FunCall photo_storage_poll w25_read_data
          CFI (cfiCond66) FunCall photo_storage_poll w25_read_data
        B.W      w25_read_data
          CFI EndBlock cfiCond66
          CFI EndBlock cfiCond67
          CFI EndBlock cfiCond68
          CFI EndBlock cfiPicker69

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond70 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_34
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond71 Using cfiCommon0
          CFI (cfiCond71) Function photo_storage_poll
          CFI (cfiCond71) Conditional ??CrossCallReturnLabel_36
          CFI (cfiCond71) R4 Frame(CFA, -28)
          CFI (cfiCond71) R5 Frame(CFA, -24)
          CFI (cfiCond71) R6 Frame(CFA, -20)
          CFI (cfiCond71) R7 Frame(CFA, -16)
          CFI (cfiCond71) R8 Frame(CFA, -12)
          CFI (cfiCond71) R9 Frame(CFA, -8)
          CFI (cfiCond71) R14 Frame(CFA, -4)
          CFI (cfiCond71) CFA R13+32
          CFI Block cfiCond72 Using cfiCommon0
          CFI (cfiCond72) Function photo_storage_poll
          CFI (cfiCond72) Conditional ??CrossCallReturnLabel_37
          CFI (cfiCond72) R4 Frame(CFA, -28)
          CFI (cfiCond72) R5 Frame(CFA, -24)
          CFI (cfiCond72) R6 Frame(CFA, -20)
          CFI (cfiCond72) R7 Frame(CFA, -16)
          CFI (cfiCond72) R8 Frame(CFA, -12)
          CFI (cfiCond72) R9 Frame(CFA, -8)
          CFI (cfiCond72) R14 Frame(CFA, -4)
          CFI (cfiCond72) CFA R13+32
          CFI Block cfiPicker73 Using cfiCommon1
          CFI (cfiPicker73) NoFunction
          CFI (cfiPicker73) Picker
        THUMB
?Subroutine12:
        LDR      R0,[R7, #+28]
          CFI Block cfiCond74 Using cfiCommon0
          CFI (cfiCond74) Function photo_storage_poll
          CFI (cfiCond74) Conditional ??CrossCallReturnLabel_35
          CFI (cfiCond74) R4 Frame(CFA, -28)
          CFI (cfiCond74) R5 Frame(CFA, -24)
          CFI (cfiCond74) R6 Frame(CFA, -20)
          CFI (cfiCond74) R7 Frame(CFA, -16)
          CFI (cfiCond74) R8 Frame(CFA, -12)
          CFI (cfiCond74) R9 Frame(CFA, -8)
          CFI (cfiCond74) R14 Frame(CFA, -4)
          CFI (cfiCond74) CFA R13+32
??Subroutine12_0:
        ADDS     R1,R0,R1
        ADDW     R1,R1,#+1033
        BX       LR
          CFI EndBlock cfiCond70
          CFI EndBlock cfiCond71
          CFI EndBlock cfiCond72
          CFI EndBlock cfiPicker73
          CFI EndBlock cfiCond74

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond75 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_33
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond76 Using cfiCommon0
          CFI (cfiCond76) Function photo_storage_poll
          CFI (cfiCond76) Conditional ??CrossCallReturnLabel_32
          CFI (cfiCond76) R4 Frame(CFA, -28)
          CFI (cfiCond76) R5 Frame(CFA, -24)
          CFI (cfiCond76) R6 Frame(CFA, -20)
          CFI (cfiCond76) R7 Frame(CFA, -16)
          CFI (cfiCond76) R8 Frame(CFA, -12)
          CFI (cfiCond76) R9 Frame(CFA, -8)
          CFI (cfiCond76) R14 Frame(CFA, -4)
          CFI (cfiCond76) CFA R13+32
          CFI Block cfiPicker77 Using cfiCommon1
          CFI (cfiPicker77) NoFunction
          CFI (cfiPicker77) Picker
        THUMB
?Subroutine11:
        MOVS     R2,#+8
        ADD      R1,R7,#+24
        LDR      R0,[R7, #+8]
          CFI (cfiCond75) FunCall photo_storage_poll w25_read_data
          CFI (cfiCond75) FunCall photo_storage_poll w25_read_data
        B.W      w25_read_data
          CFI EndBlock cfiCond75
          CFI EndBlock cfiCond76
          CFI EndBlock cfiPicker77

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond78 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_24
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond79 Using cfiCommon0
          CFI (cfiCond79) Function photo_storage_poll
          CFI (cfiCond79) Conditional ??CrossCallReturnLabel_25
          CFI (cfiCond79) R4 Frame(CFA, -28)
          CFI (cfiCond79) R5 Frame(CFA, -24)
          CFI (cfiCond79) R6 Frame(CFA, -20)
          CFI (cfiCond79) R7 Frame(CFA, -16)
          CFI (cfiCond79) R8 Frame(CFA, -12)
          CFI (cfiCond79) R9 Frame(CFA, -8)
          CFI (cfiCond79) R14 Frame(CFA, -4)
          CFI (cfiCond79) CFA R13+32
          CFI Block cfiPicker80 Using cfiCommon1
          CFI (cfiPicker80) NoFunction
          CFI (cfiPicker80) Picker
        THUMB
?Subroutine8:
        LDR.N    R0,??DataTable20
        MOVS     R1,#+0
        STRB     R1,[R0, #+61]
        BX       LR
          CFI EndBlock cfiCond78
          CFI EndBlock cfiCond79
          CFI EndBlock cfiPicker80

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable20:
        DATA32
        DC32     sjr

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable20_1:
        DATA32
        DC32     mem

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable20_2:
        DATA32
        DC32     0x50484f54

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond81 Using cfiCommon0
          CFI Function pr_scan_finish
          CFI Conditional ??CrossCallReturnLabel_23
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI Block cfiCond82 Using cfiCommon0
          CFI (cfiCond82) Function photo_storage_poll
          CFI (cfiCond82) Conditional ??CrossCallReturnLabel_22
          CFI (cfiCond82) R4 Frame(CFA, -28)
          CFI (cfiCond82) R5 Frame(CFA, -24)
          CFI (cfiCond82) R6 Frame(CFA, -20)
          CFI (cfiCond82) R7 Frame(CFA, -16)
          CFI (cfiCond82) R8 Frame(CFA, -12)
          CFI (cfiCond82) R9 Frame(CFA, -8)
          CFI (cfiCond82) R14 Frame(CFA, -4)
          CFI (cfiCond82) CFA R13+32
          CFI Block cfiPicker83 Using cfiCommon1
          CFI (cfiPicker83) NoFunction
          CFI (cfiPicker83) Picker
        THUMB
?Subroutine7:
        MOVS     R1,#+1
          CFI Block cfiCond84 Using cfiCommon0
          CFI (cfiCond84) Function pr_fail
          CFI (cfiCond84) Conditional ??CrossCallReturnLabel_21
          CFI (cfiCond84) R4 Frame(CFA, -12)
          CFI (cfiCond84) R5 Frame(CFA, -8)
          CFI (cfiCond84) R14 Frame(CFA, -4)
          CFI (cfiCond84) CFA R13+16
          CFI Block cfiCond85 Using cfiCommon0
          CFI (cfiCond85) Function photo_storage_poll
          CFI (cfiCond85) Conditional ??CrossCallReturnLabel_20
          CFI (cfiCond85) R4 Frame(CFA, -28)
          CFI (cfiCond85) R5 Frame(CFA, -24)
          CFI (cfiCond85) R6 Frame(CFA, -20)
          CFI (cfiCond85) R7 Frame(CFA, -16)
          CFI (cfiCond85) R8 Frame(CFA, -12)
          CFI (cfiCond85) R9 Frame(CFA, -8)
          CFI (cfiCond85) R14 Frame(CFA, -4)
          CFI (cfiCond85) CFA R13+32
??Subroutine7_0:
        MOVS     R0,#+21
          CFI (cfiCond81) FunCall pr_scan_finish user_send_answer_comand
          CFI (cfiCond82) FunCall photo_storage_poll user_send_answer_comand
          CFI (cfiCond84) FunCall pr_fail user_send_answer_comand
          CFI (cfiCond82) FunCall photo_storage_poll user_send_answer_comand
        B.W      user_send_answer_comand
          CFI EndBlock cfiCond81
          CFI EndBlock cfiCond82
          CFI EndBlock cfiPicker83
          CFI EndBlock cfiCond84
          CFI EndBlock cfiCond85

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond86 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_11
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond87 Using cfiCommon0
          CFI (cfiCond87) Function photo_storage_poll
          CFI (cfiCond87) Conditional ??CrossCallReturnLabel_12
          CFI (cfiCond87) R4 Frame(CFA, -28)
          CFI (cfiCond87) R5 Frame(CFA, -24)
          CFI (cfiCond87) R6 Frame(CFA, -20)
          CFI (cfiCond87) R7 Frame(CFA, -16)
          CFI (cfiCond87) R8 Frame(CFA, -12)
          CFI (cfiCond87) R9 Frame(CFA, -8)
          CFI (cfiCond87) R14 Frame(CFA, -4)
          CFI (cfiCond87) CFA R13+32
          CFI Block cfiCond88 Using cfiCommon0
          CFI (cfiCond88) Function photo_storage_poll
          CFI (cfiCond88) Conditional ??CrossCallReturnLabel_13
          CFI (cfiCond88) R4 Frame(CFA, -28)
          CFI (cfiCond88) R5 Frame(CFA, -24)
          CFI (cfiCond88) R6 Frame(CFA, -20)
          CFI (cfiCond88) R7 Frame(CFA, -16)
          CFI (cfiCond88) R8 Frame(CFA, -12)
          CFI (cfiCond88) R9 Frame(CFA, -8)
          CFI (cfiCond88) R14 Frame(CFA, -4)
          CFI (cfiCond88) CFA R13+32
          CFI Block cfiCond89 Using cfiCommon0
          CFI (cfiCond89) Function photo_storage_poll
          CFI (cfiCond89) Conditional ??CrossCallReturnLabel_14
          CFI (cfiCond89) R4 Frame(CFA, -28)
          CFI (cfiCond89) R5 Frame(CFA, -24)
          CFI (cfiCond89) R6 Frame(CFA, -20)
          CFI (cfiCond89) R7 Frame(CFA, -16)
          CFI (cfiCond89) R8 Frame(CFA, -12)
          CFI (cfiCond89) R9 Frame(CFA, -8)
          CFI (cfiCond89) R14 Frame(CFA, -4)
          CFI (cfiCond89) CFA R13+32
          CFI Block cfiPicker90 Using cfiCommon1
          CFI (cfiPicker90) NoFunction
          CFI (cfiPicker90) Picker
        THUMB
?Subroutine4:
        MOVS     R0,#+0
        STRH     R0,[R7, #+72]
        BX       LR
          CFI EndBlock cfiCond86
          CFI EndBlock cfiCond87
          CFI EndBlock cfiCond88
          CFI EndBlock cfiCond89
          CFI EndBlock cfiPicker90

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond91 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_9
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond92 Using cfiCommon0
          CFI (cfiCond92) Function photo_storage_poll
          CFI (cfiCond92) Conditional ??CrossCallReturnLabel_10
          CFI (cfiCond92) R4 Frame(CFA, -28)
          CFI (cfiCond92) R5 Frame(CFA, -24)
          CFI (cfiCond92) R6 Frame(CFA, -20)
          CFI (cfiCond92) R7 Frame(CFA, -16)
          CFI (cfiCond92) R8 Frame(CFA, -12)
          CFI (cfiCond92) R9 Frame(CFA, -8)
          CFI (cfiCond92) R14 Frame(CFA, -4)
          CFI (cfiCond92) CFA R13+32
          CFI Block cfiPicker93 Using cfiCommon1
          CFI (cfiPicker93) NoFunction
          CFI (cfiPicker93) Picker
        THUMB
?Subroutine3:
        MOVS     R0,#+1
        STRB     R0,[R7, #+118]
        ADD      R0,R5,#+2000
        STR      R0,[R7, #+64]
        BX       LR
          CFI EndBlock cfiCond91
          CFI EndBlock cfiCond92
          CFI EndBlock cfiPicker93

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond94 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_4
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond95 Using cfiCommon0
          CFI (cfiCond95) Function photo_storage_poll
          CFI (cfiCond95) Conditional ??CrossCallReturnLabel_5
          CFI (cfiCond95) R4 Frame(CFA, -28)
          CFI (cfiCond95) R5 Frame(CFA, -24)
          CFI (cfiCond95) R6 Frame(CFA, -20)
          CFI (cfiCond95) R7 Frame(CFA, -16)
          CFI (cfiCond95) R8 Frame(CFA, -12)
          CFI (cfiCond95) R9 Frame(CFA, -8)
          CFI (cfiCond95) R14 Frame(CFA, -4)
          CFI (cfiCond95) CFA R13+32
          CFI Block cfiCond96 Using cfiCommon0
          CFI (cfiCond96) Function photo_storage_poll
          CFI (cfiCond96) Conditional ??CrossCallReturnLabel_6
          CFI (cfiCond96) R4 Frame(CFA, -28)
          CFI (cfiCond96) R5 Frame(CFA, -24)
          CFI (cfiCond96) R6 Frame(CFA, -20)
          CFI (cfiCond96) R7 Frame(CFA, -16)
          CFI (cfiCond96) R8 Frame(CFA, -12)
          CFI (cfiCond96) R9 Frame(CFA, -8)
          CFI (cfiCond96) R14 Frame(CFA, -4)
          CFI (cfiCond96) CFA R13+32
          CFI Block cfiCond97 Using cfiCommon0
          CFI (cfiCond97) Function photo_storage_poll
          CFI (cfiCond97) Conditional ??CrossCallReturnLabel_7
          CFI (cfiCond97) R4 Frame(CFA, -28)
          CFI (cfiCond97) R5 Frame(CFA, -24)
          CFI (cfiCond97) R6 Frame(CFA, -20)
          CFI (cfiCond97) R7 Frame(CFA, -16)
          CFI (cfiCond97) R8 Frame(CFA, -12)
          CFI (cfiCond97) R9 Frame(CFA, -8)
          CFI (cfiCond97) R14 Frame(CFA, -4)
          CFI (cfiCond97) CFA R13+32
          CFI Block cfiCond98 Using cfiCommon0
          CFI (cfiCond98) Function photo_storage_poll
          CFI (cfiCond98) Conditional ??CrossCallReturnLabel_8
          CFI (cfiCond98) R4 Frame(CFA, -28)
          CFI (cfiCond98) R5 Frame(CFA, -24)
          CFI (cfiCond98) R6 Frame(CFA, -20)
          CFI (cfiCond98) R7 Frame(CFA, -16)
          CFI (cfiCond98) R8 Frame(CFA, -12)
          CFI (cfiCond98) R9 Frame(CFA, -8)
          CFI (cfiCond98) R14 Frame(CFA, -4)
          CFI (cfiCond98) CFA R13+32
          CFI Block cfiPicker99 Using cfiCommon1
          CFI (cfiPicker99) NoFunction
          CFI (cfiPicker99) Picker
        THUMB
?Subroutine2:
        MOVS     R0,#+1
        STRB     R0,[R7, #+113]
        BX       LR
          CFI EndBlock cfiCond94
          CFI EndBlock cfiCond95
          CFI EndBlock cfiCond96
          CFI EndBlock cfiCond97
          CFI EndBlock cfiCond98
          CFI EndBlock cfiPicker99

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond100 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_2
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond101 Using cfiCommon0
          CFI (cfiCond101) Function photo_storage_poll
          CFI (cfiCond101) Conditional ??CrossCallReturnLabel_3
          CFI (cfiCond101) R4 Frame(CFA, -28)
          CFI (cfiCond101) R5 Frame(CFA, -24)
          CFI (cfiCond101) R6 Frame(CFA, -20)
          CFI (cfiCond101) R7 Frame(CFA, -16)
          CFI (cfiCond101) R8 Frame(CFA, -12)
          CFI (cfiCond101) R9 Frame(CFA, -8)
          CFI (cfiCond101) R14 Frame(CFA, -4)
          CFI (cfiCond101) CFA R13+32
          CFI Block cfiPicker102 Using cfiCommon1
          CFI (cfiPicker102) NoFunction
          CFI (cfiPicker102) Picker
        THUMB
?Subroutine1:
        MOVS     R0,#+0
        STRH     R0,[R7, #+72]
        MOV      R0,#+4294967295
        STR      R0,[R7, #+16]
        BX       LR
          CFI EndBlock cfiCond100
          CFI EndBlock cfiCond101
          CFI EndBlock cfiPicker102

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond103 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_0
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond104 Using cfiCommon0
          CFI (cfiCond104) Function photo_storage_poll
          CFI (cfiCond104) Conditional ??CrossCallReturnLabel_1
          CFI (cfiCond104) R4 Frame(CFA, -28)
          CFI (cfiCond104) R5 Frame(CFA, -24)
          CFI (cfiCond104) R6 Frame(CFA, -20)
          CFI (cfiCond104) R7 Frame(CFA, -16)
          CFI (cfiCond104) R8 Frame(CFA, -12)
          CFI (cfiCond104) R9 Frame(CFA, -8)
          CFI (cfiCond104) R14 Frame(CFA, -4)
          CFI (cfiCond104) CFA R13+32
          CFI Block cfiPicker105 Using cfiCommon1
          CFI (cfiPicker105) NoFunction
          CFI (cfiPicker105) Picker
        THUMB
?Subroutine0:
        LDR      R0,[R4, #+0]
        LDRB     R3,[R0, #+39]
        LDRB     R2,[R0, #+50]
        BX       LR
          CFI EndBlock cfiCond103
          CFI EndBlock cfiCond104
          CFI EndBlock cfiPicker105

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock106 Using cfiCommon0
          CFI Function pr_poll
        THUMB
pr_poll:
        PUSH     {R4-R8,LR}
          CFI R14 Frame(CFA, -4)
          CFI R8 Frame(CFA, -8)
          CFI R7 Frame(CFA, -12)
          CFI R6 Frame(CFA, -16)
          CFI R5 Frame(CFA, -20)
          CFI R4 Frame(CFA, -24)
          CFI CFA R13+24
        LDR.W    R8,??DataTable36_2
        BL       ?Subroutine29
??CrossCallReturnLabel_75:
        BNE.N    ??pr_poll_0
          CFI FunCall user_storage_background_ready
        BL       user_storage_background_ready
        CMP      R0,#+0
        BEQ.N    ??pr_poll_0
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        MOV      R6,R0
        LDRB     R0,[R8, #+114]
        CMP      R0,#+0
        ITT      EQ
        LDRBEQ   R0,[R8, #+116]
        CMPEQ    R0,#+0
        BNE.N    ??pr_poll_1
        STRB     R0,[R8, #+110]
        STRB     R0,[R8, #+111]
        STRB     R0,[R8, #+112]
        MOVS     R0,#+2
        STRB     R0,[R8, #+106]
        MOVS     R0,#+FO_PHOTO
          CFI FunCall flash_claim
        BL       flash_claim
        B.N      ??pr_poll_0
??pr_poll_1:
        LDR.W    R5,??DataTable36_3
        LDR      R0,[R5, #+0]
        CBZ.N    R0,??pr_poll_2
        LDRB     R0,[R0, #+38]
        CBZ.N    R0,??pr_poll_3
        LDR      R0,[R5, #+0]
        LDRB     R0,[R0, #+50]
        CBNZ.N   R0,??pr_poll_3
        LDR      R0,[R8, #+68]
        LDR.W    R1,??DataTable36_4
        LDR      R1,[R1, #+0]
        CMP      R0,R1
        BEQ.N    ??pr_poll_3
        MOVS     R1,#+0
        LDR      R0,[R5, #+0]
        STRB     R1,[R0, #+38]
??pr_poll_3:
        LDR.W    R0,??DataTable36_5
        LDRH     R0,[R0, #+129]
        UBFX     R0,R0,#+11,#+1
        CBZ.N    R0,??pr_poll_4
          CFI FunCall photo_storage_cancel
        BL       photo_storage_cancel
??pr_poll_5:
        BL       ?Subroutine6
??CrossCallReturnLabel_17:
        ITTT     EQ
        LDREQ    R0,[R5, #+0]
        LDRBEQ   R0,[R0, #+38]
        CMPEQ    R0,#+0
        BNE.N    ??pr_poll_2
        LDRB     R0,[R8, #+106]
        CBNZ.N   R0,??pr_poll_2
        LDR      R0,[R5, #+0]
        MOVS R1,#WORK_PHOTO_CONTEXT
          CFI FunCall work_release
        BL       work_release
        MOVS     R0,#+0
        STR      R0,[R5, #+0]
??pr_poll_2:
        BL       ?Subroutine29
??CrossCallReturnLabel_76:
        ITT      EQ
        LDRBEQ   R0,[R8, #+116]
        CMPEQ    R0,#+0
        BNE.N    ??pr_poll_0
        LDR.W    R1,??DataTable36_5
        LDRH     R0,[R1, #+155]
        CBNZ.N   R0,??pr_poll_0
        BL       ?Subroutine9
??CrossCallReturnLabel_29:
        LDR      R1,[R1, #+157]
        CMP      R0,R1
        BCS.N    ??pr_poll_0
        LDR      R0,[R5, #+0]
        CMP      R0,#+0
        ITT      NE
        LDRBNE   R0,[R0, #+34]
        CMPNE    R0,#+0
        BNE.N    ??pr_poll_0
        POP      {R4-R8,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R7 SameValue
          CFI R8 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall pr_cleanup_begin
        B.W      pr_cleanup_begin
          CFI R4 Frame(CFA, -24)
          CFI R5 Frame(CFA, -20)
          CFI R6 Frame(CFA, -16)
          CFI R7 Frame(CFA, -12)
          CFI R8 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
??pr_poll_0:
        POP      {R4-R8,PC}
??pr_poll_4:
        LDR      R0,[R5, #+0]
        LDRB     R0,[R0, #+34]
        CMP      R0,#+0
        BEQ.N    ??pr_poll_6
        MOVS     R0,#+0
          CFI FunCall uart_getstate
        BL       uart_getstate
        CMP      R0,#+0
        BEQ.N    ??pr_poll_6
        MOVS     R0,#+0
          CFI FunCall uart_isReceiving
        BL       uart_isReceiving
        CMP      R0,#+0
        BNE.N    ??pr_poll_6
          CFI FunCall uart_received
        BL       uart_received
        MOV      R7,R0
        CMP      R7,#+65
        BLT.N    ??pr_poll_7
          CFI FunCall photo_stop
        BL       photo_stop
        MOVS     R7,#+0
??pr_poll_7:
        MOVS     R4,#+0
        B.N      ??pr_poll_8
??pr_poll_9:
        LDR      R0,[R5, #+0]
        LDRH     R2,[R0, #+32]
        UXTH     R0,R1
        CMP      R2,R0
        BNE.N    ??pr_poll_10
        UXTH     R1,R1
        LDR      R0,[R5, #+0]
        ADDS     R0,R0,#+115
          CFI FunCall photo_frame
        BL       photo_frame
        MOVS     R1,#+0
        LDR      R0,[R5, #+0]
        STRH     R1,[R0, #+32]
??pr_poll_10:
        ADDS     R4,R4,#+1
??pr_poll_8:
        CMP      R4,R7
        BGE.N    ??pr_poll_11
        BL       ?Subroutine6
??CrossCallReturnLabel_18:
        BEQ.N    ??pr_poll_11
        BL       ?Subroutine29
??CrossCallReturnLabel_77:
        BNE.N    ??pr_poll_11
        LDR      R0,[R5, #+0]
        LDRH     R0,[R0, #+32]
        CMP      R0,#+64
        BGE.N    ??pr_poll_12
        LDR      R0,[R5, #+0]
        LDRH     R0,[R0, #+40]
        CMP      R0,#+8
        BCS.N    ??pr_poll_13
        LDR      R0,[R5, #+0]
        LDRH     R1,[R0, #+40]
        ADD      R0,R0,R1
        LDR      R1,[R5, #+0]
        ADDS     R1,R1,R4
        LDRB     R1,[R1, #+51]
        STRB     R1,[R0, #+42]
??pr_poll_13:
        LDR      R0,[R5, #+0]
        LDRH     R1,[R0, #+40]
        MOVW     R0,#+65535
        CMP      R1,R0
        BEQ.N    ??pr_poll_14
        ADDS     R0,R1,#+1
        LDR      R1,[R5, #+0]
        STRH     R0,[R1, #+40]
??pr_poll_14:
        LDR      R0,[R5, #+0]
        LDRH     R0,[R0, #+32]
        ADDS     R2,R0,#+1
        LDR      R1,[R5, #+0]
        STRH     R2,[R1, #+32]
        LDR      R1,[R5, #+0]
        ADDS     R1,R1,R4
        LDRB     R1,[R1, #+51]
        LDR      R2,[R5, #+0]
        ADD      R0,R2,R0
        STRB     R1,[R0, #+115]
        MOVS     R1,#+6
        LDR      R0,[R5, #+0]
        LDRB     R0,[R0, #+34]
        CMP      R0,#+6
        BNE.N    ??pr_poll_9
        LDR      R0,[R5, #+0]
        LDRH     R0,[R0, #+32]
        CMP      R0,#+4
        BLT.N    ??pr_poll_9
        LDR      R0,[R5, #+0]
        LDRB     R1,[R0, #+117]
        LDRB     R0,[R0, #+118]
        ADD      R1,R1,R0, LSL #+8
        ADDS     R1,R1,#+6
        UXTH     R0,R1
        CMP      R0,#+65
        BLT.N    ??pr_poll_9
??pr_poll_12:
          CFI FunCall photo_stop
        BL       photo_stop
??pr_poll_11:
        MOVS     R0,#+0
          CFI FunCall uart_clear
        BL       uart_clear
??pr_poll_6:
        LDRB     R0,[R8, #+106]
        CMP      R0,#+0
??pr_poll_15:
        BNE.N    ??pr_poll_0
        LDR      R0,[R5, #+0]
        LDRB     R0,[R0, #+34]
        CBZ.N    R0,??pr_poll_16
        LDR      R0,[R5, #+0]
        LDR      R1,[R0, #+12]
        SUBS     R1,R6,R1
        MOVW     R0,#+60000
        CMP      R1,R0
          CFI FunCall photo_fail
        ITT      CS
        LDRCS.W  R0,??DataTable36_6
        BLCS     photo_fail
??pr_poll_16:
        BL       ?Subroutine6
??CrossCallReturnLabel_19:
        ITTT     NE
        LDRNE    R0,[R5, #+0]
        LDRBNE   R0,[R0, #+34]
        CMPNE    R0,#+7
        BEQ.N    ??pr_poll_17
        LDR      R0,[R5, #+0]
        LDR      R0,[R0, #+8]
        SUBS     R0,R6,R0
        BMI.N    ??pr_poll_17
        LDR      R0,[R5, #+0]
        LDRB     R0,[R0, #+34]
        CMP      R0,#+6
        BEQ.N    ??pr_poll_18
        LDR      R0,[R5, #+0]
        LDR      R0,[R0, #+16]
        SUBS     R0,R6,R0
        CMP      R0,#+50
        BCC.N    ??pr_poll_17
        B.N      ??pr_poll_19
??pr_poll_18:
          CFI FunCall pr_data_ready
        BL       pr_data_ready
        CBZ.N    R0,??pr_poll_17
??pr_poll_19:
        LDR      R0,[R5, #+0]
        LDRB     R0,[R0, #+35]
        ADDS     R2,R0,#+1
        LDR      R1,[R5, #+0]
        STRB     R2,[R1, #+35]
        LDR      R1,[R5, #+0]
        LDRB     R1,[R1, #+34]
        CMP      R1,#+1
        ITE      EQ
        MOVEQ    R1,#+30
        MOVNE    R1,#+3
        CMP      R0,R1
        BLT.N    ??pr_poll_20
        LDR.W    R0,??DataTable36_7
          CFI FunCall photo_fail
        BL       photo_fail
        B.N      ??pr_poll_17
??pr_poll_20:
        MOVS     R1,#+0
        LDR      R0,[R5, #+0]
        STRH     R1,[R0, #+32]
          CFI FunCall photo_issue
        BL       photo_issue
??pr_poll_17:
        BL       ?Subroutine29
??CrossCallReturnLabel_78:
        BNE.N    ??pr_poll_15
        LDR      R0,[R5, #+0]
        LDRB     R0,[R0, #+38]
        CMP      R0,#+0
        BEQ.N    ??pr_poll_21
        LDR.W    R0,??DataTable36
        LDR      R0,[R0, #+0]
        SUBS     R6,R6,R0
        MOVW     R0,#+60000
        CMP      R6,R0
        BCS.N    ??pr_poll_22
        LDR      R0,[R5, #+0]
        LDRB     R0,[R0, #+50]
        CMP      R0,#+1
        BNE.N    ??pr_poll_23
          CFI FunCall comm_channel_ready
        BL       comm_channel_ready
        CBNZ.N   R0,??pr_poll_23
??pr_poll_22:
        MOVS     R0,#+0
        LDR      R1,[R5, #+0]
        STRB     R0,[R1, #+38]
??pr_poll_21:
        B.N      ??pr_poll_5
??pr_poll_23:
        POP      {R4-R8,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R7 SameValue
          CFI R8 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall pr_send_begin
        B.W      pr_send_begin
          CFI EndBlock cfiBlock106

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond107 Using cfiCommon0
          CFI Function pr_poll
          CFI Conditional ??CrossCallReturnLabel_75
          CFI R4 Frame(CFA, -24)
          CFI R5 Frame(CFA, -20)
          CFI R6 Frame(CFA, -16)
          CFI R7 Frame(CFA, -12)
          CFI R8 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
          CFI Block cfiCond108 Using cfiCommon0
          CFI (cfiCond108) Function pr_poll
          CFI (cfiCond108) Conditional ??CrossCallReturnLabel_76
          CFI (cfiCond108) R4 Frame(CFA, -24)
          CFI (cfiCond108) R5 Frame(CFA, -20)
          CFI (cfiCond108) R6 Frame(CFA, -16)
          CFI (cfiCond108) R7 Frame(CFA, -12)
          CFI (cfiCond108) R8 Frame(CFA, -8)
          CFI (cfiCond108) R14 Frame(CFA, -4)
          CFI (cfiCond108) CFA R13+24
          CFI Block cfiCond109 Using cfiCommon0
          CFI (cfiCond109) Function pr_poll
          CFI (cfiCond109) Conditional ??CrossCallReturnLabel_77
          CFI (cfiCond109) R4 Frame(CFA, -24)
          CFI (cfiCond109) R5 Frame(CFA, -20)
          CFI (cfiCond109) R6 Frame(CFA, -16)
          CFI (cfiCond109) R7 Frame(CFA, -12)
          CFI (cfiCond109) R8 Frame(CFA, -8)
          CFI (cfiCond109) R14 Frame(CFA, -4)
          CFI (cfiCond109) CFA R13+24
          CFI Block cfiCond110 Using cfiCommon0
          CFI (cfiCond110) Function pr_poll
          CFI (cfiCond110) Conditional ??CrossCallReturnLabel_78
          CFI (cfiCond110) R4 Frame(CFA, -24)
          CFI (cfiCond110) R5 Frame(CFA, -20)
          CFI (cfiCond110) R6 Frame(CFA, -16)
          CFI (cfiCond110) R7 Frame(CFA, -12)
          CFI (cfiCond110) R8 Frame(CFA, -8)
          CFI (cfiCond110) R14 Frame(CFA, -4)
          CFI (cfiCond110) CFA R13+24
          CFI Block cfiPicker111 Using cfiCommon1
          CFI (cfiPicker111) NoFunction
          CFI (cfiPicker111) Picker
        THUMB
?Subroutine29:
        LDRB     R0,[R8, #+106]
        CMP      R0,#+0
        BX       LR
          CFI EndBlock cfiCond107
          CFI EndBlock cfiCond108
          CFI EndBlock cfiCond109
          CFI EndBlock cfiCond110
          CFI EndBlock cfiPicker111

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond112 Using cfiCommon0
          CFI Function photo_storage_poll
          CFI Conditional ??CrossCallReturnLabel_26
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond113 Using cfiCommon0
          CFI (cfiCond113) Function photo_storage_poll
          CFI (cfiCond113) Conditional ??CrossCallReturnLabel_27
          CFI (cfiCond113) R4 Frame(CFA, -28)
          CFI (cfiCond113) R5 Frame(CFA, -24)
          CFI (cfiCond113) R6 Frame(CFA, -20)
          CFI (cfiCond113) R7 Frame(CFA, -16)
          CFI (cfiCond113) R8 Frame(CFA, -12)
          CFI (cfiCond113) R9 Frame(CFA, -8)
          CFI (cfiCond113) R14 Frame(CFA, -4)
          CFI (cfiCond113) CFA R13+32
          CFI Block cfiCond114 Using cfiCommon0
          CFI (cfiCond114) Function photo_storage_poll
          CFI (cfiCond114) Conditional ??CrossCallReturnLabel_28
          CFI (cfiCond114) R4 Frame(CFA, -28)
          CFI (cfiCond114) R5 Frame(CFA, -24)
          CFI (cfiCond114) R6 Frame(CFA, -20)
          CFI (cfiCond114) R7 Frame(CFA, -16)
          CFI (cfiCond114) R8 Frame(CFA, -12)
          CFI (cfiCond114) R9 Frame(CFA, -8)
          CFI (cfiCond114) R14 Frame(CFA, -4)
          CFI (cfiCond114) CFA R13+32
          CFI Block cfiCond115 Using cfiCommon0
          CFI (cfiCond115) Function pr_poll
          CFI (cfiCond115) Conditional ??CrossCallReturnLabel_29
          CFI (cfiCond115) R4 Frame(CFA, -24)
          CFI (cfiCond115) R5 Frame(CFA, -20)
          CFI (cfiCond115) R6 Frame(CFA, -16)
          CFI (cfiCond115) R7 Frame(CFA, -12)
          CFI (cfiCond115) R8 Frame(CFA, -8)
          CFI (cfiCond115) R14 Frame(CFA, -4)
          CFI (cfiCond115) CFA R13+24
          CFI Block cfiPicker116 Using cfiCommon1
          CFI (cfiPicker116) NoFunction
          CFI (cfiPicker116) Picker
        THUMB
?Subroutine9:
        LDR.W    R0,??DataTable36_8
        LDR      R0,[R0, #+72]
        BX       LR
          CFI EndBlock cfiCond112
          CFI EndBlock cfiCond113
          CFI EndBlock cfiCond114
          CFI EndBlock cfiCond115
          CFI EndBlock cfiPicker116

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond117 Using cfiCommon0
          CFI Function pr_poll
          CFI Conditional ??CrossCallReturnLabel_17
          CFI R4 Frame(CFA, -24)
          CFI R5 Frame(CFA, -20)
          CFI R6 Frame(CFA, -16)
          CFI R7 Frame(CFA, -12)
          CFI R8 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
          CFI Block cfiCond118 Using cfiCommon0
          CFI (cfiCond118) Function pr_poll
          CFI (cfiCond118) Conditional ??CrossCallReturnLabel_18
          CFI (cfiCond118) R4 Frame(CFA, -24)
          CFI (cfiCond118) R5 Frame(CFA, -20)
          CFI (cfiCond118) R6 Frame(CFA, -16)
          CFI (cfiCond118) R7 Frame(CFA, -12)
          CFI (cfiCond118) R8 Frame(CFA, -8)
          CFI (cfiCond118) R14 Frame(CFA, -4)
          CFI (cfiCond118) CFA R13+24
          CFI Block cfiCond119 Using cfiCommon0
          CFI (cfiCond119) Function pr_poll
          CFI (cfiCond119) Conditional ??CrossCallReturnLabel_19
          CFI (cfiCond119) R4 Frame(CFA, -24)
          CFI (cfiCond119) R5 Frame(CFA, -20)
          CFI (cfiCond119) R6 Frame(CFA, -16)
          CFI (cfiCond119) R7 Frame(CFA, -12)
          CFI (cfiCond119) R8 Frame(CFA, -8)
          CFI (cfiCond119) R14 Frame(CFA, -4)
          CFI (cfiCond119) CFA R13+24
          CFI Block cfiPicker120 Using cfiCommon1
          CFI (cfiPicker120) NoFunction
          CFI (cfiPicker120) Picker
        THUMB
?Subroutine6:
        LDR      R0,[R5, #+0]
        LDRB     R0,[R0, #+34]
        CMP      R0,#+0
        BX       LR
          CFI EndBlock cfiCond117
          CFI EndBlock cfiCond118
          CFI EndBlock cfiCond119
          CFI EndBlock cfiPicker120

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock121 Using cfiCommon0
          CFI Function photo_alloc
        THUMB
photo_alloc:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        LDR.W    R4,??DataTable36_3
        LDR      R0,[R4, #+0]
        CBNZ.N   R0,??photo_alloc_0
        MOVS     R1,#+180
        MOVS     R0,#+1
        MOV R0,R1
        MOVS R1,#WORK_PHOTO_CONTEXT
          CFI FunCall work_acquire
        BL       work_acquire
        STR      R0,[R4, #+0]
??photo_alloc_0:
        LDR      R0,[R4, #+0]
        CBZ.N    R0,??photo_alloc_1
        MOVS     R0,#+1
??photo_alloc_1:
        POP      {R4,PC}
          CFI EndBlock cfiBlock121

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock122 Using cfiCommon0
          CFI Function photo_stop
        THUMB
photo_stop:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI FunCall rs485_baud
        BL       rs485_baud
        BL       ?Subroutine10
??CrossCallReturnLabel_31:
        LDR.W    R0,??DataTable36_3
        MOVS     R2,#+0
        LDR      R1,[R0, #+0]
        STRB     R2,[R1, #+34]
        LDR      R0,[R0, #+0]
        STRH     R2,[R0, #+32]
        LDR.W    R0,??DataTable36_5
        STRB     R2,[R0, #+116]
        POP      {R0,PC}
          CFI EndBlock cfiBlock122

        SECTION `.text`:CODE:NOROOT(2)
          CFI Block cfiBlock123 Using cfiCommon0
          CFI Function rs485_baud
          CFI NoCalls
        THUMB
rs485_baud:
        LDR.W    R0,??DataTable36_9
        LDRB     R0,[R0, #+174]
        SUBS     R0,R0,#+1
        CMP      R0,#+3
        BHI.N    ??rs485_baud_1
        TBB      [PC, R0]
        DATA
??rs485_baud_0:
        DC8     (??jt1_0-??rs485_baud_0)/2,(??jt1_1-??rs485_baud_0)/2,(??jt1_2-??rs485_baud_0)/2,(??jt1_3-??rs485_baud_0)/2
        THUMB
??rs485_baud_2:
??jt1_0:
        MOV      R0,#+57600
        BX       LR
??rs485_baud_3:
??jt1_1:
        MOV      R0,#+38400
        BX       LR
??rs485_baud_4:
??jt1_2:
        MOV      R0,#+19200
        BX       LR
??rs485_baud_5:
??jt1_3:
        MOV      R0,#+9600
        BX       LR
??rs485_baud_1:
        MOV      R0,#+115200
        BX       LR
          CFI EndBlock cfiBlock123

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock124 Using cfiCommon0
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
        LDR.W    R6,??DataTable36_9
        LDRB     R0,[R6, #+172]
        CMP      R0,#+2
        IT       GE
        STRBGE   R4,[R6, #+172]
        LDRB     R0,[R6, #+174]
        CMP      R0,#+5
        BLT.N    ??rs485_init_0
        LDR.W    R0,??DataTable36_10
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
        LDRNE.W  R0,??DataTable36_11
        BLNE     user_debug
          CFI FunCall rs485_baud
        BL       rs485_baud
        BL       ?Subroutine10
??CrossCallReturnLabel_30:
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
          CFI EndBlock cfiBlock124

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond125 Using cfiCommon0
          CFI Function photo_stop
          CFI Conditional ??CrossCallReturnLabel_31
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI Block cfiCond126 Using cfiCommon0
          CFI (cfiCond126) Function rs485_init
          CFI (cfiCond126) Conditional ??CrossCallReturnLabel_30
          CFI (cfiCond126) R4 Frame(CFA, -16)
          CFI (cfiCond126) R5 Frame(CFA, -12)
          CFI (cfiCond126) R6 Frame(CFA, -8)
          CFI (cfiCond126) R14 Frame(CFA, -4)
          CFI (cfiCond126) CFA R13+24
          CFI Block cfiPicker127 Using cfiCommon1
          CFI (cfiPicker127) NoFunction
          CFI (cfiPicker127) Picker
        THUMB
?Subroutine10:
        MOVS     R1,#+32
        STR      R1,[SP, #+0]
        LDR.W    R3,??DataTable36_12
        MOVS     R2,#+0
        MOV      R1,R0
        MOVS     R0,#+0
          CFI (cfiCond125) FunCall photo_stop uart_init
          CFI (cfiCond126) FunCall rs485_init uart_init
        B.W      uart_init
          CFI EndBlock cfiCond125
          CFI EndBlock cfiCond126
          CFI EndBlock cfiPicker127

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable27:
        DATA32
        DC32     photo_erased_to

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock128 Using cfiCommon0
          CFI Function rs485_set_config
        THUMB
rs485_set_config:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R4,R0
        LDR.W    R5,??DataTable36_9
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
        LDR.W    R1,??DataTable36_3
        LDR      R0,[R1, #+0]
        CMP      R0,#+0
        ITT      NE
        LDRBNE   R0,[R0, #+34]
        CMPNE    R0,#+0
        BEQ.N    ??rs485_set_config_1
        LDR.W    R0,??DataTable36_13
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
        LDR.W    R1,??DataTable36_3
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
          CFI EndBlock cfiBlock128

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock129 Using cfiCommon0
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
        LDR.W    R1,??DataTable36_14
        MOVS     R2,#+96
          CFI FunCall __aeabi_memcpy4
        BL       __aeabi_memcpy4
        MOVS     R5,#+7
        LDR.W    R6,??DataTable36_3
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
        LDR.W    R7,??DataTable36_15
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
        LDR.W    R3,??DataTable36_16
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
        LDR.W    R1,??DataTable36_17
        UXTB     R0,R5
        ADD      R0,SP,R0
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        ADDS     R5,R5,#+5
        MOVW     R0,#+10000
        BL       ?Subroutine21
??CrossCallReturnLabel_56:
        MOV      R0,#+1000
        BL       ?Subroutine21
??CrossCallReturnLabel_57:
        MOVS     R0,#+100
        SDIV     R0,R8,R0
        BL       ?Subroutine20
??CrossCallReturnLabel_54:
        BL       ?Subroutine20
??CrossCallReturnLabel_55:
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
        LDR.W    R1,??DataTable36_18
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
        ADR.N    R1,??DataTable34
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
          CFI EndBlock cfiBlock129

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond130 Using cfiCommon0
          CFI Function photo_fail
          CFI Conditional ??CrossCallReturnLabel_56
          CFI R4 Frame(CFA, -24)
          CFI R5 Frame(CFA, -20)
          CFI R6 Frame(CFA, -16)
          CFI R7 Frame(CFA, -12)
          CFI R8 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+120
          CFI Block cfiCond131 Using cfiCommon0
          CFI (cfiCond131) Function photo_fail
          CFI (cfiCond131) Conditional ??CrossCallReturnLabel_57
          CFI (cfiCond131) R4 Frame(CFA, -24)
          CFI (cfiCond131) R5 Frame(CFA, -20)
          CFI (cfiCond131) R6 Frame(CFA, -16)
          CFI (cfiCond131) R7 Frame(CFA, -12)
          CFI (cfiCond131) R8 Frame(CFA, -8)
          CFI (cfiCond131) R14 Frame(CFA, -4)
          CFI (cfiCond131) CFA R13+120
          CFI Block cfiPicker132 Using cfiCommon1
          CFI (cfiPicker132) NoFunction
          CFI (cfiPicker132) Picker
        THUMB
?Subroutine21:
        SDIV     R0,R8,R0
        MOVS     R1,#+10
        SDIV     R1,R0,R1
        MLS      R0,R4,R1,R0
        ADDS     R0,R0,#+48
        UXTB     R1,R5
        STRB     R0,[SP, R1]
        ADDS     R5,R5,#+1
        BX       LR
          CFI EndBlock cfiCond130
          CFI EndBlock cfiCond131
          CFI EndBlock cfiPicker132

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond133 Using cfiCommon0
          CFI Function photo_fail
          CFI Conditional ??CrossCallReturnLabel_54
          CFI R4 Frame(CFA, -24)
          CFI R5 Frame(CFA, -20)
          CFI R6 Frame(CFA, -16)
          CFI R7 Frame(CFA, -12)
          CFI R8 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+120
          CFI Block cfiCond134 Using cfiCommon0
          CFI (cfiCond134) Function photo_fail
          CFI (cfiCond134) Conditional ??CrossCallReturnLabel_55
          CFI (cfiCond134) R4 Frame(CFA, -24)
          CFI (cfiCond134) R5 Frame(CFA, -20)
          CFI (cfiCond134) R6 Frame(CFA, -16)
          CFI (cfiCond134) R7 Frame(CFA, -12)
          CFI (cfiCond134) R8 Frame(CFA, -8)
          CFI (cfiCond134) R14 Frame(CFA, -4)
          CFI (cfiCond134) CFA R13+120
          CFI Block cfiPicker135 Using cfiCommon1
          CFI (cfiPicker135) NoFunction
          CFI (cfiPicker135) Picker
        THUMB
?Subroutine20:
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
          CFI EndBlock cfiCond133
          CFI EndBlock cfiCond134
          CFI EndBlock cfiPicker135

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_0:
        DATA8
        DC8 "SLDN"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_1:
        DATA8
        DC8 "60 s limit"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_2:
        DATA8
        DC8 "no answer"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_3:
        DATA8
        DC8 "RS485: invalid baud code, using 115200\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_4:
        DATA8
        DC8 "RS485: C328 exclusive, other devices ignored\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_5:
        DATA8
        DC8 "Photo: capture error, RS485 settings changed\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_6:
        DATA8
        DC8 ", stage "
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_7:
        DATA8
        DC8 ", retry "
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_8:
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
?_10:
        DATA8
        DC8 "Photo: camera sync OK\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
`photo_fail::hex`:
        DATA8
        DC8 "0123456789ABCDEF"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_11:
        DATA8
        DC8 "Photo: "
        DATA
        DS8 88

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock136 Using cfiCommon0
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
        LDR.W    R5,??DataTable36_19
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
        LDR.W    R2,??DataTable36_3
        LDR      R1,[R2, #+0]
        STR      R0,[R1, #+16]
        ADD      R0,R0,#+2000
        LDR      R1,[R2, #+0]
        STR      R0,[R1, #+8]
        POP      {R0-R2,R4-R7,PC}
          CFI EndBlock cfiBlock136

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_12:
        DATA8
        DC8 170, 0, 0, 0, 0, 0
        DATA16
        DS8 2

        SECTION `.text`:CODE:NOROOT(2)
          CFI Block cfiBlock137 Using cfiCommon0
          CFI Function photo_issue
        THUMB
photo_issue:
        PUSH     {R2-R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+16
        LDR.W    R4,??DataTable36_3
        LDR      R0,[R4, #+0]
        LDRB     R0,[R0, #+34]
        SUBS     R0,R0,#+1
        CMP      R0,#+5
        BHI.N    ??photo_issue_1
        TBB      [PC, R0]
        DATA
??photo_issue_0:
        DC8     (??jt2_0-??photo_issue_0)/2,(??jt2_1-??photo_issue_0)/2,(??jt2_2-??photo_issue_0)/2,(??jt2_3-??photo_issue_0)/2
        DC8     (??jt2_4-??photo_issue_0)/2,(??jt2_5-??photo_issue_0)/2
        THUMB
??photo_issue_2:
??jt2_0:
        BL       ?Subroutine5
??CrossCallReturnLabel_15:
        MOVS     R0,#+13
        B.N      ??photo_issue_3
??photo_issue_4:
??jt2_1:
        MOVS     R0,#+7
        STR      R0,[SP, #+0]
        MOVS     R3,#+0
        MOVS     R2,#+7
        MOVS     R1,#+0
        MOVS     R0,#+1
        B.N      ??photo_issue_3
??photo_issue_5:
??jt2_2:
        MOVS     R0,#+0
        STR      R0,[SP, #+0]
        MOVS     R3,#+0
        MOVS     R2,#+64
        MOVS     R1,#+8
        MOVS     R0,#+6
        B.N      ??photo_issue_3
??photo_issue_6:
??jt2_3:
        BL       ?Subroutine5
??CrossCallReturnLabel_16:
        MOVS     R0,#+5
        B.N      ??photo_issue_3
??photo_issue_7:
??jt2_4:
        MOVS     R0,#+0
        STR      R0,[SP, #+0]
        MOVS     R3,#+0
        MOVS     R2,#+0
        MOVS     R1,#+1
        MOVS     R0,#+4
        B.N      ??photo_issue_3
??photo_issue_8:
??jt2_5:
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
        LDR      R0,[R4, #+0]
        LDRB     R0,[R0, #+34]
        CMP      R0,#+1
        BNE.N    ??photo_issue_9
        LDR      R0,[R4, #+0]
        LDR      R0,[R0, #+16]
        ADDS     R0,R0,#+50
        LDR      R1,[R4, #+0]
        STR      R0,[R1, #+8]
??photo_issue_9:
        POP      {R0,R1,R4,PC}
          CFI EndBlock cfiBlock137

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond138 Using cfiCommon0
          CFI Function photo_issue
          CFI Conditional ??CrossCallReturnLabel_15
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
          CFI Block cfiCond139 Using cfiCommon0
          CFI (cfiCond139) Function photo_issue
          CFI (cfiCond139) Conditional ??CrossCallReturnLabel_16
          CFI (cfiCond139) R4 Frame(CFA, -8)
          CFI (cfiCond139) R14 Frame(CFA, -4)
          CFI (cfiCond139) CFA R13+16
          CFI Block cfiPicker140 Using cfiCommon1
          CFI (cfiPicker140) NoFunction
          CFI (cfiPicker140) Picker
        THUMB
?Subroutine5:
        MOVS     R0,#+0
        STR      R0,[SP, #+0]
        MOVS     R3,#+0
        MOVS     R2,#+0
        MOVS     R1,#+0
        BX       LR
          CFI EndBlock cfiCond138
          CFI EndBlock cfiCond139
          CFI EndBlock cfiPicker140

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock141 Using cfiCommon0
          CFI Function photo_start
          CFI FunCall pr_start
        THUMB
photo_start:
        B.W      pr_start
          CFI EndBlock cfiBlock141

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock142 Using cfiCommon0
          CFI Function photo_storage_end
          CFI NoCalls
        THUMB
photo_storage_end:
        LDR.N    R1,??DataTable36_8
        LDR      R0,[R1, #+76]
        CMP      R0,#+8192
        ITE      CS
        SUBCS    R0,R0,#+8192
        MOVCC    R0,#+0
        LDR      R1,[R1, #+72]
        ADDS     R0,R0,R1
        BX       LR
          CFI EndBlock cfiBlock142

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock143 Using cfiCommon0
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
          CFI EndBlock cfiBlock143

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock144 Using cfiCommon0
          CFI Function photo_cleanup_active
        THUMB
photo_cleanup_active:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        LDR.N    R1,??DataTable36_8
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
          CFI EndBlock cfiBlock144

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock145 Using cfiCommon0
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
        LDR.N    R5,??DataTable36_8
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
        LDR.N    R0,??DataTable36_20
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
        MOV      R2,SP
        UXTH     R1,R0
        MOV      R0,R4
          CFI FunCall photo_backup_crc
        BL       photo_backup_crc
??photo_recover_0:
        MOVS     R0,#+0
??photo_recover_1:
        ADD      SP,SP,#+20
          CFI CFA R13+12
        POP      {R4,R5,PC}
          CFI EndBlock cfiBlock145

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable34:
        DATA8
        DC8      0x0D, 0x0A, 0x00, 0x00

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock146 Using cfiCommon0
          CFI Function photo_prepare
          CFI FunCall pr_prepare
        THUMB
photo_prepare:
        B.W      pr_prepare
          CFI EndBlock cfiBlock146

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock147 Using cfiCommon0
          CFI Function photo_frame
        THUMB
photo_frame:
        PUSH     {R2-R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+16
        LDR.N    R4,??DataTable36_3
        LDR      R2,[R4, #+0]
        LDRB     R2,[R2, #+34]
        CMP      R2,#+6
        BNE.N    ??photo_frame_0
        POP      {R2-R4,LR}
          CFI R4 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall pr_frame
        B.W      pr_frame
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
??photo_frame_0:
        LDRB     R1,[R0, #+0]
        CMP      R1,#+170
        BNE.N    ??photo_frame_1
        LDRB     R1,[R0, #+1]
        CMP      R1,#+15
        BEQ.N    ??photo_frame_2
        CMP      R2,#+1
        BNE.N    ??photo_frame_3
        CMP      R1,#+14
        BNE.N    ??photo_frame_4
        LDRB     R1,[R0, #+2]
        CMP      R1,#+13
        BNE.N    ??photo_frame_5
        MOVS     R1,#+1
        LDR      R2,[R4, #+0]
        STRB     R1,[R2, #+36]
??photo_frame_4:
        LDRB     R0,[R0, #+1]
        CMP      R0,#+13
        BNE.N    ??photo_frame_5
        MOVS     R1,#+1
        LDR      R0,[R4, #+0]
        STRB     R1,[R0, #+37]
??photo_frame_5:
        LDR      R0,[R4, #+0]
        LDRB     R0,[R0, #+36]
        CMP      R0,#+0
        ITTT     NE
        LDRNE    R0,[R4, #+0]
        LDRBNE   R0,[R0, #+37]
        CMPNE    R0,#+0
        BEQ.N    ??photo_frame_1
        MOVS     R0,#+0
        STR      R0,[SP, #+0]
        MOVS     R3,#+0
        MOVS     R2,#+0
        MOVS     R1,#+13
        MOVS     R0,#+14
          CFI FunCall photo_command
        BL       photo_command
        MOVS     R1,#+2
        LDR      R0,[R4, #+0]
        STRB     R1,[R0, #+34]
        LDR.N    R0,??DataTable36_21
          CFI FunCall user_debug
        BL       user_debug
??photo_frame_6:
        MOVS     R1,#+0
        LDR      R0,[R4, #+0]
        STRB     R1,[R0, #+35]
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        LDR      R1,[R4, #+0]
        STR      R0,[R1, #+8]
??photo_frame_1:
        POP      {R0,R1,R4,PC}
??photo_frame_3:
        CMP      R2,#+5
        IT       EQ
        CMPEQ    R1,#+10
        BNE.N    ??photo_frame_7
        LDRB     R1,[R0, #+2]
        CMP      R1,#+1
        BNE.N    ??photo_frame_1
        LDRB     R1,[R0, #+3]
        LDRB     R2,[R0, #+4]
        ORR      R1,R1,R2, LSL #+8
        LDRB     R0,[R0, #+5]
        ORR      R0,R1,R0, LSL #+16
          CFI FunCall photo_prepare
        BL       photo_prepare
        CMP      R0,#+0
        BNE.N    ??photo_frame_6
??photo_frame_2:
        POP      {R0,R1,R4,LR}
          CFI R4 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall photo_stop
        B.N      photo_stop
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
??photo_frame_7:
        CMP      R1,#+14
        BNE.N    ??photo_frame_1
        CMP      R2,#+2
        BNE.N    ??photo_frame_8
        LDRB     R0,[R0, #+2]
        CMP      R0,#+1
        BEQ.N    ??photo_frame_9
        POP      {R0,R1,R4,PC}
??photo_frame_8:
        CMP      R2,#+3
        BNE.N    ??photo_frame_10
        LDRB     R0,[R0, #+2]
        CMP      R0,#+6
        BNE.N    ??photo_frame_1
        B.N      ??photo_frame_9
??photo_frame_10:
        CMP      R2,#+4
        ITT      EQ
        LDRBEQ   R0,[R0, #+2]
        CMPEQ    R0,#+5
        BNE.N    ??photo_frame_1
??photo_frame_9:
        LDR      R0,[R4, #+0]
        ADDS     R1,R2,#+1
        STRB     R1,[R0, #+34]
        B.N      ??photo_frame_6
          CFI EndBlock cfiBlock147

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock148 Using cfiCommon0
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
          CFI EndBlock cfiBlock148

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock149 Using cfiCommon0
          CFI Function photo_request
          CFI FunCall pr_request
        THUMB
photo_request:
        B.W      pr_request
          CFI EndBlock cfiBlock149

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock150 Using cfiCommon0
          CFI Function photo_connection_lost
          CFI NoCalls
        THUMB
photo_connection_lost:
        LDR.N    R1,??DataTable36_3
        LDR      R0,[R1, #+0]
        CBZ.N    R0,??photo_connection_lost_0
        LDRB     R0,[R0, #+50]
        CMP      R0,#+1
        BNE.N    ??photo_connection_lost_1
        MOVS     R2,#+0
        LDR      R0,[R1, #+0]
        STRB     R2,[R0, #+38]
??photo_connection_lost_1:
        LDR      R0,[R1, #+0]
        LDRB     R0,[R0, #+50]
        CMP      R0,#+1
        BNE.N    ??photo_connection_lost_0
        MOVS     R1,#+1
        LDR.N    R0,??DataTable36_2
        STRB     R1,[R0, #+111]
??photo_connection_lost_0:
        BX       LR
          CFI EndBlock cfiBlock150

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36:
        DATA32
        DC32     photo_pending_since

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36_1:
        DATA32
        DC32     ?_0

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36_2:
        DATA32
        DC32     pr

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36_3:
        DATA32
        DC32     photo

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36_4:
        DATA32
        DC32     usb_bus_resets

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36_5:
        DATA32
        DC32     device

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36_6:
        DATA32
        DC32     ?_1

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36_7:
        DATA32
        DC32     ?_2

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36_8:
        DATA32
        DC32     mem

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36_9:
        DATA32
        DC32     config

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36_10:
        DATA32
        DC32     ?_3

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36_11:
        DATA32
        DC32     ?_4

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36_12:
        DATA32
        DC32     port_buffer

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36_13:
        DATA32
        DC32     ?_5

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36_14:
        DATA32
        DC32     ?_11

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36_15:
        DATA32
        DC32     ?_6

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36_16:
        DATA32
        DC32     ?_7

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36_17:
        DATA32
        DC32     ?_8

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36_18:
        DATA32
        DC32     `photo_fail::hex`

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36_19:
        DATA32
        DC32     ?_12

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36_20:
        DATA32
        DC32     0x50484f54

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable36_21:
        DATA32
        DC32     ?_10

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock151 Using cfiCommon0
          CFI Function photo_poll
          CFI FunCall pr_poll
        THUMB
photo_poll:
        B.W      pr_poll
          CFI EndBlock cfiBlock151

        SECTION `.iar_vfe_header`:DATA:NOALLOC:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
        DC32 0

        END
