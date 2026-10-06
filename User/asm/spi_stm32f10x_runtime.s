; Доработка: Кряжев А., 2026
; SPI1 внешней W25: старт и ограниченный обмен байтами/буферами.
; Приём посылает FF; после последнего байта проверяется BSY.
; Общие короткие подпрограммы сокращают повторяемый обмен, CFI описывает стек IAR.

        RTMODEL "__CPP_Runtime", "1"
        RTMODEL "__SystemLibrary", "DLib"
        RTMODEL "__dlib_file_descriptor", "0"
        RTMODEL "__dlib_full_locale_support", "0"
        RTMODEL "__dlib_version", "6"
        AAPCS BASE,INTERWORK
        PRESERVE8
        REQUIRE8

        #define SHT_PROGBITS 0x1

        EXTERN GPIO_Init
        EXTERN GPIO_StructInit
        EXTERN GPIO_WriteBit
        EXTERN RCC_APB2PeriphClockCmd
        EXTERN SPI_CalculateCRC
        EXTERN SPI_Cmd
        EXTERN SPI_I2S_DeInit
        EXTERN SPI_I2S_ReceiveData
        EXTERN SPI_Init
        EXTERN SPI_StructInit

        PUBLIC spi_init
        PUBLIC spi_initCS
        PUBLIC spi_read
        PUBLIC spi_read_byte
        PUBLIC spi_ready
        PUBLIC spi_send
        PUBLIC spi_send_byte

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

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock0 Using cfiCommon0
          CFI Function spi_ready
          CFI NoCalls
        THUMB
