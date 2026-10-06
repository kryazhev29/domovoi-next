        EXTERN idindex_poll
        EXTERN idindex_commit
        EXTERN idindex_put
        EXTERN idindex_find
        EXTERN idindex_table_commit
        EXTERN idindex_scanned
; Cortex-M3 / IAR. Executable production module.
; Reference: Tests/asm/reference/User/idindex.c
        PUBLIC ix

        RTMODEL "__CPP_Runtime", "1"
        RTMODEL "__SystemLibrary", "DLib"
        RTMODEL "__dlib_file_descriptor", "0"
        RTMODEL "__dlib_full_locale_support", "0"
        RTMODEL "__dlib_version", "6"
        AAPCS BASE,INTERWORK
        PRESERVE8
        REQUIRE8

        #define SHT_PROGBITS 0x1
        #include "flash_owner.h"
        EXTERN flash_claim
        EXTERN flash_release

        EXTERN fast_fnv10
        EXTERN mem_erase_sector
        EXTERN mem_read
        EXTERN mem_write
        EXTERN memcmp
        EXTERN idcrc_io_start
        EXTERN idcrc_io_poll
        EXTERN idcrc_io_active
        EXTERN w25_read_available
        EXTERN w25_read_data
        EXTERN timers_getMillis

        PUBLIC idindex_impl_commit
        PUBLIC idindex_impl_find
        PUBLIC idindex_impl_poll
        PUBLIC idindex_impl_put
        PUBLIC idindex_runtime_suspend
        PUBLIC idindex_runtime_active
        PUBLIC idindex_impl_scanned

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

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
ix:
        ; Existing sum/step/end/mark/state at 0/4/6/8/10 retain their ABI.
        ; Byte 11 owns the W25 operation. Bytes 12..19 remain live until
        ; verified/error, including cancellation; word 20 is the bucket.
        DS8 24

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock0 Using cfiCommon0
          CFI Function ix_bucket
        THUMB
ix_bucket:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI FunCall fast_fnv10
        BL       fast_fnv10
        EOR      R1,R0,R0, LSR #+12
        EOR      R1,R1,R0, LSR #+24
        LDR.W    R0,??DataTable9
        AND      R0,R0,R1, LSL #+6
        ADD      R0,R0,#+3145728
        POP      {R1,PC}
          CFI EndBlock cfiBlock0

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock1 Using cfiCommon0
          CFI Function ix_mark
        THUMB
