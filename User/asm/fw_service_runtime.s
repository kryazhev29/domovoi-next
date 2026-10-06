#include "../work_slots.h"
        EXTERN work_acquire
        EXTERN work_release
; Handwritten Cortex-M3 IAR cooperative command19 / rollback.
; Heap context128: data64, header32, reserve96, pos100, end104,
; CRC108, expected112, blocks116H, ch118, uniq119, phase120,
; pending121, quiet122, mode123, len124H, cancel126, config_result127.
; mode: 0 duplicate, 1 rollback, 2 download. WAIT_CONFIG8 yields ownership.
; All NOR source bytes remain owned until verified completion or error.
        AAPCS BASE,INTERWORK
        #include "flash_owner.h"
        EXTERN flash_claim
        EXTERN flash_release
        PRESERVE8
        REQUIRE8
        EXTERN mem
        EXTERN device
        EXTERN store_runtime
        EXTERN user_storage_command_ready
        EXTERN mem_tables_ready
        EXTERN fw_start_rejected
        EXTERN fw_start_refused
        EXTERN fw_same_download
        EXTERN fw_log
        EXTERN fw_abort
        EXTERN fw_rollback_pos
        EXTERN fw_rollback_end
        EXTERN fw_write_errors
        EXTERN w25_read_data
        EXTERN idcrc_io_start
        EXTERN idcrc_io_poll
        EXTERN timers_getMillis
        EXTERN fw_crc_begin
        EXTERN fw_crc_next
        EXTERN crc16_calc
        EXTERN answer11
        EXTERN user_comm_acquire
        EXTERN comm_tx_commit
        EXTERN comm_tx_release
        EXTERN usb_try_send
        EXTERN reply_send
        EXTERN __aeabi_memcpy
        EXTERN store
        EXTERN store_snapshot
        EXTERN store_save_requested
        PUBLIC fw_service_context
        SECTION .bss:DATA:REORDER:NOROOT(2)
fw_service_context:
        DS8 4
        CFI Names fs_names
        CFI StackFrame CFA R13 DATA
        CFI Resource R0:32,R1:32,R2:32,R3:32,R4:32,R5:32,R6:32,R7:32
        CFI Resource R8:32,R9:32,R10:32,R11:32,R12:32,R13:32,R14:32
        CFI EndNames fs_names
        CFI Common fs_common Using fs_names
        CFI CodeAlign 2
        CFI DataAlign 4
        CFI ReturnAddress R14 CODE
        CFI CFA R13+0
        CFI R4 SameValue
        CFI R5 SameValue
        CFI R6 SameValue
        CFI R7 SameValue
        CFI R8 SameValue
        CFI R9 SameValue
        CFI R10 SameValue
        CFI R11 SameValue
        CFI R0 Undefined
        CFI R1 Undefined
        CFI R2 Undefined
        CFI R3 Undefined
        CFI R12 Undefined
        CFI R14 SameValue
        CFI EndCommon fs_common
        PUBLIC fw_service_busy
        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block fs_busy_cfi Using fs_common
        CFI Function fw_service_busy
        CFI NoCalls
fw_service_busy:
        LDR.W R0,=fw_service_context
        LDR R0,[R0]
        BX LR
        CFI EndBlock fs_busy_cfi
        LTORG
        PUBLIC fw_service_active
        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block fw_service_active_cfi Using fs_common
        CFI Function fw_service_active