; Проверить включение SPI1 перед обменом.
spi_ready:
        LDR.N    R0,??DataTable6
        LDRH     R0,[R0, #+0]
        LSLS     R0,R0,#+24
        LSRS     R0,R0,#+30
        AND      R0,R0,#0x1
        BX       LR
          CFI EndBlock cfiBlock0

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock1 Using cfiCommon0
          CFI Function spi_wait_flag
        THUMB
; Ограниченно ждать TXE/RXNE/BSY; отказ выключает недостоверный SPI.
spi_wait_flag:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R4,R0
        MOV      R5,R1
          CFI FunCall spi_ready
        BL       spi_ready
        CBNZ.N   R0,??spi_wait_flag_0
        POP      {R1,R4,R5,PC}
??spi_wait_flag_0:
        MOV      R0,#+512
??spi_wait_flag_1:
        LDR.N    R1,??DataTable6_1
        LDRH     R2,[R1, #+0]
        ANDS     R1,R4,R2
        IT       NE
        MOVNE    R1,#+1
        MOVS     R3,R5
        IT       NE
        MOVNE    R3,#+1
        UXTB     R1,R1
        CMP      R1,R3
        BEQ.N    ??spi_wait_flag_2
        SUBS     R0,R0,#+1
        BNE.N    ??spi_wait_flag_1
        MOVS     R1,#+0
        LDR.N    R0,??DataTable6
          CFI FunCall SPI_Cmd
        BL       SPI_Cmd
        MOVS     R0,#+0
        POP      {R1,R4,R5,PC}
??spi_wait_flag_2:
        MOVS     R0,#+1
        POP      {R1,R4,R5,PC}
          CFI EndBlock cfiBlock1

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock2 Using cfiCommon0
          CFI Function spi_init
        THUMB
; Настроить SPI1 и выводы W25; частота платы 72/4 = 18 МГц.
spi_init:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        SUB      SP,SP,#+24
          CFI CFA R13+40
          CFI FunCall spi_initCS
        BL       spi_initCS
        MOVS     R1,#+1
        MOVW     R0,#+4101
          CFI FunCall RCC_APB2PeriphClockCmd
        BL       RCC_APB2PeriphClockCmd
        LDR.N    R4,??DataTable6
        MOV      R0,R4
          CFI FunCall SPI_I2S_DeInit
        BL       SPI_I2S_DeInit
        MOV      R0,SP
          CFI FunCall GPIO_StructInit
        BL       GPIO_StructInit
        MOVS     R0,#+160
        STRH     R0,[SP, #+0]
        MOVS     R0,#+24
        BL       spi_gpio_mode
??CrossCallReturnLabel_9:
        LDR.N    R5,??DataTable6_2
        MOV      R1,SP
        MOV      R0,R5
          CFI FunCall GPIO_Init
        BL       GPIO_Init
        MOVS     R0,#+64
        STRH     R0,[SP, #+0]
        MOVS     R0,#+72
        BL       spi_gpio_mode
??CrossCallReturnLabel_10:
        MOV      R1,SP
        MOV      R0,R5
          CFI FunCall GPIO_Init
        BL       GPIO_Init
        ADD      R0,SP,#+4
          CFI FunCall SPI_StructInit
        BL       SPI_StructInit
        MOV      R0,#+260
        STRH     R0,[SP, #+6]
; Плата всегда работает на 72 МГц: предделитель SPI равен 4.
        MOVS     R0,#+8
        STRH     R0,[SP, #+16]
        MOV      R0,#+512
        STRH     R0,[SP, #+14]
        ADD      R1,SP,#+4
        MOV      R0,R4
          CFI FunCall SPI_Init
        BL       SPI_Init
        MOVS     R1,#+0
        MOV      R0,R4
          CFI FunCall SPI_CalculateCRC
        BL       SPI_CalculateCRC
        MOVS     R1,#+1
        MOV      R0,R4
          CFI FunCall SPI_Cmd
        BL       SPI_Cmd
        BL       spi_wait_txe
??CrossCallReturnLabel_4:
        CBZ.N    R0,??spi_init_0
        MOV      R0,R4
          CFI FunCall SPI_I2S_ReceiveData
        BL       SPI_I2S_ReceiveData
??spi_init_0:
        ADD      SP,SP,#+28
          CFI CFA R13+12
        POP      {R4,R5,PC}
          CFI EndBlock cfiBlock2

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock3 Using cfiCommon0
          CFI Function spi_initCS
        THUMB
; Поднять CS до запуска порта, чтобы флеш не приняла ложную команду.
spi_initCS:
        PUSH     {R2-R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+16
        MOVS     R1,#+1
        MOVS     R0,#+9
          CFI FunCall RCC_APB2PeriphClockCmd
        BL       RCC_APB2PeriphClockCmd
        MOV      R0,SP
          CFI FunCall GPIO_StructInit
        BL       GPIO_StructInit
        MOV      R0,#+4096
        STRH     R0,[SP, #+0]
        MOVS     R0,#+16
        BL       spi_gpio_mode
??CrossCallReturnLabel_11:
        LDR.N    R4,??DataTable6_5
        MOV      R1,SP
        MOV      R0,R4
          CFI FunCall GPIO_Init
        BL       GPIO_Init
        MOVS     R2,#+1
        MOV      R1,#+4096
        MOV      R0,R4
          CFI FunCall GPIO_WriteBit
        BL       GPIO_WriteBit
        POP      {R0,R1,R4,PC}
          CFI EndBlock cfiBlock3

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond4 Using cfiCommon0
          CFI Function spi_init
          CFI Conditional ??CrossCallReturnLabel_9
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+40
          CFI Block cfiCond5 Using cfiCommon0
          CFI (cfiCond5) Function spi_init
          CFI (cfiCond5) Conditional ??CrossCallReturnLabel_10
          CFI (cfiCond5) R4 Frame(CFA, -12)
          CFI (cfiCond5) R5 Frame(CFA, -8)
          CFI (cfiCond5) R14 Frame(CFA, -4)
          CFI (cfiCond5) CFA R13+40
          CFI Block cfiCond6 Using cfiCommon0
          CFI (cfiCond6) Function spi_initCS
          CFI (cfiCond6) Conditional ??CrossCallReturnLabel_11
          CFI (cfiCond6) R4 Frame(CFA, -8)
          CFI (cfiCond6) R14 Frame(CFA, -4)
          CFI (cfiCond6) CFA R13+16
          CFI Block cfiPicker7 Using cfiCommon1
          CFI (cfiPicker7) NoFunction
          CFI (cfiPicker7) Picker
        THUMB
; Заполнить режим GPIO и скорость 50 МГц в структуре на стеке.
spi_gpio_mode:
        STRB     R0,[SP, #+3]
        MOVS     R0,#+3
        STRB     R0,[SP, #+2]
        BX       LR
          CFI EndBlock cfiCond4
          CFI EndBlock cfiCond5
          CFI EndBlock cfiCond6
          CFI EndBlock cfiPicker7

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock8 Using cfiCommon0
          CFI Function spi_read_byte
        THUMB
; Принять байт через общий обмен FF: хвостовой переход не добавляет стек или вызов.
spi_read_byte:
        MOVS R0,#255
          CFI FunCall spi_send_byte
        B.N spi_send_byte
          CFI EndBlock cfiBlock8

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock9 Using cfiCommon0
          CFI Function spi_read
          CFI NoCalls
        THUMB
; Принять буфер без ожидания BSY между байтами, проверив его один раз в конце.
spi_read:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        BL       spi_buffer_regs
??CrossCallReturnLabel_12:
        B.N      ??spi_read_0
??spi_read_1:
        MOVS     R0,#+255
        BL       spi_store_byte
??CrossCallReturnLabel_8:
        CBZ.N    R0,??spi_read_2
        LDRH     R0,[R6, #+0]
        STRB     R0,[R4], #+1
        ADDS     R7,R7,#+1
??spi_read_0:
        CMP      R7,R5
        BCS.N    ??spi_read_3
        BL       spi_wait_txe
??CrossCallReturnLabel_2:
        CMP      R0,#+0
        BNE.N    ??spi_read_1
??spi_read_2:
        POP      {R0,R4-R7,PC}
??spi_read_3:
        B.N      spi_finish_buffer
          CFI EndBlock cfiBlock9

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond10 Using cfiCommon0
          CFI Function spi_read
          CFI Conditional ??CrossCallReturnLabel_8
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
          CFI Block cfiCond11 Using cfiCommon0
          CFI (cfiCond11) Function spi_send
          CFI (cfiCond11) Conditional ??CrossCallReturnLabel_7
          CFI (cfiCond11) R4 Frame(CFA, -20)
          CFI (cfiCond11) R5 Frame(CFA, -16)
          CFI (cfiCond11) R6 Frame(CFA, -12)
          CFI (cfiCond11) R7 Frame(CFA, -8)
          CFI (cfiCond11) R14 Frame(CFA, -4)
          CFI (cfiCond11) CFA R13+24
          CFI Block cfiPicker12 Using cfiCommon1
          CFI (cfiPicker12) NoFunction
          CFI (cfiPicker12) Picker
        THUMB
; Записать байт в DR и перейти к общему ожиданию RXNE.
spi_store_byte:
        STRH     R0,[R6, #+0]
          CFI Block cfiCond14 Using cfiCommon0
          CFI (cfiCond14) Function spi_send_byte
          CFI (cfiCond14) Conditional ??CrossCallReturnLabel_5
          CFI (cfiCond14) R4 Frame(CFA, -12)
          CFI (cfiCond14) R5 Frame(CFA, -8)
          CFI (cfiCond14) R14 Frame(CFA, -4)
          CFI (cfiCond14) CFA R13+16
; Общий вход ожидания RXNE после записи DR.
spi_wait_rxne:
        MOVS     R1,#+1
        MOVS     R0,#+1
          CFI (cfiCond10) FunCall spi_read spi_wait_flag
          CFI (cfiCond11) FunCall spi_send spi_wait_flag
          CFI (cfiCond14) FunCall spi_send_byte spi_wait_flag
        B.N      spi_wait_flag
          CFI EndBlock cfiCond10
          CFI EndBlock cfiCond11
          CFI EndBlock cfiPicker12
          CFI EndBlock cfiCond14

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond15 Using cfiCommon0
          CFI Function spi_init
          CFI Conditional ??CrossCallReturnLabel_4
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+40
          CFI Block cfiCond17 Using cfiCommon0
          CFI (cfiCond17) Function spi_read
          CFI (cfiCond17) Conditional ??CrossCallReturnLabel_2
          CFI (cfiCond17) R4 Frame(CFA, -20)
          CFI (cfiCond17) R5 Frame(CFA, -16)
          CFI (cfiCond17) R6 Frame(CFA, -12)
          CFI (cfiCond17) R7 Frame(CFA, -8)
          CFI (cfiCond17) R14 Frame(CFA, -4)
          CFI (cfiCond17) CFA R13+24
          CFI Block cfiCond18 Using cfiCommon0
          CFI (cfiCond18) Function spi_send_byte
          CFI (cfiCond18) Conditional ??CrossCallReturnLabel_1
          CFI (cfiCond18) R4 Frame(CFA, -12)
          CFI (cfiCond18) R5 Frame(CFA, -8)
          CFI (cfiCond18) R14 Frame(CFA, -4)
          CFI (cfiCond18) CFA R13+16
          CFI Block cfiCond19 Using cfiCommon0
          CFI (cfiCond19) Function spi_send
          CFI (cfiCond19) Conditional ??CrossCallReturnLabel_0
          CFI (cfiCond19) R4 Frame(CFA, -20)
          CFI (cfiCond19) R5 Frame(CFA, -16)
          CFI (cfiCond19) R6 Frame(CFA, -12)
          CFI (cfiCond19) R7 Frame(CFA, -8)
          CFI (cfiCond19) R14 Frame(CFA, -4)
          CFI (cfiCond19) CFA R13+24
          CFI Block cfiPicker20 Using cfiCommon1
          CFI (cfiPicker20) NoFunction
          CFI (cfiPicker20) Picker
        THUMB
; Общий вход ожидания TXE: экономит повторяемую настройку аргументов.
spi_wait_txe:
        MOVS     R1,#+1
        MOVS     R0,#+2
          CFI (cfiCond15) FunCall spi_init spi_wait_flag
          CFI (cfiCond17) FunCall spi_read spi_wait_flag
          CFI (cfiCond18) FunCall spi_send_byte spi_wait_flag
          CFI (cfiCond19) FunCall spi_send spi_wait_flag
        B.N      spi_wait_flag
          CFI EndBlock cfiCond15
          CFI EndBlock cfiCond17
          CFI EndBlock cfiCond18
          CFI EndBlock cfiCond19
          CFI EndBlock cfiPicker20

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock21 Using cfiCommon0
          CFI Function spi_send_byte
        THUMB
; Обменять байт по SPI с ограниченным ожиданием всех флагов.
spi_send_byte:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R5,R0
        BL       spi_wait_txe
??CrossCallReturnLabel_1:
        CBZ.N    R0,??spi_send_byte_0
        LDR.N    R4,??DataTable6_6
        STRH     R5,[R4, #+0]
        BL       spi_wait_rxne
??CrossCallReturnLabel_5:
        CBZ.N    R0,??spi_send_byte_0
        MOVS     R1,#+0
        MOVS     R0,#+128
          CFI FunCall spi_wait_flag
        BL       spi_wait_flag
        CBNZ.N   R0,??spi_send_byte_1
??spi_send_byte_0:
        MOVS     R0,#+255
        POP      {R1,R4,R5,PC}
??spi_send_byte_1:
        LDRH     R0,[R4, #+0]
        UXTB     R0,R0
        POP      {R1,R4,R5,PC}
          CFI EndBlock cfiBlock21

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock22 Using cfiCommon0
          CFI Function spi_send
          CFI NoCalls
        THUMB
; Передать буфер, забирая ответные байты, чтобы не переполнить RX.
spi_send:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        BL       spi_buffer_regs
??CrossCallReturnLabel_13:
        B.N      ??spi_send_0
??spi_send_1:
        LDRB     R0,[R4], #+1
        BL       spi_store_byte
??CrossCallReturnLabel_7:
        CBZ.N    R0,??spi_send_2
        LDRH     R0,[R6, #+0]
        ADDS     R7,R7,#+1
??spi_send_0:
        CMP      R7,R5
        BCS.N    ??spi_send_3
        BL       spi_wait_txe
??CrossCallReturnLabel_0:
        CMP      R0,#+0
        BNE.N    ??spi_send_1
??spi_send_2:
        POP      {R0,R4-R7,PC}
??spi_send_3:
          CFI EndBlock cfiBlock22
        REQUIRE spi_finish_buffer

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock23 Using cfiCommon0
          CFI NoFunction
          CFI CFA R13+24
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
        THUMB
; Восстановить стек буферного обмена и хвостовым переходом дождаться BSY=0.
spi_finish_buffer:
        MOVS     R1,#+0
        MOVS     R0,#+128
        POP      {R2,R4-R7,LR}
          CFI CFA R13+0
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R7 SameValue
          CFI R14 SameValue
          CFI FunCall spi_read spi_wait_flag
          CFI FunCall spi_send spi_wait_flag
        B.N      spi_wait_flag
          CFI EndBlock cfiBlock23

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond24 Using cfiCommon0
          CFI Function spi_read
          CFI Conditional ??CrossCallReturnLabel_12
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
          CFI Block cfiCond25 Using cfiCommon0
          CFI (cfiCond25) Function spi_send
          CFI (cfiCond25) Conditional ??CrossCallReturnLabel_13
          CFI (cfiCond25) R4 Frame(CFA, -20)
          CFI (cfiCond25) R5 Frame(CFA, -16)
          CFI (cfiCond25) R6 Frame(CFA, -12)
          CFI (cfiCond25) R7 Frame(CFA, -8)
          CFI (cfiCond25) R14 Frame(CFA, -4)
          CFI (cfiCond25) CFA R13+24
          CFI Block cfiPicker26 Using cfiCommon1
          CFI (cfiPicker26) NoFunction
          CFI (cfiPicker26) Picker
        THUMB
; Удержать указатель, длину, счётчик и DR в регистрах внутреннего цикла.
spi_buffer_regs:
        MOV      R4,R0
        MOV      R5,R1
        MOVS     R7,#+0
        LDR.N    R6,??DataTable6_6
        BX       LR
          CFI EndBlock cfiCond24
          CFI EndBlock cfiCond25
          CFI EndBlock cfiPicker26

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable6:
        DATA32
        DC32     0x40013000

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable6_1:
        DATA32
        DC32     0x40013008

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable6_2:
        DATA32
        DC32     0x40010800

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable6_5:
        DATA32
        DC32     0x40010c00

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable6_6:
        DATA32
        DC32     0x4001300c

        END
