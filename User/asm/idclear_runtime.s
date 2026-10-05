; Hand-written Thumb-2 / Cortex-M3. Bulk MB_ID erase is deliberately not
; journal atomic. The old operation had the same partial-erase power outcome.
; Owns its replies before the first erase; no receiver or DMA pointers survive.
        PRESERVE8
        REQUIRE8
        AAPCS BASE,INTERWORK
        #include "flash_owner.h"
        EXTERN flash_claim
        EXTERN flash_release
        EXTERN malloc
        EXTERN free
        EXTERN device
        EXTERN mem
        EXTERN mem_storage_ready
        EXTERN id_cells_end
        EXTERN id_used_bytes
        EXTERN usb_bus_resets
        EXTERN usb_try_send
        EXTERN crc16_calc
        EXTERN answer11
        EXTERN user_storage_command_ready
        EXTERN mem_tables_ready
        EXTERN idindex_runtime_suspend
        EXTERN idindex_scanned
        EXTERN w25_async_start
        EXTERN w25_async_poll
        PUBLIC idclear_runtime_begin
        PUBLIC idclear_runtime_poll
        PUBLIC idclear_runtime_active
        PUBLIC idclear_runtime_cancel
        PUBLIC idclear_runtime_quiet
        PUBLIC idclear_job

        CFI Names ic_names
        CFI StackFrame CFA R13 DATA
        CFI Resource R0:32, R1:32, R2:32, R3:32, R4:32, R5:32, R6:32, R7:32
        CFI Resource R8:32, R9:32, R10:32, R11:32, R12:32, R13:32, R14:32
        CFI EndNames ic_names
        CFI Common ic_cfi Using ic_names
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
        CFI EndCommon ic_cfi

; ctx56: cursor0,end4,start8,epoch12,phase16,waiting17,cancel18,quiet19,
; touched20,reserved21,uniq22,reserved23,success24..34,error36..54.
        SECTION .bss:DATA:NOROOT(2)
idclear_job:
        DS8 4

        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block ic_active Using ic_cfi
        CFI Function idclear_runtime_active
        CFI NoCalls
idclear_runtime_active:
        LDR R0,=idclear_job
        LDR R0,[R0]
        CMP R0,#0
        IT NE
        MOVNE R0,#1
        BX LR
        CFI EndBlock ic_active

        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block ic_cancel Using ic_cfi
        CFI Function idclear_runtime_cancel
        CFI NoCalls
