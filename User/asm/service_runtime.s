; Hand-written Cortex-M3 / IAR Thumb-2 service CRC continuations.
; One owner, one heap snapshot. No caller/DMA input pointer is retained.
; NOR/data CRC: <=4 x 64 bytes and one millisecond per poll.
; A pending TX retains the completed packet; failed reads never produce a CRC.
        AAPCS BASE,INTERWORK
        #include "flash_owner.h"
        EXTERN flash_claim
        EXTERN flash_release
        PRESERVE8
        REQUIRE8
        CFI Names sc_names
        CFI StackFrame CFA R13 DATA
        CFI Resource R0:32, R1:32, R2:32, R3:32, R4:32, R5:32, R6:32, R7:32
        CFI Resource R8:32, R9:32, R10:32, R11:32, R12:32, R13:32, R14:32
        CFI EndNames sc_names
        CFI Common sc_frame Using sc_names
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
        CFI EndCommon sc_frame

        EXTERN malloc
        EXTERN free
        EXTERN __aeabi_memclr4
        EXTERN __aeabi_memcpy
        EXTERN mem
        EXTERN config
        EXTERN device
        EXTERN id_used_bytes
        EXTERN cfg_cached_active
        EXTERN w25_read_available
        EXTERN w25_read_data
        EXTERN crc16_calcInit
        EXTERN crc16_calc
        EXTERN crc16_ffInit
        EXTERN timers_getMillis
        EXTERN comm_tx_busy
        EXTERN comm_tx_commit
        EXTERN comm_tx_release
        EXTERN user_comm_acquire
        EXTERN usb_try_send
        EXTERN user_send_answer_comand
        EXTERN user_storage_flash_ready
        EXTERN mem_config_result
        EXTERN mem_tables_ready
        PUBLIC service_crc_job
        PUBLIC service_crc_active
        PUBLIC service_crc_begin_pack
        PUBLIC service_crc_begin_id
        PUBLIC service_crc_begin_config
        PUBLIC service_crc_poll
        PUBLIC service_crc_cancel
        PUBLIC service_crc_quiet
        PUBLIC reply_send
        PUBLIC answer11

; Context: address0, real4, erased8, datetime12, crc16,
; kind18, phase19, channel20, uniq21, segment22, requested-index23,
; packet-length24, FF power26, wait-active27, ID-used snapshot28,
; scratch32[64], packet96[212], wait-start308. Allocation312.
        SECTION .bss:DATA:REORDER:NOROOT(2)
service_crc_job:
        DS8 4

        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block sc_active Using sc_frame
        CFI Function service_crc_active
        CFI NoCalls
service_crc_active:
        LDR R0, =service_crc_job
        LDR R0, [R0]
        BX LR
        CFI EndBlock sc_active

        CFI Block sc_pack Using sc_frame
        CFI Function service_crc_begin_pack
service_crc_begin_pack:
        MOV R2, R1
        MOV R1, R0
        MOVS R0, #5
        MOVS R3, #0
        CFI FunCall sc_begin
        B.W sc_begin
        CFI EndBlock sc_pack

        CFI Block sc_id Using sc_frame
        CFI Function service_crc_begin_id
service_crc_begin_id:
        MOVS R3, R0
        ITE EQ
        MOVEQ R0, #6
        MOVNE R0, #15
        CFI FunCall sc_begin
        B.W sc_begin
        CFI EndBlock sc_id

        CFI Block sc_config Using sc_frame
        CFI Function service_crc_begin_config
service_crc_begin_config:
        MOV R2, R1
        MOV R1, R0
        MOVS R0, #4
        MOVS R3, #0
        CFI FunCall sc_begin
        B.W sc_begin
        CFI EndBlock sc_config

        CFI Block sc_begin_block Using sc_frame
        CFI Function sc_begin
