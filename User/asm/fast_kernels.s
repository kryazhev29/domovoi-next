; Доработка: Кряжев А., 2026
; Быстрые листовые ядра Cortex-M3: FNV, матрица CRC, проверка FF и поиск SLDN.
; Побайтовые входы допускают любое выравнивание, функции не используют стек.
; Нулевая длина не читает память; просмотр ограничен переданным буфером.
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
; Вычислить FNV-1a ровно по десяти байтам: r0 — буфер, на выходе — хеш.
; Развёрнутые шаги экономят ветви и счётчик короткого фиксированного ключа.
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
; Применить 16 выровненных столбцов uint16_t к вектору r1; r0 возвращает результат.
; RBIT/CLZ находят младший установленный бит, x&(x-1) удаляет его: нули пропускаются.
; Разреженные векторы матриц CRC FF быстрее полного обхода всех 16 столбцов.
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
        CFI EndBlock fast_gf2_apply_block
        LTORG

        PUBLIC fast_all_ff
        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block fast_all_ff_block Using fast_leaf
        CFI Function fast_all_ff
        CFI NoCalls
fast_all_ff:
; Проверить область r0 длиной r1 на FF; пустая область возвращает 1 без чтения.
; Первый отличный байт завершает поиск; выравнивание и хвост читаются по байту.
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

        PUBLIC fast_sldn
        SECTION .text:CODE:NOROOT(2)
        THUMB
        CFI Block fast_sldn_block Using fast_leaf
        CFI Function fast_sldn
        CFI NoCalls
fast_sldn:
; Найти SLDN в r0 длиной r1 и вернуть число байтов, которые можно отбросить.
; Полная сигнатура и совпавший хвост S/SL/SLD сохраняются для следующего приёма.
; Проверка длины перед каждым чтением защищает край; шаг в байт сохраняет перекрытия.
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
