; Cortex-M3 / IAR. Executable production module.
; Reference: Tests/asm/reference/User/memory.c
        PUBLIC cfg_cached_active
        PUBLIC cfg_cached_generation
        PUBLIC cfg_job
        PUBLIC cfg_legacy_checked
        PUBLIC mem_storage_ready

        RTMODEL "__CPP_Runtime", "1"
        RTMODEL "__SystemLibrary", "DLib"
        RTMODEL "__dlib_file_descriptor", "0"
        RTMODEL "__dlib_full_locale_support", "0"
        RTMODEL "__dlib_version", "6"
        AAPCS BASE,INTERWORK
        PRESERVE8
        REQUIRE8

        #define SHT_PROGBITS 0x1

        EXTERN __aeabi_memclr4
        EXTERN __aeabi_memcpy
        EXTERN crc16_calcInit
        EXTERN crc32_update
        EXTERN fast_all_ff
        EXTERN free
        EXTERN malloc
        EXTERN memcmp
        EXTERN sj_destination
        EXTERN sj_recover
        EXTERN sj_replace
        EXTERN sj_replace_pages
        EXTERN sj_runtime_blocked
        EXTERN sj_span
        EXTERN store_runtime
        EXTERN table_runtime_blocks_reads
        EXTERN user_wait_stable_power
        EXTERN w25_async_active
        EXTERN w25_async_poll
        EXTERN w25_async_start
        EXTERN w25_erase_sector
        EXTERN w25_read_data
        EXTERN w25_write_data
        EXTERN wdt_reload

        PUBLIC mem_config_begin
        PUBLIC mem_config_errors
        PUBLIC mem_config_poll
        PUBLIC mem_config_reset
        PUBLIC mem_config_result
        PUBLIC mem_config_snapshot
        PUBLIC mem_erase_mem
        PUBLIC mem_erase_sector
        PUBLIC mem_patch
        PUBLIC mem_patch2
        PUBLIC mem_read
        PUBLIC mem_recover
        PUBLIC mem_replace_pages
        PUBLIC mem_replace_sector
        PUBLIC mem_rewrite
        EXTERN idindex_runtime_active
        PUBLIC mem_tables_ready
        PUBLIC mem_write
        PUBLIC sj_io_erase
        PUBLIC sj_io_read
        PUBLIC sj_io_service
        PUBLIC sj_io_write

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

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
mem_storage_ready:
        DS8 1

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock0 Using cfiCommon0
          CFI Function sj_io_read
          CFI NoCalls
        THUMB
sj_io_read:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        BL       ?Subroutine3
??CrossCallReturnLabel_4:
        B.N      ?Subroutine0
          CFI EndBlock cfiBlock0

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock1 Using cfiCommon0
          CFI Function sj_io_write
          CFI NoCalls
        THUMB
sj_io_write:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        BL       ?Subroutine3
??CrossCallReturnLabel_3:
        CBZ.N    R0,??sj_io_write_0
        BL       ?Subroutine5
??CrossCallReturnLabel_8:
        CBZ.N    R0,??sj_io_write_0
        MOVS     R0,#+1
??sj_io_write_0:
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock1

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond2 Using cfiCommon0
          CFI Function sj_io_read
          CFI Conditional ??CrossCallReturnLabel_4
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
          CFI Block cfiCond3 Using cfiCommon0
          CFI (cfiCond3) Function sj_io_write
          CFI (cfiCond3) Conditional ??CrossCallReturnLabel_3
          CFI (cfiCond3) R4 Frame(CFA, -16)
          CFI (cfiCond3) R5 Frame(CFA, -12)
          CFI (cfiCond3) R6 Frame(CFA, -8)
          CFI (cfiCond3) R14 Frame(CFA, -4)
          CFI (cfiCond3) CFA R13+16
          CFI Block cfiPicker4 Using cfiCommon1
          CFI (cfiPicker4) NoFunction
          CFI (cfiPicker4) Picker
        THUMB
?Subroutine3:
        MOV      R6,R0
        MOV      R4,R1
        MOV      R5,R2
        MOV      R1,R5
          CFI (cfiCond2) FunCall sj_io_read sj_span
          CFI (cfiCond3) FunCall sj_io_write sj_span
        B.W      sj_span
          CFI EndBlock cfiCond2
          CFI EndBlock cfiCond3
          CFI EndBlock cfiPicker4

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock5 Using cfiCommon0
          CFI Function sj_io_erase
        THUMB
sj_io_erase:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        MOV      R4,R0
        LSLS     R0,R4,#+20
        BNE.N    ??sj_io_erase_0
        MOV      R1,#+4096
        MOV      R0,R4
          CFI FunCall sj_span
        BL       sj_span
        CBZ.N    R0,??sj_io_erase_0
        MOV      R0,R4
          CFI FunCall w25_erase_sector
        BL       w25_erase_sector
        CBZ.N    R0,??sj_io_erase_1
        MOVS     R0,#+1
        POP      {R4,PC}
??sj_io_erase_0:
        MOVS     R0,#+0
??sj_io_erase_1:
        POP      {R4,PC}
          CFI EndBlock cfiBlock5

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock6 Using cfiCommon0
          CFI Function sj_io_service
          CFI FunCall wdt_reload
        THUMB
sj_io_service:
        B.W      wdt_reload
          CFI EndBlock cfiBlock6

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock7 Using cfiCommon0
          CFI Function mem_recover
        THUMB
mem_recover:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI FunCall user_wait_stable_power
        BL       user_wait_stable_power
          CFI FunCall sj_recover
        BL       sj_recover
        BL       ?Subroutine2
??CrossCallReturnLabel_0:
        POP      {R1,PC}
          CFI EndBlock cfiBlock7

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock8 Using cfiCommon0
          CFI Function mem_tables_ready
        THUMB