sc_begin:
        PUSH {R4-R8,LR}
        CFI R4 Frame(CFA,-24)
        CFI R5 Frame(CFA,-20)
        CFI R6 Frame(CFA,-16)
        CFI R7 Frame(CFA,-12)
        CFI R8 Frame(CFA,-8)
        CFI R14 Frame(CFA,-4)
        CFI CFA R13+24
        MOV R4,R0
        MOV R5,R1
        MOV R6,R2
        MOV R7,R3
        CMP R5,#1
        BHI sc_begin_no
        CFI FunCall service_crc_active
        BL service_crc_active
        CMP R0,#0
        BNE sc_begin_no
        CFI FunCall user_storage_flash_ready
        BL user_storage_flash_ready
        CMP R0,#0
        BEQ sc_begin_no
        CFI FunCall mem_config_result
        BL mem_config_result
        CMP R0,#1
        BEQ sc_begin_no
        CFI FunCall mem_tables_ready
        BL mem_tables_ready
        CMP R0,#0
        BEQ sc_begin_bad
        CMP R7,#100
        BHI sc_begin_bad
        MOVW R0,#312
        CFI FunCall malloc
        BL malloc
        MOVS R8,R0
        BEQ sc_begin_bad
        MOVW R1,#312
        CFI FunCall __aeabi_memclr4
        BL __aeabi_memclr4
        STRB R4,[R8,#18]
        MOVS R0,#1
        STRB R0,[R8,#19]
        STRB R5,[R8,#20]
        STRB R6,[R8,#21]
        STRB R7,[R8,#23]
        LDR R0,=id_used_bytes
        LDR R0,[R0]
        STR R0,[R8,#28]
        LDR R0,=config
        LDR R0,[R0]
        STR R0,[R8,#12]
        MOVS R1,#21
        CMP R4,#6
        IT EQ
        MOVEQ R1,#211
        CMP R4,#15
        IT EQ
        MOVEQ R1,#14
        STRH R1,[R8,#24]
        SUBS R1,R1,#8
        STRH R1,[R8,#100]
        LDR R0,=0x4E444C53
        STR R0,[R8,#96]
        MOVS R0,#255
        STRB R0,[R8,#102]
        STRB R6,[R8,#103]
        STRB R4,[R8,#104]
        CMP R4,#15
        IT EQ
        STRBEQ R7,[R8,#105]
        LDR R0,=service_crc_job
        STR R8,[R0]
        ; Cannot fail: user_storage_flash_ready saw no owner in this call.
        MOVS R0,#FO_CRC
        CFI FunCall flash_claim
        BL flash_claim
        MOVS R0,#1
        B sc_begin_done
sc_begin_bad:
        CMP R4,#4
        BEQ sc_begin_no         ; store.job remains US_REPLY; retry after OOM.
        MOVS R0,#18
        CMP R4,#5
        IT EQ
        MOVEQ R0,#17
        MOVS R1,#2
        MOV R2,R5
        MOV R3,R6
        CFI FunCall user_send_answer_comand
        BL user_send_answer_comand
sc_begin_no:
        MOVS R0,#0
sc_begin_done:
        POP {R4-R8,PC}
        CFI EndBlock sc_begin_block
        LTORG

        CFI Block sc_quiet_block Using sc_frame
        CFI Function service_crc_quiet
service_crc_quiet:
        LDR R1,=service_crc_job
        LDR R1,[R1]
        CBZ R1,sc_quiet_done
        LDRB R2,[R1,#20]
        CMP R2,R0
        CFI FunCall service_crc_cancel
        BEQ service_crc_cancel
sc_quiet_done:
        BX LR
        CFI EndBlock sc_quiet_block

        CFI Block sc_cancel_block Using sc_frame
        CFI Function service_crc_cancel
service_crc_cancel:
        PUSH {R4,LR}
        CFI R4 Frame(CFA,-8)
        CFI R14 Frame(CFA,-4)
        CFI CFA R13+8
        MOVS R0,#FO_CRC
        CFI FunCall flash_release
        BL flash_release
        POP {R4,LR}
        CFI R4 SameValue
        CFI R14 SameValue
        CFI CFA R13+0
        LDR R1,=service_crc_job
        LDR R0,[R1]
        MOVS R2,#0
        STR R2,[R1]
        CFI FunCall free
        B.W free
        CFI EndBlock sc_cancel_block

; Only sc_poll calls setup, after acquiring the read owner. No NOR I/O here.
        CFI Block sc_setup_block Using sc_frame
        CFI Function sc_setup
        CFI NoCalls
sc_setup:
        PUSH {R4-R7,LR}
        SUB SP,SP,#4
        CFI R4 Frame(CFA,-20)
        CFI R5 Frame(CFA,-16)
        CFI R6 Frame(CFA,-12)
        CFI R7 Frame(CFA,-8)
        CFI R14 Frame(CFA,-4)
        CFI CFA R13+24
        MOV R4,R0
        LDRB R5,[R4,#18]
        LDRB R6,[R4,#22]
        MOVS R7,#0              ; ID offset, or zero for another partition.
        CMP R5,#5
        BNE sc_setup_other
        ADR R0,sc_parts
        LDRB R0,[R0,R6]
        B sc_setup_part
sc_setup_other:
        CMP R5,#4
        IT EQ
        MOVEQ R0,#6
        BEQ sc_setup_part
        CMP R5,#15
        BNE sc_setup_list
        LDRB R6,[R4,#23]
        SUBS R6,R6,#1
sc_setup_list:
        MOVW R7,#16000
        MUL R7,R7,R6
        MOVS R0,#2
sc_setup_part:
        LDR R1,=mem
        ADD R1,R1,R0,LSL #3
        LDR R2,[R1]
        LDR R3,[R1,#4]
        CMP R0,#2
        BNE sc_setup_nonid
        CMP R5,#5
        IT NE
        MOVNE R3,#16000
        LDR R1,[R4,#28]
        SUBS R1,R1,R7
        IT CC
        MOVCC R1,#0
        CMP R1,R3
        IT HI
        MOVHI R1,R3
        SUBS R3,R3,R1
        STR R3,[R4,#8]
        MOV R3,R1
        ADD R2,R2,R7
        B sc_setup_store
sc_setup_nonid:
        MOVS R1,#0
        STR R1,[R4,#8]
        CMP R0,#6
        BNE sc_setup_store
        LDR R1,=0x1CD400
        SUB R2,R2,R1
        LDR R1,=cfg_cached_active
        LDR R1,[R1]
        ADD R2,R2,R1
sc_setup_store:
        STR R2,[R4]
        STR R3,[R4,#4]
        MOVS R0,#0
        STRB R0,[R4,#26]
        STRB R0,[R4,#27]
        MOVW R0,#65535
        STRH R0,[R4,#16]
        MOVS R0,#2
        STRB R0,[R4,#19]
        ADD SP,SP,#4
        POP {R4-R7,PC}
        CFI EndBlock sc_setup_block
        ALIGNROM 2
        DATA
sc_parts:
        DC8 2,3,4,6,5
        ALIGNROM 2
        LTORG

        CFI Block sc_poll_block Using sc_frame
        CFI Function service_crc_poll
        THUMB
service_crc_poll:
        PUSH {R3-R7,LR}
        CFI R4 Frame(CFA,-20)
        CFI R5 Frame(CFA,-16)
        CFI R6 Frame(CFA,-12)
        CFI R7 Frame(CFA,-8)
        CFI R14 Frame(CFA,-4)
        CFI CFA R13+24
        LDR R0,=service_crc_job
        LDR R4,[R0]
        CMP R4,#0
        BEQ sc_poll_done
        CFI FunCall timers_getMillis
        BL timers_getMillis
        MOV R6,R0
        MOVS R7,#4
        LDRB R0,[R4,#19]
        CMP R0,#1
        BNE sc_poll_read_phase
        MOV R0,R4
        CFI FunCall sc_setup
        BL sc_setup
        B sc_poll_done
sc_poll_read_phase:
        CMP R0,#2
        BNE sc_poll_packet
sc_poll_read_more:
        LDR R5,[R4,#4]
        CMP R5,#0
        BEQ sc_poll_ff
        CFI FunCall w25_read_available
        BL w25_read_available
        CMP R0,#0
        BEQ sc_poll_wait        ; bounded wait; pending is not a read failure.
        MOVS R0,#0
        STRB R0,[R4,#27]
        CMP R5,#64
        IT HI
        MOVHI R5,#64
        LDR R0,[R4]
        ADD R1,R4,#32
        MOV R2,R5
        CFI FunCall w25_read_data
        BL w25_read_data
        CMP R0,#0
        BEQ sc_poll_bad
        LDR R0,[R4]
        ADDS R0,R0,R5
        STR R0,[R4]
        LDR R0,[R4,#4]
        SUBS R0,R0,R5
        STR R0,[R4,#4]
        B sc_poll_data_crc
sc_poll_ff:
        LDR R0,[R4,#8]
        CBZ R0,sc_poll_segment_done
        LDRH R1,[R4,#16]
        CFI FunCall crc16_ffInit
        BL crc16_ffInit
        STRH R0,[R4,#16]
        MOVS R0,#0
        STR R0,[R4,#8]
        B sc_poll_segment_done
sc_poll_data_crc:
        ADD R0,R4,#32
        MOV R1,R5
        LDRH R2,[R4,#16]
        CFI FunCall crc16_calcInit
        BL crc16_calcInit
        STRH R0,[R4,#16]
        SUBS R7,R7,#1
        BEQ sc_poll_done
        CFI FunCall timers_getMillis
        BL timers_getMillis
        SUBS R0,R0,R6
        CMP R0,#1
        BLO sc_poll_read_more
        B sc_poll_done
sc_poll_segment_done:
        LDRB R0,[R4,#18]
        CMP R0,#4
        BEQ sc_poll_config_packet
        LDRB R5,[R4,#22]
        ADD R1,R4,#105
        ADD R1,R1,R5,LSL #1
        CMP R0,#15
        IT EQ
        ADDEQ R1,R4,#106
        LDRH R2,[R4,#16]
        STRB R2,[R1]
        LSRS R2,R2,#8
        STRB R2,[R1,#1]
        CMP R0,#15
        BEQ sc_poll_segments_done
        ADDS R5,R5,#1
        STRB R5,[R4,#22]
        MOVS R1,#5
        CMP R0,#6
        IT EQ
        MOVEQ R1,#100
        CMP R5,R1
        BEQ sc_poll_segments_done
        MOVS R0,#1
        STRB R0,[R4,#19]
        B sc_poll_done
sc_poll_segments_done:
        MOVS R0,#3
        STRB R0,[R4,#19]
        B sc_poll_done
sc_poll_config_packet:
        MOVS R0,#18
        STRH R0,[R4,#24]
        MOVS R0,#10
        STRH R0,[R4,#100]
        MOVS R0,#12
        STRB R0,[R4,#104]
        MOVS R0,#4
        STRB R0,[R4,#105]
        ADD R0,R4,#106
        ADD R1,R4,#12
        MOVS R2,#4
        CFI FunCall __aeabi_memcpy
        BL __aeabi_memcpy
        LDRH R0,[R4,#16]
        STRH R0,[R4,#110]
        B sc_poll_segments_done
sc_poll_packet:
        CMP R0,#3
        BNE sc_poll_send
        ADD R0,R4,#100
        LDRH R1,[R4,#24]
        SUBS R1,R1,#6
        CFI FunCall crc16_calc
        BL crc16_calc
        LDRH R1,[R4,#24]
        ADD R1,R4,R1
        STRB R0,[R1,#94]
        LSRS R0,R0,#8
        STRB R0,[R1,#95]
sc_poll_complete:
        MOVS R0,#4
        STRB R0,[R4,#19]
        B sc_poll_done
sc_poll_bad:
        MOVS R0,#19
        STRH R0,[R4,#24]
        LDRB R1,[R4,#18]
        MOVS R2,#18
        CMP R1,#5
        IT EQ
        MOVEQ R2,#17
        CMP R1,#4
        IT EQ
        MOVEQ R2,#12
        LDRB R1,[R4,#103]
        MOVS R3,#2
        ADD R0,R4,#96
        CFI FunCall answer11
        BL answer11
        B sc_poll_segments_done
sc_poll_wait:
        CFI FunCall timers_getMillis
        BL timers_getMillis
        LDRB R1,[R4,#27]
        CBNZ R1,sc_poll_wait_elapsed
        STR R0,[R4,#308]
        MOVS R0,#1
        STRB R0,[R4,#27]
        B sc_poll_done
sc_poll_wait_elapsed:
        LDR R1,[R4,#308]
        SUBS R0,R0,R1
        MOVW R1,#6000
        CMP R0,R1
        BCS sc_poll_bad
        B sc_poll_done
sc_poll_send:
        LDRB R0,[R4,#20]
        CMP R0,#1
        BNE sc_poll_tx_ready
        CFI FunCall comm_tx_busy
        BL comm_tx_busy
        CMP R0,#0
        BNE sc_poll_done
sc_poll_tx_ready:
        LDRH R1,[R4,#24]
        LDRB R2,[R4,#20]
        ADD R0,R4,#96
        CFI FunCall reply_send
        BL reply_send
        CMP R0,#0
        BEQ sc_poll_done
sc_poll_free:
        CFI FunCall service_crc_cancel
        BL service_crc_cancel
sc_poll_done:
        POP {R3-R7,PC}
        CFI EndBlock sc_poll_block
        LTORG
        SECTION .text:CODE:NOROOT(1)
        CFI Block rs_block Using sc_frame
        CFI Function reply_send
        THUMB
; R0 = packet, R1 = length, R2 = channel (0 USB, otherwise server). Returns 1 when
; the whole packet is queued, 0 when the transport is busy: the owner keeps the
; packet and calls again next pass. One copy for every owner that does so.
reply_send:
        CBNZ R2,rs_server
        CFI FunCall usb_try_send
        B.W usb_try_send
rs_server:
        PUSH {R4-R6,LR}
        CFI R4 Frame(CFA,-16)
        CFI R5 Frame(CFA,-12)
        CFI R6 Frame(CFA,-8)
        CFI R14 Frame(CFA,-4)
        CFI CFA R13+16
        MOV R4,R0
        MOV R5,R1
        MOV R0,R1
        CFI FunCall user_comm_acquire
        BL user_comm_acquire
        CBZ R0,rs_done
        MOV R1,R4
        MOV R2,R5
        CFI FunCall __aeabi_memcpy
        BL __aeabi_memcpy
        MOV R0,R5
        CFI FunCall comm_tx_commit
        BL comm_tx_commit
        CBNZ R0,rs_done
        ; Not committed: hand the frame back (after a commit it is the channel's).
        CFI FunCall comm_tx_release
        BL comm_tx_release
        MOVS R0,#0
rs_done:
        POP {R4-R6,PC}
        CFI EndBlock rs_block
        SECTION .text:CODE:NOROOT(2)
        CFI Block a11_block Using sc_frame
        CFI Function answer11
        THUMB
; Answer 11, the exact 19-byte wire: R0 = packet, R1 = uniq, R2 = answered command,
; R3 = code. SLDN, length 11, ttl 255, uniq, 11, command, code, device time and
; free buffer, CRC16 of bytes 4..16.
answer11:
        PUSH {R4,LR}
        CFI R4 Frame(CFA,-8)
        CFI R14 Frame(CFA,-4)
        CFI CFA R13+8
        MOV R4,R0
        LDR R0,=0x4e444c53
        STR R0,[R4]
        MOVS R0,#11
        STRH R0,[R4,#4]
        STRB R0,[R4,#8]
        MOVS R0,#255
        STRB R0,[R4,#6]
        STRB R1,[R4,#7]
        STRB R2,[R4,#9]
        STRB R3,[R4,#10]
        LDR R1,=device
        LDR R0,[R1,#47]
        STR R0,[R4,#11]
        LDRH R0,[R1,#165]
        STRH R0,[R4,#15]
        ADD R0,R4,#4
        MOVS R1,#13
        CFI FunCall crc16_calc
        BL crc16_calc
        STRH R0,[R4,#17]
        POP {R4,PC}
        CFI EndBlock a11_block
        LTORG
        END
