; Доработка: Кряжев А., 2026. Fixed-size ID hash, Cortex-M3 AAPCS leaf.
        PUBLIC  idindex_hash

        PRESERVE8
        CFI Names hash_names
        CFI StackFrame CFA R13 DATA
        CFI Resource R0:32, R1:32, R2:32, R3:32, R4:32, R5:32, R6:32, R7:32
        CFI Resource R8:32, R9:32, R10:32, R11:32, R12:32, R13:32, R14:32
        CFI EndNames hash_names
        CFI Common hash_leaf Using hash_names
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
        CFI EndCommon hash_leaf

        SECTION .text:CODE:NOROOT(2)
        THUMB

; uint32_t idindex_hash(uint16_t index, const uint8_t id[10]);
; FNV-1a over little-endian index then exactly 10 ID bytes. No overread.
        CFI Block hash_index Using hash_leaf
        CFI Function idindex_hash
        CFI NoCalls
idindex_hash:
        UXTH    r0, r0
        LDR     r3, =16777619
        LDR     r12, =2166136261
        UXTB    r2, r0
        LSRS    r0, r0, #8
        EOR     r2, r2, r12
        MULS    r2, r3, r2
        EORS    r0, r0, r2
        MULS    r0, r3, r0
        LDRB    r2, [r1, #0]
        EORS    r0, r0, r2
        MULS    r0, r3, r0
        LDRB    r2, [r1, #1]
        EORS    r0, r0, r2
        MULS    r0, r3, r0
        LDRB    r2, [r1, #2]
        EORS    r0, r0, r2
        MULS    r0, r3, r0
        LDRB    r2, [r1, #3]
        EORS    r0, r0, r2
        MULS    r0, r3, r0
        LDRB    r2, [r1, #4]
        EORS    r0, r0, r2
        MULS    r0, r3, r0
        LDRB    r2, [r1, #5]
        EORS    r0, r0, r2
        MULS    r0, r3, r0
        LDRB    r2, [r1, #6]
        EORS    r0, r0, r2
        MULS    r0, r3, r0
        LDRB    r2, [r1, #7]
        EORS    r0, r0, r2
        MULS    r0, r3, r0
        LDRB    r2, [r1, #8]
        EORS    r0, r0, r2
        MULS    r0, r3, r0
        LDRB    r2, [r1, #9]
        EORS    r0, r0, r2
        MULS    r0, r3, r0
        BX      lr
        CFI EndBlock hash_index
        LTORG
        END
