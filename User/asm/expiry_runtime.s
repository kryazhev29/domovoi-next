#include "../work_slots.h"
        EXTERN work_acquire
        EXTERN work_release
; Hand-written Cortex-M3 Thumb-2. Owns a historical 1024-byte scan block and a
; separate 32-byte update record, never an RX/DMA pointer. Fill/lookup <=64.
        PRESERVE8
        REQUIRE8
        AAPCS BASE,INTERWORK
        #include "flash_owner.h"
        EXTERN flash_claim
        EXTERN flash_release
        EXTERN __aeabi_memcpy
        EXTERN device
        EXTERN mem
        EXTERN mem_storage_ready
        EXTERN id_cells_end
        EXTERN id_nextEnd
        EXTERN id_endScan
        EXTERN id_time_end
        EXTERN id_watch_end
        EXTERN user_storage_background_ready
        EXTERN mem_tables_ready
        EXTERN w25_read_data
        EXTERN sj_runtime_begin
        EXTERN sj_runtime_poll
        EXTERN sj_runtime_reset
        EXTERN sj_runtime_cancel
        EXTERN sj_runtime_blocked
        EXTERN log_addControlEvent
        PUBLIC expiry_runtime_begin
        PUBLIC expiry_runtime_poll
        PUBLIC expiry_runtime_active
        PUBLIC expiry_runtime_cancel
        PUBLIC expiry_job

        CFI Names ex_names
        CFI StackFrame CFA R13 DATA
        CFI Resource R0:32, R1:32, R2:32, R3:32, R4:32, R5:32, R6:32, R7:32
        CFI Resource R8:32, R9:32, R10:32, R11:32, R12:32, R13:32, R14:32
        CFI EndNames ex_names
        CFI Common ex_cfi Using ex_names
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
        CFI EndCommon ex_cfi

; ctx: scan0, used_limit4, start8, target12, find16, partition_limit20,
; phase24, cancelled25, fill_offset26, scan_time28, owned_record32..63,
; snapshot_base64, snapshot68..1091. Block snapshot preserves legacy duplicate
; index semantics when a canonical target is later in the same scan block.
        SECTION .bss:DATA:NOROOT(2)
expiry_job:
        DS8 4

        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block ex_active Using ex_cfi
        CFI Function expiry_runtime_active
        CFI NoCalls
expiry_runtime_active:
        LDR R0,=expiry_job
        LDR R0,[R0]
        CMP R0,#0
        IT NE
        MOVNE R0,#1
        BX LR
        CFI EndBlock ex_active

        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block ex_cancel Using ex_cfi
        CFI Function expiry_runtime_cancel
        CFI NoCalls