idclear_runtime_cancel:
        LDR R0,=idclear_job
        LDR R0,[R0]
        CBZ R0,ic_cancel_done
        MOVS R1,#1
        STRB R1,[R0,#18]
        STRB R1,[R0,#19]
ic_cancel_done:
        BX LR
        CFI EndBlock ic_cancel

        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block ic_quiet Using ic_cfi
        CFI Function idclear_runtime_quiet
        CFI NoCalls
; Only the USB session owns this reply. Server reconnect does not mute it.
idclear_runtime_quiet:
        CBNZ R0,ic_quiet_done
        LDR R0,=idclear_job
        LDR R0,[R0]
        CBZ R0,ic_quiet_done
        MOVS R1,#1
        STRB R1,[R0,#19]
ic_quiet_done:
        BX LR
        CFI EndBlock ic_quiet

        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block ic_release Using ic_cfi
        CFI Function idclear_release
idclear_release:
        LDR R1,=idclear_job
        LDR R0,[R1]
        MOVS R2,#0
        STR R2,[R1]
        CFI FunCall free
        B.W free
        CFI EndBlock ic_release

        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block ic_begin Using ic_cfi
        CFI Function idclear_runtime_begin
        REQUIRE idclear_runtime_poll
        REQUIRE idclear_runtime_active
        REQUIRE idclear_runtime_quiet
idclear_runtime_begin:
        PUSH {R4-R8,LR}
        CFI R4 Frame(CFA,-24)
        CFI R5 Frame(CFA,-20)
        CFI R6 Frame(CFA,-16)
        CFI R7 Frame(CFA,-12)
        CFI R8 Frame(CFA,-8)
        CFI R14 Frame(CFA,-4)
        CFI CFA R13+24
        CMP R0,#0
        BNE ic_begin_no
        UXTB R6,R1
        LDR R0,=idclear_job
        LDR R0,[R0]
        CMP R0,#0
        BNE ic_begin_no
        CFI FunCall user_storage_command_ready
        BL user_storage_command_ready
        CMP R0,#0
        BEQ ic_begin_no
        CFI FunCall mem_tables_ready
        BL mem_tables_ready
        CMP R0,#0
        BEQ ic_begin_no
        LDR R0,=mem
        LDR R4,[R0,#16]
        LDR R5,[R0,#20]
        ORR R0,R4,R5
        UBFX R0,R0,#0,#12
        CMP R0,#0
        BNE ic_begin_no
        LDR R0,=0x41000
        CMP R4,R0
        BLO ic_begin_no
        ADDS R5,R5,R4
        BCS ic_begin_no
        LDR R0,=0x1c8000
        CMP R5,R0
        BHI ic_begin_no
        MOVS R0,#56
        CFI FunCall malloc
        BL malloc
        CMP R0,#0
        BEQ ic_begin_no
        MOV R7,R0
        STR R4,[R7,#0]
        STR R5,[R7,#4]
        STR R4,[R7,#8]
        LDR R0,=usb_bus_resets
        LDR R0,[R0]
        STR R0,[R7,#12]
        MOVS R0,#1
        STR R0,[R7,#16]
        MOVS R0,#0
        STR R0,[R7,#20]
        STRB R6,[R7,#22]
        LDR R0,=0x4e444c53
        STR R0,[R7,#24]
        MOVS R0,#3
        STRH R0,[R7,#28]
        MOVS R0,#255
        STRB R0,[R7,#30]
        STRB R6,[R7,#31]
        MOVS R0,#103
        STRB R0,[R7,#32]
        ADD R0,R7,#28
        MOVS R1,#5
        CFI FunCall crc16_calc
        BL crc16_calc
        STRH R0,[R7,#33]
        CFI FunCall idindex_runtime_suspend
        BL idindex_runtime_suspend
        ; Cannot fail: user_storage_command_ready saw no owner in this call.
        MOVS R0,#FO_IDCLEAR
        CFI FunCall flash_claim
        BL flash_claim
        LDR R0,=idclear_job
        STR R7,[R0]
        MOVS R0,#1
        B ic_begin_done
ic_begin_no:
        MOVS R0,#0
ic_begin_done:
        POP {R4-R8,PC}
        CFI EndBlock ic_begin

        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block ic_error Using ic_cfi
        CFI Function idclear_error
; Called only with no pending async operation. Snapshot error bytes once.
idclear_error:
        PUSH {R4,LR}
        CFI R4 Frame(CFA,-8)
        CFI R14 Frame(CFA,-4)
        CFI CFA R13+8
        MOV R4,R0
        LDR R1,=mem_storage_ready
        MOVS R0,#0
        STRB R0,[R1]
        STRB R0,[R4,#17]
        MOVS R0,#3
        STRB R0,[R4,#16]
        ADD R0,R4,#36
        LDRB R1,[R4,#22]
        MOVS R2,#103
        MOVS R3,#2
        CFI FunCall answer11
        BL answer11
        POP {R4,PC}
        CFI EndBlock ic_error

        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block ic_poll Using ic_cfi
        CFI Function idclear_runtime_poll
idclear_runtime_poll:
        PUSH {R4-R6,LR}
        CFI R4 Frame(CFA,-16)
        CFI R5 Frame(CFA,-12)
        CFI R6 Frame(CFA,-8)
        CFI R14 Frame(CFA,-4)
        CFI CFA R13+16
        MOV R5,R0
        LDR R0,=idclear_job
        LDR R4,[R0]
        CMP R4,#0
        BEQ ic_done
        LDR R0,=usb_bus_resets
        LDR R0,[R0]
        LDR R1,[R4,#12]
        CMP R0,R1
        ITT NE
        MOVNE R0,#1
        STRBNE R0,[R4,#19]
        LDRB R0,[R4,#16]
        CMP R0,#1
        BNE ic_reply
        LDRB R0,[R4,#17]
        CMP R0,#0
        BEQ ic_no_pending
        MOV R0,R5
        CFI FunCall w25_async_poll
        BL w25_async_poll
        CMP R0,#1
        BEQ ic_done
        CMP R0,#2
        BNE ic_failed
        MOVS R0,#0
        STRB R0,[R4,#17]
        LDR R0,[R4]
        ADD R0,R0,#4096
        STR R0,[R4]
        B ic_done
ic_no_pending:
        LDR R0,[R4]
        LDR R1,[R4,#4]
        CMP R0,R1
        BEQ ic_complete
        LDRB R1,[R4,#18]
        CMP R1,#0
        BNE ic_cancel_partial
        MOVS R1,#0
        MOVS R2,#0
        MOV R3,R5
        CFI FunCall w25_async_start
        BL w25_async_start
        CMP R0,#0
        BEQ ic_failed
        MOVS R0,#1
        STRB R0,[R4,#17]
        STRB R0,[R4,#20]
        B ic_done
ic_failed:
        MOV R0,R4
        CFI FunCall idclear_error
        BL idclear_error
        B ic_done
ic_cancel_partial:
        LDRB R0,[R4,#20]
        CBZ R0,ic_release
        LDR R1,=mem_storage_ready
        MOVS R0,#0
        STRB R0,[R1]
        B ic_release
ic_complete:
        LDR R1,=device
        MOVS R0,#0
        STRH R0,[R1,#81]
        LDR R1,=id_cells_end
        STRH R0,[R1]
        LDR R1,=id_used_bytes
        STR R0,[R1]
        MOVS R1,#0
        CFI FunCall idindex_scanned
        BL idindex_scanned
        MOVS R0,#2
        STRB R0,[R4,#16]
        B ic_done
ic_reply:
        LDRB R1,[R4,#19]
        CBNZ R1,ic_release
        CMP R0,#2
        ITEE EQ
        ADDEQ R0,R4,#24
        ADDNE R0,R4,#36
        MOVNE R1,#19
        IT EQ
        MOVEQ R1,#11
        CFI FunCall usb_try_send
        BL usb_try_send
        CBZ R0,ic_done
ic_release:
        MOVS R0,#FO_IDCLEAR
        CFI FunCall flash_release
        BL flash_release
        CFI FunCall idclear_release
        BL idclear_release
ic_done:
        POP {R4-R6,PC}
        CFI EndBlock ic_poll
        END
