; Ядра Cortex-M3 AAPCS без вложенных вызовов; байтовые входы допускают любое выравнивание.
        PRESERVE8
        CFI Names fast_names
        CFI StackFrame CFA R13 DATA
        CFI Resource R0:32, R1:32, R2:32, R3:32, R4:32, R5:32, R6:32, R7:32
        CFI Resource R8:32, R9:32, R10:32, R11:32, R12:32, R13:32, R14:32
        CFI EndNames fast_names
        CFI Common fast_leaf Using fast_names
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
        CFI EndCommon fast_leaf

        PUBLIC fast_fnv10
        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block fast_fnv10_block Using fast_leaf
        CFI Function fast_fnv10
        CFI NoCalls
fast_fnv10:
        MOV r1, r0
        LDR r0, =2166136261
        LDR r3, =16777619
        LDRB r2, [r1, #0]
        EORS r0, r0, r2
        MULS r0, r3, r0
        LDRB r2, [r1, #1]
        EORS r0, r0, r2
        MULS r0, r3, r0
        LDRB r2, [r1, #2]
        EORS r0, r0, r2
        MULS r0, r3, r0
        LDRB r2, [r1, #3]
        EORS r0, r0, r2
        MULS r0, r3, r0
        LDRB r2, [r1, #4]
        EORS r0, r0, r2
        MULS r0, r3, r0
        LDRB r2, [r1, #5]
        EORS r0, r0, r2
        MULS r0, r3, r0
        LDRB r2, [r1, #6]
        EORS r0, r0, r2
        MULS r0, r3, r0
        LDRB r2, [r1, #7]
        EORS r0, r0, r2
        MULS r0, r3, r0
        LDRB r2, [r1, #8]
        EORS r0, r0, r2
        MULS r0, r3, r0
        LDRB r2, [r1, #9]
        EORS r0, r0, r2
        MULS r0, r3, r0
        BX lr
        CFI EndBlock fast_fnv10_block
        LTORG

        PUBLIC fast_gf2_apply
        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block fast_gf2_apply_block Using fast_leaf
        CFI Function fast_gf2_apply
        CFI NoCalls
fast_gf2_apply:
        MOVS r2, #0
        CBZ r1, gf_done
gf_loop:
        LDRH r3, [r0], #2
        LSRS r1, r1, #1
        IT CS
        EORCS r2, r2, r3
        BNE gf_loop
gf_done:
        MOV r0, r2
        BX lr
        CFI EndBlock fast_gf2_apply_block
        LTORG

        PUBLIC fast_gf2_sparse
        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block fast_gf2_sparse_block Using fast_leaf
        CFI Function fast_gf2_sparse
        CFI NoCalls
fast_gf2_sparse:
        MOVS r3, #0
        CBZ r1, gs_done
gs_loop:
        RBIT r2, r1
        CLZ r2, r2
        LDRH r2, [r0, r2, LSL #1]
        EORS r3, r3, r2
        SUBS r2, r1, #1
        ANDS r1, r1, r2
        BNE gs_loop
gs_done:
        MOV r0, r3
        BX lr
        CFI EndBlock fast_gf2_sparse_block
        LTORG

        PUBLIC fast_gf2_unrolled
        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block fast_gf2_unrolled_block Using fast_leaf
        CFI Function fast_gf2_unrolled
        CFI NoCalls
fast_gf2_unrolled:
        MOVS r2, #0
        LSRS r1, r1, #1
        ITT CS
        LDRHCS r3, [r0, #0]
        EORCS r2, r2, r3
        LSRS r1, r1, #1
        ITT CS
        LDRHCS r3, [r0, #2]
        EORCS r2, r2, r3
        LSRS r1, r1, #1
        ITT CS
        LDRHCS r3, [r0, #4]
        EORCS r2, r2, r3
        LSRS r1, r1, #1
        ITT CS
        LDRHCS r3, [r0, #6]
        EORCS r2, r2, r3
        LSRS r1, r1, #1
        ITT CS
        LDRHCS r3, [r0, #8]
        EORCS r2, r2, r3
        LSRS r1, r1, #1
        ITT CS
        LDRHCS r3, [r0, #10]
        EORCS r2, r2, r3
        LSRS r1, r1, #1
        ITT CS
        LDRHCS r3, [r0, #12]
        EORCS r2, r2, r3
        LSRS r1, r1, #1
        ITT CS
        LDRHCS r3, [r0, #14]
        EORCS r2, r2, r3
        LSRS r1, r1, #1
        ITT CS
        LDRHCS r3, [r0, #16]
        EORCS r2, r2, r3
        LSRS r1, r1, #1
        ITT CS
        LDRHCS r3, [r0, #18]
        EORCS r2, r2, r3
        LSRS r1, r1, #1
        ITT CS
        LDRHCS r3, [r0, #20]
        EORCS r2, r2, r3
        LSRS r1, r1, #1
        ITT CS
        LDRHCS r3, [r0, #22]
        EORCS r2, r2, r3
        LSRS r1, r1, #1
        ITT CS
        LDRHCS r3, [r0, #24]
        EORCS r2, r2, r3
        LSRS r1, r1, #1
        ITT CS
        LDRHCS r3, [r0, #26]
        EORCS r2, r2, r3
        LSRS r1, r1, #1
        ITT CS
        LDRHCS r3, [r0, #28]
        EORCS r2, r2, r3
        LSRS r1, r1, #1
        ITT CS
        LDRHCS r3, [r0, #30]
        EORCS r2, r2, r3
        MOV r0, r2
        BX lr
        CFI EndBlock fast_gf2_unrolled_block
        LTORG

        PUBLIC fast_all_ff
        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block fast_all_ff_block Using fast_leaf
        CFI Function fast_all_ff
        CFI NoCalls
fast_all_ff:
        CBZ r1, ff_true
ff_prefix:
        LDRB r2, [r0], #1
        CMP r2, #255
        BNE ff_false
        SUBS r1, r1, #1
        BEQ ff_true
        TST r0, #3
        BNE ff_prefix
ff_words:
        CMP r1, #4
        BLO ff_tail
ff_word_loop:
        LDR r2, [r0], #4
        CMN r2, #1
        BNE ff_false
        SUBS r1, r1, #4
        CMP r1, #4
        BHS ff_word_loop
ff_tail:
        CBZ r1, ff_true
ff_bytes:
        LDRB r2, [r0], #1
        CMP r2, #255
        BNE ff_false
        SUBS r1, r1, #1
        BNE ff_bytes
ff_true:
        MOVS r0, #1
        BX lr
ff_false:
        MOVS r0, #0
        BX lr
        CFI EndBlock fast_all_ff_block
        LTORG

        PUBLIC fast_id_equal
        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block fast_id_equal_block Using fast_leaf
        CFI Function fast_id_equal
        CFI NoCalls
fast_id_equal:
        LDRB r2, [r0, #0]
        LDRB r3, [r1, #0]
        CMP r2, r3
        BNE id_false
        LDRB r2, [r0, #1]
        LDRB r3, [r1, #1]
        CMP r2, r3
        BNE id_false
        LDRB r2, [r0, #2]
        LDRB r3, [r1, #2]
        CMP r2, r3
        BNE id_false
        LDRB r2, [r0, #3]
        LDRB r3, [r1, #3]
        CMP r2, r3
        BNE id_false
        LDRB r2, [r0, #4]
        LDRB r3, [r1, #4]
        CMP r2, r3
        BNE id_false
        LDRB r2, [r0, #5]
        LDRB r3, [r1, #5]
        CMP r2, r3
        BNE id_false
        LDRB r2, [r0, #6]
        LDRB r3, [r1, #6]
        CMP r2, r3
        BNE id_false
        LDRB r2, [r0, #7]
        LDRB r3, [r1, #7]
        CMP r2, r3
        BNE id_false
        LDRB r2, [r0, #8]
        LDRB r3, [r1, #8]
        CMP r2, r3
        BNE id_false
        LDRB r2, [r0, #9]
        LDRB r3, [r1, #9]
        CMP r2, r3
        BNE id_false
        MOVS r0, #1
        BX lr
id_false:
        MOVS r0, #0
        BX lr
        CFI EndBlock fast_id_equal_block
        LTORG

        PUBLIC fast_sldn
        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block fast_sldn_block Using fast_leaf
        CFI Function fast_sldn
        CFI NoCalls
fast_sldn:
        MOV r12, r0
        CBZ r1, sig_done
sig_next:
        LDRB r2, [r0]
        CMP r2, #83
        BNE sig_skip
        CMP r1, #1
        BEQ sig_done
        LDRB r2, [r0, #1]
        CMP r2, #76
        BNE sig_skip
        CMP r1, #2
        BEQ sig_done
        LDRB r2, [r0, #2]
        CMP r2, #68
        BNE sig_skip
        CMP r1, #3
        BEQ sig_done
        LDRB r2, [r0, #3]
        CMP r2, #78
        BEQ sig_done
sig_skip:
        ADDS r0, r0, #1
        SUBS r1, r1, #1
        BNE sig_next
sig_done:
        SUB r0, r0, r12
        BX lr
        CFI EndBlock fast_sldn_block
        LTORG
        END