fw_service_active:
        CFI NoCalls
        LDR.W R1,=fw_service_context
        LDR R1,[R1]
        MOVS R0,#0
        CBZ R1,fsa_return
        LDRB R0,[R1,#120]
        CMP R0,#8
        BNE fsa_yes
        LDR.W R0,=store
        LDRB R0,[R0,#18]
        CMP R0,#0
        ITE EQ
        MOVEQ R0,#1
        MOVNE R0,#0
        BX LR
fsa_yes:
        MOVS R0,#1
fsa_return:
        BX LR
        CFI EndBlock fw_service_active_cfi
        LTORG
        PUBLIC fw_service_config_result
        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block fw_service_config_result_cfi Using fs_common
        CFI Function fw_service_config_result
; WAIT_CONFIG takes the Flash back as soon as config reports its result,
; before the store machine clears store[18] (no gap for another owner).
fw_service_config_result:
        LDR.W R1,=fw_service_context
        LDR R1,[R1]
        CBZ R1,fscr_return
        LDRB R2,[R1,#120]
        CMP R2,#8
        BNE fscr_return
        CMP R0,#2
        BEQ fscr_store
        CMP R0,#3
        BNE fscr_return
fscr_store:
        STRB R0,[R1,#127]
        MOVS R0,#FO_FWSERVICE
        CFI FunCall flash_claim
        B.W flash_claim
fscr_return:
        BX LR
        CFI EndBlock fw_service_config_result_cfi
        LTORG
        PUBLIC fw_service_quiet
        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block fw_service_quiet_cfi Using fs_common
        CFI Function fw_service_quiet
fw_service_quiet:
        CFI NoCalls
        LDR.W R1,=fw_service_context
        LDR R1,[R1]
        CBZ R1,fsq_return
        LDRB R2,[R1,#118]
        CMP R0,R2
        BNE fsq_return
        MOVS R0,#1
        STRB R0,[R1,#122]
fsq_return:
        BX LR
        CFI EndBlock fw_service_quiet_cfi
        LTORG
        PUBLIC fw_service_cancel
        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block fw_service_cancel_cfi Using fs_common
        CFI Function fw_service_cancel
fw_service_cancel:
        PUSH {R4-R7,LR}
        CFI R14 Frame(CFA, -4)
        CFI R7 Frame(CFA, -8)
        CFI R6 Frame(CFA, -12)
        CFI R5 Frame(CFA, -16)
        CFI R4 Frame(CFA, -20)
        CFI CFA R13+20
        SUB SP,SP,#4
        CFI CFA R13+24
        LDR.W R0,=fw_service_context
        LDR R4,[R0]
        CMP R4,#0
        BEQ fsc_return
        MOVS R1,#1
        STRB R1,[R4,#126]
        STRB R1,[R4,#122]
        LDRB R1,[R4,#121]
        CMP R1,#0
        BNE fsc_return
        ; WAIT_CONFIG owns no NOR source: the config job has its own copy.
        MOVS R1,#0
        STR R1,[R0]
        MOVS R0,#FO_FWSERVICE
        CFI FunCall flash_release
        BL flash_release
        LDR R0,[R4,#96]
        MOVS R1,#WORK_FW_DATA
        CFI FunCall work_release
        BL work_release
        MOV R0,R4
        MOVS R1,#WORK_FW_CONTEXT
        CFI FunCall work_release
        BL work_release
fsc_return:
        ADD SP,SP,#4
        CFI CFA R13+20
        POP {R4-R7,PC}
        CFI CFA R13+24
        CFI EndBlock fw_service_cancel_cfi
        LTORG
        PUBLIC fw_service_begin
        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block fw_service_begin_cfi Using fs_common
        CFI Function fw_service_begin
fw_service_begin:
        PUSH {R4-R7,LR}
        CFI R14 Frame(CFA, -4)
        CFI R7 Frame(CFA, -8)
        CFI R6 Frame(CFA, -12)
        CFI R5 Frame(CFA, -16)
        CFI R4 Frame(CFA, -20)
        CFI CFA R13+20
        SUB SP,SP,#12
        CFI CFA R13+32
        MOV R4,R0
        MOV R5,R1
        MOV R6,R2
        LDR.W R0,=store_runtime
        LDRB R0,[R0]
        CMP R0,#0
        BEQ fsb_return
        LDR.W R0,=fw_service_context
        LDR R0,[R0]
        CMP R0,#0
        BNE fsb_handled
        CFI FunCall user_storage_command_ready
        BL user_storage_command_ready
        CMP R0,#0
        BEQ fsb_handled
        CFI FunCall mem_tables_ready
        BL mem_tables_ready
        CMP R0,#0
        BEQ fsb_handled
        LDRH R0,[R4,#14]
        STR R0,[SP]
        LDRH R0,[R4]
        LDR R1,[R4,#2]
        LDR R2,[R4,#6]
        LDR R3,[R4,#10]
        CFI FunCall fw_start_rejected
        BL fw_start_rejected
        CMP R0,#0
        BEQ fsb_params_ok
        LDR.W R1,=fs_param
        MOVS R0,#133
        CFI FunCall fw_log
        BL fw_log
        MOVS R1,#1
        B fsb_error
fsb_params_ok:
        MOVS R0,#128
        MOVS R1,#WORK_FW_CONTEXT
        CFI FunCall work_acquire
        BL work_acquire
        MOVS R7,R0
        BEQ fsb_handled
        MOVW R0,#1024
        MOVS R1,#WORK_FW_DATA
        CFI FunCall work_acquire
        BL work_acquire
        CMP R0,#0
        BEQ fsb_free
        STR R0,[R7,#96]
        MOVS R0,#0
        STR R0,[R7,#120]
        STR R0,[R7,#124]
        STRB R5,[R7,#118]
        STRB R6,[R7,#119]
        LDRH R0,[R4,#14]
        STRH R0,[R7,#116]
        LDRH R0,[R4]
        CMP R0,#1
        BEQ fsb_refused
        LDRH R0,[R4,#14]
        STR R0,[SP]
        STR R5,[SP,#4]
        LDRH R0,[R4]
        LDR R1,[R4,#2]
        LDR R2,[R4,#6]
        LDR R3,[R4,#10]
        CFI FunCall fw_same_download
        BL fw_same_download
        CMP R0,#0
        BEQ fsb_refused
        LDR.W R1,=device
        MOVS R0,#0
        STR R0,[R1,#174]
        MOVS R0,#6
        B fsb_publish
fsb_refused:
        CFI FunCall fw_start_refused
        BL fw_start_refused
        CMP R0,#0
        BEQ fsb_prepare
        MOV R1,R0
        MOVS R0,#133
        CFI FunCall fw_log
        BL fw_log
        LDR R0,[R7,#96]
        MOVS R1,#0
        STR R1,[R7,#96]
        MOVS R1,#WORK_FW_DATA
        CFI FunCall work_release
        BL work_release
        MOVS R0,#2
        STRB R0,[R7,#127]
        MOVS R0,#18
        B fsb_publish
fsb_prepare:
        LDRH R0,[R4]
        CMP R0,#1
        BNE fsb_header
        STRB R0,[R7,#123]
        B fsb_publish
fsb_header:
        MVN R0,#0
        ADD R1,R7,#64
        MOVS R2,#8
fsb_ff:
        STR R0,[R1],#4
        SUBS R2,R2,#1
        BNE fsb_ff
        LDR.W R1,=device
        LDRB R0,[R1,#19]
        STRB R0,[R7,#68]
        LDRH R0,[R4]
        STRH R0,[R7,#69]
        LDR R0,[R4,#6]
        STR R0,[R7,#72]
        LDR R0,[R4,#10]
        STR R0,[R7,#76]
        LDR R0,[R4,#2]
        STR R0,[R7,#84]
        MOVS R0,#2
        STRB R0,[R7,#123]
        MOVS R0,#4
fsb_publish:
        STRB R0,[R7,#120]
        ; Cannot fail: user_storage_command_ready saw no owner in this call.
        MOVS R0,#FO_FWSERVICE
        CFI FunCall flash_claim
        BL flash_claim
        LDR.W R0,=fw_service_context
        STR R7,[R0]
        B fsb_handled
fsb_free:
        MOV R0,R7
        MOVS R1,#WORK_FW_CONTEXT
        CFI FunCall work_release
        BL work_release
        B fsb_handled
fsb_error:
        ; Parameter failures also use an owned, retryable bounded response.
        STR R1,[SP]
        MOVS R0,#128
        MOVS R1,#WORK_FW_CONTEXT
        CFI FunCall work_acquire
        BL work_acquire
        MOVS R7,R0
        BEQ fsb_handled
        MOVS R0,#0
        STR R0,[R7,#96]
        STR R0,[R7,#120]
        STR R0,[R7,#124]
        STRB R5,[R7,#118]
        STRB R6,[R7,#119]
        LDR R0,[SP]
        STRB R0,[R7,#127]
        MOVS R0,#18
        B fsb_publish
fsb_handled:
        MOVS R0,#1
fsb_return:
        ADD SP,SP,#12
        CFI CFA R13+20
        POP {R4-R7,PC}
        CFI CFA R13+32
        CFI EndBlock fw_service_begin_cfi
        LTORG
        PUBLIC fw_service_poll
        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block fw_service_poll_cfi Using fs_common
        CFI Function fw_service_poll
fw_service_poll:
        PUSH {R4-R7,LR}
        CFI R14 Frame(CFA, -4)
        CFI R7 Frame(CFA, -8)
        CFI R6 Frame(CFA, -12)
        CFI R5 Frame(CFA, -16)
        CFI R4 Frame(CFA, -20)
        CFI CFA R13+20
        SUB SP,SP,#4
        CFI CFA R13+24
        LDR.W R0,=fw_service_context
        LDR R4,[R0]
        CMP R4,#0
        BEQ.W fsp_return
        LDRB R5,[R4,#120]
        LDRB R0,[R4,#121]
        CMP R0,#0
        BEQ fsp_dispatch
        CFI FunCall timers_getMillis
        BL timers_getMillis
        CFI FunCall idcrc_io_poll
        BL idcrc_io_poll
        CMP R0,#1
        BEQ.W fsp_return
        MOVS R1,#0
        STRB R1,[R4,#121]
        LDRB R1,[R4,#126]
        CMP R1,#0
        BNE.W fsp_cancel
        CMP R0,#2
        BNE.W fsp_io_error
        CMP R5,#4
        BEQ fsp_to_header
        CMP R5,#5
        BEQ fsp_to_apply
        CMP R5,#10
        BEQ fsp_to_read
        ; A verified copy write advances the cursor exactly once.
        LDR R0,[R4,#100]
        LDRH R1,[R4,#124]
        ADDS R0,R0,R1
        STR R0,[R4,#100]
        LDR.W R1,=fw_rollback_pos
        STR R0,[R1]
        LDR R1,[R4,#104]
        CMP R0,R1
        BCS fsp_to_final
        LSLS R1,R0,#20
        BEQ fsp_to_erase
fsp_to_read:
        MOVS R0,#11
        B fsp_phase
fsp_to_erase:
        MOVS R0,#10
        B fsp_phase
fsp_to_final:
        MOVS R0,#13
        B fsp_phase
fsp_to_header:
        MOVS R0,#5
        B fsp_phase
fsp_to_apply:
        MOVS R0,#6
        B fsp_phase
fsp_dispatch:
        LDRB R0,[R4,#126]
        CMP R0,#0
        BNE.W fsp_cancel
        CMP R5,#1
        BEQ.W fsp_backup_header
        CMP R5,#2
        BEQ.W fsp_crc
        CMP R5,#4
        BEQ.W fsp_erase
        CMP R5,#5
        BEQ.W fsp_header
        CMP R5,#6
        BEQ.W fsp_apply
        CMP R5,#8
        BEQ.W fsp_wait_config
        CMP R5,#10
        BEQ.W fsp_erase
        CMP R5,#11
        BEQ.W fsp_copy_read
        CMP R5,#12
        BEQ.W fsp_copy_write
        CMP R5,#13
        BEQ.W fsp_final_header
        CMP R5,#14
        BEQ.W fsp_crc
        CMP R5,#16
        BEQ.W fsp_send
        CMP R5,#18
        BNE.W fsp_return
        LDRB R6,[R4,#127]
        B.W fsp_answer
fsp_erase:
        LDR.W R0,=mem
        LDR R0,[R0]
        CMP R5,#10
        ITT EQ
        LDREQ R1,[R4,#100]
        ADDEQ R0,R0,R1
        MOVS R1,#0
        MOVS R2,#0
        B fsp_start
fsp_header:
        LDR.W R0,=mem
        LDR R0,[R0]
        ADD R1,R4,#64
        MOVS R2,#32
        B fsp_start
fsp_copy_write:
        LDR.W R0,=mem
        LDR R0,[R0]
        LDR R1,[R4,#100]
        ADDS R0,R0,R1
        MOV R1,R4
        LDRH R2,[R4,#124]
fsp_start:
        MOV R6,R0
        MOV R7,R1
        STR R2,[SP]
        CFI FunCall timers_getMillis
        BL timers_getMillis
        MOV R3,R0
        MOV R0,R6
        MOV R1,R7
        LDR R2,[SP]
        CFI FunCall idcrc_io_start
        BL idcrc_io_start
        CMP R0,#0
        BEQ.W fsp_io_error
        MOVS R0,#1
        STRB R0,[R4,#121]
        B.W fsp_return
fsp_copy_read:
        LDR R6,[R4,#100]
        LDR R2,[R4,#104]
        SUBS R2,R2,R6
        CMP R2,#64
        IT HI
        MOVHI R2,#64
        STRH R2,[R4,#124]
        LDR.W R0,=mem
        LDR R0,[R0,#8]
        ADDS R0,R0,R6
        MOV R1,R4
        CFI FunCall w25_read_data
        BL w25_read_data
        CMP R0,#0
        BEQ.W fsp_read_error
        CMP R6,#0
        BNE fsp_copy_read_done
        MVN R0,#0
        STR R0,[R4]
fsp_copy_read_done:
        MOVS R0,#12
        B fsp_phase
fsp_backup_header:
        LDR.W R0,=mem
        LDR R0,[R0,#8]
        B fsp_read_header
fsp_final_header:
        LDR.W R0,=mem
        LDR R0,[R0]
fsp_read_header:
        ADD R1,R4,#64
        MOVS R2,#32
        CFI FunCall w25_read_data
        BL w25_read_data
        CMP R0,#0
        BEQ.W fsp_read_error
        LDR R0,[R4,#84]
        CMN R0,#1
        BEQ.W fsp_bad_header
        LDR R0,[R4,#76]
        CMN R0,#1
        BEQ.W fsp_bad_header
        STR R0,[R4,#112]
        LDRH R0,[R4,#69]
        MOVW R1,#65535
        CMP R0,R1
        BEQ.W fsp_bad_header
        LDR R0,[R4,#72]
        CMP R0,#0
        BEQ.W fsp_bad_crc
        TST R0,#3
        BNE.W fsp_bad_crc
        LDR.W R1,=0x17e00
        CMP R0,R1
        BHI.W fsp_bad_crc
        ADD R0,R0,#512
        STR R0,[R4,#104]
        MOV R0,#512
        STR R0,[R4,#100]
        CFI FunCall fw_crc_begin
        BL fw_crc_begin
        STR R0,[R4,#108]
        MOVS R0,#2
        CMP R5,#13
        IT EQ
        MOVEQ R0,#14
        B fsp_phase
fsp_crc:
        LDR R6,[R4,#100]
        LDR R2,[R4,#104]
        SUBS R2,R2,R6
        CMP R2,#64
        IT HI
        MOVHI R2,#64
        STRH R2,[R4,#124]
        LDR.W R0,=mem
        CMP R5,#2
        ITE EQ
        LDREQ R0,[R0,#8]
        LDRNE R0,[R0]
        ADDS R0,R0,R6
        MOV R1,R4
        CFI FunCall w25_read_data
        BL w25_read_data
        CMP R0,#0
        BEQ.W fsp_read_error
        LDR R0,[R4,#108]
        MOV R6,R4
        LDRH R7,[R4,#124]
        ADD R7,R7,R4
fsp_crc_word:
        LDR R1,[R6],#4
        CFI FunCall fw_crc_next
        BL fw_crc_next
        CMP R6,R7
        BCC fsp_crc_word
        STR R0,[R4,#108]
        LDR R1,[R4,#100]
        LDRH R2,[R4,#124]
        ADDS R1,R1,R2
        STR R1,[R4,#100]
        LDR R2,[R4,#104]
        CMP R1,R2
        BCC.W fsp_return
        LDR R1,[R4,#112]
        CMP R0,R1
        BNE.W fsp_bad_crc
        CMP R5,#14
        BEQ.W fsp_success
        ; Backup is fully checked before any staging erase or success ACK.
        LDR.W R0,=fw_rollback_end
        STR R2,[R0]
        LDR.W R0,=fw_rollback_pos
        MOVS R1,#0
        STR R1,[R0]
        LDR.W R0,=device
        LDRB R1,[R4,#118]
        STRB R1,[R0,#178]
        LDRH R1,[R0,#129]
        ORR R1,R1,#0x800
        STRH R1,[R0,#129]
        B fsp_handoff
fsp_apply:
        LDRB R0,[R4,#123]
        CMP R0,#0
        BEQ fsp_handoff
        LDR.W R5,=device
        LDRH R0,[R4,#116]
        STRH R0,[R5,#148]
        MOVS R0,#0
        STRH R0,[R5,#172]
        STRB R0,[R5,#150]
        STR R0,[R5,#174]
        LDR.W R1,=fw_write_errors
        STRB R0,[R1]
        LDRB R6,[R4,#118]
        STRB R6,[R5,#178]
        MOVS R0,#132
        MOVS R1,#0
        CFI FunCall fw_log
        BL fw_log
        CMP R6,#0
        BEQ fsp_handoff
        MOVS R0,#1
        STRB R0,[R5,#164]
        MOVW R0,#600
        STRH R0,[R5,#179]
        LDRH R0,[R5,#129]
        ORR R0,R0,#0x800
        STRH R0,[R5,#129]
fsp_handoff:
        ; No allocator runs between freeing the reserved capacity and snapshot.
        ; Detach the Flash gate, preserving this context until config verifies.
        LDR.W R0,=fw_service_context
        MOVS R1,#0
        STR R1,[R0]
        LDR R0,[R4,#96]
        STR R1,[R4,#96]
        MOVS R1,#WORK_FW_DATA
        CFI FunCall work_release
        BL work_release
        MOVS R0,#FO_FWSERVICE
        CFI FunCall flash_release
        BL flash_release
        CFI FunCall store_snapshot
        BL store_snapshot
        MOV R5,R0
        LDR.W R0,=fw_service_context
        STR R4,[R0]
        CMP R5,#0
        BNE fsp_handoff_ok
        MOVS R0,#FO_FWSERVICE
        CFI FunCall flash_claim
        BL flash_claim
        B.W fsp_io_error
fsp_handoff_ok:
        LDR.W R0,=store
        MOVS R1,#19
        STRB R1,[R0,#21]
        LDRB R1,[R4,#118]
        STRB R1,[R0,#22]
        LDRB R1,[R4,#119]
        STRB R1,[R0,#23]
        MOVS R1,#1
        STRB R1,[R0,#24]
        MOVS R1,#9
        STRB R1,[R0,#18]
        LDR.W R0,=store_save_requested
        MOVS R1,#0
        STRB R1,[R0]
        MOVS R0,#8
        B fsp_phase
fsp_wait_config:
        LDR.W R0,=store
        LDRB R0,[R0,#18]
        CMP R0,#0
        BNE.W fsp_return
        ; The Flash is ours again: store[18] leaves 9 only after config
        ; reported 2/3, and fw_service_config_result claimed it then.
        LDRB R0,[R4,#127]
        CMP R0,#2
        BNE.W fsp_io_error
        MOVS R6,#0
        B fsp_answer
fsp_success:
        LDR.W R0,=fw_rollback_end
        MOVS R1,#0
        STR R1,[R0]
        MOVS R0,#137
        CFI FunCall fw_log
        BL fw_log
        LDR.W R1,=device
        MOVS R0,#1
        STRB R0,[R1,#171]
        B fsp_cancel
fsp_bad_header:
        CMP R5,#13
        BCS fsp_final_crc_error
        LDR.W R1,=fs_no_backup
        B fsp_error_log
fsp_bad_crc:
        CMP R5,#14
        BEQ fsp_final_crc_error
        LDR.W R1,=fs_bad_backup
        B fsp_error_log
fsp_read_error:
        CMP R5,#13
        BCS fsp_final_crc_error
        CMP R5,#2
        BEQ fsp_bad_crc
        CMP R5,#1
        BEQ.W fsp_before_ack_cleanup
        LDR.W R1,=fs_read
        B fsp_error_log
fsp_final_crc_error:
        LDR.W R1,=fs_crc
        B fsp_error_log
fsp_io_error:
        LDRB R0,[R4,#120]
        CMP R0,#10
        BCC.W fsp_before_ack_cleanup
        LDR.W R1,=fs_io
fsp_error_log:
        LDRB R0,[R4,#120]
        CMP R0,#10
        BCC fsp_before_ack_error
        CMP R0,#16
        BCS fsp_before_ack_error
        ; Preserve fw_end cleanup/logging, retaining the owned status reply.
        LDR.W R0,=fw_service_context
        MOVS R2,#0
        STR R2,[R0]
        MOV R6,R1
        MOVS R0,#FO_FWSERVICE
        CFI FunCall flash_release
        BL flash_release
        MOV R0,R6
        CFI FunCall fw_abort
        BL fw_abort
        MOVS R0,#FO_FWSERVICE
        CFI FunCall flash_claim
        BL flash_claim
        LDR.W R0,=fw_service_context
        STR R4,[R0]
        LDRB R0,[R4,#118]
        CMP R0,#0
        BEQ fsp_status_reply
        LDR.W R0,=device
        LDRB R0,[R0,#133]
        CMP R0,#0
        BEQ.W fsp_cancel
fsp_status_reply:
        LDR.W R0,=0x4e444c53
        STR R0,[R4]
        MOVS R0,#5
        STRH R0,[R4,#4]
        MOVS R0,#255
        STRB R0,[R4,#6]
        MOVS R0,#0
        STRB R0,[R4,#7]
        MOVS R0,#4
        STRB R0,[R4,#8]
        LDR.W R0,=device
        LDRB R0,[R0,#150]
        STRB R0,[R4,#9]
        MOVS R0,#253
        STRB R0,[R4,#10]
        ADD R0,R4,#4
        MOVS R1,#7
        CFI FunCall crc16_calc
        BL crc16_calc
        STRH R0,[R4,#11]
        MOVS R0,#13
        STRH R0,[R4,#124]
        MOVS R0,#16
        B fsp_phase
fsp_before_ack_error:
        MOVS R0,#133
        CFI FunCall fw_log
        BL fw_log
fsp_before_ack_cleanup:
        ; Rollback setup/config failure must never expose a copy cursor.
        LDR.W R0,=fw_rollback_end
        MOVS R1,#0
        STR R1,[R0]
        LDR.W R0,=device
        LDRH R1,[R0,#129]
        BIC R1,R1,#0x800
        STRH R1,[R0,#129]
        ; Abort failed setup state without cancelling this owned error reply.
        MOVS R1,#0
        STRB R1,[R0,#164]
        STRH R1,[R0,#179]
        STRH R1,[R0,#148]
        STRH R1,[R0,#172]
        LDR.W R0,=fw_write_errors
        STRB R1,[R0]
        MOVS R6,#2
fsp_answer:
        ; Exact existing answer command wire: 19 bytes, immutable on retry.
        MOV R0,R4
        LDRB R1,[R4,#119]
        MOVS R2,#19
        MOV R3,R6
        CFI FunCall answer11
        BL answer11
        MOVS R0,#19
        STRH R0,[R4,#124]
        MOVS R0,#16
fsp_phase:
        STRB R0,[R4,#120]
        B fsp_return
fsp_send:
        LDRB R0,[R4,#122]
        CMP R0,#0
        BNE fsp_sent
        MOV R0,R4
        LDRH R1,[R4,#124]
        LDRB R2,[R4,#118]
        CFI FunCall reply_send
        BL reply_send
        CMP R0,#0
        BEQ fsp_return
fsp_sent:
        LDRB R0,[R4,#8]
        CMP R0,#11
        BNE fsp_cancel
        LDRB R0,[R4,#10]
        CMP R0,#0
        BNE fsp_cancel
        LDRB R0,[R4,#123]
        CMP R0,#1
        BNE fsp_cancel
        MOVS R0,#0
        STR R0,[R4,#100]
        MOVS R0,#10
        B fsp_phase
fsp_cancel:
        CFI FunCall fw_service_cancel
        BL fw_service_cancel
fsp_return:
        ADD SP,SP,#4
        CFI CFA R13+20
        POP {R4-R7,PC}
        CFI CFA R13+24
        CFI EndBlock fw_service_poll_cfi
        LTORG
        SECTION .rodata:CONST:REORDER:NOROOT(2)
fs_param: DC8 "PARAM",0
fs_no_backup: DC8 "NO BACKUP",0
fs_bad_backup: DC8 "BAD BACKUP",0
fs_read: DC8 "READ",0
fs_crc: DC8 "CRC",0
fs_io: DC8 "WRITE",0
        END

