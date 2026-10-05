#include "../work_slots.h"
        EXTERN work_acquire
        EXTERN work_release
; Handwritten Cortex-M3 / IAR cooperative command 9/10 upload.
; Context (36 heap bytes): packet0, pos4, limit8, USB epoch12,
; total16/first18/wanted20/copied22/seen24/crc26/offset28/wirelen30,
; command32/channel33/uniq34/phase35. No incoming packet is retained.
        AAPCS BASE,INTERWORK
        #include "flash_owner.h"
        EXTERN flash_claim
        EXTERN flash_release
        PRESERVE8
        REQUIRE8
        EXTERN mem
        EXTERN device
        EXTERN store_runtime
        EXTERN usb_bus_resets
        EXTERN mem_tables_ready
        EXTERN w25_read_data
        EXTERN log_blank
        EXTERN log_valid
        EXTERN log_lastIndex
        EXTERN log_unacked
        EXTERN temp_unacked
        EXTERN user_comm_acquire
        EXTERN comm_tx_commit
        EXTERN comm_tx_release
        EXTERN usb_try_send
        EXTERN reply_send
        EXTERN crc16_calcInit
        EXTERN __aeabi_memcpy
        EXTERN store_upload_request
        EXTERN work_upload_ack
        EXTERN upload_index
        EXTERN upload_finish
        EXTERN user_send_anyData
        PUBLIC upload_runtime_begin
        PUBLIC upload_runtime_active
        PUBLIC upload_runtime_poll
        PUBLIC upload_runtime_cancel
        PUBLIC upload_runtime_quiet
        PUBLIC upload_runtime_context

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
upload_runtime_context:
        DS8 4

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block up_begin_cfi Using cfiCommon0
          CFI Function upload_runtime_begin
        THUMB