ix_mark:
        PUSH     {R2-R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+16
        MOVS     R2,#+0
        STRD     R2,R2,[SP, #+0]
        CBZ.N    R0,??ix_mark_0
        STR      R1,[SP, #+0]
        MVNS     R1,R1
        LDR.W    R0,??DataTable9_1
        EORS     R1,R0,R1
        STR      R1,[SP, #+4]
??ix_mark_0:
        LDR.W    R4,??DataTable9_2
        LDRH     R0,[R4, #+8]
        CMP      R0,#+512
        BCC.N    ??ix_mark_1
        MOV      R0,#+3407872
          CFI FunCall mem_erase_sector
        BL       mem_erase_sector
        CBZ.N    R0,??ix_mark_2
        MOVS     R0,#+0
??ix_mark_1:
        ADDS     R1,R0,#+1
        STRH     R1,[R4, #+8]
        MOVS     R2,#+8
        MOV      R1,SP
        LSLS     R0,R0,#+3
        ADD      R0,R0,#+3407872
          CFI FunCall mem_write
        BL       mem_write
??ix_mark_2:
        POP      {R1,R2,R4,PC}
          CFI EndBlock cfiBlock1

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock2 Using cfiCommon0
          CFI Function ix_rebuild
          CFI NoCalls
        THUMB
ix_rebuild:
        ; Invalidate durably in poll BEFORE the first destructive erase.
        ; A restart while W25 owns source cannot overwrite that source.
        LDR.W    R2,??DataTable9_2
        LDRB     R1,[R2, #+11]
        MOVS     R0,#+1
        CMP      R1,#+0
        IT       NE
        MOVNE    R0,#+8
        STRB     R0,[R2, #+10]
        MOVS     R0,#+0
        STRH     R0,[R2, #+4]
        BX       LR
          CFI EndBlock cfiBlock2

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock3 Using cfiCommon0
          CFI Function ix_last
        THUMB
ix_last:
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
        MOVS     R5,#+0
        LDR.W    R6,??DataTable9_2
        MOV      R0,#+512
        STRH     R0,[R6, #+8]
        MOVS     R7,#+0
??ix_last_0:
        MOVS     R2,#+64
        MOV      R1,SP
        LSLS     R0,R7,#+3
        ADD      R0,R0,#+3407872
          CFI FunCall mem_read
        BL       mem_read
        CBZ.N    R0,??ix_last_1
        MOVS     R1,#+0
        LDR.W    R3,??DataTable9_1
??ix_last_2:
        LDR      R2,[SP, R1, LSL #+3]
        CMN      R2,#+1
        ITTTT    EQ
        MOVEQ    R2,SP
        ADDEQ    R2,R2,R1, LSL #+3
        LDREQ    R2,[R2, #+4]
        CMNEQ    R2,#+1
        BEQ.N    ??ix_last_3
        MOV      R2,SP
        ADD      R2,R2,R1, LSL #+3
        LDR      R2,[R2, #+4]
        LDR      R5,[SP, R1, LSL #+3]
        MVNS     R5,R5
        EORS     R5,R3,R5
        CMP      R2,R5
        ITE      EQ
        MOVEQ    R5,#+1
        MOVNE    R5,#+0
        B.N      ??ix_last_4
??ix_last_3:
        ADDS     R7,R1,R7
        STRH     R7,[R6, #+8]
        B.N      ??ix_last_5
??ix_last_4:
        LDR      R0,[SP, R1, LSL #+3]
        STR      R0,[R4, #+0]
        ADDS     R1,R1,#+1
        CMP      R1,#+8
        BCC.N    ??ix_last_2
        ADDS     R7,R7,#+8
        CMP      R7,#+512
        BCC.N    ??ix_last_0
??ix_last_5:
        MOV      R0,R5
??ix_last_1:
        ADD      SP,SP,#+68
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI EndBlock cfiBlock3

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock4 Using cfiCommon0
          CFI Function idindex_impl_scanned
        THUMB
idindex_impl_scanned:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R4,R0
        MOVS     R0,#+0
        STR      R0,[SP, #+0]
        LDR.W    R5,??DataTable9_2
        STR      R4,[R5, #+0]
        STRH     R1,[R5, #+6]
        LDRB     R0,[R5, #+10]
        CBNZ.N   R0,??idindex_scanned_0
        MOV      R0,SP
          CFI FunCall ix_last
        BL       ix_last
        CBZ.N    R0,??idindex_scanned_0
        LDR      R0,[SP, #+0]
        CMP      R0,R4
        BNE.N    ??idindex_scanned_0
        MOVS     R0,#+3
        STRB     R0,[R5, #+10]
        POP      {R0,R4,R5,PC}
??idindex_scanned_0:
        LDRB     R0,[R5, #+10]
        CBNZ.N   R0,??idindex_scanned_1
        MOVS     R0,#+1
        STRB     R0,[R5, #+10]
??idindex_scanned_1:
          CFI FunCall ix_rebuild
        BL       ix_rebuild
        POP      {R0,R4,R5,PC}
          CFI EndBlock cfiBlock4

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock5 Using cfiCommon0
          CFI Function idindex_impl_find
        THUMB
idindex_impl_find:
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
        MOV      R6,R1
        MOV      R5,R2
        LDR.W    R0,??DataTable9_2
        LDRB     R0,[R0, #+10]
        CMP      R0,#+3
        BNE.N    ??idindex_find_0
        MOV      R0,R6
          CFI FunCall ix_bucket
        BL       ix_bucket
        MOVS     R2,#+64
        MOV      R1,SP
          CFI FunCall mem_read
        BL       mem_read
        CBZ.N    R0,??idindex_find_0
        MOVS     R7,#+0
??idindex_find_1:
        LDRH     R1,[SP, R7, LSL #+1]
        MOVW     R0,#+65535
        CMP      R1,R0
        BEQ.N    ??idindex_find_2
        MOVS     R0,R1
        BEQ.N    ??idindex_find_3
        MOVS     R2,#+32
        MOV      R1,R5
        SUBS     R0,R0,#+1
        ADD      R0,R4,R0, LSL #+5
          CFI FunCall mem_read
        BL       mem_read
        CBZ.N    R0,??idindex_find_0
        LDRB     R0,[R5, #+31]
        CMP      R0,#+255
        BEQ.N    ??idindex_find_3
        LDRB     R0,[R5, #+0]
        CMP      R0,#+255
        ITT      EQ
        LDRBEQ   R0,[R5, #+1]
        CMPEQ    R0,#+255
        BEQ.N    ??idindex_find_3
        MOVS     R2,#+10
        MOV      R1,R6
        ADDS     R0,R5,#+2
          CFI FunCall memcmp
        BL       memcmp
        CBZ.N    R0,??idindex_find_4
??idindex_find_3:
        ADDS     R7,R7,#+1
        CMP      R7,#+32
        BCC.N    ??idindex_find_1
        B.N      ??idindex_find_2
??idindex_find_0:
        MOV      R0,#+4294967295
        B.N      ??idindex_find_5
??idindex_find_4:
        MOVS     R0,#+1
        B.N      ??idindex_find_5
??idindex_find_2:
        MOVS     R0,#+0
??idindex_find_5:
        ADD      SP,SP,#+68
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI EndBlock cfiBlock5

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock6 Using cfiCommon0
          CFI Function ix_add
        THUMB
ix_add:
        PUSH     {R0,R4,R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        SUB      SP,SP,#+72
          CFI CFA R13+88
        MOV      R0,R1
          CFI FunCall ix_bucket
        BL       ix_bucket
        MOV      R4,R0
        MOVS     R2,#+64
        ADD      R1,SP,#+4
          CFI FunCall mem_read
        BL       mem_read
        CBZ.N    R0,??ix_add_0
        MOVS     R5,#+0
        ADD      R2,SP,#+4
??ix_add_1:
        LDRH     R0,[R2, R5, LSL #+1]
        LDRH     R1,[SP, #+72]
        CMP      R0,R1
        BEQ.N    ??ix_add_2
        MOV      R1,R0
        MOVW     R0,#+65535
        CMP      R1,R0
        BEQ.N    ??ix_add_3
        ADDS     R5,R5,#+1
        CMP      R5,#+32
        BCC.N    ??ix_add_1
        B.N      ??ix_add_0
??ix_add_2:
        MOVS     R0,#+1
        B.N      ??ix_add_4
??ix_add_0:
        MOVS     R0,#+0
??ix_add_4:
        ADD      SP,SP,#+76
          CFI CFA R13+12
        POP      {R4,R5,PC}
          CFI CFA R13+88
??ix_add_3:
        MOVS     R0,#+0
        STRH     R0,[SP, #+0]
        MOVS     R2,#+2
        ADD      R1,SP,#+72
        ADD      R0,R4,R5, LSL #+1
          CFI FunCall mem_write
        BL       mem_write
        CMP      R0,#+0
        BEQ.N    ??ix_add_0
        MOVS     R2,#+2
        MOV      R1,SP
        ADD      R0,R4,R5, LSL #+1
          CFI FunCall mem_read
        BL       mem_read
        CMP      R0,#+0
        BEQ.N    ??ix_add_0
        LDRH     R1,[SP, #+0]
        LDRH     R0,[SP, #+72]
        CMP      R1,R0
        BNE.N    ??ix_add_0
        B.N      ??ix_add_2
          CFI EndBlock cfiBlock6

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock7 Using cfiCommon0
          CFI Function idindex_impl_put
        THUMB
idindex_impl_put:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        LDR.W    R4,??DataTable9_2
        LDRB     R2,[R4, #+10]
        CMP      R2,#+3
        BEQ.N    ??idindex_put_0
        CMP      R2,#+2
        BNE.N    ??idindex_put_1
        LDRH     R2,[R4, #+4]
        CMP      R0,R2
        BCS.N    ??idindex_put_1
??idindex_put_0:
          CFI FunCall ix_add
        BL       ix_add
        CBZ.N    R0,??idindex_put_2
??idindex_put_1:
        MOVS     R0,#+1
        POP      {R4,PC}
??idindex_put_2:
        LDRB     R0,[R4, #+10]
        CMP      R0,#+3
        BNE.N    ??idindex_put_3
          CFI FunCall ix_rebuild
        BL       ix_rebuild
        B.N      ??idindex_put_4
??idindex_put_3:
        MOVS     R0,#+4
        STRB     R0,[R4, #+10]
??idindex_put_4:
        MOVS     R0,#+0
        POP      {R4,PC}
          CFI EndBlock cfiBlock7

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock8 Using cfiCommon0
          CFI Function idindex_impl_commit
        THUMB
idindex_impl_commit:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        LDR.W    R4,??DataTable9_2
        LDR      R2,[R4, #+0]
        EORS     R0,R0,R2
        STR      R0,[R4, #+0]
        LDRH     R0,[R4, #+6]
        CMP      R1,R0
        IT       CC
        MOVCC    R1,R0
        STRH     R1,[R4, #+6]
        LDRB     R0,[R4, #+10]
        CMP      R0,#+3
        BNE.N    ??idindex_commit_0
        LDR      R1,[R4, #+0]
        MOVS     R0,#+1
          CFI FunCall ix_mark
        BL       ix_mark
        CBNZ.N   R0,??idindex_commit_0
        MOVS     R0,#+4
        STRB     R0,[R4, #+10]
??idindex_commit_0:
        POP      {R4,PC}
          CFI EndBlock cfiBlock8

        ; Handwritten bounded rebuild, one W25 action OR a bounded read per
        ; invocation. No heap. The 8-byte persistent source has a single owner.
        ; state: 1 invalidate, 2 cell, 3 ready, 4 off, 5 erase, 6 bucket,
        ;        7 final mark, 8 restart after an outstanding operation drains.
        ; pending: 1 log erase, 2 invalid mark, 3 index erase, 4 bucket write,
        ;          5 final log erase, 6 final mark. Only verified advances.
        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock9 Using cfiCommon0
          CFI Function idindex_impl_poll
        THUMB
idindex_impl_poll:
        PUSH     {R4-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+20
        SUB      SP,SP,#+68
          CFI CFA R13+88
        MOV      R6,R0
        LDR.W    R4,ix_async_data
        LDRB     R7,[R4, #+11]
        CMP      R7,#+0
        BEQ.W    ix_poll_dispatch
          CFI FunCall timers_getMillis
        BL       timers_getMillis
          CFI FunCall idcrc_io_poll
        BL       idcrc_io_poll
        CMP      R0,#+1
        BEQ.W    ix_poll_return
        MOV      R5,R0
        MOVS     R0,#+FO_INDEX
          CFI FunCall flash_release
        BL       flash_release
        MOV      R0,R5
        MOVS     R1,#+0
        STRB     R1,[R4, #+11]
        CMP      R0,#+2
        BNE.W    ix_poll_error
        LDRB     R0,[R4, #+10]
        CMP      R0,#+4
        BEQ.W    ix_poll_return
        CMP      R0,#+8
        BEQ.W    ix_poll_restart
        CMP      R7,#+1
        BEQ.W    ix_poll_log_erased
        CMP      R7,#+5
        BEQ.W    ix_poll_log_erased
        CMP      R7,#+2
        BEQ.W    ix_poll_invalidated
        CMP      R7,#+3
        BEQ.W    ix_poll_erased
        CMP      R7,#+4
        BEQ.W    ix_poll_advance
        CMP      R7,#+6
        BNE.W    ix_poll_error
        LDRH     R0,[R4, #+8]
        ADDS     R0,R0,#+1
        STRH     R0,[R4, #+8]
        MOVS     R0,#+3
        B.W      ix_poll_state
ix_poll_restart:
        MOVS     R0,#+1
        B.W      ix_poll_state
ix_poll_log_erased:
        MOVS     R0,#+0
        STRH     R0,[R4, #+8]
        B.W      ix_poll_return
ix_poll_invalidated:
        LDRH     R0,[R4, #+8]
        ADDS     R0,R0,#+1
        STRH     R0,[R4, #+8]
        MOVS     R0,#+5
        B.W      ix_poll_state
ix_poll_erased:
        LDRH     R0,[R4, #+4]
        ADDS     R0,R0,#+1
        CMP      R0,#+64
        BCC.N    ix_poll_erase_cursor
        MOVS     R0,#+2
        STRB     R0,[R4, #+10]
        MOVS     R0,#+1
ix_poll_erase_cursor:
        STRH     R0,[R4, #+4]
        B.W      ix_poll_return
ix_poll_dispatch:
        LDRB     R0,[R4, #+10]
        CMP      R0,#+1
        BEQ.N    ix_poll_available
        CMP      R0,#+2
        BEQ.N    ix_poll_available
        CMP      R0,#+5
        BCC.W    ix_poll_return
        CMP      R0,#+7
        BHI.W    ix_poll_return
ix_poll_available:
          CFI FunCall idcrc_io_active
        BL       idcrc_io_active
        CMP      R0,#+0
        BNE.W    ix_poll_return
          CFI FunCall w25_read_available
        BL       w25_read_available
        CMP      R0,#+0
        BEQ.W    ix_poll_return
        LDRB     R0,[R4, #+10]
        CMP      R0,#+1
        BEQ.W    ix_poll_mark
        CMP      R0,#+7
        BEQ.W    ix_poll_mark
        CMP      R0,#+5
        BEQ.W    ix_poll_erase
        CMP      R0,#+6
        BEQ.W    ix_poll_bucket
        ; One 12-byte row per pass. The only retained value is its bucket.
        LDRH     R0,[R4, #+4]
        LDRH     R1,[R4, #+6]
        CMP      R0,R1
        BHI.W    ix_poll_finish
        SUBS     R0,R0,#+1
        ADD      R0,R6,R0, LSL #+5
        MOV      R1,SP
        MOVS     R2,#+32
          CFI FunCall w25_read_data
        BL       w25_read_data
        CMP      R0,#+0
        BEQ.W    ix_poll_error
        LDRH     R0,[SP, #+0]
        MOVW     R1,#+65535
        CMP      R0,R1
        BEQ.W    ix_poll_advance
        LDRB     R0,[SP, #+31]
        CMP      R0,#+255
        BEQ.W    ix_poll_advance
        ADD      R0,SP,#+2
          CFI FunCall ix_bucket
        BL       ix_bucket
        STR      R0,[R4, #+20]
        MOVS     R0,#+6
        B.W      ix_poll_state
ix_poll_bucket:
        LDR      R0,[R4, #+20]
        MOV      R1,SP
        MOVS     R2,#+64
          CFI FunCall w25_read_data
        BL       w25_read_data
        CMP      R0,#+0
        BEQ.W    ix_poll_error
        LDRH     R2,[R4, #+4]
        MOVS     R3,#+0
        MOVW     R1,#+65535
ix_poll_slot:
        LDRH     R0,[SP,R3]
        CMP      R0,R2
        BEQ.N    ix_poll_advance
        CMP      R0,R1
        BEQ.N    ix_poll_slot_free
        ADDS     R3,R3,#+2
        CMP      R3,#+64
        BCC.N    ix_poll_slot
        B.W      ix_poll_error
ix_poll_slot_free:
        LDR      R0,[R4, #+20]
        ADD      R6,R0,R3
        STRH     R2,[R4, #+12]
        ADD      R5,R4,#+12
        MOVS     R7,#+4
        MOVS     R2,#+2
        B.N      ix_poll_start
ix_poll_advance:
        LDRH     R0,[R4, #+4]
        LDRH     R1,[R4, #+6]
        CMP      R0,R1
        BCS.N    ix_poll_finish
        ADDS     R0,R0,#+1
        STRH     R0,[R4, #+4]
        MOVS     R0,#+2
        B.N      ix_poll_state
ix_poll_finish:
        MOVS     R0,#+7
        B.N      ix_poll_state
ix_poll_erase:
        LDRH     R0,[R4, #+4]
        LSLS     R0,R0,#+12
        ADD      R6,R0,#+3145728
        MOVS     R7,#+3
        B.N      ix_poll_start_erase
ix_poll_mark:
        ; Reserve a durable invalid marker even after boot with a mismatched
        ; checksum. This prevents accepting a partially rebuilt equal-sum cache.
        MOVS     R7,#+2
        CMP      R0,#+7
        IT       EQ
        MOVEQ    R7,#+6
        LDRH     R0,[R4, #+8]
        CMP      R0,#+512
        BCC.N    ix_poll_mark_write
        SUBS     R7,R7,#+1
        MOV      R6,#+3407872
ix_poll_start_erase:
        MOVS     R5,#+0
        MOVS     R2,#+0
        B.N      ix_poll_start
ix_poll_mark_write:
        LSLS     R0,R0,#+3
        ADD      R6,R0,#+3407872
        MOVS     R0,#+0
        MOVS     R1,#+0
        CMP      R7,#+6
        BNE.N    ix_poll_mark_source
        LDR      R0,[R4, #+0]
        LDR.W    R1,ix_async_magic
        MVN      R3,R0
        EOR      R1,R1,R3
ix_poll_mark_source:
        STR      R0,[R4, #+12]
        STR      R1,[R4, #+16]
        ADD      R5,R4,#+12
        MOVS     R2,#+8
ix_poll_start:
        ; R2 is caller-saved; keep length in an otherwise unused stack word.
        STR      R2,[SP, #+64]
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        MOV      R3,R0
        MOV      R0,R6
        MOV      R1,R5
        LDR      R2,[SP, #+64]
          CFI FunCall idcrc_io_start
        BL       idcrc_io_start
        CMP      R0,#+0
        BEQ.N    ix_poll_error
        ; Cannot fail: user_loop starts a step only with no owner.
        MOVS     R0,#+FO_INDEX
          CFI FunCall flash_claim
        BL       flash_claim
        STRB     R7,[R4, #+11]
        B.N      ix_poll_return
ix_poll_error:
        MOVS     R0,#+4
ix_poll_state:
        STRB     R0,[R4, #+10]
ix_poll_return:
        ADD      SP,SP,#+68
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI EndBlock cfiBlock9

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock11 Using cfiCommon0
          CFI Function idindex_runtime_active
          CFI NoCalls
        THUMB
idindex_runtime_active:
        LDR.W    R0,ix_async_data
        LDRB     R0,[R0, #+11]
        CMP      R0,#+0
        IT       NE
        MOVNE    R0,#+1
        BX       LR
          CFI EndBlock cfiBlock11

        SECTION `.text`:CODE:NOROOT(2)
        DATA
ix_async_data:
        DC32 ix
ix_async_magic:
        DC32 0x58444931

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock10 Using cfiCommon0
          CFI Function idindex_runtime_suspend
          CFI NoCalls
        THUMB
idindex_runtime_suspend:
        MOVS     R1,#+4
        LDR.W    R0,??DataTable9_2
        STRB     R1,[R0, #+10]
        BX       LR
          CFI EndBlock cfiBlock10

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable9:
        DATA32
        DC32     0x3ffc0

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable9_1:
        DATA32
        DC32     0x58444931

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable9_2:
        DATA32
        DC32     ix

        SECTION `.iar_vfe_header`:DATA:NOALLOC:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
        DC32 0

        PUBLIC idindex_impl_table_commit
        SECTION .text:CODE:NOROOT(2)
        CFI Block table_commit_block Using cfiCommon0
        CFI Function idindex_impl_table_commit
        THUMB
idindex_impl_table_commit:
        LDR R2,=ix
        LDR R2,[R2]
        EORS R0,R0,R2
        CFI TailCall idindex_scanned
        B.W idindex_scanned
        CFI EndBlock table_commit_block
        LTORG
        END