expiry_runtime_cancel:
        LDR R0,=expiry_job
        LDR R0,[R0]
        CBZ R0,ex_cancel_done
        MOVS R1,#1
        STRB R1,[R0,#25]
ex_cancel_done:
        BX LR
        CFI EndBlock ex_cancel

        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block ex_release Using ex_cfi
        CFI Function expiry_release
expiry_release:
        PUSH {R4,LR}
        CFI R4 Frame(CFA,-8)
        CFI R14 Frame(CFA,-4)
        CFI CFA R13+8
        MOVS R0,#FO_EXPIRY
        CFI FunCall flash_release
        BL flash_release
        LDR R4,=expiry_job
        LDR R0,[R4]
        MOVS R1,#WORK_EXPIRY
        CFI FunCall work_release
        BL work_release
        MOVS R0,#0
        STR R0,[R4]
        POP {R4,PC}
        CFI EndBlock ex_release

        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block ex_begin Using ex_cfi
        CFI Function expiry_runtime_begin
        REQUIRE expiry_runtime_poll
        REQUIRE expiry_runtime_active
expiry_runtime_begin:
        PUSH {R4-R8,LR}
        CFI R4 Frame(CFA,-24)
        CFI R5 Frame(CFA,-20)
        CFI R6 Frame(CFA,-16)
        CFI R7 Frame(CFA,-12)
        CFI R8 Frame(CFA,-8)
        CFI R14 Frame(CFA,-4)
        CFI CFA R13+24
        LDR R0,=expiry_job
        LDR R0,[R0]
        CMP R0,#0
        BNE ex_begin_no
        CFI FunCall user_storage_background_ready
        BL user_storage_background_ready
        CMP R0,#0
        BEQ ex_begin_no
        CFI FunCall mem_tables_ready
        BL mem_tables_ready
        CMP R0,#0
        BEQ ex_begin_no
        LDR R0,=device
        LDR R7,[R0,#47]
        LDR R0,=1600000000
        CMP R7,R0
        BLO ex_begin_no
        LDR R0,=id_endScan
        LDRB R0,[R0]
        CBNZ R0,ex_begin_needed
        LDR R0,=id_nextEnd
        LDR R0,[R0]
        CBZ R0,ex_begin_no
        CMP R7,R0
        BLO ex_begin_no
ex_begin_needed:
        LDR R0,=mem
        LDR R4,[R0,#16]
        LDR R5,[R0,#20]
        TST R4,#31
        BNE ex_begin_no
        TST R5,#31
        BNE ex_begin_no
        ADDS R8,R4,R5
        BCS ex_begin_no
        LDR R0,=id_cells_end
        LDRH R6,[R0]
        LSLS R6,R6,#5
        CMP R6,R5
        BHI ex_begin_no
        ADD R6,R6,R4
        MOVW R0,#1092
        MOVS R1,#WORK_EXPIRY
        CFI FunCall work_acquire
        BL work_acquire
        CBZ R0,ex_begin_oom
        STR R4,[R0]
        STR R6,[R0,#4]
        STR R4,[R0,#8]
        STR R8,[R0,#20]
        STR R7,[R0,#28]
        STR R4,[R0,#64]
        MOVS R1,#6
        STR R1,[R0,#24]
        LDR R2,=expiry_job
        STR R0,[R2]
        ; Cannot fail: user_storage_background_ready saw no owner.
        MOVS R0,#FO_EXPIRY
        CFI FunCall flash_claim
        BL flash_claim
        MOVS R1,#0
        LDR R2,=id_endScan
        STRB R1,[R2]
        LDR R2,=id_nextEnd
        STR R1,[R2]
        MOVS R0,#1
        POP {R4-R8,PC}
ex_begin_oom:
        LDR R1,=id_endScan
        MOVS R0,#1
        STRB R0,[R1]
ex_begin_no:
        MOVS R0,#0
        POP {R4-R8,PC}
        CFI EndBlock ex_begin

; This callback is called only while the sector journal owns the shared page.
; It rereads a 64-byte original chunk and overlays the operation-owned record.
        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block ex_chunk Using ex_cfi
        CFI Function expiry_chunk
expiry_chunk:
        PUSH {R4-R8,LR}
        CFI R4 Frame(CFA,-24)
        CFI R5 Frame(CFA,-20)
        CFI R6 Frame(CFA,-16)
        CFI R7 Frame(CFA,-12)
        CFI R8 Frame(CFA,-8)
        CFI R14 Frame(CFA,-4)
        CFI CFA R13+24
        MOV R4,R0
        MOV R5,R2
        LDR R6,[R4,#12]
        BIC R7,R6,#0xff0
        BIC R7,R7,#15
        ADD R7,R7,R1
        MOV R0,R7
        MOV R1,R5
        MOVS R2,#64
        CFI FunCall w25_read_data
        BL w25_read_data
        CBZ R0,ex_chunk_done
        SUBS R6,R6,R7
        CMP R6,#32
        BHI ex_chunk_ok
        ADD R0,R5,R6
        ADD R1,R4,#32
        MOVS R2,#32
        CFI FunCall __aeabi_memcpy
        BL __aeabi_memcpy
ex_chunk_ok:
        MOVS R0,#1
ex_chunk_done:
        POP {R4-R8,PC}
        CFI EndBlock ex_chunk

        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block ex_poll Using ex_cfi
        CFI Function expiry_runtime_poll
expiry_runtime_poll:
        PUSH {R4-R8,LR}
        CFI R4 Frame(CFA,-24)
        CFI R5 Frame(CFA,-20)
        CFI R6 Frame(CFA,-16)
        CFI R7 Frame(CFA,-12)
        CFI R8 Frame(CFA,-8)
        CFI R14 Frame(CFA,-4)
        CFI CFA R13+24
        SUB SP,SP,#64
        CFI CFA R13+88
        MOV R5,R0
        LDR R0,=expiry_job
        LDR R4,[R0]
        CMP R4,#0
        BEQ ex_done
        LDRB R0,[R4,#24]
        CMP R0,#5
        BEQ ex_journal
        LDRB R1,[R4,#25]
        CMP R1,#0
        BNE ex_retry
        CMP R0,#1
        BEQ ex_scan
        CMP R0,#2
        BEQ ex_canonical
        CMP R0,#3
        BEQ ex_find
        CMP R0,#4
        BEQ ex_journal_begin
        CMP R0,#6
        BEQ ex_fill
        B ex_retry

ex_fill:
        LDR R6,[R4,#64]
        LDR R7,[R4,#4]
        SUB R7,R7,R6
        CMP R7,#1024
        IT HI
        MOVHI R7,#1024
        LDRH R8,[R4,#26]
        SUBS R2,R7,R8
        BEQ ex_fill_done
        CMP R2,#64
        IT HI
        MOVHI R2,#64
        ADD R0,R6,R8
        ADD R1,R4,#68
        ADD R1,R1,R8
        ADD R8,R8,R2
        CFI FunCall w25_read_data
        BL w25_read_data
        CMP R0,#0
        BEQ ex_retry
        STRH R8,[R4,#26]
        CMP R8,R7
        BLO ex_done
ex_fill_done:
        MOVS R0,#1
        STRB R0,[R4,#24]
        B ex_done
ex_scan:
        LDR R6,[R4]
        LDR R0,[R4,#4]
        CMP R6,R0
        BHS ex_release_normal
        LDR R0,[R4,#64]
        SUB R7,R6,R0
        CMP R7,#1024
        BLO ex_scan_ready
        STR R6,[R4,#64]
        MOVS R0,#0
        STRH R0,[R4,#26]
        MOVS R0,#6
        STRB R0,[R4,#24]
        B ex_done
ex_scan_ready:
        ADD R7,R7,R4
        ADD R7,R7,#68
        ADDS R6,R6,#32
        STR R6,[R4]
        LDRH R0,[R7]
        MOVW R1,#65535
        CMP R0,R1
        BEQ ex_done
        MOV R0,R7
        CFI FunCall id_time_end
        BL id_time_end
        CMP R0,#0
        BEQ ex_done
        LDR R1,[R4,#28]
        CMP R1,R0
        BHS ex_due
        CFI FunCall id_watch_end
        BL id_watch_end
        B ex_done
ex_due:
        LDR R2,=86400
        UDIV R0,R0,R2
        UDIV R1,R1,R2
        CMP R0,R1
        BNE ex_done
        ADD R0,R4,#32
        MOV R1,R7
        MOVS R2,#32
        CFI FunCall __aeabi_memcpy
        BL __aeabi_memcpy
        MOVS R0,#56
        STRB R0,[R4,#60]
        MOVS R0,#2
        STRB R0,[R4,#24]
        B ex_done

ex_canonical:
        LDRH R0,[R4,#32]
        CMP R0,#0
        BEQ ex_find_init
        MOVW R1,#65535
        CMP R0,R1
        BEQ ex_find_init
        SUBS R0,R0,#1
        LSLS R0,R0,#5
        LDR R6,[R4,#8]
        LDR R1,[R4,#20]
        SUB R1,R1,R6
        CMP R0,R1
        BHS ex_find_init
        ADD R6,R6,R0
        MOV R0,R6
        MOV R1,SP
        MOVS R2,#2
        CFI FunCall w25_read_data
        BL w25_read_data
        CMP R0,#0
        BEQ ex_retry
        LDRH R0,[SP]
        LDRH R1,[R4,#32]
        CMP R0,R1
        BEQ ex_found
ex_find_init:
        LDR R0,[R4,#8]
        STR R0,[R4,#16]
        MOVS R0,#3
        STRB R0,[R4,#24]
        B ex_done
ex_find:
        LDR R6,[R4,#16]
        LDR R0,[R4,#20]
        SUBS R7,R0,R6
        BEQ ex_retry
        CMP R7,#64
        IT HI
        MOVHI R7,#64
        MOV R0,R6
        MOV R1,SP
        MOV R2,R7
        CFI FunCall w25_read_data
        BL w25_read_data
        CMP R0,#0
        BEQ ex_retry
        LDRH R0,[SP]
        LDRH R1,[R4,#32]
        CMP R0,R1
        BEQ ex_found
        CMP R7,#64
        BNE ex_find_advance
        LDRH R0,[SP,#32]
        CMP R0,R1
        BNE ex_find_advance
        ADDS R6,R6,#32
        B ex_found
ex_find_advance:
        ADD R6,R6,R7
        STR R6,[R4,#16]
        B ex_done
ex_found:
        STR R6,[R4,#12]
        MOVS R0,#4
        STRB R0,[R4,#24]
        B ex_done

ex_journal_begin:
        LDR R0,[R4,#12]
        BIC R0,R0,#0xff0
        BIC R0,R0,#15
        LDR R1,=expiry_chunk
        MOV R2,R4
        CFI FunCall sj_runtime_begin
        BL sj_runtime_begin
        CMP R0,#0
        BEQ ex_retry
        MOVS R0,#5
        STRB R0,[R4,#24]
        B ex_done
ex_journal:
        LDRB R0,[R4,#25]
        CBZ R0,ex_journal_poll
        CFI FunCall sj_runtime_cancel
        BL sj_runtime_cancel
ex_journal_poll:
        MOV R0,R5
        CFI FunCall sj_runtime_poll
        BL sj_runtime_poll
        CMP R0,#1
        BEQ ex_done
        MOV R6,R0
        CFI FunCall sj_runtime_reset
        BL sj_runtime_reset
        CMP R6,#2
        BNE ex_journal_error
        MOVS R0,#1
        STRB R0,[R4,#24]
        MOVS R0,#138
        ADD R1,R4,#34
        MOVS R2,#255
        CFI FunCall log_addControlEvent
        BL log_addControlEvent
        LDRB R0,[R4,#25]
        CMP R0,#0
        BEQ ex_done
        B ex_retry
ex_journal_error:
        CFI FunCall sj_runtime_blocked
        BL sj_runtime_blocked
        CBZ R0,ex_retry
        LDR R1,=mem_storage_ready
        MOVS R0,#0
        STRB R0,[R1]
ex_retry:
        LDR R1,=id_endScan
        MOVS R0,#1
        STRB R0,[R1]
ex_release_normal:
        CFI FunCall expiry_release
        BL expiry_release
ex_done:
        ADD SP,SP,#64
        CFI CFA R13+24
        POP {R4-R8,PC}
        CFI EndBlock ex_poll
        END