mem_tables_ready:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI FunCall idindex_runtime_active
        BL       idindex_runtime_active
        CBNZ.N   R0,??mem_tables_ready_0
          CFI FunCall table_runtime_blocks_reads
        BL       table_runtime_blocks_reads
        CBNZ.N   R0,??mem_tables_ready_0
          CFI FunCall sj_runtime_blocked
        BL       sj_runtime_blocked
        CBNZ.N   R0,??mem_tables_ready_0
        LDR.W    R0,??DataTable19
        LDRB     R0,[R0, #+0]
        CBNZ.N   R0,??mem_tables_ready_1
        LDR.W    R0,??DataTable19_1
        LDRB     R0,[R0, #+0]
        CBNZ.N   R0,??mem_tables_ready_0
          CFI FunCall mem_recover
        BL       mem_recover
        CBZ.N    R0,??mem_tables_ready_2
??mem_tables_ready_1:
        MOVS     R0,#+1
        POP      {R1,PC}
??mem_tables_ready_0:
        MOVS     R0,#+0
??mem_tables_ready_2:
        POP      {R1,PC}
          CFI EndBlock cfiBlock8

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock9 Using cfiCommon0
          CFI Function mem_guard
        THUMB
mem_guard:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R4,R0
        MOV      R5,R1
        MOV      R1,#+4294967295
        SUBS     R1,R1,R4
        CMP      R1,R5
        BCS.N    ??mem_guard_0
        MOVS     R0,#+0
        POP      {R1,R4,R5,PC}
??mem_guard_0:
        MOVS     R0,R5
        BEQ.N    ??mem_guard_1
        ADDS     R5,R5,R4
        SUBS     R5,R5,#+1
        LSRS     R5,R5,#+12
        LSRS     R4,R4,#+12
        LSLS     R4,R4,#+12
        B.N      ??mem_guard_2
??mem_guard_3:
        CMP      R4,R5, LSL #+12
        BEQ.N    ??mem_guard_1
        ADD      R4,R4,#+4096
??mem_guard_2:
        MOV      R0,R4
          CFI FunCall sj_destination
        BL       sj_destination
        CMP      R0,#+0
        BEQ.N    ??mem_guard_3
        POP      {R0,R4,R5,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall mem_tables_ready
        B.N      mem_tables_ready
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
??mem_guard_1:
        MOVS     R0,#+1
        POP      {R1,R4,R5,PC}
          CFI EndBlock cfiBlock9

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock10 Using cfiCommon0
          CFI Function mem_reserved
          CFI NoCalls
        THUMB
mem_reserved:
        MOVS     R2,R1
        BEQ.N    ??mem_reserved_0
        MOV      R3,#+4294967295
        SUBS     R3,R3,R0
        CMP      R3,R1
        BCC.N    ??mem_reserved_1
        LDR.W    R2,??DataTable19_2
        CMP      R0,R2
        BCS.N    ??mem_reserved_0
        ADDS     R0,R1,R0
        LDR.W    R1,??DataTable21
        B.N      ?Subroutine1
??mem_reserved_1:
        MOVS     R0,#+1
        BX       LR
??mem_reserved_0:
        MOVS     R0,#+0
        BX       LR
          CFI EndBlock cfiBlock10

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock11 Using cfiCommon0
          CFI Function mem_replace_sector
        THUMB
mem_replace_sector:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R5,R0
        MOV      R4,R1
          CFI FunCall sj_destination
        BL       sj_destination
        CMP      R0,#+0
        IT       NE
        CMPNE    R4,#+0
        BNE.N    ??mem_replace_sector_0
        MOVS     R0,#+0
        POP      {R1,R4,R5,PC}
??mem_replace_sector_0:
          CFI FunCall user_wait_stable_power
        BL       user_wait_stable_power
        MOV      R1,R4
        MOV      R0,R5
          CFI FunCall sj_replace
        BL       sj_replace
        BL       ?Subroutine2
??CrossCallReturnLabel_1:
        POP      {R1,R4,R5,PC}
          CFI EndBlock cfiBlock11

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond12 Using cfiCommon0
          CFI Function mem_recover
          CFI Conditional ??CrossCallReturnLabel_0
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI Block cfiCond13 Using cfiCommon0
          CFI (cfiCond13) Function mem_replace_sector
          CFI (cfiCond13) Conditional ??CrossCallReturnLabel_1
          CFI (cfiCond13) R4 Frame(CFA, -12)
          CFI (cfiCond13) R5 Frame(CFA, -8)
          CFI (cfiCond13) R14 Frame(CFA, -4)
          CFI (cfiCond13) CFA R13+16
          CFI Block cfiCond14 Using cfiCommon0
          CFI (cfiCond14) Function mem_replace_pages
          CFI (cfiCond14) Conditional ??CrossCallReturnLabel_2
          CFI (cfiCond14) R4 Frame(CFA, -16)
          CFI (cfiCond14) R5 Frame(CFA, -12)
          CFI (cfiCond14) R6 Frame(CFA, -8)
          CFI (cfiCond14) R14 Frame(CFA, -4)
          CFI (cfiCond14) CFA R13+16
          CFI Block cfiPicker15 Using cfiCommon1
          CFI (cfiPicker15) NoFunction
          CFI (cfiPicker15) Picker
        THUMB
?Subroutine2:
        CBZ.N    R0,??Subroutine2_0
        MOVS     R0,#+1
??Subroutine2_0:
        LDR.W    R1,??DataTable19
        STRB     R0,[R1, #+0]
        BX       LR
          CFI EndBlock cfiCond12
          CFI EndBlock cfiCond13
          CFI EndBlock cfiCond14
          CFI EndBlock cfiPicker15

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock16 Using cfiCommon0
          CFI Function mem_replace_pages
        THUMB
mem_replace_pages:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        MOV      R4,R0
        MOV      R6,R1
        MOV      R5,R2
          CFI FunCall sj_destination
        BL       sj_destination
        CBZ.N    R0,??mem_replace_pages_0
        MOVS     R0,R6
        BNE.N    ??mem_replace_pages_1
??mem_replace_pages_0:
        MOVS     R0,#+0
        POP      {R4-R6,PC}
??mem_replace_pages_1:
          CFI FunCall user_wait_stable_power
        BL       user_wait_stable_power
        MOV      R2,R5
        MOV      R1,R6
        MOV      R0,R4
          CFI FunCall sj_replace_pages
        BL       sj_replace_pages
        BL       ?Subroutine2
??CrossCallReturnLabel_2:
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock16

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock17 Using cfiCommon0
          CFI Function mem_overlay
        THUMB
mem_overlay:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        MOV      R4,R1
        MOV      R6,R2
        LDR      R1,[SP, #+24]
        MOV      R5,R4
        CMP      R5,R6
        IT       CC
        MOVCC    R5,R6
        ADD      R2,R4,#+256
        ADDS     R7,R1,R6
        CMP      R7,R2
        IT       CC
        ADDCC    R2,R1,R6
        CMP      R5,R2
        BCS.N    ??mem_overlay_0
        SUBS     R2,R2,R5
        ADDS     R1,R3,R5
        RSBS     R6,R6,#+0
        ADD      R1,R1,R6
        ADD      R0,R0,R5
        RSBS     R4,R4,#+0
        ADD      R0,R0,R4
        POP      {R3-R7,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R7 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall __aeabi_memcpy
        B.W      __aeabi_memcpy
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
??mem_overlay_0:
        POP      {R0,R4-R7,PC}
          CFI EndBlock cfiBlock17

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock18 Using cfiCommon0
          CFI Function mem_image_page
        THUMB
mem_image_page:
        PUSH     {R2-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+24
        MOV      R6,R0
        MOV      R4,R1
        MOV      R5,R2
        MOV      R2,#+256
        MOV      R1,R5
        LDR      R0,[R6, #+0]
        ADDS     R0,R4,R0
          CFI FunCall w25_read_data
        BL       w25_read_data
        CBZ.N    R0,??mem_image_page_0
        LDRH     R0,[R6, #+16]
        STR      R0,[SP, #+0]
        LDR      R3,[R6, #+8]
        LDR      R2,[R6, #+4]
        MOV      R1,R4
        MOV      R0,R5
          CFI FunCall mem_overlay
        BL       mem_overlay
        LDRH     R0,[R6, #+18]
        STR      R0,[SP, #+0]
        LDR      R3,[R6, #+12]
        LDR      R2,[R6, #+4]
        LDRH     R0,[R6, #+16]
        ADDS     R2,R0,R2
        MOV      R1,R4
        MOV      R0,R5
          CFI FunCall mem_overlay
        BL       mem_overlay
        MOVS     R0,#+1
??mem_image_page_0:
        POP      {R1,R2,R4-R6,PC}
          CFI EndBlock cfiBlock18

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
mem_config_errors:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
cfg_legacy_checked:
        DS8 1

        SECTION `.data`:DATA:REORDER:NOROOT(2)
        DATA
cfg_cached_active:
        DATA32
        DC32 1'889'280

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
cfg_cached_generation:
        DS8 4

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock19 Using cfiCommon0
          CFI Function cfg_image_crc
        THUMB
cfg_image_crc:
        PUSH     {R1,R4-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        SUB      SP,SP,#+64
          CFI CFA R13+88
        MOV      R7,R0
        MOV      R4,R2
        MOVS     R2,#+4
        ADD      R1,SP,#+64
        MOV      R0,#+4294967295
          CFI FunCall crc32_update
        BL       crc32_update
        MOV      R5,R0
        MOVS     R6,#+0
??cfg_image_crc_0:
        MOVS     R2,#+64
        MOV      R1,SP
        ADDS     R0,R6,R7
          CFI FunCall w25_read_data
        BL       w25_read_data
        CBZ.N    R0,??cfg_image_crc_1
        MOVS     R2,#+64
        MOV      R1,SP
        MOV      R0,R5
          CFI FunCall crc32_update
        BL       crc32_update
        MOV      R5,R0
        ADDS     R6,R6,#+64
        CMP      R6,#+4096
        BLT.N    ??cfg_image_crc_0
        B.N      ??cfg_image_crc_2
??cfg_image_crc_1:
        ADD      SP,SP,#+68
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI CFA R13+88
??cfg_image_crc_2:
        STR      R5,[R4, #+0]
        MOVS     R0,#+1
        B.N      ??cfg_image_crc_1
          CFI EndBlock cfiBlock19

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock20 Using cfiCommon0
          CFI Function cfg_valid
        THUMB
cfg_valid:
        PUSH     {R3-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+20
        SUB      SP,SP,#+20
          CFI CFA R13+40
        MOV      R4,R0
        MOV      R5,R1
        MOVS     R2,#+16
        ADD      R1,SP,#+4
        ADD      R0,R4,#+4096
          CFI FunCall w25_read_data
        BL       w25_read_data
        CBZ.N    R0,??cfg_valid_0
        LDR      R1,[SP, #+4]
        LDR.W    R0,??DataTable21_1
        CMP      R1,R0
        BNE.N    ??cfg_valid_0
        ADD      R6,SP,#+4
        LDR      R1,[SP, #+16]
        LDR      R0,[R6, #+8]
        MVNS     R0,R0
        CMP      R1,R0
        BNE.N    ??cfg_valid_0
        MOV      R2,SP
        LDR      R1,[R6, #+4]
        MOV      R0,R4
          CFI FunCall cfg_image_crc
        BL       cfg_image_crc
        CBZ.N    R0,??cfg_valid_0
        LDR      R1,[SP, #+0]
        LDR      R0,[R6, #+8]
        CMP      R1,R0
        BEQ.N    ??cfg_valid_1
??cfg_valid_0:
        MOVS     R0,#+0
        B.N      ??cfg_valid_2
??cfg_valid_1:
        LDR      R0,[R6, #+4]
        STR      R0,[R5, #+0]
        MOVS     R0,#+1
??cfg_valid_2:
        ADD      SP,SP,#+24
          CFI CFA R13+16
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock20

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock21 Using cfiCommon0
          CFI Function cfg_select
        THUMB
cfg_select:
        PUSH     {R1-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+32
        MOV      R5,R0
        MOVS     R0,#+0
        STR      R0,[SP, #+4]
        STR      R0,[SP, #+0]
        LDR.W    R6,??DataTable21_2
        ADD      R1,SP,#+4
        MOV      R0,R6
          CFI FunCall cfg_valid
        BL       cfg_valid
        MOV      R4,R0
        LDR.W    R7,??DataTable21_3
        MOV      R1,SP
        MOV      R0,R7
          CFI FunCall cfg_valid
        BL       cfg_valid
        CBZ.N    R0,??cfg_select_0
        CBZ.N    R4,??cfg_select_1
        LDR      R1,[SP, #+0]
        LDR      R0,[SP, #+4]
        SUBS     R1,R1,R0
        CMP      R1,#+1
        BLT.N    ??cfg_select_2
??cfg_select_1:
        LDR      R0,[SP, #+0]
        STR      R0,[R5, #+0]
        LDR.W    R0,??DataTable21_4
        STR      R7,[R0, #+0]
        BL       ?Subroutine13
??CrossCallReturnLabel_25:
        MOV      R0,R7
        POP      {R1-R7,PC}
??cfg_select_0:
        CBZ.N    R4,??cfg_select_3
??cfg_select_2:
        LDR      R0,[SP, #+4]
        STR      R0,[R5, #+0]
        LDR.W    R0,??DataTable21_4
        STR      R6,[R0, #+0]
        BL       ?Subroutine13
??CrossCallReturnLabel_26:
        MOV      R0,R6
        POP      {R1-R7,PC}
??cfg_select_3:
        MOVS     R1,#+0
        BL       ??Subroutine13_0
??CrossCallReturnLabel_27:
        STR      R1,[R5, #+0]
        LDR.W    R0,??DataTable21_5
        LDR.W    R1,??DataTable21_4
        STR      R0,[R1, #+0]
        POP      {R1-R7,PC}
          CFI EndBlock cfiBlock21

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock22 Using cfiCommon0
          CFI Function cfg_pages_equal
        THUMB
cfg_pages_equal:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        SUB      SP,SP,#+288
          CFI CFA R13+312
        MOV      R4,R0
        MOV      R5,R1
        MOVS     R6,#+0
??cfg_pages_equal_0:
          CFI FunCall wdt_reload
        BL       wdt_reload
        ADD      R2,SP,#+32
        UXTH     R1,R6
        MOV      R0,R5
          CFI FunCall mem_image_page
        BL       mem_image_page
        CBZ.N    R0,??cfg_pages_equal_1
        MOVS     R7,#+0
??cfg_pages_equal_2:
        MOVS     R2,#+32
        MOV      R1,SP
        ADDS     R0,R6,R4
        ADDS     R0,R7,R0
          CFI FunCall w25_read_data
        BL       w25_read_data
        CBZ.N    R0,??cfg_pages_equal_1
        BL       ?Subroutine10
??CrossCallReturnLabel_20:
        CMP      R0,#+0
        BNE.N    ??cfg_pages_equal_1
        ADDS     R7,R7,#+32
        CMP      R7,#+256
        BCC.N    ??cfg_pages_equal_2
        B.N      ??cfg_pages_equal_3
??cfg_pages_equal_1:
        MOVS     R0,#+0
        B.N      ??cfg_pages_equal_4
??cfg_pages_equal_3:
        ADD      R6,R6,#+256
        CMP      R6,#+4096
        BLT.N    ??cfg_pages_equal_0
        MOVS     R0,#+1
??cfg_pages_equal_4:
        ADD      SP,SP,#+292
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI EndBlock cfiBlock22

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock23 Using cfiCommon0
          CFI Function cfg_equal_flash
        THUMB
cfg_equal_flash:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        SUB      SP,SP,#+128
          CFI CFA R13+144
        MOV      R6,R0
        MOV      R4,R1
        MOVS     R5,#+0
??cfg_equal_flash_0:
        MOVS     R2,#+64
        ADD      R1,SP,#+64
        ADDS     R0,R5,R6
          CFI FunCall w25_read_data
        BL       w25_read_data
        CBZ.N    R0,??cfg_equal_flash_1
        MOVS     R2,#+64
        MOV      R1,SP
        ADDS     R0,R5,R4
          CFI FunCall w25_read_data
        BL       w25_read_data
        CBZ.N    R0,??cfg_equal_flash_1
        MOVS     R2,#+64
        MOV      R1,SP
        ADD      R0,SP,#+64
          CFI FunCall memcmp
        BL       memcmp
        CMP      R0,#+0
        BNE.N    ??cfg_equal_flash_1
        ADDS     R5,R5,#+64
        CMP      R5,#+4096
        BLT.N    ??cfg_equal_flash_0
        B.N      ??cfg_equal_flash_2
??cfg_equal_flash_1:
        MOVS     R0,#+0
        B.N      ??cfg_equal_flash_3
??cfg_equal_flash_2:
        MOVS     R0,#+1
??cfg_equal_flash_3:
        ADD      SP,SP,#+128
          CFI CFA R13+16
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock23

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock24 Using cfiCommon0
          CFI Function cfg_legacy_crc
        THUMB
cfg_legacy_crc:
        PUSH     {R4-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+20
        SUB      SP,SP,#+68
          CFI CFA R13+88
        MOV      R4,R0
        MOVW     R5,#+65535
        MOVS     R7,#+0
??cfg_legacy_crc_0:
        RSB      R6,R7,#+510
        UXTH     R0,R6
        CMP      R0,#+65
        IT       CS
        MOVCS    R6,#+64
        UXTH     R2,R6
        ADD      R1,SP,#+4
        ADDS     R0,R7,R4
        BL       ?Subroutine11
??CrossCallReturnLabel_22:
        CBZ.N    R0,??cfg_legacy_crc_1
        MOV      R2,R5
        UXTH     R1,R6
        ADD      R0,SP,#+4
          CFI FunCall crc16_calcInit
        BL       crc16_calcInit
        MOV      R5,R0
        ADDS     R7,R6,R7
        UXTH     R7,R7
        CMP      R7,#+510
        BLT.N    ??cfg_legacy_crc_0
        MOVS     R2,#+2
        MOV      R1,SP
        LDR.W    R0,??DataTable21_6
        ADDS     R0,R0,R4
          CFI FunCall w25_read_data
        BL       w25_read_data
        CBZ.N    R0,??cfg_legacy_crc_1
        LDRH     R0,[SP, #+0]
        CMP      R5,R0
        IT       EQ
        MOVEQ    R0,#+1
        BEQ.N    ??cfg_legacy_crc_2
??cfg_legacy_crc_1:
        MOVS     R0,#+0
??cfg_legacy_crc_2:
        ADD      SP,SP,#+68
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI EndBlock cfiBlock24

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock25 Using cfiCommon0
          CFI Function cfg_publish_pages
        THUMB
cfg_publish_pages:
        PUSH     {R3-R8,LR}
          CFI R14 Frame(CFA, -4)
          CFI R8 Frame(CFA, -8)
          CFI R7 Frame(CFA, -12)
          CFI R6 Frame(CFA, -16)
          CFI R5 Frame(CFA, -20)
          CFI R4 Frame(CFA, -24)
          CFI CFA R13+28
        SUB      SP,SP,#+276
          CFI CFA R13+304
        MOV      R8,R0
        MOV      R5,R2
        LDR.W    R6,??DataTable21_2
        CMP      R1,R6
        IT       EQ
        LDREQ.W  R6,??DataTable21_3
        ADD      R0,R6,#+4096
          CFI FunCall w25_erase_sector
        BL       w25_erase_sector
        CMP      R0,#+0
        BEQ.N    ??cfg_publish_pages_0
        MOVS     R2,#+16
        ADD      R1,SP,#+4
        ADD      R0,R6,#+4096
          CFI FunCall w25_read_data
        BL       w25_read_data
        CMP      R0,#+0
        BEQ.N    ??cfg_publish_pages_0
        LDR      R0,[SP, #+4]
        CMN      R0,#+1
        ITTT     EQ
        ADDEQ    R7,SP,#+4
        LDREQ    R0,[R7, #+4]
        CMNEQ    R0,#+1
        BNE.N    ??cfg_publish_pages_0
        LDR      R0,[R7, #+8]
        CMN      R0,#+1
        ITT      EQ
        LDREQ    R0,[R7, #+12]
        CMNEQ    R0,#+1
        BNE.N    ??cfg_publish_pages_0
        MOV      R0,R6
          CFI FunCall w25_erase_sector
        BL       w25_erase_sector
        CMP      R0,#+0
        BEQ.N    ??cfg_publish_pages_0
        LDR.W    R0,??DataTable21_1
        STR      R0,[SP, #+4]
        ADDS     R5,R5,#+1
        STR      R5,[R7, #+4]
        MOVS     R2,#+4
        ADD      R1,SP,#+8
        MOV      R0,#+4294967295
          CFI FunCall crc32_update
        BL       crc32_update
        MOV      R5,R0
        MOVS     R4,#+0
??cfg_publish_pages_1:
          CFI FunCall wdt_reload
        BL       wdt_reload
        ADD      R2,SP,#+20
        UXTH     R1,R4
        MOV      R0,R8
          CFI FunCall mem_image_page
        BL       mem_image_page
        CMP      R0,#+0
        BEQ.N    ??cfg_publish_pages_0
        MOV      R2,#+256
        ADD      R1,SP,#+20
        MOV      R0,R5
          CFI FunCall crc32_update
        BL       crc32_update
        MOV      R5,R0
        MOV      R2,#+256
        ADD      R1,SP,#+20
        ADDS     R0,R4,R6
          CFI FunCall w25_write_data
        BL       w25_write_data
        CBZ.N    R0,??cfg_publish_pages_0
        ADD      R4,R4,#+256
        CMP      R4,#+4096
        BLT.N    ??cfg_publish_pages_1
        MOV      R1,R8
        MOV      R0,R6
          CFI FunCall cfg_pages_equal
        BL       cfg_pages_equal
        CBZ.N    R0,??cfg_publish_pages_0
        STR      R5,[R7, #+8]
        MVNS     R0,R5
        STR      R0,[R7, #+12]
        MOVS     R2,#+16
        ADD      R1,SP,#+4
        ADD      R0,R6,#+4096
          CFI FunCall w25_write_data
        BL       w25_write_data
        CBZ.N    R0,??cfg_publish_pages_0
        MOV      R1,SP
        MOV      R0,R6
          CFI FunCall cfg_valid
        BL       cfg_valid
        CBZ.N    R0,??cfg_publish_pages_0
        LDR      R0,[SP, #+0]
        LDR      R1,[R7, #+4]
        CMP      R0,R1
        BNE.N    ??cfg_publish_pages_0
        LDR.W    R0,??DataTable21_4
        STR      R6,[R0, #+0]
        LDR      R1,[SP, #+0]
        BL       ??Subroutine13_0
??CrossCallReturnLabel_28:
        MOVS     R0,#+1
??cfg_publish_pages_2:
        ADD      SP,SP,#+280
          CFI CFA R13+24
        POP      {R4-R8,PC}
          CFI CFA R13+304
??cfg_publish_pages_0:
        MOVS     R0,#+0
        B.N      ??cfg_publish_pages_2
          CFI EndBlock cfiBlock25

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond26 Using cfiCommon0
          CFI Function cfg_select
          CFI Conditional ??CrossCallReturnLabel_25
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+32
          CFI Block cfiCond27 Using cfiCommon0
          CFI (cfiCond27) Function cfg_select
          CFI (cfiCond27) Conditional ??CrossCallReturnLabel_26
          CFI (cfiCond27) R4 Frame(CFA, -20)
          CFI (cfiCond27) R5 Frame(CFA, -16)
          CFI (cfiCond27) R6 Frame(CFA, -12)
          CFI (cfiCond27) R7 Frame(CFA, -8)
          CFI (cfiCond27) R14 Frame(CFA, -4)
          CFI (cfiCond27) CFA R13+32
          CFI Block cfiPicker28 Using cfiCommon1
          CFI (cfiPicker28) NoFunction
          CFI (cfiPicker28) Picker
        THUMB
?Subroutine13:
        LDR      R1,[R5, #+0]
          CFI Block cfiCond29 Using cfiCommon0
          CFI (cfiCond29) Function cfg_select
          CFI (cfiCond29) Conditional ??CrossCallReturnLabel_27
          CFI (cfiCond29) R4 Frame(CFA, -20)
          CFI (cfiCond29) R5 Frame(CFA, -16)
          CFI (cfiCond29) R6 Frame(CFA, -12)
          CFI (cfiCond29) R7 Frame(CFA, -8)
          CFI (cfiCond29) R14 Frame(CFA, -4)
          CFI (cfiCond29) CFA R13+32
          CFI Block cfiCond30 Using cfiCommon0
          CFI (cfiCond30) Function cfg_publish_pages
          CFI (cfiCond30) Conditional ??CrossCallReturnLabel_28
          CFI (cfiCond30) R4 Frame(CFA, -24)
          CFI (cfiCond30) R5 Frame(CFA, -20)
          CFI (cfiCond30) R6 Frame(CFA, -16)
          CFI (cfiCond30) R7 Frame(CFA, -12)
          CFI (cfiCond30) R8 Frame(CFA, -8)
          CFI (cfiCond30) R14 Frame(CFA, -4)
          CFI (cfiCond30) CFA R13+304
          CFI Block cfiCond31 Using cfiCommon0
          CFI (cfiCond31) Function mem_config_poll
          CFI (cfiCond31) Conditional ??CrossCallReturnLabel_29
          CFI (cfiCond31) R4 Frame(CFA, -16)
          CFI (cfiCond31) R5 Frame(CFA, -12)
          CFI (cfiCond31) R6 Frame(CFA, -8)
          CFI (cfiCond31) R14 Frame(CFA, -4)
          CFI (cfiCond31) CFA R13+80
??Subroutine13_0:
        LDR.W    R0,??DataTable21_7
        STR      R1,[R0, #+0]
        BX       LR
          CFI EndBlock cfiCond26
          CFI EndBlock cfiCond27
          CFI EndBlock cfiPicker28
          CFI EndBlock cfiCond29
          CFI EndBlock cfiCond30
          CFI EndBlock cfiCond31

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock32 Using cfiCommon0
          CFI Function cfg_publish
        THUMB
cfg_publish:
        PUSH     {R4-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+20
        SUB      SP,SP,#+20
          CFI CFA R13+40
        MOV      R5,R0
        MOV      R7,R1
        MOV      R6,R2
        MOV      R4,R3
        MOV      R0,SP
        MOVS     R1,#+20
          CFI FunCall __aeabi_memclr4
        BL       __aeabi_memclr4
        STR      R7,[SP, #+0]
        STR      R5,[SP, #+8]
        CMP      R5,#+0
        ITE      EQ
        MOVEQ    R0,#+0
        MOVNE    R0,#+4096
        STRH     R0,[SP, #+16]
        MOV      R2,R4
        MOV      R1,R6
        MOV      R0,SP
          CFI FunCall cfg_publish_pages
        BL       cfg_publish_pages
        ADD      SP,SP,#+20
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI EndBlock cfiBlock32

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
        DATA32
        DC32 0, 0, 0x0, 0x0
        DATA16
        DC16 0, 0

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock33 Using cfiCommon0
          CFI Function cfg_prepare
        THUMB
cfg_prepare:
        PUSH     {R2-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+24
        LDR.W    R4,??DataTable21_8
        LDRB     R0,[R4, #+0]
        CBZ.N    R0,??cfg_prepare_0
        MOVS     R0,#+1
        POP      {R1,R2,R4-R6,PC}
??cfg_prepare_0:
        MOV      R0,SP
          CFI FunCall cfg_select
        BL       cfg_select
        MOV      R6,R0
        MOVS     R0,#+0
          CFI FunCall cfg_legacy_crc
        BL       cfg_legacy_crc
        CBZ.N    R0,??cfg_prepare_1
        LDR.W    R5,??DataTable21_5
        CMP      R6,R5
        BEQ.N    ??cfg_prepare_2
        MOV      R0,#+512
          CFI FunCall cfg_legacy_crc
        BL       cfg_legacy_crc
        CBZ.N    R0,??cfg_prepare_1
        MOV      R1,R5
        MOV      R0,R6
          CFI FunCall cfg_equal_flash
        BL       cfg_equal_flash
        CBNZ.N   R0,??cfg_prepare_1
??cfg_prepare_2:
        LDR      R3,[SP, #+0]
        MOV      R2,R6
        MOV      R1,R5
        MOVS     R0,#+0
          CFI FunCall cfg_publish
        BL       cfg_publish
        CBNZ.N   R0,??cfg_prepare_1
        BL       ?Subroutine14
??CrossCallReturnLabel_30:
        POP      {R1,R2,R4-R6,PC}
??cfg_prepare_1:
        MOVS     R0,#+1
        STRB     R0,[R4, #+0]
        POP      {R1,R2,R4-R6,PC}
          CFI EndBlock cfiBlock33

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock34 Using cfiCommon0
          CFI Function cfg_blank_sector
        THUMB
cfg_blank_sector:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        SUB      SP,SP,#+64
          CFI CFA R13+80
        MOV      R5,R0
        MOVS     R4,#+0
??cfg_blank_sector_0:
        MOVS     R2,#+64
        MOV      R1,SP
        ADDS     R0,R4,R5
          CFI FunCall w25_read_data
        BL       w25_read_data
        CBZ.N    R0,??cfg_blank_sector_1
        MOVS     R1,#+64
        MOV      R0,SP
          CFI FunCall fast_all_ff
        BL       fast_all_ff
        CBZ.N    R0,??cfg_blank_sector_1
        ADDS     R4,R4,#+64
        CMP      R4,#+4096
        BLT.N    ??cfg_blank_sector_0
        B.N      ??cfg_blank_sector_2
??cfg_blank_sector_1:
        MOVS     R0,#+0
        B.N      ??cfg_blank_sector_3
??cfg_blank_sector_2:
        MOVS     R0,#+1
??cfg_blank_sector_3:
        ADD      SP,SP,#+68
          CFI CFA R13+12
        POP      {R4,R5,PC}
          CFI EndBlock cfiBlock34

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock35 Using cfiCommon0
          CFI Function cfg_mirror
        THUMB
cfg_mirror:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        SUB      SP,SP,#+288
          CFI CFA R13+312
        MOV      R6,R0
        LDR.W    R4,??DataTable21_5
        MOV      R1,R6
        MOV      R0,R4
          CFI FunCall cfg_pages_equal
        BL       cfg_pages_equal
        CBZ.N    R0,??cfg_mirror_0
        MOVS     R0,#+1
        B.N      ??cfg_mirror_1
??cfg_mirror_0:
        LDR.W    R5,??DataTable21_9
        MOV      R0,R5
          CFI FunCall w25_erase_sector
        BL       w25_erase_sector
        CMP      R0,#+0
        BEQ.N    ??cfg_mirror_2
        MOV      R0,R5
          CFI FunCall cfg_blank_sector
        BL       cfg_blank_sector
        CMP      R0,#+0
        BEQ.N    ??cfg_mirror_2
        MOV      R0,#+1892352
          CFI FunCall w25_erase_sector
        BL       w25_erase_sector
        CMP      R0,#+0
        BEQ.N    ??cfg_mirror_2
        MOV      R0,#+1892352
          CFI FunCall cfg_blank_sector
        BL       cfg_blank_sector
        CMP      R0,#+0
        BEQ.N    ??cfg_mirror_2
        MOV      R5,#+1024
??cfg_mirror_3:
          CFI FunCall wdt_reload
        BL       wdt_reload
        BL       ?Subroutine6
??CrossCallReturnLabel_12:
        CMP      R0,#+0
        BEQ.N    ??cfg_mirror_2
        BL       ?Subroutine8
??CrossCallReturnLabel_16:
        CMP      R0,#+0
        BEQ.N    ??cfg_mirror_2
        ADD      R5,R5,#+256
        CMP      R5,#+4096
        BLT.N    ??cfg_mirror_3
        MOV      R5,#+512
        MOV      R7,R5
??cfg_mirror_4:
        ADD      R2,SP,#+32
        UXTH     R1,R7
        BL       ??Subroutine6_0
??CrossCallReturnLabel_9:
        CMP      R0,#+0
        BEQ.N    ??cfg_mirror_2
        MOV      R2,#+256
        ADD      R1,SP,#+32
        ADDS     R0,R4,R7
          CFI FunCall w25_write_data
        BL       w25_write_data
        CMP      R0,#+0
        BEQ.N    ??cfg_mirror_2
        ADD      R7,R7,#+256
        CMP      R7,#+1024
        BLT.N    ??cfg_mirror_4
??cfg_mirror_5:
        BL       ?Subroutine6
??CrossCallReturnLabel_11:
        CBZ.N    R0,??cfg_mirror_2
        MOVS     R7,#+0
??cfg_mirror_6:
        MOVS     R2,#+32
        MOV      R1,SP
        ADDS     R0,R7,R5
        ADDS     R0,R4,R0
          CFI FunCall w25_read_data
        BL       w25_read_data
        CBZ.N    R0,??cfg_mirror_2
        BL       ?Subroutine10
??CrossCallReturnLabel_19:
        CMP      R0,#+0
        BNE.N    ??cfg_mirror_2
        ADDS     R7,R7,#+32
        CMP      R7,#+256
        BCC.N    ??cfg_mirror_6
        ADD      R5,R5,#+256
        CMP      R5,#+4096
        BLT.N    ??cfg_mirror_5
        MOVS     R5,#+0
??cfg_mirror_7:
        BL       ?Subroutine6
??CrossCallReturnLabel_10:
        CBZ.N    R0,??cfg_mirror_2
        BL       ?Subroutine8
??CrossCallReturnLabel_15:
        CBZ.N    R0,??cfg_mirror_2
        ADD      R5,R5,#+256
        CMP      R5,#+512
        BLT.N    ??cfg_mirror_7
        B.N      ??cfg_mirror_8
??cfg_mirror_2:
        MOVS     R0,#+0
        B.N      ??cfg_mirror_1
??cfg_mirror_8:
        MOV      R1,R6
        MOV      R0,R4
          CFI FunCall cfg_pages_equal
        BL       cfg_pages_equal
??cfg_mirror_1:
        ADD      SP,SP,#+292
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI EndBlock cfiBlock35

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond36 Using cfiCommon0
          CFI Function cfg_pages_equal
          CFI Conditional ??CrossCallReturnLabel_20
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+312
          CFI Block cfiCond37 Using cfiCommon0
          CFI (cfiCond37) Function cfg_mirror
          CFI (cfiCond37) Conditional ??CrossCallReturnLabel_19
          CFI (cfiCond37) R4 Frame(CFA, -20)
          CFI (cfiCond37) R5 Frame(CFA, -16)
          CFI (cfiCond37) R6 Frame(CFA, -12)
          CFI (cfiCond37) R7 Frame(CFA, -8)
          CFI (cfiCond37) R14 Frame(CFA, -4)
          CFI (cfiCond37) CFA R13+312
          CFI Block cfiPicker38 Using cfiCommon1
          CFI (cfiPicker38) NoFunction
          CFI (cfiPicker38) Picker
        THUMB
?Subroutine10:
        MOVS     R2,#+32
        MOV      R1,SP
        ADD      R0,SP,#+32
        ADD      R0,R0,R7
          CFI (cfiCond36) FunCall cfg_pages_equal memcmp
          CFI (cfiCond37) FunCall cfg_mirror memcmp
        B.W      memcmp
          CFI EndBlock cfiCond36
          CFI EndBlock cfiCond37
          CFI EndBlock cfiPicker38

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond39 Using cfiCommon0
          CFI Function cfg_mirror
          CFI Conditional ??CrossCallReturnLabel_16
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+312
          CFI Block cfiCond40 Using cfiCommon0
          CFI (cfiCond40) Function cfg_mirror
          CFI (cfiCond40) Conditional ??CrossCallReturnLabel_15
          CFI (cfiCond40) R4 Frame(CFA, -20)
          CFI (cfiCond40) R5 Frame(CFA, -16)
          CFI (cfiCond40) R6 Frame(CFA, -12)
          CFI (cfiCond40) R7 Frame(CFA, -8)
          CFI (cfiCond40) R14 Frame(CFA, -4)
          CFI (cfiCond40) CFA R13+312
          CFI Block cfiPicker41 Using cfiCommon1
          CFI (cfiPicker41) NoFunction
          CFI (cfiPicker41) Picker
        THUMB
?Subroutine8:
        MOV      R2,#+256
        ADD      R1,SP,#+32
        ADDS     R0,R4,R5
          CFI (cfiCond39) FunCall cfg_mirror w25_write_data
          CFI (cfiCond39) FunCall cfg_mirror w25_write_data
        B.W      w25_write_data
          CFI EndBlock cfiCond39
          CFI EndBlock cfiCond40
          CFI EndBlock cfiPicker41

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond42 Using cfiCommon0
          CFI Function cfg_mirror
          CFI Conditional ??CrossCallReturnLabel_12
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+312
          CFI Block cfiCond43 Using cfiCommon0
          CFI (cfiCond43) Function cfg_mirror
          CFI (cfiCond43) Conditional ??CrossCallReturnLabel_11
          CFI (cfiCond43) R4 Frame(CFA, -20)
          CFI (cfiCond43) R5 Frame(CFA, -16)
          CFI (cfiCond43) R6 Frame(CFA, -12)
          CFI (cfiCond43) R7 Frame(CFA, -8)
          CFI (cfiCond43) R14 Frame(CFA, -4)
          CFI (cfiCond43) CFA R13+312
          CFI Block cfiCond44 Using cfiCommon0
          CFI (cfiCond44) Function cfg_mirror
          CFI (cfiCond44) Conditional ??CrossCallReturnLabel_10
          CFI (cfiCond44) R4 Frame(CFA, -20)
          CFI (cfiCond44) R5 Frame(CFA, -16)
          CFI (cfiCond44) R6 Frame(CFA, -12)
          CFI (cfiCond44) R7 Frame(CFA, -8)
          CFI (cfiCond44) R14 Frame(CFA, -4)
          CFI (cfiCond44) CFA R13+312
          CFI Block cfiPicker45 Using cfiCommon1
          CFI (cfiPicker45) NoFunction
          CFI (cfiPicker45) Picker
        THUMB
?Subroutine6:
        ADD      R2,SP,#+32
        UXTH     R1,R5
          CFI Block cfiCond46 Using cfiCommon0
          CFI (cfiCond46) Function cfg_mirror
          CFI (cfiCond46) Conditional ??CrossCallReturnLabel_9
          CFI (cfiCond46) R4 Frame(CFA, -20)
          CFI (cfiCond46) R5 Frame(CFA, -16)
          CFI (cfiCond46) R6 Frame(CFA, -12)
          CFI (cfiCond46) R7 Frame(CFA, -8)
          CFI (cfiCond46) R14 Frame(CFA, -4)
          CFI (cfiCond46) CFA R13+312
??Subroutine6_0:
        MOV      R0,R6
          CFI (cfiCond42) FunCall cfg_mirror mem_image_page
          CFI (cfiCond42) FunCall cfg_mirror mem_image_page
          CFI (cfiCond42) FunCall cfg_mirror mem_image_page
          CFI (cfiCond42) FunCall cfg_mirror mem_image_page
        B.N      mem_image_page
          CFI EndBlock cfiCond42
          CFI EndBlock cfiCond43
          CFI EndBlock cfiCond44
          CFI EndBlock cfiPicker45
          CFI EndBlock cfiCond46

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock47 Using cfiCommon0
          CFI Function cfg_commit_parts
        THUMB
cfg_commit_parts:
        PUSH     {R3-R9,LR}
          CFI R14 Frame(CFA, -4)
          CFI R9 Frame(CFA, -8)
          CFI R8 Frame(CFA, -12)
          CFI R7 Frame(CFA, -16)
          CFI R6 Frame(CFA, -20)
          CFI R5 Frame(CFA, -24)
          CFI R4 Frame(CFA, -28)
          CFI CFA R13+32
        SUB      SP,SP,#+24
          CFI CFA R13+56
        MOV      R9,R0
        MOV      R6,R1
        MOV      R8,R2
        MOV      R7,R3
          CFI FunCall cfg_prepare
        BL       cfg_prepare
        CMP      R0,#+0
        BEQ.N    ??cfg_commit_parts_0
        LDR      R4,[SP, #+56]
        MOV      R0,SP
          CFI FunCall cfg_select
        BL       cfg_select
        MOV      R5,R0
        STR      R5,[SP, #+4]
        STR      R9,[SP, #+8]
        STR      R6,[SP, #+12]
        STR      R7,[SP, #+16]
        STRH     R8,[SP, #+20]
        STRH     R4,[SP, #+22]
        LDR.W    R1,??DataTable21_5
        CMP      R5,R1
        BNE.N    ??cfg_commit_parts_1
        LDR      R2,[SP, #+0]
        ADD      R0,SP,#+4
          CFI FunCall cfg_publish_pages
        BL       cfg_publish_pages
        CBZ.N    R0,??cfg_commit_parts_2
        LDR.W    R0,??DataTable21_2
        STR      R0,[SP, #+4]
        ADD      R0,SP,#+4
          CFI FunCall cfg_mirror
        BL       cfg_mirror
        B.N      ??cfg_commit_parts_3
??cfg_commit_parts_1:
        ADD      R0,SP,#+4
          CFI FunCall cfg_mirror
        BL       cfg_mirror
        CBNZ.N   R0,??cfg_commit_parts_4
??cfg_commit_parts_2:
        BL       ?Subroutine14
??CrossCallReturnLabel_31:
        B.N      ??cfg_commit_parts_0
??cfg_commit_parts_4:
        ADD      R1,SP,#+4
        MOV      R0,R5
          CFI FunCall cfg_pages_equal
        BL       cfg_pages_equal
        CBNZ.N   R0,??cfg_commit_parts_5
        LDR      R2,[SP, #+0]
        MOV      R1,R5
        ADD      R0,SP,#+4
          CFI FunCall cfg_publish_pages
        BL       cfg_publish_pages
??cfg_commit_parts_3:
        MOVS     R1,R0
        BNE.N    ??cfg_commit_parts_6
        LDR.W    R2,??DataTable21_10
        LDR      R0,[R2, #+0]
        ADDS     R0,R0,#+1
        STR      R0,[R2, #+0]
??cfg_commit_parts_6:
        MOVS     R0,R1
        BEQ.N    ??cfg_commit_parts_0
??cfg_commit_parts_5:
        MOVS     R0,#+1
??cfg_commit_parts_0:
        ADD      SP,SP,#+28
          CFI CFA R13+28
        POP      {R4-R9,PC}
          CFI EndBlock cfiBlock47

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond48 Using cfiCommon0
          CFI Function cfg_prepare
          CFI Conditional ??CrossCallReturnLabel_30
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
          CFI Block cfiCond49 Using cfiCommon0
          CFI (cfiCond49) Function cfg_commit_parts
          CFI (cfiCond49) Conditional ??CrossCallReturnLabel_31
          CFI (cfiCond49) R4 Frame(CFA, -28)
          CFI (cfiCond49) R5 Frame(CFA, -24)
          CFI (cfiCond49) R6 Frame(CFA, -20)
          CFI (cfiCond49) R7 Frame(CFA, -16)
          CFI (cfiCond49) R8 Frame(CFA, -12)
          CFI (cfiCond49) R9 Frame(CFA, -8)
          CFI (cfiCond49) R14 Frame(CFA, -4)
          CFI (cfiCond49) CFA R13+56
          CFI Block cfiPicker50 Using cfiCommon1
          CFI (cfiPicker50) NoFunction
          CFI (cfiPicker50) Picker
        THUMB
?Subroutine14:
        LDR.W    R1,??DataTable21_10
        LDR      R0,[R1, #+0]
        ADDS     R0,R0,#+1
        STR      R0,[R1, #+0]
        MOVS     R0,#+0
        BX       LR
          CFI EndBlock cfiCond48
          CFI EndBlock cfiCond49
          CFI EndBlock cfiPicker50

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock51 Using cfiCommon0
          CFI Function cfg_range
          CFI NoCalls
        THUMB
cfg_range:
        LDR.W    R2,??DataTable21_11
        ADDS     R2,R2,R0
        CMP      R2,#+4096
        BCS.N    ??cfg_range_0
        LDR.W    R2,??DataTable21_12
        SUBS     R0,R2,R0
        B.N      ?Subroutine1
??cfg_range_0:
        MOVS     R0,#+0
        BX       LR
          CFI EndBlock cfiBlock51

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock52 Using cfiCommon0
          CFI NoFunction
        THUMB
?Subroutine1:
        CMP      R0,R1
        SBCS     R0,R0,R0
        MVNS     R0,R0
        LSRS     R0,R0,#+31
        BX       LR
          CFI EndBlock cfiBlock52

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock53 Using cfiCommon0
          CFI Function cfg_read
        THUMB
cfg_read:
        PUSH     {R2-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+24
        MOV      R4,R0
        MOV      R5,R1
        MOV      R6,R2
          CFI FunCall mem_config_result
        BL       mem_config_result
        CMP      R0,#+1
        BNE.N    ??cfg_read_0
        LDR.W    R0,??DataTable21_4
        LDR      R0,[R0, #+0]
        B.N      ??cfg_read_1
??cfg_read_0:
          CFI FunCall cfg_prepare
        BL       cfg_prepare
        CBZ.N    R0,??cfg_read_2
        MOV      R0,SP
          CFI FunCall cfg_select
        BL       cfg_select
        B.N      ??cfg_read_1
??cfg_read_2:
        LDR.W    R0,??DataTable21_5
??cfg_read_1:
        MOV      R2,R6
        MOV      R1,R5
        ADDS     R4,R4,R0
        LDR.W    R0,??DataTable21_11
        ADDS     R0,R0,R4
          CFI FunCall w25_read_data
        BL       w25_read_data
        POP      {R1,R2,R4-R6,PC}
          CFI EndBlock cfiBlock53

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock54 Using cfiCommon0
          CFI Function mem_read
        THUMB
mem_read:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        BL       ?Subroutine4
??CrossCallReturnLabel_6:
        BL       ?Subroutine16
??CrossCallReturnLabel_36:
        CBZ.N    R0,??mem_read_0
        MOV      R2,R5
        MOV      R1,R4
        MOV      R0,R6
        POP      {R4-R6,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall cfg_read
        B.N      cfg_read
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
??mem_read_0:
        MOV      R1,R5
        MOV      R0,R6
          CFI FunCall mem_guard
        BL       mem_guard
          CFI EndBlock cfiBlock54
        REQUIRE ?Subroutine0

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock55 Using cfiCommon0
          CFI NoFunction
          CFI CFA R13+16
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
        THUMB
?Subroutine0:
        CBZ.N    R0,??Subroutine0_0
        MOV      R2,R5
        MOV      R1,R4
        MOV      R0,R6
          CFI FunCall sj_io_read w25_read_data
          CFI FunCall mem_read w25_read_data
        BL       w25_read_data
        CBZ.N    R0,??Subroutine0_0
        MOVS     R0,#+1
??Subroutine0_0:
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock55

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond56 Using cfiCommon0
          CFI Function mem_read
          CFI Conditional ??CrossCallReturnLabel_36
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
          CFI Block cfiCond57 Using cfiCommon0
          CFI (cfiCond57) Function mem_write
          CFI (cfiCond57) Conditional ??CrossCallReturnLabel_35
          CFI (cfiCond57) R4 Frame(CFA, -16)
          CFI (cfiCond57) R5 Frame(CFA, -12)
          CFI (cfiCond57) R6 Frame(CFA, -8)
          CFI (cfiCond57) R14 Frame(CFA, -4)
          CFI (cfiCond57) CFA R13+16
          CFI Block cfiPicker58 Using cfiCommon1
          CFI (cfiPicker58) NoFunction
          CFI (cfiPicker58) Picker
        THUMB
?Subroutine16:
        MOV      R1,R5
          CFI Block cfiCond59 Using cfiCommon0
          CFI (cfiCond59) Function mem_patch2
          CFI (cfiCond59) Conditional ??CrossCallReturnLabel_34
          CFI (cfiCond59) R4 Frame(CFA, -32)
          CFI (cfiCond59) R5 Frame(CFA, -28)
          CFI (cfiCond59) R6 Frame(CFA, -24)
          CFI (cfiCond59) R7 Frame(CFA, -20)
          CFI (cfiCond59) R8 Frame(CFA, -16)
          CFI (cfiCond59) R9 Frame(CFA, -12)
          CFI (cfiCond59) R10 Frame(CFA, -8)
          CFI (cfiCond59) R14 Frame(CFA, -4)
          CFI (cfiCond59) CFA R13+56
??Subroutine16_0:
        MOV      R0,R6
          CFI (cfiCond56) FunCall mem_read cfg_range
          CFI (cfiCond57) FunCall mem_write cfg_range
          CFI (cfiCond59) FunCall mem_patch2 cfg_range
        B.N      cfg_range
          CFI EndBlock cfiCond56
          CFI EndBlock cfiCond57
          CFI EndBlock cfiPicker58
          CFI EndBlock cfiCond59

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock60 Using cfiCommon0
          CFI Function mem_write
        THUMB
mem_write:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        BL       ?Subroutine4
??CrossCallReturnLabel_5:
        BL       ?Subroutine16
??CrossCallReturnLabel_35:
        CBZ.N    R0,??mem_write_0
        MOV      R2,R5
        MOV      R1,R4
        MOV      R0,R6
        POP      {R4-R6,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall mem_patch
        B.N      mem_patch
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
??mem_write_0:
        MOV      R1,R5
        MOV      R0,R6
          CFI FunCall mem_reserved
        BL       mem_reserved
        CBNZ.N   R0,??mem_write_1
        MOV      R1,R5
        MOV      R0,R6
          CFI FunCall mem_guard
        BL       mem_guard
        CBZ.N    R0,??mem_write_1
        BL       ?Subroutine5
??CrossCallReturnLabel_7:
        CBZ.N    R0,??mem_write_2
        MOVS     R0,#+1
        POP      {R4-R6,PC}
??mem_write_1:
        MOVS     R0,#+0
??mem_write_2:
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock60

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond61 Using cfiCommon0
          CFI Function sj_io_write
          CFI Conditional ??CrossCallReturnLabel_8
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
          CFI Block cfiCond62 Using cfiCommon0
          CFI (cfiCond62) Function mem_write
          CFI (cfiCond62) Conditional ??CrossCallReturnLabel_7
          CFI (cfiCond62) R4 Frame(CFA, -16)
          CFI (cfiCond62) R5 Frame(CFA, -12)
          CFI (cfiCond62) R6 Frame(CFA, -8)
          CFI (cfiCond62) R14 Frame(CFA, -4)
          CFI (cfiCond62) CFA R13+16
          CFI Block cfiPicker63 Using cfiCommon1
          CFI (cfiPicker63) NoFunction
          CFI (cfiPicker63) Picker
        THUMB
?Subroutine5:
        MOV      R2,R5
        MOV      R1,R4
        MOV      R0,R6
          CFI (cfiCond61) FunCall sj_io_write w25_write_data
          CFI (cfiCond62) FunCall mem_write w25_write_data
        B.W      w25_write_data
          CFI EndBlock cfiCond61
          CFI EndBlock cfiCond62
          CFI EndBlock cfiPicker63

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond64 Using cfiCommon0
          CFI Function mem_read
          CFI Conditional ??CrossCallReturnLabel_6
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
          CFI Block cfiCond65 Using cfiCommon0
          CFI (cfiCond65) Function mem_write
          CFI (cfiCond65) Conditional ??CrossCallReturnLabel_5
          CFI (cfiCond65) R4 Frame(CFA, -16)
          CFI (cfiCond65) R5 Frame(CFA, -12)
          CFI (cfiCond65) R6 Frame(CFA, -8)
          CFI (cfiCond65) R14 Frame(CFA, -4)
          CFI (cfiCond65) CFA R13+16
          CFI Block cfiPicker66 Using cfiCommon1
          CFI (cfiPicker66) NoFunction
          CFI (cfiPicker66) Picker
        THUMB
?Subroutine4:
        MOV      R6,R0
        MOV      R4,R1
        MOV      R5,R2
          CFI (cfiCond64) FunCall mem_read user_wait_stable_power
          CFI (cfiCond65) FunCall mem_write user_wait_stable_power
        B.W      user_wait_stable_power
          CFI EndBlock cfiCond64
          CFI EndBlock cfiCond65
          CFI EndBlock cfiPicker66

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock67 Using cfiCommon0
          CFI Function mem_patch2
        THUMB
mem_patch2:
        PUSH     {R3-R10,LR}
          CFI R14 Frame(CFA, -4)
          CFI R10 Frame(CFA, -8)
          CFI R9 Frame(CFA, -12)
          CFI R8 Frame(CFA, -16)
          CFI R7 Frame(CFA, -20)
          CFI R6 Frame(CFA, -24)
          CFI R5 Frame(CFA, -28)
          CFI R4 Frame(CFA, -32)
          CFI CFA R13+36
        SUB      SP,SP,#+20
          CFI CFA R13+56
        MOV      R6,R0
        MOV      R4,R1
        MOVS     R8,R2
        MOV      R5,R3
        LDR      R9,[SP, #+56]
        ADD      R10,R9,R8
        BEQ.N    ??mem_patch2_0
        CBZ.N    R4,??mem_patch2_1
??mem_patch2_0:
        MOVS     R0,R9
        BEQ.N    ??mem_patch2_2
        CBZ.N    R5,??mem_patch2_1
??mem_patch2_2:
        MOVW     R0,#+4097
        CMP      R10,R0
        BCS.N    ??mem_patch2_1
          CFI FunCall user_wait_stable_power
        BL       user_wait_stable_power
        UXTH     R1,R10
        BL       ??Subroutine16_0
??CrossCallReturnLabel_34:
        CBZ.N    R0,??mem_patch2_3
        STR      R9,[SP, #+0]
        MOV      R3,R5
        MOV      R2,R8
        MOV      R1,R4
        ADD      R6,R6,#+11264
        UXTH     R0,R6
          CFI FunCall cfg_commit_parts
        BL       cfg_commit_parts
        B.N      ??mem_patch2_4
??mem_patch2_3:
        LSRS     R7,R6,#+12
        LSLS     R7,R7,#+12
        RSB      R0,R6,#+4096
        ADDS     R0,R7,R0
        CMP      R0,R10
        BCC.N    ??mem_patch2_1
        MOV      R0,R7
          CFI FunCall sj_destination
        BL       sj_destination
        CBZ.N    R0,??mem_patch2_1
          CFI FunCall mem_tables_ready
        BL       mem_tables_ready
        CBNZ.N   R0,??mem_patch2_5
??mem_patch2_1:
        MOVS     R0,#+0
        B.N      ??mem_patch2_4
??mem_patch2_5:
        STR      R7,[SP, #+0]
        SUBS     R6,R6,R7
        STR      R6,[SP, #+4]
        STR      R4,[SP, #+8]
        STR      R5,[SP, #+12]
        STRH     R8,[SP, #+16]
        STRH     R9,[SP, #+18]
        MOV      R2,SP
        LDR.W    R1,??DataTable21_13
        MOV      R0,R7
          CFI FunCall mem_replace_pages
        BL       mem_replace_pages
??mem_patch2_4:
        ADD      SP,SP,#+24
          CFI CFA R13+32
        POP      {R4-R10,PC}
          CFI EndBlock cfiBlock67

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock68 Using cfiCommon0
          CFI Function mem_patch
        THUMB
mem_patch:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
        MOVS     R3,#+0
        STR      R3,[SP, #+0]
          CFI FunCall mem_patch2
        BL       mem_patch2
        POP      {R1,PC}
          CFI EndBlock cfiBlock68

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock69 Using cfiCommon0
          CFI Function mem_erase_sector
        THUMB
mem_erase_sector:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        MOV      R4,R0
          CFI FunCall user_wait_stable_power
        BL       user_wait_stable_power
        LSRS     R4,R4,#+12
        MOV      R1,#+4096
        LSLS     R0,R4,#+12
          CFI FunCall mem_reserved
        BL       mem_reserved
        CBNZ.N   R0,??mem_erase_sector_0
        MOV      R1,#+4096
        LSLS     R0,R4,#+12
          CFI FunCall mem_guard
        BL       mem_guard
        CBZ.N    R0,??mem_erase_sector_0
        LSLS     R0,R4,#+12
          CFI FunCall w25_erase_sector
        BL       w25_erase_sector
        CBZ.N    R0,??mem_erase_sector_1
        MOVS     R0,#+1
        POP      {R4,PC}
??mem_erase_sector_0:
        MOVS     R0,#+0
??mem_erase_sector_1:
        POP      {R4,PC}
          CFI EndBlock cfiBlock69

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock70 Using cfiCommon0
          CFI Function mem_rewrite
          CFI FunCall mem_patch
        THUMB
mem_rewrite:
        B.N      mem_patch
          CFI EndBlock cfiBlock70

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock71 Using cfiCommon0
          CFI Function mem_erase_mem
        THUMB
mem_erase_mem:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R5,R0
        MOV      R4,R1
          CFI FunCall user_wait_stable_power
        BL       user_wait_stable_power
        ASRS     R0,R5,#+11
        ADD      R5,R5,R0, LSR #+20
        BFC      R5,#+0,#+12
        B.N      ??mem_erase_mem_0
??mem_erase_mem_1:
        ADD      R5,R5,#+4096
??mem_erase_mem_0:
        CMP      R4,R5
        BCC.N    ??mem_erase_mem_2
          CFI FunCall wdt_reload
        BL       wdt_reload
        MOV      R0,R5
          CFI FunCall mem_erase_sector
        BL       mem_erase_sector
        CMP      R0,#+0
        BNE.N    ??mem_erase_mem_1
        POP      {R1,R4,R5,PC}
??mem_erase_mem_2:
        MOVS     R0,#+1
        POP      {R1,R4,R5,PC}
          CFI EndBlock cfiBlock71

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
cfg_job:
        DS8 104

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock72 Using cfiCommon0
          CFI Function mem_config_result
          CFI NoCalls
        THUMB
mem_config_result:
        LDR.W    R0,??DataTable21_14
        LDRB     R0,[R0, #+99]
        BX       LR
          CFI EndBlock cfiBlock72

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock73 Using cfiCommon0
          CFI Function mem_config_snapshot
          CFI NoCalls
        THUMB
mem_config_snapshot:
        LDR.W    R0,??DataTable21_14
        LDR      R0,[R0, #+0]
        BX       LR
          CFI EndBlock cfiBlock73

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock74 Using cfiCommon0
          CFI Function mem_config_reset
        THUMB
mem_config_reset:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        BL       ?Subroutine15
??CrossCallReturnLabel_32:
        BEQ.N    ??mem_config_reset_0
        LDR      R0,[R4, #+0]
          CFI FunCall free
        BL       free
        MOVS     R0,#+0
        STR      R0,[R4, #+0]
        STRB     R0,[R4, #+99]
        STRB     R0,[R4, #+100]
??mem_config_reset_0:
        POP      {R4,PC}
          CFI EndBlock cfiBlock74

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock75 Using cfiCommon0
          CFI Function mem_config_begin
        THUMB
mem_config_begin:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        MOV      R4,R0
        MOV      R5,R1
        LDR.W    R6,??DataTable21_14
        LDRB     R0,[R6, #+99]
        CBNZ.N   R0,??mem_config_begin_0
          CFI FunCall w25_async_active
        BL       w25_async_active
        CBNZ.N   R0,??mem_config_begin_0
        LDR.W    R0,??DataTable21_8
        LDRB     R0,[R0, #+0]
        CMP      R0,#+0
        IT       NE
        CMPNE    R4,#+0
        BEQ.N    ??mem_config_begin_0
        CBZ.N    R5,??mem_config_begin_0
        MOV      R0,#+1024
          CFI FunCall malloc
        BL       malloc
        STR      R0,[R6, #+0]
        CBNZ.N   R0,??mem_config_begin_1
??mem_config_begin_0:
        MOVS     R0,#+0
        POP      {R1,R4-R7,PC}
??mem_config_begin_1:
        MOV      R7,#+512
        MOV      R2,R7
        MOV      R1,R4
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        MOV      R2,R7
        MOV      R1,R5
        LDR      R0,[R6, #+0]
        ADD      R0,R0,#+512
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        LDR.W    R0,??DataTable21_4
        LDR      R1,[R0, #+0]
        STR      R1,[R6, #+68]
        LDR.W    R2,??DataTable21_2
        CMP      R1,R2
        IT       EQ
        LDREQ.W  R2,??DataTable21_3
        STR      R2,[R6, #+72]
        LDR.N    R1,??DataTable21_1
        STR      R1,[R6, #+76]
        LDR.W    R1,??DataTable21_7
        LDR      R1,[R1, #+0]
        ADDS     R1,R1,#+1
        STR      R1,[R6, #+80]
        MOVS     R1,#+0
        STRH     R1,[R6, #+96]
        STRB     R1,[R6, #+102]
        STRB     R1,[R6, #+101]
        LDR      R1,[R0, #+0]
        LDR.N    R0,??DataTable21_5
        CMP      R1,R0
        ITE      EQ
        MOVEQ    R0,#+1
        MOVNE    R0,#+0
        STRB     R0,[R6, #+103]
        MOVS     R0,#+0
        STRB     R0,[R6, #+100]
        MOVS     R0,#+1
        STRB     R0,[R6, #+98]
        STRB     R0,[R6, #+99]
        POP      {R1,R4-R7,PC}
          CFI EndBlock cfiBlock75

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock76 Using cfiCommon0
          CFI Function cfg_async_page
        THUMB
cfg_async_page:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
        LDR.N    R3,??DataTable21_14
        LDRH     R0,[R3, #+96]
        CMP      R0,#+1024
        BCS.N    ??cfg_async_page_0
        MOVS     R2,#+64
        LDR      R1,[R3, #+0]
        ADD      R1,R1,R0
        ADDS     R0,R3,#+4
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        MOVS     R0,#+1
        POP      {R1,PC}
??cfg_async_page_0:
        MOVS     R2,#+64
        ADDS     R1,R3,#+4
        LDR      R0,[R3, #+68]
        LDRH     R3,[R3, #+96]
        ADDS     R0,R3,R0
        POP      {R3,LR}
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall w25_read_data
        B.W      w25_read_data
          CFI EndBlock cfiBlock76

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19:
        DATA32
        DC32     mem_storage_ready

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_1:
        DATA32
        DC32     store_runtime

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable19_2:
        DATA32
        DC32     0x1fb000

        SECTION `.text`:CODE:NOROOT(2)
          CFI Block cfiBlock77 Using cfiCommon0
          CFI Function mem_config_poll
        THUMB
mem_config_poll:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        SUB      SP,SP,#+64
          CFI CFA R13+80
        MOV      R5,R0
        BL       ?Subroutine15
??CrossCallReturnLabel_33:
        BNE.N    ??mem_config_poll_1
        LDRB     R0,[R4, #+100]
        CBZ.N    R0,??mem_config_poll_2
        MOV      R0,R5
          CFI FunCall w25_async_poll
        BL       w25_async_poll
        CMP      R0,#+1
        BEQ.N    ??mem_config_poll_1
        MOVS     R1,#+0
        STRB     R1,[R4, #+100]
        CMP      R0,#+2
??mem_config_poll_3:
        BNE.W    ??mem_config_poll_4
        LDRB     R0,[R4, #+103]
        CMP      R0,#+0
        BEQ.N    ??mem_config_poll_1
        LDRB     R0,[R4, #+98]
        CMP      R0,#+2
        BNE.N    ??mem_config_poll_1
        LDR      R0,[R4, #+72]
        STR      R0,[R4, #+68]
        LDR.N    R1,??DataTable21_4
        STR      R0,[R1, #+0]
        LDR      R0,[R4, #+80]
        LDR.N    R1,??DataTable21_7
        STR      R0,[R1, #+0]
??mem_config_poll_1:
        B.N      ??mem_config_poll_5
??mem_config_poll_2:
        LDRB     R0,[R4, #+98]
        SUBS     R0,R0,#+1
        CMP      R0,#+11
        BHI.N    ??mem_config_poll_3
        TBH      [PC, R0, LSL #+1]
        DATA
??mem_config_poll_0:
        DC16     0xC,0x46,0x57,0x71
        DC16     0x71,0x71,0x5F,0x64
        DC16     0x71,0xD2,0xF1,0x107
          CFI FunCall cfg_async_page
        THUMB
??mem_config_poll_6:
        BL       cfg_async_page
        CMP      R0,#+0
        BEQ.N    ??mem_config_poll_7
        MOVS     R2,#+64
        MOV      R1,SP
        LDR      R0,[R4, #+68]
        BL       ?Subroutine12
??CrossCallReturnLabel_24:
        CMP      R0,#+0
        BEQ.N    ??mem_config_poll_7
        BL       ?Subroutine9
??CrossCallReturnLabel_18:
        CBZ.N    R0,??mem_config_poll_8
        MOVS     R0,#+1
        STRB     R0,[R4, #+101]
??mem_config_poll_8:
        MOVS     R2,#+64
        MOV      R1,SP
        LDRH     R0,[R4, #+96]
        BL       ?Subroutine11
??CrossCallReturnLabel_21:
        CMP      R0,#+0
        BEQ.N    ??mem_config_poll_7
        BL       ?Subroutine9
??CrossCallReturnLabel_17:
        CBZ.N    R0,??mem_config_poll_9
        MOVS     R0,#+1
        STRB     R0,[R4, #+102]
??mem_config_poll_9:
        LDRH     R1,[R4, #+96]
        ADDS     R1,R1,#+64
        UXTH     R0,R1
        CMP      R0,#+4096
        BNE.N    ??mem_config_poll_10
        MOVS     R1,#+0
        LDRB     R0,[R4, #+103]
        CBNZ.N   R0,??mem_config_poll_11
        LDRB     R0,[R4, #+102]
        CBZ.N    R0,??mem_config_poll_12
        MOVS     R0,#+2
        B.N      ??mem_config_poll_13
??mem_config_poll_12:
        LDRB     R0,[R4, #+101]
        CBZ.N    R0,??mem_config_poll_14
??mem_config_poll_11:
        MOVS     R0,#+7
        B.N      ??mem_config_poll_13
??mem_config_poll_14:
        MOVS     R0,#+12
??mem_config_poll_13:
        STRB     R0,[R4, #+98]
??mem_config_poll_10:
        STRH     R1,[R4, #+96]
        B.N      ??mem_config_poll_5
??mem_config_poll_15:
        LDR.N    R6,??DataTable21_9
        MOVS     R0,#+3
??mem_config_poll_16:
        STRB     R0,[R4, #+98]
        MOV      R3,R5
        MOVS     R2,#+0
        MOVS     R1,#+0
        MOV      R0,R6
          CFI FunCall w25_async_start
        BL       w25_async_start
        CMP      R0,#+0
??mem_config_poll_7:
        BEQ.W    ??mem_config_poll_4
        MOVS     R0,#+1
        STRB     R0,[R4, #+100]
        B.N      ??mem_config_poll_5
??mem_config_poll_17:
        MOV      R6,#+1892352
        MOV      R0,#+1024
        STRH     R0,[R4, #+96]
        MOVS     R0,#+4
        B.N      ??mem_config_poll_16
??mem_config_poll_18:
        LDR      R6,[R4, #+72]
        ADD      R6,R6,#+4096
        MOVS     R0,#+8
        B.N      ??mem_config_poll_16
??mem_config_poll_19:
        LDR      R6,[R4, #+72]
        MOVS     R0,#+0
        STRH     R0,[R4, #+96]
        MOVS     R0,#+9
        STRB     R0,[R4, #+98]
        BL       ?Subroutine7
??CrossCallReturnLabel_14:
        STR      R0,[R4, #+84]
        LDRB     R0,[R4, #+98]
        B.N      ??mem_config_poll_16
??mem_config_poll_20:
          CFI FunCall cfg_async_page
        BL       cfg_async_page
        CMP      R0,#+0
        BEQ.N    ??mem_config_poll_21
        LDRB     R0,[R4, #+98]
        CMP      R0,#+9
        ITE      EQ
        LDREQ    R0,[R4, #+72]
        LDRNE.N  R0,??DataTable21_5
        MOV      R3,R5
        MOVS     R2,#+64
        ADDS     R1,R4,#+4
        LDRH     R5,[R4, #+96]
        ADDS     R0,R5,R0
          CFI FunCall w25_async_start
        BL       w25_async_start
        CMP      R0,#+0
        BEQ.N    ??mem_config_poll_21
        LDRB     R0,[R4, #+98]
        CMP      R0,#+9
        BNE.N    ??mem_config_poll_22
        MOVS     R2,#+64
        ADDS     R1,R4,#+4
        LDR      R0,[R4, #+84]
          CFI FunCall crc32_update
        BL       crc32_update
        STR      R0,[R4, #+84]
??mem_config_poll_22:
        LDRB     R0,[R4, #+98]
        LDRH     R1,[R4, #+96]
        ADDS     R1,R1,#+64
        STRH     R1,[R4, #+96]
        MOVS     R1,#+1
        STRB     R1,[R4, #+100]
        CMP      R0,#+4
        BNE.N    ??mem_config_poll_23
        LDRH     R1,[R4, #+96]
        CMP      R1,#+4096
        BNE.N    ??mem_config_poll_24
        MOV      R0,#+512
        STRH     R0,[R4, #+96]
        MOVS     R0,#+5
        B.N      ??mem_config_poll_24
??mem_config_poll_23:
        CMP      R0,#+5
        BNE.N    ??mem_config_poll_25
        LDRH     R1,[R4, #+96]
        CMP      R1,#+1024
        BNE.N    ??mem_config_poll_24
        MOVS     R0,#+0
        STRH     R0,[R4, #+96]
        MOVS     R0,#+6
        B.N      ??mem_config_poll_24
??mem_config_poll_25:
        CMP      R0,#+6
        BNE.N    ??mem_config_poll_26
        LDRH     R1,[R4, #+96]
        CMP      R1,#+512
        BNE.N    ??mem_config_poll_24
        LDRB     R0,[R4, #+103]
        CBNZ.N   R0,??mem_config_poll_27
        LDRB     R0,[R4, #+101]
        CBNZ.N   R0,??mem_config_poll_28
??mem_config_poll_27:
        MOVS     R0,#+12
        B.N      ??mem_config_poll_24
??mem_config_poll_28:
        MOVS     R0,#+7
        B.N      ??mem_config_poll_24
??mem_config_poll_26:
        CMP      R0,#+9
        ITT      EQ
        LDRHEQ   R1,[R4, #+96]
        CMPEQ    R1,#+4096
        BNE.N    ??mem_config_poll_24
        MOVS     R0,#+0
        STRH     R0,[R4, #+96]
        BL       ?Subroutine7
??CrossCallReturnLabel_13:
        STR      R0,[R4, #+92]
        MOVS     R0,#+10
        B.N      ??mem_config_poll_24
??mem_config_poll_29:
        MOVS     R2,#+64
        MOV      R1,SP
        LDR      R0,[R4, #+72]
        BL       ?Subroutine12
??CrossCallReturnLabel_23:
        CMP      R0,#+0
??mem_config_poll_21:
        BEQ.N    ??mem_config_poll_4
        MOVS     R2,#+64
        MOV      R1,SP
        LDR      R0,[R4, #+92]
          CFI FunCall crc32_update
        BL       crc32_update
        STR      R0,[R4, #+92]
        LDRH     R0,[R4, #+96]
        ADDS     R0,R0,#+64
        STRH     R0,[R4, #+96]
        LDRH     R0,[R4, #+96]
        CMP      R0,#+4096
        BNE.N    ??mem_config_poll_5
        LDR      R1,[R4, #+92]
        LDR      R0,[R4, #+84]
        CMP      R1,R0
        BNE.N    ??mem_config_poll_4
        MVNS     R0,R0
        STR      R0,[R4, #+88]
        MOVS     R0,#+11
        B.N      ??mem_config_poll_24
??mem_config_poll_30:
        MOV      R3,R5
        MOVS     R2,#+16
        ADD      R1,R4,#+76
        LDR      R0,[R4, #+72]
        ADD      R0,R0,#+4096
          CFI FunCall w25_async_start
        BL       w25_async_start
        CBZ.N    R0,??mem_config_poll_4
        MOVS     R0,#+1
        STRB     R0,[R4, #+100]
        LDRB     R0,[R4, #+103]
        CMP      R0,#+0
        ITE      NE
        MOVNE    R0,#+2
        MOVEQ    R0,#+12
??mem_config_poll_24:
        STRB     R0,[R4, #+98]
        B.N      ??mem_config_poll_5
??mem_config_poll_31:
        LDRB     R0,[R4, #+101]
        CMP      R0,#+0
        ITT      EQ
        LDRBEQ   R0,[R4, #+103]
        CMPEQ    R0,#+0
        BEQ.N    ??CrossCallReturnLabel_29
        LDR      R1,[R4, #+72]
        LDR.N    R0,??DataTable21_4
        STR      R1,[R0, #+0]
        LDR      R1,[R4, #+80]
        BL       ??Subroutine13_0
??CrossCallReturnLabel_29:
        MOVS     R0,#+2
        B.N      ??mem_config_poll_32
??mem_config_poll_4:
        LDR.N    R1,??DataTable21_10
        LDR      R0,[R1, #+0]
        ADDS     R0,R0,#+1
        STR      R0,[R1, #+0]
        MOVS     R0,#+3
??mem_config_poll_32:
        STRB     R0,[R4, #+99]
??mem_config_poll_5:
        ADD      SP,SP,#+64
          CFI CFA R13+16
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock77

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond78 Using cfiCommon0
          CFI Function mem_config_reset
          CFI Conditional ??CrossCallReturnLabel_32
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI Block cfiCond79 Using cfiCommon0
          CFI (cfiCond79) Function mem_config_poll
          CFI (cfiCond79) Conditional ??CrossCallReturnLabel_33
          CFI (cfiCond79) R4 Frame(CFA, -16)
          CFI (cfiCond79) R5 Frame(CFA, -12)
          CFI (cfiCond79) R6 Frame(CFA, -8)
          CFI (cfiCond79) R14 Frame(CFA, -4)
          CFI (cfiCond79) CFA R13+80
          CFI Block cfiPicker80 Using cfiCommon1
          CFI (cfiPicker80) NoFunction
          CFI (cfiPicker80) Picker
        THUMB
?Subroutine15:
        LDR.N    R4,??DataTable21_14
        LDRB     R0,[R4, #+99]
        CMP      R0,#+1
        BX       LR
          CFI EndBlock cfiCond78
          CFI EndBlock cfiCond79
          CFI EndBlock cfiPicker80

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable21:
        DATA32
        DC32     0x1f9001

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable21_1:
        DATA32
        DC32     0x32474643

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable21_2:
        DATA32
        DC32     0x1f5000

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable21_3:
        DATA32
        DC32     0x1f7000

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable21_4:
        DATA32
        DC32     cfg_cached_active

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable21_5:
        DATA32
        DC32     0x1cd400

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable21_6:
        DATA32
        DC32     0x1cd5fe

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable21_7:
        DATA32
        DC32     cfg_cached_generation

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable21_8:
        DATA32
        DC32     cfg_legacy_checked

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable21_9:
        DATA32
        DC32     0x1cd000

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable21_10:
        DATA32
        DC32     mem_config_errors

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable21_11:
        DATA32
        DC32     0xffe32c00

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable21_12:
        DATA32
        DC32     0x1ce400

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable21_13:
        DATA32
        DC32     mem_image_page

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable21_14:
        DATA32
        DC32     cfg_job

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond81 Using cfiCommon0
          CFI Function mem_config_poll
          CFI Conditional ??CrossCallReturnLabel_24
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+80
          CFI Block cfiCond82 Using cfiCommon0
          CFI (cfiCond82) Function mem_config_poll
          CFI (cfiCond82) Conditional ??CrossCallReturnLabel_23
          CFI (cfiCond82) R4 Frame(CFA, -16)
          CFI (cfiCond82) R5 Frame(CFA, -12)
          CFI (cfiCond82) R6 Frame(CFA, -8)
          CFI (cfiCond82) R14 Frame(CFA, -4)
          CFI (cfiCond82) CFA R13+80
          CFI Block cfiPicker83 Using cfiCommon1
          CFI (cfiPicker83) NoFunction
          CFI (cfiPicker83) Picker
        THUMB
?Subroutine12:
        LDRH     R3,[R4, #+96]
        ADDS     R0,R3,R0
          CFI (cfiCond81) FunCall mem_config_poll w25_read_data
          CFI (cfiCond81) FunCall mem_config_poll w25_read_data
        B.W      w25_read_data
          CFI EndBlock cfiCond81
          CFI EndBlock cfiCond82
          CFI EndBlock cfiPicker83

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond84 Using cfiCommon0
          CFI Function cfg_legacy_crc
          CFI Conditional ??CrossCallReturnLabel_22
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+88
          CFI Block cfiCond85 Using cfiCommon0
          CFI (cfiCond85) Function mem_config_poll
          CFI (cfiCond85) Conditional ??CrossCallReturnLabel_21
          CFI (cfiCond85) R4 Frame(CFA, -16)
          CFI (cfiCond85) R5 Frame(CFA, -12)
          CFI (cfiCond85) R6 Frame(CFA, -8)
          CFI (cfiCond85) R14 Frame(CFA, -4)
          CFI (cfiCond85) CFA R13+80
          CFI Block cfiPicker86 Using cfiCommon1
          CFI (cfiPicker86) NoFunction
          CFI (cfiPicker86) Picker
        THUMB
?Subroutine11:
        ADD      R0,R0,#+1835008
        ADD      R0,R0,#+54272
          CFI (cfiCond84) FunCall cfg_legacy_crc w25_read_data
          CFI (cfiCond85) FunCall mem_config_poll w25_read_data
        B.W      w25_read_data
          CFI EndBlock cfiCond84
          CFI EndBlock cfiCond85
          CFI EndBlock cfiPicker86

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond87 Using cfiCommon0
          CFI Function mem_config_poll
          CFI Conditional ??CrossCallReturnLabel_18
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+80
          CFI Block cfiCond88 Using cfiCommon0
          CFI (cfiCond88) Function mem_config_poll
          CFI (cfiCond88) Conditional ??CrossCallReturnLabel_17
          CFI (cfiCond88) R4 Frame(CFA, -16)
          CFI (cfiCond88) R5 Frame(CFA, -12)
          CFI (cfiCond88) R6 Frame(CFA, -8)
          CFI (cfiCond88) R14 Frame(CFA, -4)
          CFI (cfiCond88) CFA R13+80
          CFI Block cfiPicker89 Using cfiCommon1
          CFI (cfiPicker89) NoFunction
          CFI (cfiPicker89) Picker
        THUMB
?Subroutine9:
        MOVS     R2,#+64
        ADDS     R1,R4,#+4
        MOV      R0,SP
          CFI (cfiCond87) FunCall mem_config_poll memcmp
          CFI (cfiCond87) FunCall mem_config_poll memcmp
        B.W      memcmp
          CFI EndBlock cfiCond87
          CFI EndBlock cfiCond88
          CFI EndBlock cfiPicker89

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond90 Using cfiCommon0
          CFI Function mem_config_poll
          CFI Conditional ??CrossCallReturnLabel_14
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+80
          CFI Block cfiCond91 Using cfiCommon0
          CFI (cfiCond91) Function mem_config_poll
          CFI (cfiCond91) Conditional ??CrossCallReturnLabel_13
          CFI (cfiCond91) R4 Frame(CFA, -16)
          CFI (cfiCond91) R5 Frame(CFA, -12)
          CFI (cfiCond91) R6 Frame(CFA, -8)
          CFI (cfiCond91) R14 Frame(CFA, -4)
          CFI (cfiCond91) CFA R13+80
          CFI Block cfiPicker92 Using cfiCommon1
          CFI (cfiPicker92) NoFunction
          CFI (cfiPicker92) Picker
        THUMB
?Subroutine7:
        MOVS     R2,#+4
        ADD      R1,R4,#+80
        MOV      R0,#+4294967295
          CFI (cfiCond90) FunCall mem_config_poll crc32_update
          CFI (cfiCond90) FunCall mem_config_poll crc32_update
        B.W      crc32_update
          CFI EndBlock cfiCond90
          CFI EndBlock cfiCond91
          CFI EndBlock cfiPicker92

        SECTION `.iar_vfe_header`:DATA:NOALLOC:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
        DC32 0

        END