upload_runtime_begin:
        PUSH {R4-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+20
        SUB SP,SP,#+4
          CFI CFA R13+24
        MOV R4,R0
        MOV R5,R1
        MOV R6,R2
        LDR.W R0,up_runtime
        LDRB R0,[R0]
        CMP R0,#+0
        BEQ.W up_begin_return
        LDR.W R0,up_context
        LDR R0,[R0]
        CMP R0,#+0
        BNE.W up_begin_handled
        ; Capture the public media/recovery gate before publishing this owner.
        ; Reads below bypass mem_read only after this check succeeds.
          CFI FunCall mem_tables_ready
        BL mem_tables_ready
        CMP R0,#+0
        BEQ.W up_begin_handled
        ; Reserve BOTH objects before ACK can request durable cursor writes.
        MOVS R0,#+36
        MOVS R1,#WORK_UPLOAD_CONTEXT
          CFI FunCall work_acquire
        BL work_acquire
        CMP R0,#+0
        BEQ.W up_begin_handled
        MOV R7,R0
        MOVW R0,#+1040
        MOVS R1,#WORK_UPLOAD_PACKET
          CFI FunCall work_acquire
        BL work_acquire
        CMP R0,#+0
        BEQ.N up_begin_free_context
        STR R0,[R7]
        MOV R0,R4
        MOV R1,R5
        MOV R2,R6
          CFI FunCall store_upload_request
        BL store_upload_request
        CMP R0,#+0
        BEQ.N up_begin_free_packet
        MOV R0,R4
        MOV R1,R5
        MOV R2,R7
          CFI FunCall work_upload_ack
        BL work_upload_ack
        CMP R0,#+0
        BEQ.N up_begin_handled
        MOV R0,R4
        MOV R1,R5
          CFI FunCall upload_index
        BL upload_index
        STRH R0,[R7,#+18]
        MOV R0,R7
        MOVS R1,#+0
        STRH R1,[R0,#+16]
        STR R1,[R0,#+20]
        STR R1,[R0,#+24]
        STR R1,[R0,#+28]
        STRB R4,[R0,#+32]
        STRB R5,[R0,#+33]
        STRB R6,[R0,#+34]
        MOVS R1,#+1
        STRB R1,[R0,#+35]
        LDR.W R1,up_usb_epoch
        LDR R1,[R1]
        STR R1,[R0,#+12]
        LDR.W R1,up_mem
        CMP R4,#+10
        ITE EQ
        ADDEQ R1,R1,#+56
        ADDNE R1,R1,#+64
        LDR R2,[R1]
        LDR R3,[R1,#+4]
        STR R2,[R0,#+4]
        ADDS R2,R2,R3
        STR R2,[R0,#+8]
        LDR.W R1,up_context
        STR R0,[R1]
        ; Cannot fail: commands start only with no owner; the config job
        ; started by the ACK above is not a byte owner.
        MOVS R0,#+FO_UPLOAD
          CFI FunCall flash_claim
        BL flash_claim
        B.N up_begin_handled
up_begin_free_packet:
        LDR R0,[R7]
        MOVS R1,#WORK_UPLOAD_PACKET
          CFI FunCall work_release
        BL work_release
up_begin_free_context:
        MOV R0,R7
        MOVS R1,#WORK_UPLOAD_CONTEXT
          CFI FunCall work_release
        BL work_release
up_begin_handled:
        MOVS R0,#+1
up_begin_return:
        ADD SP,SP,#+4
          CFI CFA R13+20
        POP {R4-R7,PC}
          CFI EndBlock up_begin_cfi

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block up_active_cfi Using cfiCommon0
          CFI Function upload_runtime_active
          CFI NoCalls
        THUMB
upload_runtime_active:
        LDR.W R0,up_context
        LDR R0,[R0]
        CMP R0,#+0
        IT NE
        MOVNE R0,#+1
        BX LR
          CFI EndBlock up_active_cfi

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block up_quiet_cfi Using cfiCommon0
          CFI Function upload_runtime_quiet
        THUMB
upload_runtime_quiet:
        LDR.W R1,up_context
        LDR R1,[R1]
        CBZ R1,up_quiet_return
        LDRB R1,[R1,#+33]
        CMP R0,R1
        BNE.N up_quiet_return
          CFI TailCall upload_runtime_cancel
        B.W upload_runtime_cancel
up_quiet_return:
        BX LR
          CFI EndBlock up_quiet_cfi

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block up_cancel_cfi Using cfiCommon0
          CFI Function upload_runtime_cancel
        THUMB
upload_runtime_cancel:
        PUSH {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        LDR.W R0,up_context
        LDR R4,[R0]
        CBZ R4,up_cancel_return
        MOVS R1,#+0
        STR R1,[R0]
        MOVS R0,#+FO_UPLOAD
          CFI FunCall flash_release
        BL flash_release
        LDR R0,[R4]
        CBZ R0,up_cancel_context
        MOVS R1,#WORK_UPLOAD_PACKET
          CFI FunCall work_release
        BL work_release
up_cancel_context:
        MOV R0,R4
        MOVS R1,#WORK_UPLOAD_CONTEXT
          CFI FunCall work_release
        BL work_release
up_cancel_return:
        POP {R4,PC}
          CFI EndBlock up_cancel_cfi

        SECTION `.text`:CODE:NOROOT(2)
        DATA
up_context: DC32 upload_runtime_context
up_mem: DC32 mem
up_device: DC32 device
up_runtime: DC32 store_runtime
up_usb_epoch: DC32 usb_bus_resets
up_last_index: DC32 log_lastIndex
up_log_pending: DC32 log_unacked
up_temp_pending: DC32 temp_unacked

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block up_poll_cfi Using cfiCommon0
          CFI Function upload_runtime_poll
        THUMB
upload_runtime_poll:
        PUSH {R4-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+20
        SUB SP,SP,#+44
          CFI CFA R13+64
        LDR.W R0,up_context
        LDR R4,[R0]
        CMP R4,#+0
        BEQ.W up_poll_return
        LDRB R5,[R4,#+32]
        LDRB R0,[R4,#+33]
        CBNZ R0,up_poll_epoch_ok
        LDR.W R0,up_usb_epoch
        LDR R0,[R0]
        LDR R1,[R4,#+12]
        CMP R0,R1
        BNE.W up_poll_cancel
up_poll_epoch_ok:
        LDRB R0,[R4,#+35]
        CMP R0,#+1
        BEQ.W up_poll_count
        CMP R0,#+2
        BEQ.W up_poll_allocate
        CMP R0,#+3
        BEQ.W up_poll_fill
        CMP R0,#+4
        BEQ.W up_poll_crc
        CMP R0,#+5
        BEQ.W up_poll_send
        B.W up_poll_cancel
up_poll_count:
        LDR R6,[R4,#+4]
        LDR R1,[R4,#+8]
        ADD R0,R6,#+32
        CMP R0,R1
        BHI.W up_poll_count_done
        MOV R0,R6
        MOV R1,SP
        MOVS R2,#+32
          CFI FunCall w25_read_data
        BL w25_read_data
        CMP R0,#+0
        BEQ.W up_poll_cancel
        CMP R5,#+10
        BEQ.N up_poll_count_log
        LDRH R0,[SP]
        MOVW R1,#+65535
        CMP R0,R1
        BEQ.N up_poll_count_done
        B.N up_poll_count_valid
up_poll_count_log:
        MOV R0,SP
          CFI FunCall log_blank
        BL log_blank
        CBNZ R0,up_poll_count_done
        MOV R0,SP
          CFI FunCall log_valid
        BL log_valid
        CBZ R0,up_poll_count_next
        LDR.W R1,up_last_index
        LDR R0,[R1]
        LDR R2,[SP]
        CMP R0,R2
        IT CC
        STRCC R2,[R1]
up_poll_count_valid:
        LDRH R0,[R4,#+16]
        ADDS R0,R0,#+1
        STRH R0,[R4,#+16]
up_poll_count_next:
        ADD R6,R6,#+32
        STR R6,[R4,#+4]
        B.W up_poll_return
up_poll_count_done:
        LDR.W R7,up_device
        LDRH R0,[R4,#+16]
        CMP R5,#+10
        BNE.N up_poll_count_temp_done
        STRH R0,[R7,#+89]
        STR R6,[R7,#+91]
        B.N up_poll_count_published
up_poll_count_temp_done:
        STRH R0,[R7,#+83]
        STR R6,[R7,#+85]
up_poll_count_published:
        LDRH R1,[R4,#+18]
        CMP R0,R1
        BLS.W up_poll_empty
        SUBS R0,R0,R1
        CMP R5,#+10
        ITE EQ
        MOVEQ R1,#+32
        MOVNE R1,#+100
        CMP R0,R1
        IT HI
        MOVHI R0,R1
        STRH R0,[R4,#+20]
        CMP R5,#+10
        ITE EQ
        MOVEQ R1,#+32
        MOVNE R1,#+10
        MULS R0,R0,R1
        ADDS R0,R0,#+14
        STRH R0,[R4,#+30]
        MOVS R0,#+2
        B.W up_poll_phase
up_poll_allocate:
        LDR R0,[R4]
up_poll_allocated:
        STR R0,[R4]
        MOV R7,R0
        MOVW R0,#+19539
        MOVT R0,#+20036
        STR R0,[R7]
        LDRH R0,[R4,#+30]
        SUBS R0,R0,#+8
        STRH R0,[R7,#+4]
        MOVS R0,#+255
        STRB R0,[R7,#+6]
        LDRB R0,[R4,#+34]
        STRB R0,[R7,#+7]
        STRB R5,[R7,#+8]
        LDRH R0,[R4,#+16]
        LDRH R1,[R4,#+18]
        SUBS R0,R0,R1
        LDRH R2,[R4,#+20]
        SUBS R0,R0,R2
        STRB R0,[R7,#+9]
        LSRS R0,R0,#+8
        STRB R0,[R7,#+10]
        STRB R2,[R7,#+11]
        LDR.W R0,up_mem
        CMP R5,#+10
        BNE.N up_poll_allocate_temp
        LDR R0,[R0,#+56]
        B.N up_poll_allocate_pos
up_poll_allocate_temp:
        LDR R0,[R0,#+64]
        ADD R0,R0,R1,LSL #+5
up_poll_allocate_pos:
        STR R0,[R4,#+4]
        MOVS R0,#+3
        B.W up_poll_phase
up_poll_fill:
        LDR R6,[R4,#+4]
        LDR R1,[R4,#+8]
        ADD R0,R6,#+32
        CMP R0,R1
        BHI.W up_poll_cancel
        CMP R5,#+10
        BEQ.N up_poll_fill_log
        LDRH R0,[R4,#+22]
        MOVS R1,#+10
        MULS R0,R0,R1
        LDR R1,[R4]
        ADDS R1,R1,R0
        ADDS R1,R1,#+12
        ADDS R0,R6,#+2
        MOVS R2,#+10
          CFI FunCall w25_read_data
        BL w25_read_data
        CMP R0,#+0
        BEQ.W up_poll_cancel
        B.N up_poll_fill_copied
up_poll_fill_log:
        MOV R0,R6
        MOV R1,SP
        MOVS R2,#+32
          CFI FunCall w25_read_data
        BL w25_read_data
        CMP R0,#+0
        BEQ.W up_poll_cancel
        MOV R0,SP
          CFI FunCall log_blank
        BL log_blank
        CMP R0,#+0
        BNE.W up_poll_cancel
        MOV R0,SP
          CFI FunCall log_valid
        BL log_valid
        CBZ R0,up_poll_fill_next
        LDRH R0,[R4,#+24]
        ADDS R1,R0,#+1
        STRH R1,[R4,#+24]
        LDRH R1,[R4,#+18]
        CMP R0,R1
        BCC.N up_poll_fill_next
        MOVS R0,#+0
        STR R0,[SP,#+24]
        STR R0,[SP,#+28]
        LDRH R0,[R4,#+22]
        LDR R1,[R4]
        ADD R0,R1,R0,LSL #+5
        ADDS R0,R0,#+12
        MOV R1,SP
        MOVS R2,#+32
          CFI FunCall __aeabi_memcpy
        BL __aeabi_memcpy
up_poll_fill_copied:
        LDRH R0,[R4,#+22]
        ADDS R0,R0,#+1
        STRH R0,[R4,#+22]
        LDRH R1,[R4,#+20]
        CMP R0,R1
        BCC.N up_poll_fill_next
        MOVW R0,#+65535
        STRH R0,[R4,#+26]
        MOVS R0,#+4
        STRH R0,[R4,#+28]
        B.N up_poll_phase
up_poll_fill_next:
        ADD R6,R6,#+32
        STR R6,[R4,#+4]
        B.N up_poll_return
up_poll_crc:
        LDRH R6,[R4,#+28]
        LDRH R0,[R4,#+30]
        SUBS R0,R0,#+2
        SUB R7,R0,R6
        CMP R7,#+64
        IT HI
        MOVHI R7,#+64
        LDR R0,[R4]
        ADDS R0,R0,R6
        MOV R1,R7
        LDRH R2,[R4,#+26]
          CFI FunCall crc16_calcInit
        BL crc16_calcInit
        STRH R0,[R4,#+26]
        ADDS R6,R6,R7
        STRH R6,[R4,#+28]
        LDRH R1,[R4,#+30]
        SUBS R1,R1,#+2
        CMP R6,R1
        BCC.N up_poll_return
        LDR R1,[R4]
        STRH R0,[R1,R6]
        MOVS R0,#+5
up_poll_phase:
        STRB R0,[R4,#+35]
up_poll_return:
        ADD SP,SP,#+44
          CFI CFA R13+20
        POP {R4-R7,PC}
          CFI CFA R13+64
up_poll_cancel:
          CFI FunCall upload_runtime_cancel
        BL upload_runtime_cancel
        B.N up_poll_return
up_poll_send:
        LDR R0,[R4]
        LDRH R1,[R4,#+30]
        LDRB R2,[R4,#+33]
          CFI FunCall reply_send
        BL reply_send
        CMP R0,#+0
        BEQ.W up_poll_return
        ; USB: a reply queued across a USB reset belongs to the old session.
        LDRB R0,[R4,#+33]
        CBNZ R0,up_poll_sent
        LDR.W R0,up_usb_epoch
        LDR R0,[R0]
        LDR R1,[R4,#+12]
        CMP R0,R1
        BNE.N up_poll_cancel
up_poll_sent:
        LDRH R0,[R4,#+20]
        CMP R0,#+0
        BEQ.W up_poll_cancel
        LDRH R0,[R4,#+18]
        LDRH R1,[R4,#+20]
        ADDS R0,R0,R1
        LDR.W R1,up_log_pending
        CMP R5,#+10
        IT NE
        LDRNE.W R1,up_temp_pending
        LDRB R2,[R4,#+33]
        STRH R0,[R1,R2,LSL #+1]
        ; Preserve the old ready-bit behavior: logs only for server;
        ; temporary IDs for both channels.
        CMP R5,#+10
        BNE.N up_poll_ready_temp
        CMP R2,#+0
        BEQ.N up_poll_cancel
        MOV R1,#+4096
        B.N up_poll_ready_set
up_poll_ready_temp:
        MOV R1,#+16384
up_poll_ready_set:
        LDR.W R2,up_device
        LDRH R0,[R2,#+129]
        ORRS R0,R0,R1
        STRH R0,[R2,#+129]
        B.N up_poll_cancel
up_poll_empty:
        ; Temporarily release the Flash/read gate, keeping our reserved packet.
        ; A durable cleanup may take ownership; an already-empty reply retains
        ; its own packet and retries a busy transport without another request.
        LDR.W R0,up_context
        MOVS R1,#+0
        STR R1,[R0]
        MOVS R0,#+FO_UPLOAD
          CFI FunCall flash_release
        BL flash_release
        MOV R0,R5
          CFI FunCall upload_finish
        BL upload_finish
        MOV R6,R0
        MOVS R0,#+FO_UPLOAD
          CFI FunCall flash_claim
        BL flash_claim
        LDR.W R0,up_context
        STR R4,[R0]
        CMP R6,#+0
        BEQ.W up_poll_cancel
        LDR R7,[R4]
        MOVW R0,#+19539
        MOVT R0,#+20036
        STR R0,[R7]
        ; Empty answer 9 and 10 alike (Protocol2: 2 bytes left + 1 byte count).
        MOVS R0,#+6
        STRH R0,[R7,#+4]
        ADDS R0,R0,#+8
        STRH R0,[R4,#+30]
        MOVS R0,#+255
        STRB R0,[R7,#+6]
        LDRB R0,[R4,#+34]
        STRB R0,[R7,#+7]
        STRB R5,[R7,#+8]
        MOVS R0,#+0
        STRB R0,[R7,#+9]
        STRB R0,[R7,#+10]
        STRB R0,[R7,#+11]
        STRH R0,[R4,#+20]
        MOVW R0,#+65535
        STRH R0,[R4,#+26]
        MOVS R0,#+4
        STRH R0,[R4,#+28]
        B.W up_poll_phase
          CFI EndBlock up_poll_cfi
        END
