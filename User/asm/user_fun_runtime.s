; Cortex-M3 / IAR. Executable production module.
; Reference: Tests/asm/reference/User/user_fun.c
        PUBLIC comm_credential
        PUBLIC comm_host
        PUBLIC comm_port
        PUBLIC comm_settings
        PUBLIC fw_job
        PUBLIC fw_rollback_end
        PUBLIC fw_rollback_pos
        PUBLIC fw_status_pending
        PUBLIC fw_write_errors
        PUBLIC id_cells_end
        PUBLIC id_endScan
        PUBLIC id_nextEnd
        PUBLIC id_time_end
        PUBLIC id_watch_end
        EXTERN expiry_runtime_active
        EXTERN expiry_runtime_poll
        EXTERN expiry_runtime_begin
        EXTERN expiry_runtime_cancel
        PUBLIC id_used_bytes
        PUBLIC log_job
        PUBLIC log_lastIndex
        PUBLIC log_pending
        PUBLIC log_pendingCount
        PUBLIC log_timeKnown
        PUBLIC log_unacked
        PUBLIC log_usbReadIndex
        PUBLIC reader_out
        PUBLIC server_out_event
        PUBLIC server_rx_buffer
        PUBLIC server_rx_busy
        PUBLIC server_rx_cancelled
        PUBLIC server_rx_commands
        PUBLIC server_rx_compacted
        PUBLIC server_rx_count
        PUBLIC server_rx_crc
        PUBLIC server_rx_defer
        PUBLIC server_rx_last
        PUBLIC server_rx_offset
        PUBLIC server_rx_pos
        PUBLIC server_rx_provisional
        PUBLIC server_rx_ready
        PUBLIC server_rx_total
        PUBLIC server_rx_work
        PUBLIC status_reported
        PUBLIC status_reported_ms
        PUBLIC store
        PUBLIC store_reset_requested
        PUBLIC store_runtime
        PUBLIC store_save_requested
        PUBLIC sync_pending
        PUBLIC sync_quiet
        PUBLIC table_fault
        PUBLIC table_job
        PUBLIC temp_unacked
        PUBLIC temp_usbReadIndex
        PUBLIC unreg_sms_pending

        RTMODEL "__CPP_Runtime", "1"
        RTMODEL "__SystemLibrary", "DLib"
        RTMODEL "__dlib_file_descriptor", "0"
        RTMODEL "__dlib_full_locale_support", "0"
        RTMODEL "__dlib_version", "6"
        RTMODEL "__iar_require _Printf", "flags,int_specials,widths"
        AAPCS BASE,INTERWORK
        PRESERVE8
        REQUIRE8

        #define SHT_PROGBITS 0x1

        EXTERN service_crc_active
        EXTERN service_crc_begin_pack
        EXTERN service_crc_begin_id
        EXTERN service_crc_begin_config
        EXTERN service_crc_poll
        EXTERN service_crc_cancel
        EXTERN service_crc_quiet
        EXTERN CRC32
        EXTERN WG_init
        EXTERN WG_process
        EXTERN __aeabi_memclr
        EXTERN __aeabi_memclr4
        EXTERN __aeabi_memcpy
        EXTERN __aeabi_memcpy4
        EXTERN __aeabi_memmove
        EXTERN __aeabi_memmove4
        EXTERN __aeabi_memset4
        EXTERN adc_getinjectedvalue_mV
        EXTERN adc_startInjectedConvertion
        EXTERN app_boot_error_reported
        EXTERN app_boot_error_text
        EXTERN app_boot_error_unreported
        EXTERN app_boot_metadata
        EXTERN app_boot_request
        EXTERN app_trial_active
        EXTERN app_trial_confirm
        EXTERN app_trial_expired
        EXTERN app_trial_rollback
        EXTERN atoi
        EXTERN boot_manual_apply
        EXTERN boot_manual_busy
        EXTERN boot_manual_command
        EXTERN chkdigitstr
        EXTERN comm_call_active
        EXTERN comm_channel_diag
        EXTERN comm_channel_ready
        EXTERN comm_channel_reopen
        EXTERN comm_info
        EXTERN comm_modem_diag
        EXTERN comm_modem_off
        EXTERN comm_modem_ready
        EXTERN comm_modem_restart
        EXTERN comm_modem_state
        EXTERN comm_modem_status
        EXTERN comm_recover_diag
        EXTERN comm_server_address
        EXTERN comm_server_diag
        EXTERN comm_server_online
        EXTERN comm_server_registered
        EXTERN comm_server_status_ack
        EXTERN comm_server_status_sent
        EXTERN comm_tx_acquire
        EXTERN comm_tx_busy
        EXTERN comm_tx_commit
        EXTERN comm_tx_release
        EXTERN comm_uart_stats
        EXTERN crc16_calc
        EXTERN crc16_calcInit
        EXTERN crc16_ffInit
        EXTERN fast_all_ff
        EXTERN fast_sldn
        EXTERN fault_record
        EXTERN free
        EXTERN idindex_find
        EXTERN idindex_hash
        EXTERN idindex_poll
        EXTERN idindex_runtime_active
        EXTERN idindex_runtime_suspend
        EXTERN idindex_scanned
        EXTERN led_fun100ms
        EXTERN led_load
        EXTERN loop_max_ms
        EXTERN loop_stalls
        EXTERN malloc
        EXTERN mem_config_begin
        EXTERN mem_config_errors
        EXTERN mem_config_poll
        EXTERN mem_config_reset
        EXTERN mem_config_result
        EXTERN mem_config_snapshot
        EXTERN mem_erase_sector
        EXTERN mem_patch
        EXTERN mem_patch2
        EXTERN mem_read
        EXTERN mem_recover
        EXTERN mem_tables_ready
        EXTERN mem_write
        EXTERN memchr
        EXTERN memcmp
        EXTERN out_getmodestate
        EXTERN out_getstate
        EXTERN out_load
        EXTERN out_process
        EXTERN photo_cleanup_active
        EXTERN photo_confirmed_offset
        EXTERN photo_connection_lost
        EXTERN photo_poll
        EXTERN photo_recover
        EXTERN photo_request_answer
        EXTERN photo_start
        EXTERN photo_storage_busy
        EXTERN photo_storage_cancel
        EXTERN photo_storage_end
        EXTERN photo_storage_poll
        EXTERN rs485_init
        EXTERN rs485_set_config
        EXTERN sj_runtime_begin
        EXTERN sj_runtime_blocked
        EXTERN sj_runtime_cancel
        EXTERN sj_runtime_poll
        EXTERN sj_runtime_reset
        EXTERN skipchar
        EXTERN sprintf
        EXTERN stack_free_bytes
        EXTERN strcat
        EXTERN strcmp
        EXTERN strcpy
        EXTERN strlen
        EXTERN strncat
        EXTERN strncmp
        EXTERN strncpy
        EXTERN strrchr
        EXTERN timers_get
        EXTERN timers_getMillis
        EXTERN timers_pause
        EXTERN timers_set
        EXTERN timers_take
        EXTERN uart_clear
        EXTERN uart_getstate
        EXTERN uart_isReceiving
        EXTERN uart_setRecTimeout
        EXTERN uart_timer1ms
        EXTERN usb_bus_resets
        EXTERN usb_diag_fill
        EXTERN usb_diag_fill_addr
        EXTERN usb_diag_fill_errors
        EXTERN usb_diag_fill_regs
        EXTERN usb_get_state
        EXTERN usb_init
        EXTERN usb_send
        EXTERN usb_sendstr
        EXTERN usb_timer1ms
        EXTERN user_comm_poll
        EXTERN user_comm_sms
        EXTERN user_comm_start
        EXTERN user_strcpy
        EXTERN w25_async_active
        EXTERN w25_async_poll
        EXTERN w25_async_start
        EXTERN w25_read_available
        EXTERN w25_read_data
        EXTERN w25_read_devID
        EXTERN wdt_reload

        PUBLIC RF433_readedID
        PUBLIC RFID_readedID
        PUBLIC WG_readedID
        PUBLIC _A_tempStrBuffer
        PUBLIC config
        PUBLIC device
        PUBLIC gsm_check_data
        PUBLIC gsm_user_debug
        PUBLIC log_addControlEvent
        PUBLIC log_addEvent
        PUBLIC log_addPhotoEvent
        PUBLIC log_addSystemEvent
        PUBLIC log_addSystemEventID
        PUBLIC log_addUpdateEvent
        PUBLIC log_checkPending
        PUBLIC log_clearLogBuffer
        PUBLIC log_flushPending
        PUBLIC log_getRecordsCount
        PUBLIC log_setTime
        PUBLIC mem
        PUBLIC port_buffer
        PUBLIC port_check_data
        PUBLIC server_rx_begin_pass
        PUBLIC server_rx_process
        PUBLIC table_runtime_blocks_reads
        PUBLIC timers_user_handle
        PUBLIC uniqIDCnt
        PUBLIC usb_data_reading
        PUBLIC user_activateOut
        PUBLIC user_addIdToTempBuffer
        PUBLIC user_comm_acquire
        PUBLIC user_calcCRC16InFlash
        PUBLIC user_checkFirmware
        PUBLIC user_checkIdAndControl
        PUBLIC user_checkIdTimeEnd
        PUBLIC user_checkShedule
        PUBLIC user_clearMemBlock
        PUBLIC user_clearTempIdBuffer
        PUBLIC user_comanderror
        PUBLIC user_comm_balance
        PUBLIC user_comm_boot
        PUBLIC user_comm_busy
        PUBLIC user_comm_link
        PUBLIC user_comm_local_time
        PUBLIC user_comm_low_signal
        PUBLIC user_comm_rx
        PUBLIC user_comm_rx_capacity
        PUBLIC user_comm_rx_done
        PUBLIC user_comm_server
        PUBLIC user_comm_session
        PUBLIC user_comm_ussd
        PUBLIC user_comm_volte
        PUBLIC user_dataconnection
        PUBLIC user_dayIsHoliday
        PUBLIC user_debug
        PUBLIC user_errorconnection
        PUBLIC user_factoryReset
        PUBLIC user_findID
        PUBLIC user_findTempID
        PUBLIC user_getFWblockFromServer
        PUBLIC user_getGroup
        PUBLIC user_getNumbersCount
        PUBLIC user_getPhotoCount
        PUBLIC user_getShedule
        PUBLIC user_getTempIDRecordsCount
        PUBLIC user_getweakday
        PUBLIC user_imeivalid
        PUBLIC user_incomingcall
        PUBLIC user_indication
        PUBLIC user_init
        PUBLIC user_initvariables
        PUBLIC user_last10digitsnumber
        PUBLIC user_loop
        PUBLIC user_parseUSSD
        PUBLIC user_parse_comand
        PUBLIC user_readconfig
        PUBLIC user_readinputs
        PUBLIC user_readsavedinformation
        PUBLIC user_saveindex
        PUBLIC user_savereset
        PUBLIC user_send_Photos
        PUBLIC user_send_answer_comand
        PUBLIC user_send_anyData
        PUBLIC user_send_configToConfigurator
        PUBLIC user_send_crcMemoryID
        PUBLIC user_send_crcPack
        PUBLIC user_send_identPackToConfigurator
        PUBLIC user_send_logRecords
        PUBLIC user_send_readedIDtoServer
        PUBLIC user_send_registrationData
        PUBLIC user_send_smsToServer
        PUBLIC user_send_statusPack
        PUBLIC user_send_statusPackSecond
        PUBLIC user_send_statusPackToConfigurator
        PUBLIC user_send_statusUpdateFW
        PUBLIC user_send_tempIDs
        PUBLIC user_send_timeshtamp
        PUBLIC user_sendsms
        PUBLIC user_setdefault
        PUBLIC user_smsreceived
        PUBLIC user_smssended
        PUBLIC user_storage_background_ready
        PUBLIC user_storage_command_ready
        PUBLIC user_storage_flash_ready
        PUBLIC user_storage_poll
        PUBLIC user_timeToInt
        PUBLIC user_unipassword
        PUBLIC user_updateID
        PUBLIC user_wait_stable_power
        PUBLIC user_writeConfigAndDevice
        PUBLIC user_writeconfig

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
          CFI Function NVIC_SystemReset
          CFI NoCalls
        THUMB
NVIC_SystemReset:
        DSB      SY
        LDR.W    R2,??DataTable32
        LDR      R1,[R2, #+0]
        AND      R1,R1,#0x700
        LDR.W    R0,??DataTable37
        ORRS     R1,R0,R1
        STR      R1,[R2, #+0]
        DSB      SY
??NVIC_SystemReset_0:
        B.N      ??NVIC_SystemReset_0
          CFI EndBlock cfiBlock0

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
mem:
        DS8 80

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
config:
        DS8 512

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
device:
        DS8 512

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
store_runtime:
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
store_save_requested:
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
store_reset_requested:
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
store:
        DS8 32

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock1 Using cfiCommon0
          CFI Function user_storage_command_ready
        THUMB
user_storage_command_ready:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI FunCall store_usb_session
        BL       store_usb_session
          CFI FunCall expiry_runtime_active
        BL expiry_runtime_active
        CMP R0,#0
        BNE.W ??user_storage_command_ready_0
          CFI FunCall idindex_runtime_active
        BL idindex_runtime_active
        CMP R0,#0
        BNE.W ??user_storage_command_ready_0
          CFI FunCall service_crc_active
        BL service_crc_active
        CMP R0,#0
        BNE.W ??user_storage_command_ready_0
          CFI FunCall photo_storage_busy
        BL       photo_storage_busy
        CBNZ.N   R0,??user_storage_command_ready_0
          CFI FunCall table_runtime_active
        BL       table_runtime_active
        CBNZ.N   R0,??user_storage_command_ready_0
          CFI FunCall fw_async_active
        BL       fw_async_active
        CBNZ.N   R0,??user_storage_command_ready_0
          CFI FunCall log_async_active
        BL       log_async_active
        CBNZ.N   R0,??user_storage_command_ready_0
        LDR.W    R0,??DataTable32_1
        LDRB     R1,[R0, #+18]
        LDR.W    R0,??DataTable38
        LDRB     R0,[R0, #+0]
        ORRS     R1,R0,R1
        BNE.N    ??user_storage_command_ready_0
          CFI FunCall mem_config_result
        BL       mem_config_result
        CBNZ.N   R0,??user_storage_command_ready_0
          CFI FunCall w25_async_active
        BL       w25_async_active
        CBNZ.N   R0,??user_storage_command_ready_0
          CFI FunCall w25_read_available
        BL       w25_read_available
        CBZ.N    R0,??user_storage_command_ready_1
        MOVS     R0,#+1
        POP      {R1,PC}
??user_storage_command_ready_0:
        MOVS     R0,#+0
??user_storage_command_ready_1:
        POP      {R1,PC}
          CFI EndBlock cfiBlock1

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock2 Using cfiCommon0
          CFI Function store_command_allowed
        THUMB
store_command_allowed:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
        CMP      R0,#+24
        IT       NE
        CMPNE    R0,#+27
        BEQ.N    ??store_command_allowed_0
          CFI FunCall user_storage_command_ready
        BL       user_storage_command_ready
        CBZ.N    R0,??store_command_allowed_1
??store_command_allowed_0:
        MOVS     R0,#+1
??store_command_allowed_1:
        POP      {R1,PC}
          CFI EndBlock cfiBlock2

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock3 Using cfiCommon0
          CFI Function user_storage_flash_ready
        THUMB
user_storage_flash_ready:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI FunCall expiry_runtime_active
        BL expiry_runtime_active
        CMP R0,#0
        BNE.W ??user_storage_flash_ready_0
          CFI FunCall idindex_runtime_active
        BL idindex_runtime_active
        CMP R0,#0
        BNE.W ??user_storage_flash_ready_0
          CFI FunCall service_crc_active
        BL service_crc_active
        CMP R0,#0
        BNE.W ??user_storage_flash_ready_0
          CFI FunCall photo_storage_busy
        BL       photo_storage_busy
        CBNZ.N   R0,??user_storage_flash_ready_0
          CFI FunCall table_runtime_active
        BL       table_runtime_active
        CBNZ.N   R0,??user_storage_flash_ready_0
          CFI FunCall fw_async_active
        BL       fw_async_active
        CBNZ.N   R0,??user_storage_flash_ready_0
          CFI FunCall log_async_active
        BL       log_async_active
        CBNZ.N   R0,??user_storage_flash_ready_0
        LDR.W    R0,??DataTable32_1
        LDRB     R0,[R0, #+19]
        CMP      R0,#+9
        BEQ.N    ??user_storage_flash_ready_0
          CFI FunCall w25_async_active
        BL       w25_async_active
        CBNZ.N   R0,??user_storage_flash_ready_0
          CFI FunCall w25_read_available
        BL       w25_read_available
        CBZ.N    R0,??user_storage_flash_ready_1
        MOVS     R0,#+1
        POP      {R1,PC}
??user_storage_flash_ready_0:
        MOVS     R0,#+0
??user_storage_flash_ready_1:
        POP      {R1,PC}
          CFI EndBlock cfiBlock3

        SECTION `.bss`:DATA:REORDER:NOROOT(1)
        DATA
log_usbReadIndex:
        DS8 2

        SECTION `.bss`:DATA:REORDER:NOROOT(1)
        DATA
temp_usbReadIndex:
        DS8 2

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
log_unacked:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
temp_unacked:
        DS8 4

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock4 Using cfiCommon0
          CFI Function upload_index
          CFI NoCalls
        THUMB
upload_index:
        CMP      R0,#+10
        BNE.N    ??upload_index_0
        CBNZ.N   R1,??upload_index_1
        LDR.W    R0,??DataTable32_2
        B.N      ??upload_index_2
??upload_index_1:
        LDR.W    R0,??DataTable32_3
        LDRH     R0,[R0, #+151]
        BX       LR
??upload_index_0:
        CBNZ.N   R1,??upload_index_3
        LDR.W    R0,??DataTable32_4
??upload_index_2:
        LDRH     R0,[R0, #+0]
        BX       LR
??upload_index_3:
        LDR.W    R0,??DataTable32_3
        LDRH     R0,[R0, #+153]
        BX       LR
          CFI EndBlock cfiBlock4

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock5 Using cfiCommon0
          CFI Function upload_setIndex
          CFI NoCalls
        THUMB
upload_setIndex:
        CMP      R0,#+10
        BNE.N    ??upload_setIndex_0
        CBNZ.N   R1,??upload_setIndex_1
        LDR.W    R0,??DataTable32_2
        B.N      ??upload_setIndex_2
??upload_setIndex_1:
        LDR.W    R0,??DataTable32_3
        STRH     R2,[R0, #+151]
        BX       LR
??upload_setIndex_0:
        CBNZ.N   R1,??upload_setIndex_3
        LDR.W    R0,??DataTable32_4
??upload_setIndex_2:
        STRH     R2,[R0, #+0]
        BX       LR
??upload_setIndex_3:
        LDR.W    R0,??DataTable32_3
        STRH     R2,[R0, #+153]
        BX       LR
          CFI EndBlock cfiBlock5

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock6 Using cfiCommon0
          CFI Function upload_store
          CFI NoCalls
        THUMB
upload_store:
        PUSH     {R3,R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+12
        LDR.W    R0,??DataTable32_5
        STR      R0,[SP, #+0]
        LDR.W    R0,??DataTable32_3
        ADD      R1,R0,#+181
        LDR      R3,[SP, #+0]
        STR      R3,[R1, #+0]
        LDR.W    R4,??DataTable32_2
        ADD      R1,R0,#+185
        BL       ?Subroutine13
??CrossCallReturnLabel_20:
        LDR.W    R1,??DataTable32_4
        ADD      R3,R0,#+187
        LDRB     R0,[R1, #+0]
        STRB     R0,[R3, #+0]
        LDRB     R0,[R1, #+1]
        STRB     R0,[R3, #+1]
        POP      {R0,R4,PC}
          CFI EndBlock cfiBlock6

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock7 Using cfiCommon0
          CFI Function upload_restore
        THUMB
upload_restore:
        PUSH     {R2-R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+16
        ADD      R4,R0,#+181
        LDR      R3,[R4, #+0]
        STR      R3,[SP, #+0]
        LDR.W    R1,??DataTable32_5
        CMP      R3,R1
        BNE.N    ??upload_restore_0
        LDR.W    R1,??DataTable32_3
        LDRH     R2,[R0, #+151]
        STRH     R2,[R1, #+151]
        LDRH     R2,[R0, #+153]
        STRH     R2,[R1, #+153]
        ADD      R4,R0,#+185
        LDR.W    R1,??DataTable32_2
        BL       ?Subroutine13
??CrossCallReturnLabel_21:
        ADD      R3,R0,#+187
        LDR.W    R0,??DataTable32_4
        BL       ?Subroutine14
??CrossCallReturnLabel_23:
          CFI FunCall upload_store
        BL       upload_store
??upload_restore_0:
        POP      {R0,R1,R4,PC}
          CFI EndBlock cfiBlock7

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond8 Using cfiCommon0
          CFI Function upload_store
          CFI Conditional ??CrossCallReturnLabel_20
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+12
          CFI Block cfiCond9 Using cfiCommon0
          CFI (cfiCond9) Function upload_restore
          CFI (cfiCond9) Conditional ??CrossCallReturnLabel_21
          CFI (cfiCond9) R4 Frame(CFA, -8)
          CFI (cfiCond9) R14 Frame(CFA, -4)
          CFI (cfiCond9) CFA R13+16
          CFI Block cfiCond10 Using cfiCommon0
          CFI (cfiCond10) Function log_valid
          CFI (cfiCond10) Conditional ??CrossCallReturnLabel_22
          CFI (cfiCond10) R4 Frame(CFA, -8)
          CFI (cfiCond10) R14 Frame(CFA, -4)
          CFI (cfiCond10) CFA R13+16
          CFI Block cfiPicker11 Using cfiCommon1
          CFI (cfiPicker11) NoFunction
          CFI (cfiPicker11) Picker
        THUMB
?Subroutine13:
        LDRB     R3,[R4, #+0]
        STRB     R3,[R1, #+0]
        LDRB     R3,[R4, #+1]
        STRB     R3,[R1, #+1]
        BX       LR
          CFI EndBlock cfiCond8
          CFI EndBlock cfiCond9
          CFI EndBlock cfiCond10
          CFI EndBlock cfiPicker11

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock12 Using cfiCommon0
          CFI Function upload_reset
        THUMB
upload_reset:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        MOV      R4,R0
        MOVS     R2,#+0
        MOVS     R1,#+0
          CFI FunCall upload_setIndex
        BL       upload_setIndex
        MOVS     R2,#+0
        BL       ?Subroutine21
??CrossCallReturnLabel_54:
        CMP      R4,#+10
        ITE      EQ
        LDREQ.W  R0,??DataTable35
        LDRNE.W  R0,??DataTable43
        MOVS     R1,#+0
        STRH     R1,[R0, #+2]
        STRH     R1,[R0, #+0]
        POP      {R4,LR}
          CFI R4 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall upload_store
        B.N      upload_store
          CFI EndBlock cfiBlock12

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock13 Using cfiCommon0
          CFI Function upload_ack
        THUMB
upload_ack:
        PUSH     {R4-R8,LR}
          CFI R14 Frame(CFA, -4)
          CFI R8 Frame(CFA, -8)
          CFI R7 Frame(CFA, -12)
          CFI R6 Frame(CFA, -16)
          CFI R5 Frame(CFA, -20)
          CFI R4 Frame(CFA, -24)
          CFI CFA R13+24
        MOV      R4,R0
        MOV      R5,R1
        BL       ?Subroutine25
??CrossCallReturnLabel_76:
        CBZ.N    R0,??upload_ack_0
        MOV      R0,R4
        POP      {R4-R8,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R7 SameValue
          CFI R8 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall store_ack
        B.W      store_ack
          CFI R4 Frame(CFA, -24)
          CFI R5 Frame(CFA, -20)
          CFI R6 Frame(CFA, -16)
          CFI R7 Frame(CFA, -12)
          CFI R8 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
??upload_ack_0:
        CMP      R4,#+10
        ITE      EQ
        LDREQ.W  R6,??DataTable35
        LDRNE.W  R6,??DataTable43
        MOVS     R7,R5
        IT       NE
        MOVNE    R7,#+1
        LDRH     R0,[R6, R7, LSL #+1]
        CBZ.N    R0,??upload_ack_1
        BL       ??Subroutine23_0
??CrossCallReturnLabel_73:
        MOV      R8,R0
        LDRH     R2,[R6, R7, LSL #+1]
        MOV      R1,R5
        BL       ??Subroutine21_0
??CrossCallReturnLabel_52:
          CFI FunCall upload_store
        BL       upload_store
          CFI FunCall user_writeConfigAndDevice
        BL       user_writeConfigAndDevice
        CBNZ.N   R0,??upload_ack_2
        MOV      R2,R8
        MOV      R1,R5
        BL       ??Subroutine21_0
??CrossCallReturnLabel_51:
          CFI FunCall upload_store
        BL       upload_store
        MOVS     R0,#+0
        B.N      ??upload_ack_3
??upload_ack_2:
        MOVS     R0,#+0
        STRH     R0,[R6, R7, LSL #+1]
??upload_ack_1:
        MOVS     R0,#+1
??upload_ack_3:
        POP      {R4-R8,PC}
          CFI EndBlock cfiBlock13

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock14 Using cfiCommon0
          CFI Function upload_finish
        THUMB
upload_finish:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        MOV      R4,R0
        BL       ?Subroutine25
??CrossCallReturnLabel_77:
        CBZ.N    R0,??upload_finish_0
        MOV      R0,R4
        POP      {R1,R4-R7,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R7 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall store_finish
        B.W      store_finish
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
??upload_finish_0:
        BL       ?Subroutine23
??CrossCallReturnLabel_75:
        MOV      R5,R0
        MOVS     R1,#+0
        BL       ??Subroutine23_0
??CrossCallReturnLabel_72:
        MOV      R6,R0
        CMP      R4,#+10
        LDR.W    R0,??DataTable32_3
        LDRH     R1,[R0, #+129]
        BNE.N    ??upload_finish_1
        UBFX     R1,R1,#+12,#+1
        CMP      R1,#+0
        ITT      EQ
        LDRHEQ   R0,[R0, #+89]
        CMPEQ    R0,#+0
        BNE.N    ??upload_finish_2
??upload_finish_3:
        MOVS     R7,#+0
        ORRS     R0,R6,R5
        BEQ.N    ??upload_finish_4
??upload_finish_5:
        MOV      R0,R4
          CFI FunCall upload_reset
        BL       upload_reset
          CFI FunCall user_writeConfigAndDevice
        BL       user_writeConfigAndDevice
        CBNZ.N   R0,??upload_finish_6
        MOV      R2,R5
        BL       ?Subroutine21
??CrossCallReturnLabel_53:
        MOV      R2,R6
        MOVS     R1,#+0
        BL       ??Subroutine21_0
??CrossCallReturnLabel_50:
          CFI FunCall upload_store
        BL       upload_store
        MOVS     R0,#+0
        POP      {R1,R4-R7,PC}
??upload_finish_1:
        UBFX     R1,R1,#+14,#+1
        CMP      R1,#+0
        ITT      EQ
        LDRHEQ   R0,[R0, #+83]
        CMPEQ    R0,#+0
        BEQ.N    ??upload_finish_3
??upload_finish_2:
        MOVS     R7,#+1
        B.N      ??upload_finish_5
??upload_finish_6:
        CBZ.N    R7,??upload_finish_4
        CMP      R4,#+10
        BNE.N    ??upload_finish_7
          CFI FunCall log_clearLogBuffer
        BL       log_clearLogBuffer
        B.N      ??upload_finish_8
??upload_finish_7:
          CFI FunCall user_clearTempIdBuffer
        BL       user_clearTempIdBuffer
??upload_finish_8:
        CBZ.N    R0,??upload_finish_9
??upload_finish_4:
        CMP      R4,#+10
        LDR.W    R2,??DataTable32_3
        LDRH     R1,[R2, #+129]
        ITE      EQ
        MOVWEQ   R0,#+61439
        MOVWNE   R0,#+49151
        ANDS     R1,R0,R1
        STRH     R1,[R2, #+129]
        MOVS     R0,#+1
??upload_finish_9:
        POP      {R1,R4-R7,PC}
          CFI EndBlock cfiBlock14

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
port_buffer:
        DS8 32

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
_A_tempStrBuffer:
        DS8 128

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
uniqIDCnt:
        DS8 1

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock15 Using cfiCommon0
          CFI Function gsm_user_debug
        THUMB
gsm_user_debug:
        LDR.W    R1,??DataTable32_3
        LDRB     R1,[R1, #+170]
        LSLS     R1,R1,#+31
          CFI FunCall usb_sendstr
        IT       MI
        BMI.W    usb_sendstr
        BX       LR
          CFI EndBlock cfiBlock15

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock16 Using cfiCommon0
          CFI Function user_debug
        THUMB
user_debug:
        LDR.W    R1,??DataTable32_3
        LDRB     R1,[R1, #+170]
        LSLS     R1,R1,#+29
          CFI FunCall usb_sendstr
        IT       MI
        BMI.W    usb_sendstr
        BX       LR
          CFI EndBlock cfiBlock16

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock17 Using cfiCommon0
          CFI Function timers_user_handle
        THUMB
timers_user_handle:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI FunCall usb_timer1ms
        BL       usb_timer1ms
        POP      {R0,LR}
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall uart_timer1ms
        B.W      uart_timer1ms
          CFI EndBlock cfiBlock17

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock18 Using cfiCommon0
          CFI Function usb_data_reading
        THUMB
usb_data_reading:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R5,R0
        MOV      R4,R1
        CMP      R4,#+11
        BLT.N    ??usb_data_reading_0
        MOVS     R2,#+4
        LDR.W    R1,??DataTable44_1
          CFI FunCall memcmp
        BL       memcmp
        CBNZ.N   R0,??usb_data_reading_0
        LDRB     R1,[R5, #+4]
        LDRB     R0,[R5, #+5]
        ORR      R1,R1,R0, LSL #+8
        CMP      R1,#+3
        BLT.N    ??usb_data_reading_0
        ADDS     R1,R1,#+8
        CMP      R1,R4
        BNE.N    ??usb_data_reading_0
        MOVS     R1,#+0
        ADDS     R0,R5,#+4
        POP      {R2,R4,R5,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall user_parse_comand
        B.W      user_parse_comand
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
??usb_data_reading_0:
        POP      {R0,R4,R5,PC}
          CFI EndBlock cfiBlock18

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock19 Using cfiCommon0
          CFI Function reader_id
        THUMB
reader_id:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        MOV      R4,R0
        MOV      R5,R3
        LDR.W    R0,??DataTable32_3
        LDRB     R6,[R0, #+114]
        CMP      R6,R1
        BNE.N    ??reader_id_0
        CMP      R1,#+3
        BNE.N    ??reader_id_1
        LDRB     R1,[R0, #+115]
        CMP      R1,R2
        BNE.N    ??reader_id_0
??reader_id_1:
        MOVS     R1,#+0
        STRB     R1,[R0, #+113]
        STRB     R1,[R0, #+114]
        MOVS     R2,#+0
        MOV      R1,R4
        MOVS     R0,#+78
          CFI FunCall log_addControlEvent
        BL       log_addControlEvent
        MOVS     R1,#+1
        MOV      R0,R4
          CFI FunCall user_send_readedIDtoServer
        BL       user_send_readedIDtoServer
??reader_id_0:
        MOV      R1,R5
        MOV      R0,R4
        POP      {R4-R6,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall user_checkIdAndControl
        B.W      user_checkIdAndControl
          CFI EndBlock cfiBlock19

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock20 Using cfiCommon0
          CFI Function WG_readedID
        THUMB
WG_readedID:
        PUSH     {R0-R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+24
        BL       ?Subroutine29
??CrossCallReturnLabel_93:
        LDR.W    R4,??DataTable43_1
        MOV      R2,SP
        LDR.W    R1,??DataTable44_2
        MOV      R0,R4
          CFI FunCall sprintf
        BL       sprintf
        MOV      R0,R4
          CFI FunCall user_debug
        BL       user_debug
        MOVS     R3,#+2
        MOVS     R2,#+0
        MOVS     R1,#+1
        B.N      ?Subroutine1
          CFI EndBlock cfiBlock20

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond21 Using cfiCommon0
          CFI Function WG_readedID
          CFI Conditional ??CrossCallReturnLabel_93
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
          CFI Block cfiCond22 Using cfiCommon0
          CFI (cfiCond22) Function RF433_readedID
          CFI (cfiCond22) Conditional ??CrossCallReturnLabel_92
          CFI (cfiCond22) R4 Frame(CFA, -8)
          CFI (cfiCond22) R14 Frame(CFA, -4)
          CFI (cfiCond22) CFA R13+24
          CFI Block cfiPicker23 Using cfiCommon1
          CFI (cfiPicker23) NoFunction
          CFI (cfiPicker23) Picker
        THUMB
?Subroutine29:
        MOV      R2,R0
          CFI Block cfiCond24 Using cfiCommon0
          CFI (cfiCond24) Function RFID_readedID
          CFI (cfiCond24) Conditional ??CrossCallReturnLabel_91
          CFI (cfiCond24) R4 Frame(CFA, -12)
          CFI (cfiCond24) R5 Frame(CFA, -8)
          CFI (cfiCond24) R14 Frame(CFA, -4)
          CFI (cfiCond24) CFA R13+24
??Subroutine29_0:
        LDR.W    R1,??DataTable44_3
        MOV      R0,SP
          CFI (cfiCond21) FunCall WG_readedID sprintf
          CFI (cfiCond22) FunCall RF433_readedID sprintf
          CFI (cfiCond24) FunCall RFID_readedID sprintf
        B.W      sprintf
          CFI EndBlock cfiCond21
          CFI EndBlock cfiCond22
          CFI EndBlock cfiPicker23
          CFI EndBlock cfiCond24

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock25 Using cfiCommon0
          CFI Function RF433_readedID
        THUMB
RF433_readedID:
        PUSH     {R0-R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+24
        BL       ?Subroutine29
??CrossCallReturnLabel_92:
        LDR.W    R4,??DataTable43_1
        MOV      R2,SP
        LDR.W    R1,??DataTable43_2
        MOV      R0,R4
          CFI FunCall sprintf
        BL       sprintf
        MOV      R0,R4
          CFI FunCall user_debug
        BL       user_debug
        MOVS     R3,#+3
        MOVS     R2,#+0
        MOVS     R1,#+2
          CFI EndBlock cfiBlock25
        REQUIRE ?Subroutine1

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock26 Using cfiCommon0
          CFI NoFunction
          CFI CFA R13+24
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
        THUMB
?Subroutine1:
        MOV      R0,SP
          CFI FunCall WG_readedID reader_id
          CFI FunCall RF433_readedID reader_id
        BL       reader_id
        POP      {R0-R4,PC}
          CFI EndBlock cfiBlock26

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock27 Using cfiCommon0
          CFI Function RFID_readedID
        THUMB
RFID_readedID:
        PUSH     {R1-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+24
        MOV      R2,R0
        MOV      R5,R1
        BL       ??Subroutine29_0
??CrossCallReturnLabel_91:
        LDR.W    R4,??DataTable43_1
        MOV      R2,SP
        LDR.W    R1,??DataTable44_4
        MOV      R0,R4
          CFI FunCall sprintf
        BL       sprintf
        MOV      R0,R4
          CFI FunCall user_debug
        BL       user_debug
        MOVS     R3,#+4
        MOV      R2,R5
        MOVS     R1,#+3
        MOV      R0,SP
          CFI FunCall reader_id
        BL       reader_id
        POP      {R0-R2,R4,R5,PC}
          CFI EndBlock cfiBlock27

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock28 Using cfiCommon0
          CFI Function user_imeivalid
        THUMB
user_imeivalid:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
        MOVS     R1,#+15
        LDR.W    R0,??DataTable32_3
          CFI FunCall chkdigitstr
        BL       chkdigitstr
        CBZ.N    R0,??user_imeivalid_0
        MOVS     R0,#+1
??user_imeivalid_0:
        POP      {R1,PC}
          CFI EndBlock cfiBlock28

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock29 Using cfiCommon0
          CFI Function user_unipassword
        THUMB
user_unipassword:
        PUSH     {R2-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+24
          CFI FunCall user_imeivalid
        BL       user_imeivalid
        CBNZ.N   R0,??user_unipassword_0
        MOV      R0,#+32768
        POP      {R1,R2,R4-R6,PC}
??user_unipassword_0:
        LDR.W    R5,??DataTable32_3
        ADD      R0,R5,#+11
          CFI FunCall atoi
        BL       atoi
        MOV      R6,R0
        MOVS     R0,#+0
??user_unipassword_1:
        ADDS     R1,R5,R0
        LDRB     R1,[R1, #+8]
        STRB     R1,[SP, R0]
        ADDS     R0,R0,#+1
        CMP      R0,#+3
        BLT.N    ??user_unipassword_1
        BL       ?Subroutine19
??CrossCallReturnLabel_44:
        MOV      R4,R0
        MOVS     R0,#+0
??user_unipassword_2:
        LDRB     R1,[R5, R0]
        STRB     R1,[SP, R0]
        ADDS     R0,R0,#+1
        CMP      R0,#+3
        BLT.N    ??user_unipassword_2
        BL       ?Subroutine19
??CrossCallReturnLabel_43:
        MOVS     R1,#+0
??user_unipassword_3:
        ADDS     R2,R5,R1
        LDRB     R2,[R2, #+3]
        STRB     R2,[SP, R1]
        ADDS     R1,R1,#+1
        CMP      R1,#+3
        BLT.N    ??user_unipassword_3
        MOVS     R2,#+0
        STRB     R2,[SP, R1]
        ADD      R0,R0,R6, LSL #+1
        MOVW     R1,#+65533
        MLA      R4,R1,R4,R0
        SUB      R4,R4,#+32768
        MOV      R0,SP
          CFI FunCall atoi
        BL       atoi
        SUBS     R4,R4,R0
        UXTH     R0,R4
        POP      {R1,R2,R4-R6,PC}
          CFI EndBlock cfiBlock29

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond30 Using cfiCommon0
          CFI Function user_unipassword
          CFI Conditional ??CrossCallReturnLabel_44
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
          CFI Block cfiCond31 Using cfiCommon0
          CFI (cfiCond31) Function user_unipassword
          CFI (cfiCond31) Conditional ??CrossCallReturnLabel_43
          CFI (cfiCond31) R4 Frame(CFA, -16)
          CFI (cfiCond31) R5 Frame(CFA, -12)
          CFI (cfiCond31) R6 Frame(CFA, -8)
          CFI (cfiCond31) R14 Frame(CFA, -4)
          CFI (cfiCond31) CFA R13+24
          CFI Block cfiPicker32 Using cfiCommon1
          CFI (cfiPicker32) NoFunction
          CFI (cfiPicker32) Picker
        THUMB
?Subroutine19:
        MOVS     R1,#+0
        STRB     R1,[SP, R0]
        MOV      R0,SP
          CFI (cfiCond30) FunCall user_unipassword atoi
          CFI (cfiCond30) FunCall user_unipassword atoi
        B.W      atoi
          CFI EndBlock cfiCond30
          CFI EndBlock cfiCond31
          CFI EndBlock cfiPicker32

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock33 Using cfiCommon0
          CFI Function user_wait_stable_power
          CFI NoCalls
        THUMB
user_wait_stable_power:
        BX       LR
          CFI EndBlock cfiBlock33

        SECTION `.text`:CODE:NOROOT(2)
          CFI Block cfiBlock34 Using cfiCommon0
          CFI Function user_timeToInt
          CFI NoCalls
        THUMB
user_timeToInt:
        PUSH     {R4-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+20
        MOV      R6,R2
        LDR      R4,[SP, #+20]
        LDR      R2,[SP, #+24]
        LDR.W    R7,??DataTable44_5
        MOV      R12,#+3600
        MOVS     R5,#+60
        MULS     R4,R5,R4
        MLA      R3,R12,R3,R4
        ADDS     R3,R2,R3
        SUBS     R6,R6,#+1
        MLA      R6,R7,R6,R3
        SUBS     R3,R1,#+1
        LDR.W    R2,??DataTable44_6
        MLA      R6,R2,R3,R6
        SUBS     R2,R1,#+3
        CMP      R2,#+9
        BHI.N    ??user_timeToInt_1
        TBB      [PC, R2]
        DATA
??user_timeToInt_0:
        DC8      0x5,0x5,0x8,0x8
        DC8      0xB,0xB,0xB,0xE
        DC8      0xE,0x11
        THUMB
??user_timeToInt_2:
        LDR.W    R2,??DataTable44_7
        B.N      ??user_timeToInt_3
??user_timeToInt_4:
        LDR.W    R2,??DataTable44_8
        B.N      ??user_timeToInt_3
??user_timeToInt_5:
        LDR.W    R2,??DataTable44_9
        B.N      ??user_timeToInt_3
??user_timeToInt_6:
        LDR.W    R2,??DataTable44_10
        B.N      ??user_timeToInt_3
??user_timeToInt_7:
        LDR.W    R2,??DataTable44_11
??user_timeToInt_3:
        ADDS     R6,R2,R6
??user_timeToInt_1:
        SUBW     R0,R0,#+1970
        ADDS     R3,R0,#+2
        LDR.W    R2,??DataTable44_12
        MLA      R6,R2,R0,R6
        ASRS     R2,R3,#+1
        ADD      R3,R3,R2, LSR #+30
        ASRS     R3,R3,#+2
        MLA      R6,R7,R3,R6
        ADDS     R0,R0,#+2
        ADD      R2,R0,R2, LSR #+30
        BIC      R2,R2,#0x3
        SUBS     R0,R0,R2
        BNE.N    ??user_timeToInt_8
        CMP      R1,#+3
        ITT      LT
        LDRLT.W  R0,??DataTable44_13
        ADDLT    R6,R0,R6
??user_timeToInt_8:
        MOV      R0,R6
        POP      {R4-R7,PC}
          CFI EndBlock cfiBlock34

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock35 Using cfiCommon0
          CFI Function user_errorconnection
        THUMB
user_errorconnection:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
        LDR.W    R0,??DataTable44_14
          CFI FunCall user_debug
        BL       user_debug
        LDR.W    R0,??DataTable32_3
        MOVS     R1,#+0
        STRB     R1,[R0, #+133]
        STRB     R1,[R0, #+137]
        MOVS     R0,#+13
        POP      {R1,LR}
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall log_addSystemEvent
        B.W      log_addSystemEvent
          CFI EndBlock cfiBlock35

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
server_rx_buffer:
        DS8 2'048

        SECTION `.bss`:DATA:REORDER:NOROOT(1)
        DATA
server_rx_count:
        DS8 2

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
server_rx_last:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
server_rx_defer:
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
server_rx_busy:
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
server_rx_provisional:
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(1)
        DATA
server_rx_ready:
        DS8 2

        SECTION `.bss`:DATA:REORDER:NOROOT(1)
        DATA
server_rx_work:
        DS8 2

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
server_rx_compacted:
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
server_rx_commands:
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
server_rx_cancelled:
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(1)
        DATA
server_rx_offset:
        DS8 2

        SECTION `.bss`:DATA:REORDER:NOROOT(1)
        DATA
server_rx_total:
        DS8 2

        SECTION `.bss`:DATA:REORDER:NOROOT(1)
        DATA
server_rx_pos:
        DS8 2

        SECTION `.bss`:DATA:REORDER:NOROOT(1)
        DATA
server_rx_crc:
        DS8 2

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock36 Using cfiCommon0
          CFI Function server_rx_reset
          CFI NoCalls
        THUMB
server_rx_reset:
        LDR.W    R0,??DataTable44_15
        LDRB     R0,[R0, #+0]
        CBZ.N    R0,??server_rx_reset_0
        MOVS     R1,#+1
        LDR.W    R0,??DataTable44_16
        STRB     R1,[R0, #+0]
        B.N      ??server_rx_reset_1
??server_rx_reset_0:
        MOVS     R1,#+0
        LDR.W    R0,??DataTable44_17
        STRH     R1,[R0, #+0]
??server_rx_reset_1:
        MOVS     R1,#+0
        LDR.W    R0,??DataTable44_18
        STRH     R1,[R0, #+0]
        LDR.W    R0,??DataTable44_19
        STRH     R1,[R0, #+0]
        LDR.W    R0,??DataTable44_20
        STRH     R1,[R0, #+0]
        LDR.W    R0,??DataTable44_21
        STRH     R1,[R0, #+0]
        LDR.W    R0,??DataTable44_22
        STRB     R1,[R0, #+0]
        BX       LR
          CFI EndBlock cfiBlock36

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock37 Using cfiCommon0
          CFI Function server_rx_begin_pass
        THUMB
server_rx_begin_pass:
        LDR.W    R0,??DataTable44_15
        LDRB     R0,[R0, #+0]
        CMP      R0,#+0
        BEQ.N    ??server_rx_begin_pass_0
        BX       LR
??server_rx_begin_pass_0:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
        MOV      R1,#+256
        LDR.W    R0,??DataTable44_23
        STRH     R1,[R0, #+0]
        MOVS     R1,#+0
        LDR.W    R0,??DataTable44_24
        STRB     R1,[R0, #+0]
        MOVS     R1,#+1
        LDR.W    R0,??DataTable44_25
        STRB     R1,[R0, #+0]
        LDR.W    R0,??DataTable44_17
        LDRH     R0,[R0, #+0]
        CBZ.N    R0,??server_rx_begin_pass_1
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        LDR.W    R1,??DataTable44_26
        LDR      R1,[R1, #+0]
        SUBS     R0,R0,R1
        MOVW     R1,#+30000
        CMP      R0,R1
        BCC.N    ??server_rx_begin_pass_1
        POP      {R0,LR}
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall server_rx_reset
        B.N      server_rx_reset
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
??server_rx_begin_pass_1:
        POP      {R0,PC}
          CFI EndBlock cfiBlock37

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock38 Using cfiCommon0
          CFI Function server_rx_consume
        THUMB
server_rx_consume:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        LDR.W    R5,??DataTable44_17
        LDRH     R4,[R5, #+0]
        SUBS     R4,R4,R0
        LDR.W    R3,??DataTable44_27
        UXTH     R2,R4
        ADDS     R1,R3,R0
        MOV      R0,R3
          CFI FunCall __aeabi_memmove
        BL       __aeabi_memmove
        STRH     R4,[R5, #+0]
        POP      {R0,R4,R5,PC}
          CFI EndBlock cfiBlock38

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock39 Using cfiCommon0
          CFI Function server_rx_scan
        THUMB
server_rx_scan:
        PUSH     {R3-R11,LR}
          CFI R14 Frame(CFA, -4)
          CFI R11 Frame(CFA, -8)
          CFI R10 Frame(CFA, -12)
          CFI R9 Frame(CFA, -16)
          CFI R8 Frame(CFA, -20)
          CFI R7 Frame(CFA, -24)
          CFI R6 Frame(CFA, -28)
          CFI R5 Frame(CFA, -32)
          CFI R4 Frame(CFA, -36)
          CFI CFA R13+40
        LDR.W    R9,??DataTable44_21
        LDRH     R0,[R9, #+0]
        CMP      R0,#+0
        ITTT     EQ
        LDREQ.W  R0,??DataTable44_15
        LDRBEQ   R0,[R0, #+0]
        CMPEQ    R0,#+0
        BNE.N    ??server_rx_scan_0
        LDR.W    R5,??DataTable44_23
        LDRH     R0,[R5, #+0]
        CMP      R0,#+0
        LDR.W    R6,??DataTable44_20
        LDR.W    R10,??DataTable44_19
        LDR.W    R11,??DataTable44_28
        BNE.N    ??server_rx_scan_1
??server_rx_scan_0:
        B.N      ??server_rx_scan_2
??server_rx_scan_3:
        LDRH     R0,[R6, #+0]
        ADDS     R0,R0,#+1
        STRH     R0,[R6, #+0]
??server_rx_scan_1:
        LDR.W    R0,??DataTable44_17
        LDRH     R2,[R6, #+0]
        LDRH     R1,[R0, #+0]
        CMP      R2,R1
        BCS.W    ??server_rx_scan_4
        LDRH     R1,[R5, #+0]
        CMP      R1,#+8
        BCC.N    ??server_rx_scan_5
        LDR.W    R2,??DataTable44_27
        LDRH     R1,[R6, #+0]
        ADD      R8,R2,R1
        LDRH     R7,[R0, #+0]
        SUBS     R7,R7,R1
        UXTH     R7,R7
        LDRH     R0,[R10, #+0]
        CMP      R0,#+0
        BNE.N    ??server_rx_scan_6
        CMP      R7,#+65
        ITE      CS
        MOVCS    R4,#+64
        MOVCC    R4,R7
        LDRH     R0,[R5, #+0]
        SUBS     R0,R0,#+8
        CMP      R0,R4
        BCS.N    ??server_rx_scan_7
        LDRH     R0,[R5, #+0]
        SUB      R4,R0,#+8
        UXTH     R4,R4
??server_rx_scan_7:
        CMP      R4,#+0
        BEQ.W    ??server_rx_scan_4
        MOV      R1,R4
        MOV      R0,R8
          CFI FunCall fast_sldn
        BL       fast_sldn
        UXTH     R0,R0
        ADDS     R2,R0,#+4
        CMP      R2,R4
        IT       HI
        MOVHI    R2,R4
        LDRH     R1,[R5, #+0]
        SUBS     R2,R1,R2
        STRH     R2,[R5, #+0]
        LDRH     R1,[R6, #+0]
        ADDS     R1,R0,R1
        STRH     R1,[R6, #+0]
        CMP      R0,R4
        BEQ.N    ??server_rx_scan_1
        SUBS     R1,R4,R0
        CMP      R1,#+4
        BCS.N    ??server_rx_scan_8
        CMP      R4,R7
        BLT.N    ??server_rx_scan_1
??server_rx_scan_8:
        ADD      R8,R8,R0
        SUBS     R7,R7,R0
        UXTH     R7,R7
        LDRH     R0,[R5, #+0]
        SUBS     R0,R0,#+8
        STRH     R0,[R5, #+0]
        CMP      R7,#+6
??server_rx_scan_5:
        BCC.N    ??server_rx_scan_4
        LDRB     R0,[R8, #+4]
        LDRB     R1,[R8, #+5]
        ORR      R0,R0,R1, LSL #+8
        SUBS     R2,R0,#+3
        MOVW     R1,#+2038
        CMP      R2,R1
        BCS.N    ??server_rx_scan_3
        ADD      R1,R0,#+8
        CMP      R7,R1
        BCC.N    ??server_rx_scan_4
        ADDS     R0,R0,#+8
        STRH     R0,[R10, #+0]
        MOVS     R1,#+4
        LDR.W    R0,??DataTable44_18
        STRH     R1,[R0, #+0]
        MOVW     R1,#+65535
        STRH     R1,[R11, #+0]
??server_rx_scan_6:
        LDR.W    R7,??DataTable44_18
        LDRH     R0,[R10, #+0]
        SUBS     R1,R0,#+2
        LDRH     R0,[R7, #+0]
        SUBS     R1,R1,R0
        UXTH     R1,R1
        LDRH     R4,[R5, #+0]
        CMP      R4,R1
        IT       GT
        MOVGT    R4,R1
        CBZ.N    R4,??server_rx_scan_9
        LDRH     R2,[R11, #+0]
        MOV      R1,R4
        LDRH     R0,[R7, #+0]
        ADD      R0,R8,R0
          CFI FunCall crc16_calcInit
        BL       crc16_calcInit
        STRH     R0,[R11, #+0]
        LDRH     R0,[R7, #+0]
        ADDS     R0,R4,R0
        STRH     R0,[R7, #+0]
        LDRH     R0,[R5, #+0]
        SUBS     R4,R0,R4
        STRH     R4,[R5, #+0]
??server_rx_scan_9:
        LDRH     R1,[R7, #+0]
        ADDS     R1,R1,#+2
        LDRH     R0,[R10, #+0]
        CMP      R1,R0
        BNE.N    ??server_rx_scan_4
        LDRH     R0,[R11, #+0]
        LDRH     R1,[R10, #+0]
        ADD      R1,R8,R1
        LDRB     R1,[R1, #-2]
        LDRH     R2,[R10, #+0]
        ADD      R2,R8,R2
        LDRB     R2,[R2, #-1]
        ORR      R1,R1,R2, LSL #+8
        CMP      R0,R1
        ITTT     NE
        MOVNE    R0,#+0
        STRHNE   R0,[R7, #+0]
        STRHNE   R0,[R10, #+0]
        BNE.W    ??server_rx_scan_3
        LDRH     R0,[R6, #+0]
        CMP      R0,#+0
        ITTT     NE
        LDRNE.W  R0,??DataTable44_24
        LDRBNE   R0,[R0, #+0]
        CMPNE    R0,#+0
        BNE.N    ??server_rx_scan_10
        LDRH     R0,[R10, #+0]
        STRH     R0,[R9, #+0]
        MOVS     R0,#+0
        STRH     R0,[R7, #+0]
        STRH     R0,[R10, #+0]
??server_rx_scan_4:
        LDRH     R0,[R6, #+0]
        CBZ.N    R0,??server_rx_scan_2
??server_rx_scan_10:
        LDR.W    R4,??DataTable44_24
        LDRB     R0,[R4, #+0]
        CBNZ.N   R0,??server_rx_scan_2
        LDRH     R0,[R6, #+0]
          CFI FunCall server_rx_consume
        BL       server_rx_consume
        MOVS     R0,#+0
        STRH     R0,[R6, #+0]
        MOVS     R0,#+1
        STRB     R0,[R4, #+0]
??server_rx_scan_2:
        POP      {R0,R4-R11,PC}
          CFI EndBlock cfiBlock39

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock40 Using cfiCommon0
          CFI Function user_dataconnection
        THUMB
user_dataconnection:
        PUSH     {R4-R8,LR}
          CFI R14 Frame(CFA, -4)
          CFI R8 Frame(CFA, -8)
          CFI R7 Frame(CFA, -12)
          CFI R6 Frame(CFA, -16)
          CFI R5 Frame(CFA, -20)
          CFI R4 Frame(CFA, -24)
          CFI CFA R13+24
        MOV      R7,R0
        MOV      R8,R1
        MOV      R5,R2
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        MOV      R6,R0
        LDR.N    R0,??DataTable32_3
        LDRSB    R0,[R0, #+127]
        CMP      R7,R0
        ITTT     EQ
        LDREQ.W  R0,??DataTable44_16
        LDRBEQ   R0,[R0, #+0]
        CMPEQ    R0,#+0
        BNE.N    ??user_dataconnection_0
        LDR.W    R0,??DataTable44_15
        LDRB     R0,[R0, #+0]
        CBNZ.N   R0,??user_dataconnection_1
        LDR.W    R0,??DataTable44_17
        LDRH     R0,[R0, #+0]
        CBZ.N    R0,??user_dataconnection_1
        LDR.W    R0,??DataTable44_26
        LDR      R1,[R0, #+0]
        SUBS     R1,R6,R1
        MOVW     R0,#+30000
        CMP      R1,R0
          CFI FunCall server_rx_reset
        IT       CS
        BLCS     server_rx_reset
??user_dataconnection_1:
        MOVS     R0,R5
        BEQ.N    ??user_dataconnection_0
        LDR.W    R0,??DataTable44_26
        STR      R6,[R0, #+0]
        LDR.W    R6,??DataTable44_17
        LDR.W    R4,??DataTable48
??user_dataconnection_2:
        LDRH     R0,[R6, #+0]
        CMP      R0,#+2048
        BNE.N    ??user_dataconnection_3
        LDRB     R7,[R4, #+0]
          CFI FunCall server_rx_process
        BL       server_rx_process
        STRB     R7,[R4, #+0]
??user_dataconnection_3:
        LDRH     R0,[R6, #+0]
        RSB      R0,R0,#+2048
        UXTH     R0,R0
        CBZ.N    R0,??user_dataconnection_0
        UXTH     R7,R5
        CMP      R7,R0
        IT       GT
        MOVGT    R7,R0
        MOV      R2,R7
        MOV      R1,R8
        LDR.W    R3,??DataTable44_27
        LDRH     R0,[R6, #+0]
        ADD      R0,R3,R0
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        LDRH     R0,[R6, #+0]
        ADDS     R0,R7,R0
        STRH     R0,[R6, #+0]
        ADD      R8,R8,R7
        SUBS     R5,R5,R7
          CFI FunCall server_rx_scan
        BL       server_rx_scan
        LDRB     R0,[R4, #+0]
        CBNZ.N   R0,??user_dataconnection_4
          CFI FunCall server_rx_process
        BL       server_rx_process
??user_dataconnection_4:
        UXTH     R0,R5
        CMP      R0,#+0
        BNE.N    ??user_dataconnection_2
??user_dataconnection_0:
        POP      {R4-R8,PC}
          CFI EndBlock cfiBlock40

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock41 Using cfiCommon0
          CFI Function server_rx_process
        THUMB
server_rx_process:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        LDR.W    R4,??DataTable44_15
        LDR.W    R0,??DataTable44_22
        LDRB     R1,[R0, #+0]
        LDRB     R0,[R4, #+0]
        ORRS     R1,R0,R1
        BNE.N    ??server_rx_process_0
        LDR.W    R0,??DataTable48
        STRB     R1,[R0, #+0]
          CFI FunCall server_rx_scan
        BL       server_rx_scan
        LDR.W    R5,??DataTable44_21
        LDRH     R0,[R5, #+0]
        CMP      R0,#+0
        ITTT     NE
        LDRNE.W  R6,??DataTable44_25
        LDRBNE   R0,[R6, #+0]
        CMPNE    R0,#+0
        BEQ.N    ??server_rx_process_0
          CFI FunCall comm_tx_busy
        BL       comm_tx_busy
        CBNZ.N   R0,??server_rx_process_0
        LDR.W    R7,??DataTable44_27
        LDRB     R0,[R7, #+8]
          CFI FunCall store_command_allowed
        BL       store_command_allowed
        CBZ.N    R0,??server_rx_process_0
        LDRB     R0,[R6, #+0]
        SUBS     R0,R0,#+1
        STRB     R0,[R6, #+0]
        LDRH     R6,[R5, #+0]
        MOVS     R0,#+0
        STRH     R0,[R5, #+0]
        MOVS     R0,#+1
        STRB     R0,[R4, #+0]
        MOVS     R1,#+1
        ADDS     R0,R7,#+4
          CFI FunCall user_parse_comand
        BL       user_parse_comand
        MOVS     R0,#+0
        STRB     R0,[R4, #+0]
        LDR.W    R1,??DataTable44_16
        LDRB     R0,[R1, #+0]
        CBZ.N    R0,??server_rx_process_1
        MOVS     R0,#+0
        STRB     R0,[R1, #+0]
        POP      {R0,R4-R7,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R7 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall server_rx_reset
        B.N      server_rx_reset
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
??server_rx_process_1:
        MOV      R0,R6
          CFI FunCall server_rx_consume
        BL       server_rx_consume
        POP      {R0,R4-R7,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R7 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall server_rx_scan
        B.N      server_rx_scan
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
??server_rx_process_0:
        POP      {R0,R4-R7,PC}
          CFI EndBlock cfiBlock41

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock42 Using cfiCommon0
          CFI Function server_rx_verified
          CFI NoCalls
        THUMB
server_rx_verified:
        LDR.W    R1,??DataTable44_15
        LDRB     R1,[R1, #+0]
        CBZ.N    R1,??server_rx_verified_0
        LDR.W    R1,??DataTable54
        CMP      R0,R1
        BNE.N    ??server_rx_verified_0
        MOVS     R0,#+1
        BX       LR
??server_rx_verified_0:
        MOVS     R0,#+0
        BX       LR
          CFI EndBlock cfiBlock42

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock43 Using cfiCommon0
          CFI Function user_comm_rx_capacity
          CFI NoCalls
        THUMB
user_comm_rx_capacity:
        LDR.W    R0,??DataTable44_16
        LDRB     R0,[R0, #+0]
        CMP      R0,#+0
        ITEEE    NE
        MOVNE    R0,#+0
        LDREQ.W  R0,??DataTable44_17
        LDRHEQ   R0,[R0, #+0]
        RSBEQ    R0,R0,#+2048
        UXTH     R0,R0
        BX       LR
          CFI EndBlock cfiBlock43

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock44 Using cfiCommon0
          CFI Function user_comm_rx
        THUMB
user_comm_rx:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        MOV      R4,R0
        MOV      R6,R1
          CFI FunCall user_comm_rx_capacity
        BL       user_comm_rx_capacity
        MOV      R5,R0
        CMP      R5,R6
        IT       HI
        MOVHI    R5,R6
        MOVS     R1,#+1
        LDR.W    R0,??DataTable44_22
        STRB     R1,[R0, #+0]
        LDR.W    R0,??DataTable48
        STRB     R1,[R0, #+0]
        MOV      R2,R5
        MOV      R1,R4
        LDR.N    R0,??DataTable32_3
        LDRSB    R0,[R0, #+127]
          CFI FunCall user_dataconnection
        BL       user_dataconnection
        MOV      R0,R5
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock44

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock45 Using cfiCommon0
          CFI Function user_comm_rx_done
        THUMB
user_comm_rx_done:
        MOVS     R2,#+0
        LDR.W    R0,??DataTable48_1
        STRB     R2,[R0, #+0]
        CMP      R1,#+0
          CFI FunCall server_rx_reset
        IT       NE
        BNE.W    server_rx_reset
        BX       LR
          CFI EndBlock cfiBlock45

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock46 Using cfiCommon0
          CFI Function user_comm_session
        THUMB
user_comm_session:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        MOV      R4,R0
          CFI FunCall server_rx_reset
        BL       server_rx_reset
        MOVS     R0,#+1
          CFI FunCall fw_async_quiet
        BL       fw_async_quiet
        MOVS     R0,#+1
          CFI FunCall table_runtime_quiet
        BL       table_runtime_quiet
        MOVS R0,#1
          CFI FunCall service_crc_quiet
        BL service_crc_quiet
        LDR.N    R0,??DataTable32_3
        MOVS     R1,#+0
        STRB     R1,[R0, #+127]
        STRB     R1,[R0, #+133]
        STRB     R1,[R0, #+137]
        LDR.W    R0,??DataTable43
        STRH     R1,[R0, #+2]
        LDR.N    R0,??DataTable35
        STRH     R1,[R0, #+2]
        LDR.N    R1,??DataTable32_1
        LDRB     R0,[R1, #+18]
        CBZ.N    R0,??user_comm_session_0
        LDRB     R0,[R1, #+22]
        CMP      R0,#+1
        IT       EQ
        STRBEQ   R0,[R1, #+24]
??user_comm_session_0:
        LDRB     R0,[R1, #+30]
        AND      R0,R0,#0x5
        STRB     R0,[R1, #+30]
        CBNZ.N   R4,??user_comm_session_1
        POP      {R4,LR}
          CFI R4 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall photo_connection_lost
        B.W      photo_connection_lost
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
??user_comm_session_1:
        POP      {R4,PC}
          CFI EndBlock cfiBlock46

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable32:
        DATA32
        DC32     0xe000ed0c

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable32_1:
        DATA32
        DC32     store

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable32_2:
        DATA32
        DC32     log_usbReadIndex

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable32_3:
        DATA32
        DC32     device

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable32_4:
        DATA32
        DC32     temp_usbReadIndex

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable32_5:
        DATA32
        DC32     0x314c5055

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock47 Using cfiCommon0
          CFI Function user_smsreceived
        THUMB
user_smsreceived:
        PUSH     {R4-R10,LR}
          CFI R14 Frame(CFA, -4)
          CFI R10 Frame(CFA, -8)
          CFI R9 Frame(CFA, -12)
          CFI R8 Frame(CFA, -16)
          CFI R7 Frame(CFA, -20)
          CFI R6 Frame(CFA, -24)
          CFI R5 Frame(CFA, -28)
          CFI R4 Frame(CFA, -32)
          CFI CFA R13+32
        SUB      SP,SP,#+40
          CFI CFA R13+72
        MOV      R8,R0
        MOV      R5,R1
        LDR.W    R9,??DataTable56
        LDRB     R0,[R9, #+99]
        CMP      R0,#+0
        ITT      NE
        LDRBNE   R0,[R5, #+0]
        CMPNE    R0,#+35
        BEQ.N    ??user_smsreceived_0
        MOV      R0,R5
          CFI FunCall user_send_smsToServer
        BL       user_send_smsToServer
        CMP      R0,#+0
        BEQ.W    ??user_smsreceived_1
??user_smsreceived_0:
        MOV      R0,R8
          CFI FunCall user_last10digitsnumber
        BL       user_last10digitsnumber
        MOV      R1,R0
        MOVS     R0,#+35
          CFI FunCall log_addSystemEventID
        BL       log_addSystemEventID
        LDR.W    R6,??DataTable43_1
        MOV      R3,R5
        MOVS     R2,#+111
        LDR.W    R1,??DataTable54_1
        MOV      R0,R6
          CFI FunCall sprintf
        BL       sprintf
        MOV      R0,R6
          CFI FunCall user_debug
        BL       user_debug
        MOVS     R2,#+1
        MOV      R1,R5
        MOVS     R0,#+10
          CFI FunCall skipchar
        BL       skipchar
        CMP      R0,R5
        ITT      NE
        MOVNE    R1,#+0
        STRBNE   R1,[R0, #-1]
        LDRB     R0,[R5, #+0]
        CMP      R0,#+35
        BNE.W    ??user_smsreceived_2
        MOVS     R4,#+0
        MOVS     R7,#+1
        MOVS     R2,#+1
        MOV      R1,R5
        MOVS     R0,#+32
          CFI FunCall skipchar
        BL       skipchar
        CMP      R0,R5
        BEQ.N    ??user_smsreceived_3
        MOVS     R3,#+4
        MOVS     R2,#+44
        MOV      R1,R0
        MOV      R0,R6
          CFI FunCall user_strcpy
        BL       user_strcpy
        LDR.W    R0,??DataTable54_2
        LDR      R1,[R0, #+4]
        LDR      R0,[R6, #+0]
        CMP      R1,R0
        BNE.N    ??user_smsreceived_4
        MOVS     R4,#+1
        B.N      ??user_smsreceived_4
??user_smsreceived_3:
        MOVS     R7,#+0
??user_smsreceived_4:
        MOVS     R2,#+1
        MOV      R1,R5
        MOVS     R0,#+44
          CFI FunCall skipchar
        BL       skipchar
        MOV      R10,R0
        CMP      R10,R5
        IT       EQ
        MOVEQ    R7,#+0
        MOVS     R3,#+10
        MOVS     R2,#+32
        ADDS     R1,R5,#+1
        MOV      R0,R6
          CFI FunCall user_strcpy
        BL       user_strcpy
        MOV      R1,R6
        ADD      R0,SP,#+28
          CFI FunCall strcpy
        BL       strcpy
        MOV      R1,R6
        B.N      ??user_smsreceived_5
??user_smsreceived_6:
        SUBS     R0,R0,#+97
        CMP      R0,#+26
        BCS.N    ??user_smsreceived_7
        LDRB     R0,[R1, #+0]
        SUBS     R0,R0,#+32
        STRB     R0,[R1, #+0]
??user_smsreceived_7:
        ADDS     R1,R1,#+1
??user_smsreceived_5:
        LDRB     R0,[R1, #+0]
        CMP      R0,#+0
        BNE.N    ??user_smsreceived_6
        CMP      R4,#+0
        BEQ.W    ??user_smsreceived_8
          CFI FunCall fw_update_quiet
        BL       fw_update_quiet
        CBZ.N    R0,??user_smsreceived_9
        ADR.N    R1,??DataTable40
        BL       ??Subroutine46_0
??CrossCallReturnLabel_166:
        CBZ.N    R0,??user_smsreceived_10
        BL       ?Subroutine46
??CrossCallReturnLabel_168:
        CBZ.N    R0,??user_smsreceived_10
        LDR.W    R1,??DataTable55
        BL       ??Subroutine46_0
??CrossCallReturnLabel_165:
        CBZ.N    R0,??user_smsreceived_10
        LDR.W    R1,??DataTable56_1
        BL       ??Subroutine46_0
??CrossCallReturnLabel_164:
        CBNZ.N   R0,??user_smsreceived_9
??user_smsreceived_10:
        LDR.W    R1,??DataTable55_1
        B.N      ??user_smsreceived_11
??user_smsreceived_9:
        ADR.N    R1,??DataTable40
        BL       ??Subroutine46_0
??CrossCallReturnLabel_163:
        CBNZ.N   R0,??user_smsreceived_12
        CMP      R7,#+0
        BEQ.N    ??user_smsreceived_13
        MOVS     R2,#+10
        MOV      R1,R10
        ADD      R0,R6,#+96
          CFI FunCall strncpy
        BL       strncpy
        MOVS     R0,#+0
        STRB     R0,[R6, #+106]
        MOVS     R1,#+7
        ADD      R0,R6,#+96
          CFI FunCall user_addIdToTempBuffer
        BL       user_addIdToTempBuffer
        MOVS     R2,#+0
        ADD      R1,R6,#+96
        MOVS     R0,#+67
          CFI FunCall log_addControlEvent
        BL       log_addControlEvent
        BL       ?Subroutine35
??CrossCallReturnLabel_117:
        ADD      R1,R6,#+96
          CFI FunCall strcat
        BL       strcat
        LDR.W    R1,??DataTable56_2
        B.N      ??user_smsreceived_11
??user_smsreceived_12:
        LDR.W    R1,??DataTable57
        BL       ??Subroutine46_0
??CrossCallReturnLabel_162:
        CBNZ.N   R0,??user_smsreceived_14
        BL       ?Subroutine35
??CrossCallReturnLabel_116:
        MOV      R1,R9
        B.N      ??user_smsreceived_15
??user_smsreceived_14:
        LDR.W    R1,??DataTable57_1
        BL       ??Subroutine46_0
??CrossCallReturnLabel_161:
        CBNZ.N   R0,??user_smsreceived_16
          CFI FunCall comm_server_address
        BL       comm_server_address
          CFI FunCall user_comm_server
        BL       user_comm_server
        LDRB     R1,[R9, #+79]
        CMP      R1,#+6
        ITE      CC
        LSLCC    R1,R1,#+2
        MOVCS    R1,#+0
        LDRB     R2,[R9, #+98]
        STR      R2,[SP, #+24]
        LDRB     R2,[R9, #+97]
        STR      R2,[SP, #+20]
        LDRB     R2,[R9, #+96]
        STR      R2,[SP, #+16]
        LDRB     R2,[R9, #+95]
        STR      R2,[SP, #+12]
        LDRH     R2,[R9, #+20]
        STR      R2,[SP, #+8]
        ADDS     R0,R0,#+1
        STR      R0,[SP, #+4]
        LDRB     R0,[R9, #+80]
        STR      R0,[SP, #+0]
        LDR.W    R0,??DataTable58
        LDR      R3,[R0, R1]
        LDRB     R2,[R9, #+74]
        LDR.W    R1,??DataTable58_1
        MOV      R0,R6
          CFI FunCall sprintf
        BL       sprintf
        B.N      ??user_smsreceived_17
??user_smsreceived_16:
        BL       ?Subroutine46
??CrossCallReturnLabel_167:
        CBNZ.N   R0,??user_smsreceived_18
        LDR.W    R1,??DataTable59_1
        BL       ??Subroutine35_0
??CrossCallReturnLabel_115:
        MOV      R1,R8
          CFI FunCall user_sendsms
        BL       user_sendsms
        MOVS     R0,#+5
        STRB     R0,[R9, #+111]
        B.N      ??user_smsreceived_2
??user_smsreceived_18:
        LDR.W    R1,??DataTable55
        BL       ??Subroutine46_0
??CrossCallReturnLabel_160:
        CMP      R0,#+0
        BNE.N    ??user_smsreceived_19
        CMP      R7,#+0
??user_smsreceived_13:
        BEQ.N    ??user_smsreceived_20
        MOV      R0,R10
          CFI FunCall atoi
        BL       atoi
        UXTB     R4,R0
        CMP      R4,#+1
        IT       NE
        CMPNE    R4,#+2
        BNE.N    ??user_smsreceived_21
        LDR.W    R0,??DataTable54_2
        MOVS     R1,#+11
        MULS     R1,R1,R4
        ADD      R0,R0,R1
        LDRB     R0,[R0, #+186]
        CBZ.N    R0,??user_smsreceived_22
        CMP      R4,#+1
        ITE      EQ
        MOVEQ    R0,#+0
        MOVNE    R0,#+1
        MOVS     R1,#+0
        STR      R1,[SP, #+0]
        MOVS     R3,#+0
        MOVS     R2,#+0
        MOVS     R1,#+1
          CFI FunCall out_load
        BL       out_load
        CMP      R4,#+1
        ITE      NE
        LDRNE.W  R1,??DataTable59_2
        LDREQ.W  R1,??DataTable59_3
        B.N      ??user_smsreceived_23
??user_smsreceived_22:
        LDR.W    R1,??DataTable59_4
        B.N      ??user_smsreceived_23
??user_smsreceived_21:
        LDR.W    R1,??DataTable59_5
??user_smsreceived_23:
        MOV      R0,R6
??user_smsreceived_15:
          CFI FunCall strcat
        BL       strcat
??user_smsreceived_17:
        MOV      R1,R8
        MOV      R0,R6
        B.N      ??user_smsreceived_24
??user_smsreceived_20:
        LDR.W    R1,??DataTable59_5
        B.N      ??user_smsreceived_11
??user_smsreceived_19:
        LDR.W    R1,??DataTable60
        BL       ??Subroutine46_0
??CrossCallReturnLabel_159:
        CBZ.N    R0,??user_smsreceived_2
        LDR.W    R1,??DataTable56_1
        BL       ??Subroutine46_0
??CrossCallReturnLabel_158:
        CBNZ.N   R0,??user_smsreceived_2
          CFI FunCall user_factoryReset
        BL       user_factoryReset
        LDR.W    R1,??DataTable60_1
        B.N      ??user_smsreceived_11
??user_smsreceived_8:
        LDR.W    R1,??DataTable61
??user_smsreceived_11:
        BL       ??Subroutine35_0
??CrossCallReturnLabel_114:
        MOV      R1,R8
??user_smsreceived_24:
          CFI FunCall user_sendsms
        BL       user_sendsms
??user_smsreceived_2:
        MOVS     R0,#+1
??user_smsreceived_1:
        ADD      SP,SP,#+40
          CFI CFA R13+32
        POP      {R4-R10,PC}
          CFI EndBlock cfiBlock47

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond48 Using cfiCommon0
          CFI Function user_smsreceived
          CFI Conditional ??CrossCallReturnLabel_168
          CFI R4 Frame(CFA, -32)
          CFI R5 Frame(CFA, -28)
          CFI R6 Frame(CFA, -24)
          CFI R7 Frame(CFA, -20)
          CFI R8 Frame(CFA, -16)
          CFI R9 Frame(CFA, -12)
          CFI R10 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+72
          CFI Block cfiCond49 Using cfiCommon0
          CFI (cfiCond49) Function user_smsreceived
          CFI (cfiCond49) Conditional ??CrossCallReturnLabel_167
          CFI (cfiCond49) R4 Frame(CFA, -32)
          CFI (cfiCond49) R5 Frame(CFA, -28)
          CFI (cfiCond49) R6 Frame(CFA, -24)
          CFI (cfiCond49) R7 Frame(CFA, -20)
          CFI (cfiCond49) R8 Frame(CFA, -16)
          CFI (cfiCond49) R9 Frame(CFA, -12)
          CFI (cfiCond49) R10 Frame(CFA, -8)
          CFI (cfiCond49) R14 Frame(CFA, -4)
          CFI (cfiCond49) CFA R13+72
          CFI Block cfiPicker50 Using cfiCommon1
          CFI (cfiPicker50) NoFunction
          CFI (cfiPicker50) Picker
        THUMB
?Subroutine46:
        LDR.W    R1,??DataTable61_1
          CFI Block cfiCond51 Using cfiCommon0
          CFI (cfiCond51) Function user_smsreceived
          CFI (cfiCond51) Conditional ??CrossCallReturnLabel_166
          CFI (cfiCond51) R4 Frame(CFA, -32)
          CFI (cfiCond51) R5 Frame(CFA, -28)
          CFI (cfiCond51) R6 Frame(CFA, -24)
          CFI (cfiCond51) R7 Frame(CFA, -20)
          CFI (cfiCond51) R8 Frame(CFA, -16)
          CFI (cfiCond51) R9 Frame(CFA, -12)
          CFI (cfiCond51) R10 Frame(CFA, -8)
          CFI (cfiCond51) R14 Frame(CFA, -4)
          CFI (cfiCond51) CFA R13+72
          CFI Block cfiCond52 Using cfiCommon0
          CFI (cfiCond52) Function user_smsreceived
          CFI (cfiCond52) Conditional ??CrossCallReturnLabel_165
          CFI (cfiCond52) R4 Frame(CFA, -32)
          CFI (cfiCond52) R5 Frame(CFA, -28)
          CFI (cfiCond52) R6 Frame(CFA, -24)
          CFI (cfiCond52) R7 Frame(CFA, -20)
          CFI (cfiCond52) R8 Frame(CFA, -16)
          CFI (cfiCond52) R9 Frame(CFA, -12)
          CFI (cfiCond52) R10 Frame(CFA, -8)
          CFI (cfiCond52) R14 Frame(CFA, -4)
          CFI (cfiCond52) CFA R13+72
          CFI Block cfiCond53 Using cfiCommon0
          CFI (cfiCond53) Function user_smsreceived
          CFI (cfiCond53) Conditional ??CrossCallReturnLabel_164
          CFI (cfiCond53) R4 Frame(CFA, -32)
          CFI (cfiCond53) R5 Frame(CFA, -28)
          CFI (cfiCond53) R6 Frame(CFA, -24)
          CFI (cfiCond53) R7 Frame(CFA, -20)
          CFI (cfiCond53) R8 Frame(CFA, -16)
          CFI (cfiCond53) R9 Frame(CFA, -12)
          CFI (cfiCond53) R10 Frame(CFA, -8)
          CFI (cfiCond53) R14 Frame(CFA, -4)
          CFI (cfiCond53) CFA R13+72
          CFI Block cfiCond54 Using cfiCommon0
          CFI (cfiCond54) Function user_smsreceived
          CFI (cfiCond54) Conditional ??CrossCallReturnLabel_163
          CFI (cfiCond54) R4 Frame(CFA, -32)
          CFI (cfiCond54) R5 Frame(CFA, -28)
          CFI (cfiCond54) R6 Frame(CFA, -24)
          CFI (cfiCond54) R7 Frame(CFA, -20)
          CFI (cfiCond54) R8 Frame(CFA, -16)
          CFI (cfiCond54) R9 Frame(CFA, -12)
          CFI (cfiCond54) R10 Frame(CFA, -8)
          CFI (cfiCond54) R14 Frame(CFA, -4)
          CFI (cfiCond54) CFA R13+72
          CFI Block cfiCond55 Using cfiCommon0
          CFI (cfiCond55) Function user_smsreceived
          CFI (cfiCond55) Conditional ??CrossCallReturnLabel_162
          CFI (cfiCond55) R4 Frame(CFA, -32)
          CFI (cfiCond55) R5 Frame(CFA, -28)
          CFI (cfiCond55) R6 Frame(CFA, -24)
          CFI (cfiCond55) R7 Frame(CFA, -20)
          CFI (cfiCond55) R8 Frame(CFA, -16)
          CFI (cfiCond55) R9 Frame(CFA, -12)
          CFI (cfiCond55) R10 Frame(CFA, -8)
          CFI (cfiCond55) R14 Frame(CFA, -4)
          CFI (cfiCond55) CFA R13+72
          CFI Block cfiCond56 Using cfiCommon0
          CFI (cfiCond56) Function user_smsreceived
          CFI (cfiCond56) Conditional ??CrossCallReturnLabel_161
          CFI (cfiCond56) R4 Frame(CFA, -32)
          CFI (cfiCond56) R5 Frame(CFA, -28)
          CFI (cfiCond56) R6 Frame(CFA, -24)
          CFI (cfiCond56) R7 Frame(CFA, -20)
          CFI (cfiCond56) R8 Frame(CFA, -16)
          CFI (cfiCond56) R9 Frame(CFA, -12)
          CFI (cfiCond56) R10 Frame(CFA, -8)
          CFI (cfiCond56) R14 Frame(CFA, -4)
          CFI (cfiCond56) CFA R13+72
          CFI Block cfiCond57 Using cfiCommon0
          CFI (cfiCond57) Function user_smsreceived
          CFI (cfiCond57) Conditional ??CrossCallReturnLabel_160
          CFI (cfiCond57) R4 Frame(CFA, -32)
          CFI (cfiCond57) R5 Frame(CFA, -28)
          CFI (cfiCond57) R6 Frame(CFA, -24)
          CFI (cfiCond57) R7 Frame(CFA, -20)
          CFI (cfiCond57) R8 Frame(CFA, -16)
          CFI (cfiCond57) R9 Frame(CFA, -12)
          CFI (cfiCond57) R10 Frame(CFA, -8)
          CFI (cfiCond57) R14 Frame(CFA, -4)
          CFI (cfiCond57) CFA R13+72
          CFI Block cfiCond58 Using cfiCommon0
          CFI (cfiCond58) Function user_smsreceived
          CFI (cfiCond58) Conditional ??CrossCallReturnLabel_159
          CFI (cfiCond58) R4 Frame(CFA, -32)
          CFI (cfiCond58) R5 Frame(CFA, -28)
          CFI (cfiCond58) R6 Frame(CFA, -24)
          CFI (cfiCond58) R7 Frame(CFA, -20)
          CFI (cfiCond58) R8 Frame(CFA, -16)
          CFI (cfiCond58) R9 Frame(CFA, -12)
          CFI (cfiCond58) R10 Frame(CFA, -8)
          CFI (cfiCond58) R14 Frame(CFA, -4)
          CFI (cfiCond58) CFA R13+72
          CFI Block cfiCond59 Using cfiCommon0
          CFI (cfiCond59) Function user_smsreceived
          CFI (cfiCond59) Conditional ??CrossCallReturnLabel_158
          CFI (cfiCond59) R4 Frame(CFA, -32)
          CFI (cfiCond59) R5 Frame(CFA, -28)
          CFI (cfiCond59) R6 Frame(CFA, -24)
          CFI (cfiCond59) R7 Frame(CFA, -20)
          CFI (cfiCond59) R8 Frame(CFA, -16)
          CFI (cfiCond59) R9 Frame(CFA, -12)
          CFI (cfiCond59) R10 Frame(CFA, -8)
          CFI (cfiCond59) R14 Frame(CFA, -4)
          CFI (cfiCond59) CFA R13+72
??Subroutine46_0:
        ADD      R0,SP,#+28
          CFI (cfiCond48) FunCall user_smsreceived strcmp
          CFI (cfiCond48) FunCall user_smsreceived strcmp
          CFI (cfiCond48) FunCall user_smsreceived strcmp
          CFI (cfiCond48) FunCall user_smsreceived strcmp
          CFI (cfiCond48) FunCall user_smsreceived strcmp
          CFI (cfiCond48) FunCall user_smsreceived strcmp
          CFI (cfiCond48) FunCall user_smsreceived strcmp
          CFI (cfiCond48) FunCall user_smsreceived strcmp
          CFI (cfiCond48) FunCall user_smsreceived strcmp
          CFI (cfiCond48) FunCall user_smsreceived strcmp
          CFI (cfiCond48) FunCall user_smsreceived strcmp
        B.W      strcmp
          CFI EndBlock cfiCond48
          CFI EndBlock cfiCond49
          CFI EndBlock cfiPicker50
          CFI EndBlock cfiCond51
          CFI EndBlock cfiCond52
          CFI EndBlock cfiCond53
          CFI EndBlock cfiCond54
          CFI EndBlock cfiCond55
          CFI EndBlock cfiCond56
          CFI EndBlock cfiCond57
          CFI EndBlock cfiCond58
          CFI EndBlock cfiCond59

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond60 Using cfiCommon0
          CFI Function user_smsreceived
          CFI Conditional ??CrossCallReturnLabel_117
          CFI R4 Frame(CFA, -32)
          CFI R5 Frame(CFA, -28)
          CFI R6 Frame(CFA, -24)
          CFI R7 Frame(CFA, -20)
          CFI R8 Frame(CFA, -16)
          CFI R9 Frame(CFA, -12)
          CFI R10 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+72
          CFI Block cfiCond61 Using cfiCommon0
          CFI (cfiCond61) Function user_smsreceived
          CFI (cfiCond61) Conditional ??CrossCallReturnLabel_116
          CFI (cfiCond61) R4 Frame(CFA, -32)
          CFI (cfiCond61) R5 Frame(CFA, -28)
          CFI (cfiCond61) R6 Frame(CFA, -24)
          CFI (cfiCond61) R7 Frame(CFA, -20)
          CFI (cfiCond61) R8 Frame(CFA, -16)
          CFI (cfiCond61) R9 Frame(CFA, -12)
          CFI (cfiCond61) R10 Frame(CFA, -8)
          CFI (cfiCond61) R14 Frame(CFA, -4)
          CFI (cfiCond61) CFA R13+72
          CFI Block cfiPicker62 Using cfiCommon1
          CFI (cfiPicker62) NoFunction
          CFI (cfiPicker62) Picker
        THUMB
?Subroutine35:
        ADR.N    R1,??DataTable44
          CFI Block cfiCond63 Using cfiCommon0
          CFI (cfiCond63) Function user_smsreceived
          CFI (cfiCond63) Conditional ??CrossCallReturnLabel_115
          CFI (cfiCond63) R4 Frame(CFA, -32)
          CFI (cfiCond63) R5 Frame(CFA, -28)
          CFI (cfiCond63) R6 Frame(CFA, -24)
          CFI (cfiCond63) R7 Frame(CFA, -20)
          CFI (cfiCond63) R8 Frame(CFA, -16)
          CFI (cfiCond63) R9 Frame(CFA, -12)
          CFI (cfiCond63) R10 Frame(CFA, -8)
          CFI (cfiCond63) R14 Frame(CFA, -4)
          CFI (cfiCond63) CFA R13+72
          CFI Block cfiCond64 Using cfiCommon0
          CFI (cfiCond64) Function user_smsreceived
          CFI (cfiCond64) Conditional ??CrossCallReturnLabel_114
          CFI (cfiCond64) R4 Frame(CFA, -32)
          CFI (cfiCond64) R5 Frame(CFA, -28)
          CFI (cfiCond64) R6 Frame(CFA, -24)
          CFI (cfiCond64) R7 Frame(CFA, -20)
          CFI (cfiCond64) R8 Frame(CFA, -16)
          CFI (cfiCond64) R9 Frame(CFA, -12)
          CFI (cfiCond64) R10 Frame(CFA, -8)
          CFI (cfiCond64) R14 Frame(CFA, -4)
          CFI (cfiCond64) CFA R13+72
??Subroutine35_0:
        MOV      R0,R6
          CFI (cfiCond60) FunCall user_smsreceived strcat
          CFI (cfiCond60) FunCall user_smsreceived strcat
          CFI (cfiCond60) FunCall user_smsreceived strcat
          CFI (cfiCond60) FunCall user_smsreceived strcat
        B.W      strcat
          CFI EndBlock cfiCond60
          CFI EndBlock cfiCond61
          CFI EndBlock cfiPicker62
          CFI EndBlock cfiCond63
          CFI EndBlock cfiCond64

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable35:
        DATA32
        DC32     log_unacked

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_0:
        DATA8
        DC8 "SLDN"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_1:
        DATA8
        DC8 "00%08X"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_2:
        DATA8
        DC8 ">>U Wiegand ID: %s\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_3:
        DATA8
        DC8 ">>U RF433 ID: %s\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_4:
        DATA8
        DC8 ">>U RFID ID: %s\015\012"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_5:
        DATA8
        DC8 ">>U Error connection ...\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_6:
        DATA8
        DC8 ">>U SMS recv: %.*s\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
        DATA8
        DC8 "add"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_8:
        DATA8
        DC8 "reset"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_9:
        DATA8
        DC8 "setout"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_10:
        DATA8
        DC8 "default"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_11:
        DATA8
        DC8 ": Error \"Update in progress\""
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
        DATA8
        DC8 ": "
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_13:
        DATA8
        DC8 " - Ok"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_14:
        DATA8
        DC8 ": Error \"Format not correct\""
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_15:
        DATA8
        DC8 "imei"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_16:
        DATA8
        DC8 "status"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_17:
        DATA8
        DC8 0x53, 0x54, 0x41, 0x54, 0x55, 0x53, 0x3A, 0x0D
        DC8 0x0A, 0x2D, 0x53, 0x49, 0x47, 0x4E, 0x41, 0x4C
        DC8 0x3A, 0x20, 0x25, 0x64, 0x0D, 0x0A, 0x2D, 0x4E
        DC8 0x45, 0x54, 0x3A, 0x20, 0x25, 0x73, 0x0D, 0x0A
        DC8 0x2D, 0x43, 0x4C, 0x4F, 0x55, 0x44, 0x3A, 0x20
        DC8 0x25, 0x64, 0x2F, 0x25, 0x64, 0x0D, 0x0A, 0x2D
        DC8 0x46, 0x57, 0x3A, 0x20, 0x25, 0x64, 0x0D, 0x0A
        DC8 0x2D, 0x54, 0x52, 0x49, 0x47, 0x3A, 0x20, 0x25
        DC8 0x64, 0x2F, 0x25, 0x64, 0x2F, 0x25, 0x64, 0x2F
        DC8 0x25, 0x64, 0
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_18:
        DATA8
        DC8 "Not network"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_19:
        DATA8
        DC8 "gprs"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_20:
        DATA8
        DC8 "edge"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_21:
        DATA8
        DC8 "3G"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_22:
        DATA8
        DC8 "4G"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_23:
        DATA8
        DC8 "5G"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_24:
        DATA8
        DC8 ": Go to reset"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_25:
        DATA8
        DC8 ": Out 1 enabled"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_26:
        DATA8
        DC8 ": Out 2 enabled"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_27:
        DATA8
        DC8 ": Error, Out is blocked"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_28:
        DATA8
        DC8 "update"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_29:
        DATA8
        DC8 ": Complete"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_30:
        DATA8
        DC8 ": Error \"Pin not valid\""

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_31:
        DATA8
        DC8 ">>U Sending SMS ...\015\012"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_32:
        DATA8
        DC8 ">>U SMS sended ...\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_33:
        DATA8
        DC8 ">>U InCalling: %s\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_34:
        DATA8
        DC8 ">>U Comand error ...\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_35:
        DATA8
        DC8 ">>U Parse USSD ...\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_36:
        DATA8
        DC8 ">>U Clear mem block ...\015\012"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_37:
        DATA8
        DC8 ">>U Reset on time\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_38:
        DATA8
        DC8 ">>U Factory on time\015\012"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_39:
        DATA8
        DC8 ">>U Not answer to reg pack\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_40:
        DATA8
        DC8 ">>U Not answer to status\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_41:
        DATA8
        DC8 "INSTALL"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_42:
        DATA8
        DC8 "TIMEOUT"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_43:
        DATA8
        DC8 ">>U Trial expired, rollback\015\012"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_44:
        DATA8
        DC8 ">>U Reset on work time\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_45:
        DATA8
        DC8 ">>U Read config ...\015\012"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_46:
        DATA8
        DC8 ">>U Set default params ...\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_47:
        DATA8
        DC8 "gate.gsmgate.ru:15010"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_48:
        DATA8
        DC8 "gate.gsmgate.ru:15020"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
        DATA8
        DC8 "+7"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_50:
        DATA8
        DC8 "internet,gdata,gdata"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_51:
        DATA8
        DC8 ">>U Factory reset\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_52:
        DATA8
        DC8 ">>U Send any data (cmd %d)\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_53:
        DATA8
        DC8 ">>U Send reg data (cmd 1)\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_54:
        DATA8
        DC8 ">>U Send status (cmd 2)\015\012"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_55:
        DATA8
        DC8 ">>U Send status2 (cmd 3)\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_56:
        DATA8
        DC8 ">>U Send status update (cmd 4)\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_57:
        DATA8
        DC8 ">>U Send crc data (cmd 5)\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_58:
        DATA8
        DC8 ">>U Send crc data (cmd 6)\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_59:
        DATA8
        DC8 ">>U Send crc data (cmd 16)\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_60:
        DATA8
        DC8 ">>U Send sms to server (cmd 7)\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_61:
        DATA8
        DC8 ": ID is unregistred!"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_62:
        DATA8
        DC8 ">>U Send answer (cmd 11)\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_63:
        DATA8
        DC8 ">>U Send timeshtamp (cmd 12)\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_64:
        DATA8
        DC8 ">>U Send log data (cmd 10)\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_65:
        DATA8
        DC8 ">>U Send temp IDs (cmd 9)\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_66:
        DATA8
        DC8 ">>U Get FW block %d (cmd 13)\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_67:
        DATA8
        DC8 "BOOT "
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_68:
        DATA8
        DC8 "PARAM"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_69:
        DATA8
        DC8 "BUSY"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_70:
        DATA8
        DC8 "OLD BOOT"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_71:
        DATA8
        DC8 "BOOT BUSY"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_72:
        DATA8
        DC8 "IN TRIAL"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_73:
        DATA8
        DC8 "NO BACKUP"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_74:
        DATA8
        DC8 "BAD BACKUP"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_75:
        DATA8
        DC8 "CRC"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_76:
        DATA8
        DC8 "WRITE"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_77:
        DATA8
        DC8 "CANCEL"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_78:
        DATA8
        DC8 "HW"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_79:
        DATA8
        DC8 "TRIAL FAIL"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_80:
        DATA8
        DC8 "BACKUP"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_81:
        DATA8
        DC8 "READ"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_82:
        DATA8
        DC8 ">>U Recv cmd reg (cmd 1)\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_83:
        DATA8
        DC8 ">>U Recv cmd reset (cmd 2)\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_84:
        DATA8
        DC8 ">>U Recv cmd factory (cmd 3)\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_85:
        DATA8
        DC8 ">>U Recv cmd triggers (cmd 5)\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_86:
        DATA8
        DC8 ">>U Recv cmd get IDs (cmd 6)\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_87:
        DATA8
        DC8 ">>U Recv cmd set out (cmd 8)\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_88:
        DATA8
        DC8 ">>U Recv cmd get log (cmd 9)\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_89:
        DATA8
        DC8 ">>U Recv cmd get IDs (cmd 10)\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_90:
        DATA8
        DC8 ">>U Recv cmd send sms (cmd 11)\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_91:
        DATA8
        DC8 ">>U Recv cmd settings (cmd 12)\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_92:
        DATA8
        DC8 ">>U Recv cmd get crc (cmd 17)\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_93:
        DATA8
        DC8 ">>U Recv cmd crc ID (cmd 18)\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_94:
        DATA8
        DC8 ">>U Recv cmd fw update (cmd 19)\015\012"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_95:
        DATA8
        DC8 ">>U Recv FW block %u (cmd 20)\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_96:
        DATA8
        DC8 ">>U Recv cmd get photo (cmd 21)\015\012"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_97:
        DATA8
        DC8 ">>U Recv cmd get status (cmd 22)\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_98:
        DATA8
        DC8 ">>U Recv cmd set time (cmd 23)\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_99:
        DATA8
        DC8 ">>U Recv cmd answer (cmd 24)\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_100:
        DATA8
        DC8 ">>U Recv cmd (cmd 27)\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_101:
        DATA8
        DC8 ">>U Check ID: Phone\015\012"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_102:
        DATA8
        DC8 ">>U Check ID: Wiegand\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_103:
        DATA8
        DC8 ">>U Check ID: RF433\015\012"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_104:
        DATA8
        DC8 ">>U Check ID: RFID\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_105:
        DATA8
        DC8 ">>U Check ID: AUTO\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_106:
        DATA8
        DC8 ">>U Check ID: Server\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_107:
        DATA8
        DC8 ">>U Check ID: Unblock hand\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_108:
        DATA8
        DC8 ">>U Check ID: Unblock on scene\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_109:
        DATA8
        DC8 ">>U Check ID: Open by temp ID\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_110:
        DATA8
        DC8 ">>U Check ID: Open any user\015\012"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_111:
        DATA8
        DC8 ">>U Check ID: Open all by schedule\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_112:
        DATA8
        DC8 ">>U Check ID: Blocked on date\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_113:
        DATA8
        DC8 ">>U Check ID: Use time expired\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_114:
        DATA8
        DC8 ">>U Check ID: Undefine group\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_115:
        DATA8
        DC8 ">>U Check ID: Group is blocked\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_116:
        DATA8
        DC8 ">>U Check ID: Any time\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_117:
        DATA8
        DC8 ">>U Check ID: Undefine schedule\015\012"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_118:
        DATA8
        DC8 ">>U Check ID: Shedule is blocked\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_119:
        DATA8
        DC8 ">>U Check ID: Undefine day\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_120:
        DATA8
        DC8 ">>U Check ID: Disabled by time\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_121:
        DATA8
        DC8 ">>U Check ID: Not check holiday\015\012"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_122:
        DATA8
        DC8 ">>U Check ID: Not founded\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_123:
        DATA8
        DC8 0x3E, 0x3E, 0x55, 0x20, 0x43, 0x68, 0x65, 0x63
        DC8 0x6B, 0x20, 0x49, 0x44, 0x3A, 0x20, 0x43, 0x6F
        DC8 0x6E, 0x74, 0x72, 0x6F, 0x6C, 0x20, 0x6F, 0x75
        DC8 0x74, 0x20, 0x64, 0x69, 0x73, 0x61, 0x62, 0x6C
        DC8 0x65, 0x64, 0x20, 0x74, 0x6F, 0x20, 0x63, 0x75
        DC8 0x72, 0x72, 0x65, 0x6E, 0x74, 0x20, 0x74, 0x79
        DC8 0x70, 0x65, 0x0D, 0x0A, 0
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_124:
        DATA8
        DC8 ">>U Act Out: No output to switch\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_125:
        DATA8
        DC8 ">>U Act Out: Out 1\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_126:
        DATA8
        DC8 ">>U Act Out: Out 2\015\012"
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_127:
        DATA8
        DC8 ">>U Check Shedule: Undefine schedule\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_128:
        DATA8
        DC8 ">>U Check Shedule: Shedule is blocked\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_129:
        DATA8
        DC8 ">>U Check Shedule: Undefine day\015\012"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_130:
        DATA8
        DC8 ">>U Check Shedule: Disabled by time\015\012"
        DATA16
        DS8 2

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_131:
        DATA8
        DC8 ">>U Check Shedule: Not check holiday\015\012"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
        DATA8
        DC8 "%d"
        DS8 1

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
`user_smsreceived{2}{3}{11}{12}{13}{15}{16}{20}{21}{23}{24}{25}::net`:
        DATA32
        DC32 ?_18, ?_19, ?_20, ?_21, ?_22, ?_23

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock65 Using cfiCommon0
          CFI Function user_sendsms
        THUMB
user_sendsms:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R5,R0
        MOV      R4,R1
        LDR.W    R0,??DataTable61_2
          CFI FunCall user_debug
        BL       user_debug
        MOV      R1,R5
        MOV      R0,R4
          CFI FunCall user_comm_sms
        BL       user_comm_sms
        CBZ.N    R0,??user_sendsms_0
        MOV      R0,R4
          CFI FunCall user_last10digitsnumber
        BL       user_last10digitsnumber
        MOV      R1,R0
        MOVS     R0,#+36
        POP      {R2,R4,R5,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall log_addSystemEventID
        B.W      log_addSystemEventID
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
??user_sendsms_0:
        POP      {R0,R4,R5,PC}
          CFI EndBlock cfiBlock65

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock66 Using cfiCommon0
          CFI Function user_smssended
        THUMB
user_smssended:
        LDR.W    R0,??DataTable61_3
          CFI FunCall user_debug
        B.W      user_debug
          CFI EndBlock cfiBlock66

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable37:
        DATA32
        DC32     0x5fa0004

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock67 Using cfiCommon0
          CFI Function user_incomingcall
        THUMB
user_incomingcall:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        MOV      R4,R0
        MOV      R5,R1
        LDR.N    R6,??DataTable43_1
        MOV      R2,R4
        LDR.W    R1,??DataTable60_2
        MOV      R0,R6
          CFI FunCall sprintf
        BL       sprintf
        MOV      R0,R6
          CFI FunCall user_debug
        BL       user_debug
        CMP      R5,#+1
        BNE.N    ??user_incomingcall_0
        LDR.W    R6,??DataTable56
        MOVS     R2,#+15
        MOV      R1,R4
        ADD      R0,R6,#+58
          CFI FunCall strncpy
        BL       strncpy
        MOVS     R0,#+0
        STRB     R0,[R6, #+73]
??user_incomingcall_0:
        MOV      R0,R4
          CFI FunCall user_last10digitsnumber
        BL       user_last10digitsnumber
        MOV      R1,R5
          CFI FunCall user_checkIdAndControl
        BL       user_checkIdAndControl
        MOVS     R0,#+1
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock67

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable38:
        DATA32
        DC32     store_reset_requested

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock68 Using cfiCommon0
          CFI Function user_comanderror
        THUMB
user_comanderror:
        LDR.W    R0,??DataTable62
          CFI FunCall user_debug
        B.W      user_debug
          CFI EndBlock cfiBlock68

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock69 Using cfiCommon0
          CFI Function user_parseUSSD
        THUMB
user_parseUSSD:
        LDR.W    R0,??DataTable62_1
          CFI FunCall user_debug
        B.W      user_debug
          CFI EndBlock cfiBlock69

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable40:
        DATA8
        DC8      "add"

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock70 Using cfiCommon0
          CFI Function user_init
        THUMB
user_init:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
          CFI FunCall w25_read_devID
        BL       w25_read_devID
        LDR.W    R1,??DataTable63
        MOVS     R2,#+0
        STR      R2,[R1, #+0]
        MOV      R3,#+131072
        STR      R3,[R1, #+4]
        MOV      R2,#+135168
        STR      R2,[R1, #+8]
        STR      R3,[R1, #+12]
        MOV      R2,#+266240
        STR      R2,[R1, #+16]
        LDR.W    R2,??DataTable61_4
        STR      R2,[R1, #+20]
        LDR.W    R2,??DataTable61_5
        STR      R2,[R1, #+24]
        MOV      R3,#+2048
        STR      R3,[R1, #+28]
        LDR.W    R2,??DataTable61_6
        STR      R2,[R1, #+32]
        STR      R3,[R1, #+36]
        MOV      R2,#+1884160
        STR      R2,[R1, #+40]
        MOV      R2,#+1024
        STR      R2,[R1, #+44]
        LDR.W    R2,??DataTable61_7
        STR      R2,[R1, #+48]
        MOV      R2,#+4096
        STR      R2,[R1, #+52]
        LDR.W    R2,??DataTable61_8
        STR      R2,[R1, #+56]
        MOV      R2,#+98304
        STR      R2,[R1, #+60]
        LDR.W    R2,??DataTable61_9
        STR      R2,[R1, #+64]
        MOV      R2,#+49152
        STR      R2,[R1, #+68]
        MOVW     R2,#+6082
        CMP      R0,R2
        BNE.N    ??user_init_0
        MOV      R2,#+2097152
        STR      R2,[R1, #+72]
        MOV      R2,#+1048576
        B.N      ??user_init_1
??user_init_0:
        MOVS     R2,#+0
        STR      R2,[R1, #+72]
??user_init_1:
        STR      R2,[R1, #+76]
        LDR.W    R1,??DataTable56
        STRH     R0,[R1, #+16]
          CFI FunCall mem_recover
        BL       mem_recover
          CFI FunCall user_readconfig
        BL       user_readconfig
          CFI FunCall user_initvariables
        BL       user_initvariables
        LDR.W    R4,??DataTable63_1
        LDRH     R1,[R4, #+0]
        MOVW     R0,#+20566
        CMP      R1,R0
        BNE.N    ??user_init_2
        MOVS     R0,#+39
          CFI FunCall log_addSystemEvent
        BL       log_addSystemEvent
        CBZ.N    R0,??user_init_2
        MOVS     R0,#+0
        STRH     R0,[R4, #+0]
??user_init_2:
          CFI FunCall app_boot_metadata
        BL       app_boot_metadata
        LDR.W    R0,??DataTable54_2
        LDRB     R0,[R0, #+178]
        CBZ.N    R0,??user_init_3
          CFI FunCall WG_init
        BL       WG_init
??user_init_3:
          CFI FunCall rs485_init
        BL       rs485_init
        MOVS     R1,#+10
        MOVS     R0,#+0
          CFI FunCall uart_setRecTimeout
        BL       uart_setRecTimeout
        MOVS     R1,#+1
        LDR.W    R0,??DataTable64
        STRB     R1,[R0, #+0]
        POP      {R4,PC}
          CFI EndBlock cfiBlock70

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock71 Using cfiCommon0
          CFI Function user_clearMemBlock
        THUMB
user_clearMemBlock:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        MOV      R4,R0
        LDR.W    R5,??DataTable63
        ADD      R0,R5,R0, LSL #+3
        LDR      R0,[R0, #+4]
        CBZ.N    R0,??user_clearMemBlock_0
        LDR.W    R0,??DataTable64_1
          CFI FunCall user_debug
        BL       user_debug
        MOV      R0,R4
        LDR      R0,[R5, R0, LSL #+3]
        ADD      R1,R5,R4, LSL #+3
        LDR      R6,[R1, #+4]
        ADDS     R6,R6,R0
        MOV      R0,R4
        LDR      R5,[R5, R0, LSL #+3]
        B.N      ??user_clearMemBlock_1
??user_clearMemBlock_2:
          CFI FunCall wdt_reload
        BL       wdt_reload
        ADD      R5,R5,#+4096
??user_clearMemBlock_1:
        CMP      R5,R6
        BCS.N    ??user_clearMemBlock_3
        MOV      R0,R5
          CFI FunCall mem_erase_sector
        BL       mem_erase_sector
        CMP      R0,#+0
        BNE.N    ??user_clearMemBlock_2
        POP      {R4-R6,PC}
??user_clearMemBlock_3:
        CMP      R4,#+2
        BNE.N    ??user_clearMemBlock_0
        MOVS     R1,#+0
        MOVS     R0,#+0
          CFI FunCall idindex_scanned
        BL       idindex_scanned
??user_clearMemBlock_0:
        MOVS     R0,#+1
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock71

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
sync_pending:
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
sync_quiet:
        DS8 1

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock72 Using cfiCommon0
          CFI Function sync_packet
          CFI NoCalls
        THUMB
sync_packet:
        LDR.W    R2,??DataTable63_2
        LDRB     R1,[R2, #+0]
        ORRS     R0,R0,R1
        STRB     R0,[R2, #+0]
        MOVS     R1,#+30
        LDR.W    R0,??DataTable64_2
        STRB     R1,[R0, #+0]
        BX       LR
          CFI EndBlock cfiBlock72

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable43:
        DATA32
        DC32     temp_unacked

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable43_1:
        DATA32
        DC32     _A_tempStrBuffer

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable43_2:
        DATA32
        DC32     ?_3

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock73 Using cfiCommon0
          CFI Function sync_end_tick
        THUMB
sync_end_tick:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        LDR.W    R4,??DataTable64_2
        LDRB     R0,[R4, #+0]
        CBZ.N    R0,??sync_end_tick_0
        SUBS     R0,R0,#+1
        STRB     R0,[R4, #+0]
        LDRB     R0,[R4, #+0]
        CBNZ.N   R0,??sync_end_tick_0
        MOVS     R7,#+0
        LDR.W    R5,??DataTable63_2
??sync_end_tick_1:
        MOVS     R6,#+1
        LDRB     R0,[R5, #+0]
        LSL      R1,R6,R7
        TST      R0,R1
        BEQ.N    ??sync_end_tick_2
        ADDS     R0,R7,#+6
        UXTB     R0,R0
          CFI FunCall log_addSystemEvent
        BL       log_addSystemEvent
        CBZ.N    R0,??sync_end_tick_2
        LDRB     R0,[R5, #+0]
        LSLS     R6,R6,R7
        BIC      R6,R0,R6
        STRB     R6,[R5, #+0]
??sync_end_tick_2:
        ADDS     R7,R7,#+1
        CMP      R7,#+5
        BCC.N    ??sync_end_tick_1
        LDRB     R0,[R5, #+0]
        CBZ.N    R0,??sync_end_tick_0
        MOVS     R0,#+1
        STRB     R0,[R4, #+0]
??sync_end_tick_0:
        POP      {R0,R4-R7,PC}
          CFI EndBlock cfiBlock73

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44:
        DATA8
        DC8      0x3A, 0x20, 0x00, 0x00

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_1:
        DATA32
        DC32     ?_0

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_2:
        DATA32
        DC32     ?_2

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_3:
        DATA32
        DC32     ?_1

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_4:
        DATA32
        DC32     ?_4

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_5:
        DATA32
        DC32     0x15180

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_6:
        DATA32
        DC32     0x28de80

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_7:
        DATA32
        DC32     0xfffc0b80

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_8:
        DATA32
        DC32     0xfffaba00

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_9:
        DATA32
        DC32     0xfff96880

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_10:
        DATA32
        DC32     0xfff81700

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_11:
        DATA32
        DC32     0xfff6c580

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_12:
        DATA32
        DC32     0x1e13380

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_13:
        DATA32
        DC32     0xfffeae80

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_14:
        DATA32
        DC32     ?_5

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_15:
        DATA32
        DC32     server_rx_busy

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_16:
        DATA32
        DC32     server_rx_cancelled

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_17:
        DATA32
        DC32     server_rx_count

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_18:
        DATA32
        DC32     server_rx_pos

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_19:
        DATA32
        DC32     server_rx_total

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_20:
        DATA32
        DC32     server_rx_offset

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_21:
        DATA32
        DC32     server_rx_ready

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_22:
        DATA32
        DC32     server_rx_provisional

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_23:
        DATA32
        DC32     server_rx_work

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_24:
        DATA32
        DC32     server_rx_compacted

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_25:
        DATA32
        DC32     server_rx_commands

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_26:
        DATA32
        DC32     server_rx_last

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_27:
        DATA32
        DC32     server_rx_buffer

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable44_28:
        DATA32
        DC32     server_rx_crc

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock74 Using cfiCommon0
          CFI Function user_loop
        THUMB
user_loop:
        PUSH     {R2-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+24
        LDR.W    R5,??DataTable54_2
        LDRB     R0,[R5, #+178]
        CBZ.N    R0,??user_loop_0
          CFI FunCall user_storage_flash_ready
        BL       user_storage_flash_ready
        CBZ.N    R0,??user_loop_0
          CFI FunCall WG_process
        BL       WG_process
??user_loop_0:
          CFI FunCall user_storage_command_ready
        BL       user_storage_command_ready
        CBZ.N    R0,??user_loop_1
          CFI FunCall user_storage_flash_ready
        BL       user_storage_flash_ready
        CBZ.N    R0,??user_loop_1
        LDR.W    R0,??DataTable63
        LDR      R0,[R0, #+16]
          CFI FunCall idindex_poll
        BL       idindex_poll
??user_loop_1:
          CFI FunCall user_storage_command_ready
        BL       user_storage_command_ready
        CBZ.N    R0,??user_loop_2
          CFI FunCall user_storage_flash_ready
        BL       user_storage_flash_ready
        CBZ.N    R0,??user_loop_2
          CFI FunCall fw_rollback_poll
        BL       fw_rollback_poll
??user_loop_2:
          CFI FunCall fw_status_poll
        BL       fw_status_poll
          CFI FunCall unreg_sms_poll
        BL       unreg_sms_poll
          CFI FunCall reader_reply_poll
        BL       reader_reply_poll
        MOVS     R0,#+1
          CFI FunCall timers_get
        BL       timers_get
        CMP      R0,#+50
        BCC.N    ??user_loop_3
        MOVS     R1,#+0
        MOVS     R0,#+1
          CFI FunCall timers_set
        BL       timers_set
        LDR.W    R4,??DataTable56
        LDRB     R0,[R4, #+133]
        CMP      R0,#+0
        ITT      NE
        LDRBNE   R0,[R4, #+164]
        CMPNE    R0,#+0
        BEQ.N    ??user_loop_3
        LDR      R0,[R4, #+174]
        CBNZ.N   R0,??user_loop_4
        LDRH     R1,[R4, #+172]
        LDRB     R0,[R4, #+178]
          CFI FunCall user_getFWblockFromServer
        BL       user_getFWblockFromServer
        CBZ.N    R0,??user_loop_3
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        B.N      ??user_loop_5
??user_loop_4:
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        LDR      R1,[R4, #+174]
        SUBS     R0,R0,R1
        MOVW     R1,#+20000
        CMP      R0,R1
        BCC.N    ??user_loop_3
        MOVS     R0,#+0
??user_loop_5:
        STR      R0,[R4, #+174]
??user_loop_3:
        MOVS     R0,#+2
          CFI FunCall timers_get
        BL       timers_get
        CMP      R0,#+100
        BCC.N    ??user_loop_6
        MOVS     R1,#+0
        MOVS     R0,#+2
          CFI FunCall timers_set
        BL       timers_set
          CFI FunCall wdt_reload
        BL       wdt_reload
          CFI FunCall led_fun100ms
        BL       led_fun100ms
          CFI FunCall user_readinputs
        BL       user_readinputs
        MOVS     R0,#+0
          CFI FunCall out_getstate
        BL       out_getstate
        LDR.W    R4,??DataTable56
        LDRH     R1,[R4, #+129]
        BFI      R1,R0,#+1,#+1
        STRH     R1,[R4, #+129]
        MOVS     R0,#+1
          CFI FunCall out_getstate
        BL       out_getstate
        LDRH     R1,[R4, #+129]
        BFI      R1,R0,#+0,#+1
        STRH     R1,[R4, #+129]
        LDRB     R0,[R4, #+133]
        CBZ.N    R0,??user_loop_7
        ADD      R3,R4,#+129
        BL       ?Subroutine15
??CrossCallReturnLabel_29:
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        MOV      R1,R0
        LDRH     R0,[SP, #+0]
          CFI FunCall status_event_due
        BL       status_event_due
        CBZ.N    R0,??user_loop_7
        MOVS     R0,#+1
          CFI FunCall user_send_statusPack
        BL       user_send_statusPack
??user_loop_7:
        LDRB     R0,[R4, #+129]
        TST      R0,#0x3
        BEQ.N    ??user_loop_8
        MOVS     R0,#+64
        LDR.W    R1,??DataTable67
        STR      R0,[R1, #+0]
        B.N      ??user_loop_6
??user_loop_8:
        MOVS     R1,#+64
        LDR.W    R0,??DataTable67_1
        STR      R1,[R0, #+0]
??user_loop_6:
        MOVS     R1,#+100
        MOVS     R0,#+0
          CFI FunCall timers_take
        BL       timers_take
        MOV      R4,R0
        B.N      ??user_loop_9
??user_loop_10:
        SUBS     R4,R4,#+1
          CFI FunCall out_process
        BL       out_process
??user_loop_9:
        CMP      R4,#+0
        BNE.N    ??user_loop_10
        MOV      R1,#+1000
        MOVS     R0,#+3
          CFI FunCall timers_take
        BL       timers_take
        MOV      R4,R0
        LDR.W    R6,??DataTable56
        B.N      ??user_loop_11
??user_loop_12:
        LDRB     R0,[R6, #+164]
        CBZ.N    R0,??user_loop_13
        LDRH     R0,[R6, #+179]
        MOVS     R1,#+60
        UDIV     R2,R0,R1
        MLS      R0,R1,R2,R0
        CBNZ.N   R0,??user_loop_13
          CFI FunCall user_saveindex
        BL       user_saveindex
??user_loop_13:
          CFI FunCall app_trial_expired
        BL       app_trial_expired
        CBZ.N    R0,??user_loop_11
        LDR.W    R0,??DataTable67_2
          CFI FunCall user_debug
        BL       user_debug
          CFI FunCall app_trial_rollback
        BL       app_trial_rollback
          CFI FunCall user_savereset
        BL       user_savereset
??user_loop_11:
        CMP      R4,#+0
        BEQ.W    ??user_loop_14
        SUBS     R4,R4,#+1
          CFI FunCall wdt_reload
        BL       wdt_reload
        LDR.W    R0,??DataTable67_3
        LDR      R0,[R0, #+0]
        LSLS     R0,R0,#+16
        BPL.N    ??user_loop_15
          CFI FunCall usb_get_state
        BL       usb_get_state
        CMP      R0,#+1
        BEQ.N    ??user_loop_16
          CFI FunCall usb_init
        BL       usb_init
        B.N      ??user_loop_16
??user_loop_15:
        MOVS     R1,#+0
        STRB     R1,[R6, #+138]
??user_loop_16:
          CFI FunCall user_indication
        BL       user_indication
        LDR      R0,[R6, #+51]
        ADDS     R0,R0,#+1
        STR      R0,[R6, #+51]
        LDR      R0,[R6, #+47]
        ADDS     R0,R0,#+1
        STR      R0,[R6, #+47]
          CFI FunCall log_checkPending
        BL       log_checkPending
          CFI FunCall user_storage_flash_ready
        BL       user_storage_flash_ready
        CBZ.N    R0,??user_loop_17
          CFI FunCall user_checkIdTimeEnd
        BL       user_checkIdTimeEnd
??user_loop_17:
        LDR      R0,[R6, #+47]
          CFI FunCall user_getweakday
        BL       user_getweakday
        STRB     R0,[R6, #+55]
        LDR      R2,[R6, #+47]
        LDR.W    R0,??DataTable68
        UDIV     R0,R2,R0
        LDR.W    R1,??DataTable68
        MLS      R2,R1,R0,R2
        MOV      R0,#+3600
        UDIV     R0,R2,R0
        STRB     R0,[R6, #+56]
        LDR      R2,[R6, #+47]
        MOV      R0,#+3600
        UDIV     R0,R2,R0
        MOV      R1,#+3600
        MLS      R2,R1,R0,R2
        MOVS     R0,#+60
        UDIV     R0,R2,R0
        STRB     R0,[R6, #+57]
          CFI FunCall comm_channel_ready
        BL       comm_channel_ready
        STRB     R0,[R6, #+80]
          CFI FunCall comm_info
        BL       comm_info
        LDRB     R0,[R0, #+0]
        STRB     R0,[R6, #+74]
          CFI FunCall comm_info
        BL       comm_info
        LDRB     R0,[R0, #+1]
        STRB     R0,[R6, #+75]
          CFI FunCall comm_info
        BL       comm_info
        LDRB     R0,[R0, #+2]
        STRB     R0,[R6, #+76]
          CFI FunCall comm_info
        BL       comm_info
        LDRB     R0,[R0, #+3]
        STRB     R0,[R6, #+79]
        LDRB     R0,[R6, #+111]
        CBZ.N    R0,??user_loop_18
        SUBS     R0,R0,#+1
        STRB     R0,[R6, #+111]
        LDRB     R0,[R6, #+111]
        CBNZ.N   R0,??user_loop_18
        MOVS     R0,#+33
          CFI FunCall log_addSystemEvent
        BL       log_addSystemEvent
        LDR.W    R0,??DataTable68_1
          CFI FunCall user_debug
        BL       user_debug
          CFI FunCall user_savereset
        BL       user_savereset
??user_loop_18:
        LDRB     R0,[R6, #+112]
        CBZ.N    R0,??user_loop_19
        SUBS     R0,R0,#+1
        STRB     R0,[R6, #+112]
        LDRB     R0,[R6, #+112]
        CBNZ.N   R0,??user_loop_19
        LDR.W    R0,??DataTable68_2
          CFI FunCall user_debug
        BL       user_debug
          CFI FunCall user_factoryReset
        BL       user_factoryReset
??user_loop_19:
        LDRB     R0,[R6, #+113]
        CBZ.N    R0,??user_loop_20
        SUBS     R0,R0,#+1
        STRB     R0,[R6, #+113]
        LDRB     R0,[R6, #+113]
        CBNZ.N   R0,??user_loop_20
        STRB     R0,[R6, #+114]
        MOVS     R1,#+255
          CFI FunCall user_send_readedIDtoServer
        BL       user_send_readedIDtoServer
??user_loop_20:
          CFI FunCall sync_end_tick
        BL       sync_end_tick
        LDRB     R0,[R6, #+137]
        CBZ.N    R0,??user_loop_21
        SUBS     R0,R0,#+1
        STRB     R0,[R6, #+137]
        LDRB     R0,[R6, #+137]
        CBNZ.N   R0,??user_loop_21
        LDRB     R0,[R6, #+136]
        CMP      R0,#+1
        BEQ.N    ??user_loop_22
        CMP      R0,#+2
        BEQ.N    ??user_loop_23
        B.N      ??user_loop_21
??user_loop_22:
        LDR.W    R0,??DataTable69
        B.N      ??user_loop_24
??user_loop_23:
        LDRB     R0,[R6, #+135]
        ADDS     R0,R0,#+1
        STRB     R0,[R6, #+135]
        LDRB     R0,[R6, #+135]
        CMP      R0,#+3
        BLT.N    ??user_loop_21
        LDR.W    R0,??DataTable69_1
??user_loop_24:
          CFI FunCall user_debug
        BL       user_debug
        MOVS     R0,#+0
        STRB     R0,[R6, #+133]
          CFI FunCall comm_channel_reopen
        BL       comm_channel_reopen
??user_loop_21:
        LDRB     R0,[R6, #+133]
        CMP      R0,#+0
        ITT      NE
        LDRBNE   R0,[R6, #+134]
        CMPNE    R0,#+0
        BEQ.N    ??user_loop_25
        SUBS     R0,R0,#+1
        STRB     R0,[R6, #+134]
        LDRB     R0,[R6, #+134]
        CBNZ.N   R0,??user_loop_25
        MOVS     R0,#+15
        STRB     R0,[R6, #+134]
        MOVS     R0,#+1
          CFI FunCall user_send_statusPack
        BL       user_send_statusPack
??user_loop_25:
        LDRB     R0,[R6, #+171]
        CBZ.N    R0,??user_loop_26
        MOVS     R0,#+0
        STRB     R0,[R6, #+171]
          CFI FunCall app_boot_request
        BL       app_boot_request
        CBZ.N    R0,??user_loop_27
        MOVS     R1,#+0
        MOVS     R0,#+135
          CFI FunCall fw_log
        BL       fw_log
          CFI FunCall user_savereset
        BL       user_savereset
        B.N      ??user_loop_26
??user_loop_27:
        BL       ?Subroutine53
??CrossCallReturnLabel_185:
        LDR.W    R0,??DataTable71
          CFI FunCall fw_end
        BL       fw_end
??user_loop_26:
        LDRH     R0,[R6, #+179]
        CMP      R0,#+0
        BEQ.N    ??user_loop_28
        SUBS     R0,R0,#+1
        STRH     R0,[R6, #+179]
        LDRH     R0,[R6, #+179]
        CMP      R0,#+0
        BNE.W    ??user_loop_12
        BL       ?Subroutine53
??CrossCallReturnLabel_186:
        LDR.W    R0,??DataTable72
          CFI FunCall fw_end
        BL       fw_end
??user_loop_28:
        B.N      ??user_loop_13
??user_loop_14:
        MOVS     R0,#+4
          CFI FunCall timers_get
        BL       timers_get
        MOVW     R1,#+10000
        CMP      R0,R1
        BCC.N    ??user_loop_29
        MOVS     R1,#+0
        MOVS     R0,#+4
          CFI FunCall timers_set
        BL       timers_set
??user_loop_29:
        MOVS     R0,#+5
          CFI FunCall timers_get
        BL       timers_get
        MOVW     R1,#+60000
        CMP      R0,R1
        BCC.N    ??user_loop_30
        MOVS     R1,#+0
        MOVS     R0,#+5
          CFI FunCall timers_set
        BL       timers_set
        LDRB     R0,[R5, #+169]
        CBZ.N    R0,??user_loop_30
        LDR      R1,[R6, #+51]
        LDRB     R2,[R5, #+169]
        MOV      R0,#+3600
        MULS     R2,R0,R2
        CMP      R1,R2
        BCC.N    ??user_loop_30
        MOVS     R0,#+32
          CFI FunCall log_addSystemEvent
        BL       log_addSystemEvent
        LDR.W    R0,??DataTable74
          CFI FunCall user_debug
        BL       user_debug
          CFI FunCall user_savereset
        BL       user_savereset
??user_loop_30:
        POP      {R0,R1,R4-R6,PC}
          CFI EndBlock cfiBlock74

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond75 Using cfiCommon0
          CFI Function user_loop
          CFI Conditional ??CrossCallReturnLabel_185
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
          CFI Block cfiCond76 Using cfiCommon0
          CFI (cfiCond76) Function user_loop
          CFI (cfiCond76) Conditional ??CrossCallReturnLabel_186
          CFI (cfiCond76) R4 Frame(CFA, -16)
          CFI (cfiCond76) R5 Frame(CFA, -12)
          CFI (cfiCond76) R6 Frame(CFA, -8)
          CFI (cfiCond76) R14 Frame(CFA, -4)
          CFI (cfiCond76) CFA R13+24
          CFI Block cfiPicker77 Using cfiCommon1
          CFI (cfiPicker77) NoFunction
          CFI (cfiPicker77) Picker
        THUMB
?Subroutine53:
        LDR.W    R0,??DataTable75
        LDRB     R1,[R0, #+0]
        ADDS     R2,R1,#+1
        STRB     R2,[R0, #+0]
        BX       LR
          CFI EndBlock cfiCond75
          CFI EndBlock cfiCond76
          CFI EndBlock cfiPicker77

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock78 Using cfiCommon0
          CFI Function user_initvariables
        THUMB
user_initvariables:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        LDR.W    R4,??DataTable56
        BL       ?Subroutine54
??CrossCallReturnLabel_187:
        MOV      R0,#+4294967295
        STRB     R0,[R4, #+127]
          CFI FunCall w25_read_devID
        BL       w25_read_devID
        STRH     R0,[R4, #+16]
        MOV      R0,#+2048
        STRH     R0,[R4, #+165]
          CFI FunCall user_readsavedinformation
        BL       user_readsavedinformation
        MOVS     R0,#+1
        STRB     R0,[R4, #+18]
          CFI FunCall user_comm_boot
        BL       user_comm_boot
          CFI FunCall user_getNumbersCount
        BL       user_getNumbersCount
        STRH     R0,[R4, #+81]
          CFI FunCall log_getRecordsCount
        BL       log_getRecordsCount
        STRH     R0,[R4, #+89]
        CBZ.N    R0,??user_initvariables_0
        LDRH     R0,[R4, #+129]
        ORR      R0,R0,#0x1000
        STRH     R0,[R4, #+129]
??user_initvariables_0:
          CFI FunCall user_getTempIDRecordsCount
        BL       user_getTempIDRecordsCount
        STRH     R0,[R4, #+83]
        CBZ.N    R0,??user_initvariables_1
        LDRH     R0,[R4, #+129]
        ORR      R0,R0,#0x4000
        STRH     R0,[R4, #+129]
??user_initvariables_1:
          CFI FunCall user_getPhotoCount
        BL       user_getPhotoCount
        STRH     R0,[R4, #+155]
        CBZ.N    R0,??user_initvariables_2
        LDRH     R0,[R4, #+129]
        ORR      R0,R0,#0x2000
        STRH     R0,[R4, #+129]
??user_initvariables_2:
        POP      {R4,PC}
          CFI EndBlock cfiBlock78

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock79 Using cfiCommon0
          CFI Function config_exchange
          CFI NoCalls
        THUMB
config_exchange:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        MOVS     R4,#+0
        LDR.N    R3,??DataTable54_2
??config_exchange_0:
        LDRB     R1,[R3, R4]
        LDRB     R2,[R0, R4]
        STRB     R2,[R3, R4]
        STRB     R1,[R0, R4]
        ADDS     R4,R4,#+1
        CMP      R4,#+512
        BCC.N    ??config_exchange_0
        POP      {R4,PC}
          CFI EndBlock cfiBlock79

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable48:
        DATA32
        DC32     server_rx_defer

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable48_1:
        DATA32
        DC32     server_rx_provisional

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock80 Using cfiCommon0
          CFI Function config_terminate_strings
        THUMB
config_terminate_strings:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOVS     R5,#+0
        LDR.N    R4,??DataTable54_2
        MOVS     R2,#+32
        MOVS     R1,#+0
        ADD      R0,R4,#+8
          CFI FunCall memchr
        BL       memchr
        CBNZ.N   R0,??config_terminate_strings_0
        STRB     R0,[R4, #+39]
        MOVS     R5,#+1
??config_terminate_strings_0:
        MOVS     R2,#+32
        MOVS     R1,#+0
        ADD      R0,R4,#+40
          CFI FunCall memchr
        BL       memchr
        CBNZ.N   R0,??config_terminate_strings_1
        STRB     R0,[R4, #+71]
        MOVS     R5,#+1
??config_terminate_strings_1:
        MOVS     R2,#+32
        MOVS     R1,#+0
        ADD      R0,R4,#+72
          CFI FunCall memchr
        BL       memchr
        CBNZ.N   R0,??config_terminate_strings_2
        STRB     R0,[R4, #+103]
        MOVS     R5,#+1
??config_terminate_strings_2:
        MOVS     R2,#+32
        MOVS     R1,#+0
        ADD      R0,R4,#+104
          CFI FunCall memchr
        BL       memchr
        CBNZ.N   R0,??config_terminate_strings_3
        STRB     R0,[R4, #+135]
        MOVS     R5,#+1
??config_terminate_strings_3:
        MOVS     R2,#+8
        MOVS     R1,#+0
        ADD      R0,R4,#+137
          CFI FunCall memchr
        BL       memchr
        CBNZ.N   R0,??config_terminate_strings_4
        STRB     R0,[R4, #+144]
        MOVS     R5,#+1
??config_terminate_strings_4:
        MOVS     R2,#+16
        MOVS     R1,#+0
        ADD      R0,R4,#+145
          CFI FunCall memchr
        BL       memchr
        CBNZ.N   R0,??config_terminate_strings_5
        STRB     R0,[R4, #+160]
        MOVS     R5,#+1
??config_terminate_strings_5:
        MOVS     R2,#+8
        MOVS     R1,#+0
        ADD      R0,R4,#+161
          CFI FunCall memchr
        BL       memchr
        CBNZ.N   R0,??config_terminate_strings_6
        STRB     R0,[R4, #+168]
        B.N      ??config_terminate_strings_7
??config_terminate_strings_6:
        CBZ.N    R5,??config_terminate_strings_8
??config_terminate_strings_7:
        BL       ??Subroutine24_0
??CrossCallReturnLabel_260:
        STRH     R0,[R4, #+510]
??config_terminate_strings_8:
        POP      {R0,R4,R5,PC}
          CFI EndBlock cfiBlock80

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock81 Using cfiCommon0
          CFI Function user_readconfig
        THUMB
user_readconfig:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOVS     R5,#+20
        LDR.W    R0,??DataTable76
          CFI FunCall user_debug
        BL       user_debug
        LDR.N    R4,??DataTable54_2
??user_readconfig_0:
          CFI FunCall wdt_reload
        BL       wdt_reload
        BL       ?Subroutine27
??CrossCallReturnLabel_86:
          CFI FunCall mem_read
        BL       mem_read
        BL       ??Subroutine24_0
??CrossCallReturnLabel_259:
        LDRH     R1,[R4, #+510]
        CMP      R1,R0
        BEQ.N    ??user_readconfig_1
        MOVS     R0,#+10
          CFI FunCall timers_pause
        BL       timers_pause
        SUBS     R5,R5,#+1
        UXTB     R0,R5
        CMP      R0,#+0
        BNE.N    ??user_readconfig_0
        B.N      ??user_readconfig_2
??user_readconfig_1:
        UXTB     R5,R5
        CBNZ.N   R5,??user_readconfig_3
??user_readconfig_2:
          CFI FunCall user_setdefault
        BL       user_setdefault
        POP      {R0,R4,R5,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall user_saveindex
        B.N      user_saveindex
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
??user_readconfig_3:
        POP      {R0,R4,R5,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall config_terminate_strings
        B.N      config_terminate_strings
          CFI EndBlock cfiBlock81

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond82 Using cfiCommon0
          CFI Function user_readconfig
          CFI Conditional ??CrossCallReturnLabel_86
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
          CFI Block cfiCond83 Using cfiCommon0
          CFI (cfiCond83) Function user_writeconfig
          CFI (cfiCond83) Conditional ??CrossCallReturnLabel_87
          CFI (cfiCond83) R4 Frame(CFA, -8)
          CFI (cfiCond83) R14 Frame(CFA, -4)
          CFI (cfiCond83) CFA R13+8
          CFI Block cfiPicker84 Using cfiCommon1
          CFI (cfiPicker84) NoFunction
          CFI (cfiPicker84) Picker
        THUMB
?Subroutine27:
        MOV      R2,#+512
          CFI Block cfiCond85 Using cfiCommon0
          CFI (cfiCond85) Function user_writeConfigAndDevice
          CFI (cfiCond85) Conditional ??CrossCallReturnLabel_88
          CFI (cfiCond85) R4 Frame(CFA, -12)
          CFI (cfiCond85) R5 Frame(CFA, -8)
          CFI (cfiCond85) R14 Frame(CFA, -4)
          CFI (cfiCond85) CFA R13+16
??Subroutine27_0:
        MOV      R1,R4
        LDR.W    R0,??DataTable63
        LDR      R0,[R0, #+48]
        BX       LR
          CFI EndBlock cfiCond82
          CFI EndBlock cfiCond83
          CFI EndBlock cfiPicker84
          CFI EndBlock cfiCond85

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock86 Using cfiCommon0
          CFI Function user_writeconfig
        THUMB
user_writeconfig:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        BL       ?Subroutine25
??CrossCallReturnLabel_78:
        CBZ.N    R0,??user_writeconfig_0
        MOVS     R1,#+1
        LDR.W    R0,??DataTable77
        STRB     R1,[R0, #+0]
        POP      {R4,PC}
??user_writeconfig_0:
          CFI FunCall wdt_reload
        BL       wdt_reload
        BL       ?Subroutine24
??CrossCallReturnLabel_262:
        STRH     R0,[R4, #+510]
        BL       ?Subroutine27
??CrossCallReturnLabel_87:
          CFI FunCall mem_patch
        BL       mem_patch
        POP      {R4,LR}
          CFI R4 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall wdt_reload
        B.W      wdt_reload
          CFI EndBlock cfiBlock86

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock87 Using cfiCommon0
          CFI Function user_setdefault
        THUMB
user_setdefault:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
          CFI FunCall wdt_reload
        BL       wdt_reload
        LDR.W    R0,??DataTable77_1
          CFI FunCall user_debug
        BL       user_debug
        LDR.N    R4,??DataTable54_2
        BL       ?Subroutine54
??CrossCallReturnLabel_188:
        LDR.W    R0,??DataTable77_2
        STR      R0,[R4, #+4]
        MOVS     R2,#+22
        LDR.W    R1,??DataTable77_3
        ADD      R0,R4,#+8
          CFI FunCall __aeabi_memcpy4
        BL       __aeabi_memcpy4
        MOVS     R2,#+22
        LDR.W    R1,??DataTable77_4
        ADD      R0,R4,#+40
          CFI FunCall __aeabi_memcpy4
        BL       __aeabi_memcpy4
        MOVS     R2,#+3
        ADR.N    R1,??DataTable59
        ADD      R0,R4,#+137
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        MOVS     R2,#+21
        LDR.W    R1,??DataTable77_5
        ADD      R0,R4,#+104
          CFI FunCall __aeabi_memcpy4
        BL       __aeabi_memcpy4
        MOVS     R0,#+0
        STRB     R0,[R4, #+136]
        MOVS     R0,#+1
        STRB     R0,[R4, #+219]
        MOVS     R0,#+0
        STRB     R0,[R4, #+196]
        MOVS     R0,#+1
        STRB     R0,[R4, #+197]
        STRB     R0,[R4, #+201]
        STRB     R0,[R4, #+203]
        MOVS     R0,#+3
        STRB     R0,[R4, #+204]
        MOVS     R0,#+1
        STRB     R0,[R4, #+205]
        LDR.N    R0,??DataTable56
        MOVS     R1,#+1
        STRB     R1,[R0, #+126]
        MOVS     R1,#+0
        STRB     R1,[R0, #+169]
        LDRH     R1,[R0, #+129]
        ORR      R1,R1,#0x8000
        STRH     R1,[R0, #+129]
        MOVS     R0,#+0
          CFI FunCall log_addSystemEvent
        BL       log_addSystemEvent
        POP      {R4,LR}
          CFI R4 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall user_writeConfigAndDevice
        B.N      user_writeConfigAndDevice
          CFI EndBlock cfiBlock87

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond88 Using cfiCommon0
          CFI Function user_initvariables
          CFI Conditional ??CrossCallReturnLabel_187
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI Block cfiCond89 Using cfiCommon0
          CFI (cfiCond89) Function user_setdefault
          CFI (cfiCond89) Conditional ??CrossCallReturnLabel_188
          CFI (cfiCond89) R4 Frame(CFA, -8)
          CFI (cfiCond89) R14 Frame(CFA, -4)
          CFI (cfiCond89) CFA R13+8
          CFI Block cfiPicker90 Using cfiCommon1
          CFI (cfiPicker90) NoFunction
          CFI (cfiPicker90) Picker
        THUMB
?Subroutine54:
        MOV      R1,#+512
        MOV      R0,R4
          CFI (cfiCond88) FunCall user_initvariables __aeabi_memclr4
          CFI (cfiCond89) FunCall user_setdefault __aeabi_memclr4
        B.W      __aeabi_memclr4
          CFI EndBlock cfiCond88
          CFI EndBlock cfiCond89
          CFI EndBlock cfiPicker90

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock91 Using cfiCommon0
          CFI Function user_factoryReset
        THUMB
user_factoryReset:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI FunCall fw_async_cancel
        BL       fw_async_cancel
        LDR.W    R0,??DataTable77_6
          CFI FunCall user_debug
        BL       user_debug
        MOVS     R0,#+10
          CFI FunCall upload_reset
        BL       upload_reset
        MOVS     R0,#+9
          CFI FunCall upload_reset
        BL       upload_reset
          CFI FunCall user_writeConfigAndDevice
        BL       user_writeConfigAndDevice
        CBZ.N    R0,??user_factoryReset_0
        MOVS     R0,#+2
          CFI FunCall user_clearMemBlock
        BL       user_clearMemBlock
        MOVS     R0,#+3
          CFI FunCall user_clearMemBlock
        BL       user_clearMemBlock
        MOVS     R0,#+4
          CFI FunCall user_clearMemBlock
        BL       user_clearMemBlock
        MOVS     R0,#+5
          CFI FunCall user_clearMemBlock
        BL       user_clearMemBlock
        MOVS     R0,#+7
          CFI FunCall user_clearMemBlock
        BL       user_clearMemBlock
        MOVS     R0,#+8
          CFI FunCall user_clearMemBlock
        BL       user_clearMemBlock
        MOVS     R0,#+9
          CFI FunCall user_clearMemBlock
        BL       user_clearMemBlock
        LDR.N    R1,??DataTable56
        MOVS     R0,#+0
        STRB     R0,[R1, #+164]
        LDRH     R2,[R1, #+129]
        MOVW     R0,#+63487
        ANDS     R2,R0,R2
        STRH     R2,[R1, #+129]
        MOVS     R0,#+0
        STRH     R0,[R1, #+179]
          CFI FunCall user_setdefault
        BL       user_setdefault
          CFI FunCall user_initvariables
        BL       user_initvariables
        POP      {R0,LR}
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall user_comm_boot
        B.W      user_comm_boot
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
??user_factoryReset_0:
        POP      {R0,PC}
          CFI EndBlock cfiBlock91

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable54:
        DATA32
        DC32     server_rx_buffer+4

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable54_1:
        DATA32
        DC32     ?_6

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable54_2:
        DATA32
        DC32     config

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock92 Using cfiCommon0
          CFI Function user_readsavedinformation
        THUMB
user_readsavedinformation:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        SUB      SP,SP,#+512
          CFI CFA R13+520
        MOVS     R4,#+10
        B.N      ??user_readsavedinformation_0
??user_readsavedinformation_1:
        MOVS     R0,#+10
          CFI FunCall timers_pause
        BL       timers_pause
??user_readsavedinformation_0:
        MOV      R0,R4
        SUBS     R4,R0,#+1
        UXTB     R0,R0
        CMP      R0,#+0
        BEQ.N    ??user_readsavedinformation_2
          CFI FunCall wdt_reload
        BL       wdt_reload
        MOV      R2,#+512
        MOV      R1,SP
        LDR.W    R0,??DataTable63
        LDR      R0,[R0, #+48]
        ADD      R0,R0,#+512
          CFI FunCall mem_read
        BL       mem_read
        MOV      R1,#+510
        MOV      R0,SP
          CFI FunCall crc16_calc
        BL       crc16_calc
        LDRH     R1,[SP, #+510]
        CMP      R1,R0
        BNE.N    ??user_readsavedinformation_1
        MOVS     R0,#+0
        STRB     R0,[SP, #+15]
        LDR.N    R4,??DataTable56
        MOV      R1,SP
        MOV      R0,R4
          CFI FunCall strcpy
        BL       strcpy
        MOVS     R0,#+1
        STRB     R0,[R4, #+18]
        LDRB     R0,[SP, #+19]
        STRB     R0,[R4, #+19]
        LDRH     R0,[SP, #+20]
        STRH     R0,[R4, #+20]
        LDRB     R0,[SP, #+22]
        STRB     R0,[R4, #+22]
        LDR      R0,[SP, #+23]
        STR      R0,[R4, #+23]
        LDR      R0,[SP, #+27]
        STR      R0,[R4, #+27]
        LDR      R0,[SP, #+31]
        STR      R0,[R4, #+31]
        LDR      R0,[SP, #+35]
        STR      R0,[R4, #+35]
        LDR      R0,[SP, #+39]
        STR      R0,[R4, #+39]
        LDR      R0,[SP, #+43]
        STR      R0,[R4, #+43]
        ADD      R0,R4,#+95
        ADD      R1,SP,#+95
        MOVS     R2,#+16
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        LDR      R0,[SP, #+140]
        STR      R0,[R4, #+140]
        LDR      R0,[SP, #+144]
        STR      R0,[R4, #+144]
        LDRH     R0,[SP, #+148]
        STRH     R0,[R4, #+148]
        LDRH     R0,[SP, #+172]
        STRH     R0,[R4, #+172]
        LDRB     R0,[SP, #+150]
        STRB     R0,[R4, #+150]
        LDRB     R0,[SP, #+164]
        STRB     R0,[R4, #+164]
        LDRH     R0,[SP, #+179]
        STRH     R0,[R4, #+179]
        LDRB     R0,[SP, #+169]
        STRB     R0,[R4, #+169]
        MOV      R0,SP
          CFI FunCall upload_restore
        BL       upload_restore
        MOV      R0,SP
          CFI FunCall store_restore
        BL       store_restore
??user_readsavedinformation_2:
          CFI FunCall fw_resume_after_boot
        BL       fw_resume_after_boot
        ADD      SP,SP,#+512
          CFI CFA R13+8
        POP      {R4,PC}
          CFI EndBlock cfiBlock92

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable55:
        DATA32
        DC32     ?_9

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable55_1:
        DATA32
        DC32     ?_11

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock93 Using cfiCommon0
          CFI Function user_savereset
        THUMB
user_savereset:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
        BL       ?Subroutine25
??CrossCallReturnLabel_79:
        CBZ.N    R0,??user_savereset_0
          CFI FunCall expiry_runtime_cancel
        BL       expiry_runtime_cancel
          CFI FunCall table_runtime_cancel
        BL       table_runtime_cancel
          CFI FunCall service_crc_cancel
        BL service_crc_cancel
          CFI FunCall photo_storage_cancel
        BL       photo_storage_cancel
        MOVS     R1,#+1
        LDR.W    R0,??DataTable80
        STRB     R1,[R0, #+0]
        POP      {R0,PC}
??user_savereset_0:
          CFI FunCall log_flushPending
        BL       log_flushPending
          CFI FunCall user_saveindex
        BL       user_saveindex
          CFI FunCall wdt_reload
        BL       wdt_reload
        BL       ?Subroutine34
??CrossCallReturnLabel_113:
        POP      {R0,LR}
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall NVIC_SystemReset
        B.W      NVIC_SystemReset
          CFI EndBlock cfiBlock93

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable56:
        DATA32
        DC32     device

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable56_1:
        DATA32
        DC32     ?_10

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable56_2:
        DATA32
        DC32     ?_13

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock94 Using cfiCommon0
          CFI Function user_saveindex
        THUMB
user_saveindex:
        LDR.W    R0,??DataTable64
        LDRB     R0,[R0, #+0]
        CMP      R0,#+0
          CFI FunCall user_writeConfigAndDevice
        IT       EQ
        BEQ.W    user_writeConfigAndDevice
        MOVS     R1,#+1
        LDR.W    R0,??DataTable77
        STRB     R1,[R0, #+0]
        BX       LR
          CFI EndBlock cfiBlock94

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable57:
        DATA32
        DC32     ?_15

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable57_1:
        DATA32
        DC32     ?_16

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock95 Using cfiCommon0
          CFI Function user_writeConfigAndDevice
        THUMB
user_writeConfigAndDevice:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
          CFI FunCall wdt_reload
        BL       wdt_reload
        BL       ?Subroutine24
??CrossCallReturnLabel_261:
        STRH     R0,[R4, #+510]
        LDR.N    R5,??DataTable61_10
        BL       ?Subroutine55
??CrossCallReturnLabel_190:
        STRH     R0,[R5, #+510]
        MOV      R0,#+512
        STR      R0,[SP, #+0]
        MOV      R3,R5
        MOV      R2,R0
        BL       ??Subroutine27_0
??CrossCallReturnLabel_88:
          CFI FunCall mem_patch2
        BL       mem_patch2
        MOV      R4,R0
          CFI FunCall wdt_reload
        BL       wdt_reload
        MOV      R0,R4
        POP      {R1,R4,R5,PC}
          CFI EndBlock cfiBlock95

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable58:
        DATA32
        DC32     `user_smsreceived{2}{3}{11}{12}{13}{15}{16}{20}{21}{23}{24}{25}::net`

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable58_1:
        DATA32
        DC32     ?_17

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock96 Using cfiCommon0
          CFI Function port_check_data
        THUMB
port_check_data:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI FunCall user_storage_flash_ready
        BL       user_storage_flash_ready
        CBZ.N    R0,??port_check_data_0
          CFI FunCall photo_poll
        BL       photo_poll
        LDR.N    R0,??DataTable61_10
        LDRB     R0,[R0, #+116]
        CBNZ.N   R0,??port_check_data_0
          CFI FunCall uart_getstate
        BL       uart_getstate
        CBZ.N    R0,??port_check_data_0
        MOVS     R0,#+0
          CFI FunCall uart_isReceiving
        BL       uart_isReceiving
        CBNZ.N   R0,??port_check_data_0
        POP      {R1,LR}
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall uart_clear
        B.W      uart_clear
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
??port_check_data_0:
        POP      {R0,PC}
          CFI EndBlock cfiBlock96

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable59:
        DATA8
        DC8      0x2B, 0x37, 0x00, 0x00

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable59_1:
        DATA32
        DC32     ?_24

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable59_2:
        DATA32
        DC32     ?_26

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable59_3:
        DATA32
        DC32     ?_25

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable59_4:
        DATA32
        DC32     ?_27

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable59_5:
        DATA32
        DC32     ?_14

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock97 Using cfiCommon0
          CFI Function gsm_check_data
        THUMB
gsm_check_data:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        POP      {R1,LR}
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall user_comm_poll
        B.W      user_comm_poll
          CFI EndBlock cfiBlock97

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock98 Using cfiCommon0
          CFI Function user_calcCRC16InFlash
        THUMB
user_calcCRC16InFlash:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        SUB      SP,SP,#+1024
          CFI CFA R13+1048
        MOV      R5,R0
        MOVS     R4,R1
        MOVW     R7,#+65535
        B.N      ??user_calcCRC16InFlash_0
??user_calcCRC16InFlash_1:
        MOVW     R0,#+1025
        CMP      R4,R0
        ITE      CC
        MOVCC    R6,R4
        MOVCS    R6,#+1024
        UXTH     R2,R6
        BL       ?Subroutine59
??CrossCallReturnLabel_209:
        MOV      R2,R7
        UXTH     R1,R6
        MOV      R0,SP
          CFI FunCall crc16_calcInit
        BL       crc16_calcInit
        MOV      R7,R0
        UXTH     R0,R6
        ADDS     R5,R0,R5
        UXTH     R6,R6
        SUBS     R4,R4,R6
??user_calcCRC16InFlash_0:
        BNE.N    ??user_calcCRC16InFlash_1
        MOV      R0,R7
        B.W      ?Subroutine2
          CFI EndBlock cfiBlock98

        SECTION `.data`:DATA:REORDER:NOROOT(2)
        DATA
id_used_bytes:
        DATA32
        DC32 4'294'967'295

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock99 Using cfiCommon0
          CFI Function id_crc16
        THUMB
id_crc16:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R5,R1
        MOVS     R4,#+0
        LDR.W    R2,??DataTable82
        LDR      R1,[R2, #+0]
        CMP      R0,R1
        BCS.N    ??id_crc16_0
        LDR      R4,[R2, #+0]
        SUBS     R4,R4,R0
        CMP      R4,R5
        IT       HI
        MOVHI    R4,R5
        CBZ.N    R4,??id_crc16_0
        MOV      R1,R4
        LDR.N    R2,??DataTable63
        LDR      R2,[R2, #+16]
        ADDS     R0,R0,R2
          CFI FunCall user_calcCRC16InFlash
        BL       user_calcCRC16InFlash
        B.N      ??id_crc16_1
??id_crc16_0:
        MOVW     R0,#+65535
??id_crc16_1:
        MOV      R1,R0
        SUBS     R0,R5,R4
        POP      {R2,R4,R5,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall crc16_ffInit
        B.W      crc16_ffInit
          CFI EndBlock cfiBlock99

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable60:
        DATA32
        DC32     ?_28

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable60_1:
        DATA32
        DC32     ?_29

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable60_2:
        DATA32
        DC32     ?_33

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock100 Using cfiCommon0
          CFI Function user_comm_acquire
        THUMB
user_comm_acquire:
        PUSH     {R2-R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+16
        MOV      R4,R0
        MOV      R0,SP
          CFI FunCall comm_tx_acquire
        BL       comm_tx_acquire
        CBZ.N    R0,??user_comm_acquire_0
        LDRH     R1,[SP, #+0]
        CMP      R1,R4
        BCS.N    ??user_comm_acquire_0
          CFI FunCall comm_tx_release
        BL       comm_tx_release
        MOVS     R0,#+0
??user_comm_acquire_0:
        POP      {R1,R2,R4,PC}
          CFI EndBlock cfiBlock100

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock101 Using cfiCommon0
          CFI Function user_send_anyData
        THUMB
user_send_anyData:
        PUSH     {R3-R11,LR}
          CFI R14 Frame(CFA, -4)
          CFI R11 Frame(CFA, -8)
          CFI R10 Frame(CFA, -12)
          CFI R9 Frame(CFA, -16)
          CFI R8 Frame(CFA, -20)
          CFI R7 Frame(CFA, -24)
          CFI R6 Frame(CFA, -28)
          CFI R5 Frame(CFA, -32)
          CFI R4 Frame(CFA, -36)
          CFI CFA R13+40
        MOV      R9,R0
        MOV      R8,R1
        MOV      R7,R2
        MOV      R11,R3
        MOVW     R0,#+65525
        CMP      R7,R0
        BCS.N    ??user_send_anyData_0
        CBZ.N    R7,??user_send_anyData_1
        CMP      R8,#+0
        BEQ.N    ??user_send_anyData_0
??user_send_anyData_1:
        ADDS     R5,R7,#+3
        UXTH     R5,R5
        CMP      R11,#+1
        ADD      R0,R5,#+8
        BNE.N    ??user_send_anyData_2
        BL       ??Subroutine63_0
??CrossCallReturnLabel_219:
        B.N      ??user_send_anyData_3
??user_send_anyData_2:
          CFI FunCall malloc
        BL       malloc
??user_send_anyData_3:
        MOVS     R4,R0
        BEQ.N    ??user_send_anyData_0
        LDR      R6,[SP, #+40]
        LDR.W    R10,??DataTable82_1
        MOV      R2,R9
        LDR.W    R1,??DataTable82_2
        MOV      R0,R10
          CFI FunCall sprintf
        BL       sprintf
        MOV      R0,R10
          CFI FunCall user_debug
        BL       user_debug
        BL       ?Subroutine38
??CrossCallReturnLabel_123:
        STRB     R5,[R4, #+4]
        LSRS     R0,R5,#+8
        STRB     R0,[R4, #+5]
        MOVS     R0,#+255
        STRB     R0,[R4, #+6]
        STRB     R6,[R4, #+7]
        STRB     R9,[R4, #+8]
        CBZ.N    R7,??CrossCallReturnLabel_139
        MOV      R2,R7
        MOV      R1,R8
        BL       ?Subroutine40
??CrossCallReturnLabel_139:
        ADDS     R1,R5,#+2
        BL       ?Subroutine36
??CrossCallReturnLabel_120:
        ADDS     R1,R4,R5
        BL       ??Subroutine64_0
??CrossCallReturnLabel_224:
        MOVS     R0,R11
        BEQ.N    ??user_send_anyData_4
        CMP      R11,#+1
        BEQ.N    ??user_send_anyData_5
        B.N      ??user_send_anyData_6
??user_send_anyData_4:
        ADDS     R5,R5,#+8
        UXTH     R1,R5
        BL       ??Subroutine22_0
??CrossCallReturnLabel_67:
        MOV      R0,R4
        POP      {R1,R4-R11,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R7 SameValue
          CFI R8 SameValue
          CFI R9 SameValue
          CFI R10 SameValue
          CFI R11 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall free
        B.W      free
          CFI R4 Frame(CFA, -36)
          CFI R5 Frame(CFA, -32)
          CFI R6 Frame(CFA, -28)
          CFI R7 Frame(CFA, -24)
          CFI R8 Frame(CFA, -20)
          CFI R9 Frame(CFA, -16)
          CFI R10 Frame(CFA, -12)
          CFI R11 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+40
??user_send_anyData_0:
        POP      {R0,R4-R11,PC}
??user_send_anyData_5:
        ADDS     R5,R5,#+8
        UXTH     R0,R5
          CFI FunCall comm_tx_commit
        BL       comm_tx_commit
        B.N      ??user_send_anyData_7
??user_send_anyData_6:
        BNE.N    ??CrossCallReturnLabel_67
??user_send_anyData_7:
        POP      {R0,R4-R11,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R7 SameValue
          CFI R8 SameValue
          CFI R9 SameValue
          CFI R10 SameValue
          CFI R11 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall comm_tx_release
        B.W      comm_tx_release
          CFI EndBlock cfiBlock101

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable61:
        DATA32
        DC32     ?_30

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable61_1:
        DATA32
        DC32     ?_8

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable61_2:
        DATA32
        DC32     ?_31

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable61_3:
        DATA32
        DC32     ?_32

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable61_4:
        DATA32
        DC32     0x187000

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable61_5:
        DATA32
        DC32     0x1c9000

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable61_6:
        DATA32
        DC32     0x1ca800

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable61_7:
        DATA32
        DC32     0x1cd400

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable61_8:
        DATA32
        DC32     0x1cf400

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable61_9:
        DATA32
        DC32     0x1e8400

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable61_10:
        DATA32
        DC32     device

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock102 Using cfiCommon0
          CFI Function user_send_registrationData
        THUMB
user_send_registrationData:
        PUSH     {R3-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+20
        SUB      SP,SP,#+52
          CFI CFA R13+72
        MOV      R6,R0
        CMP      R0,#+1
        BNE.N    ??user_send_registrationData_0
        MOVS     R0,#+48
          CFI FunCall user_comm_acquire
        BL       user_comm_acquire
        MOV      R4,R0
        B.N      ??user_send_registrationData_1
??user_send_registrationData_0:
        ADD      R4,SP,#+4
??user_send_registrationData_1:
        CMP      R4,#+0
        BEQ.N    ??user_send_registrationData_2
        LDR.W    R0,??DataTable82_3
          CFI FunCall user_debug
        BL       user_debug
        BL       ?Subroutine38
??CrossCallReturnLabel_124:
        MOVS     R0,#+40
        BL       ?Subroutine65
??CrossCallReturnLabel_227:
        MOVS     R0,#+5
        BL       ?Subroutine39
??CrossCallReturnLabel_136:
        MOVS     R0,#+1
        STRB     R0,[R4, #+8]
        LDR.W    R5,??DataTable83
        MOVS     R2,#+15
        MOV      R1,R5
        BL       ?Subroutine40
??CrossCallReturnLabel_140:
        LDRB     R0,[R5, #+18]
        STRB     R0,[R4, #+24]
        LDRB     R0,[R5, #+19]
        STRB     R0,[R4, #+25]
        LDR      R0,[R5, #+31]
        STR      R0,[SP, #+0]
        ADD      R3,R4,#+26
        STR      R0,[R3, #+0]
        LDR      R0,[R5, #+39]
        STR      R0,[SP, #+0]
        ADD      R0,R4,#+30
        LDR      R2,[SP, #+0]
        STR      R2,[R0, #+0]
        LDR      R0,[R5, #+35]
        STR      R0,[SP, #+0]
        ADD      R0,R4,#+34
        LDR      R2,[SP, #+0]
        STR      R2,[R0, #+0]
        LDR      R0,[R5, #+27]
        STR      R0,[SP, #+0]
        ADD      R0,R4,#+38
        LDR      R2,[SP, #+0]
        STR      R2,[R0, #+0]
        LDR      R0,[R5, #+43]
        STR      R0,[SP, #+0]
        ADD      R0,R4,#+42
        LDR      R2,[SP, #+0]
        STR      R2,[R0, #+0]
        MOVS     R1,#+42
        BL       ??Subroutine56_0
??CrossCallReturnLabel_200:
        STRB     R0,[R4, #+46]
        LSRS     R0,R0,#+8
        STRB     R0,[R4, #+47]
        CBZ.N    R6,??user_send_registrationData_3
        CMP      R6,#+1
        BEQ.N    ??user_send_registrationData_4
        B.N      ??user_send_registrationData_2
??user_send_registrationData_3:
        MOVS     R1,#+48
        BL       ??Subroutine22_0
??CrossCallReturnLabel_66:
        B.N      ??user_send_registrationData_2
??user_send_registrationData_4:
        MOVS     R0,#+1
        STRB     R0,[R5, #+136]
        MOVS     R0,#+5
        STRB     R0,[R5, #+137]
        MOVS     R0,#+48
          CFI FunCall comm_tx_commit
        BL       comm_tx_commit
??user_send_registrationData_2:
        ADD      SP,SP,#+56
          CFI CFA R13+16
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock102

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable62:
        DATA32
        DC32     ?_34

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable62_1:
        DATA32
        DC32     ?_35

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond103 Using cfiCommon0
          CFI Function user_send_registrationData
          CFI Conditional ??CrossCallReturnLabel_136
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+72
          CFI Block cfiCond104 Using cfiCommon0
          CFI (cfiCond104) Function user_send_statusPack
          CFI (cfiCond104) Conditional ??CrossCallReturnLabel_137
          CFI (cfiCond104) R4 Frame(CFA, -16)
          CFI (cfiCond104) R5 Frame(CFA, -12)
          CFI (cfiCond104) R6 Frame(CFA, -8)
          CFI (cfiCond104) R14 Frame(CFA, -4)
          CFI (cfiCond104) CFA R13+48
          CFI Block cfiPicker105 Using cfiCommon1
          CFI (cfiPicker105) NoFunction
          CFI (cfiPicker105) Picker
        THUMB
?Subroutine39:
        STRB     R0,[R4, #+6]
          CFI Block cfiCond106 Using cfiCommon0
          CFI (cfiCond106) Function user_send_smsToServer
          CFI (cfiCond106) Conditional ??CrossCallReturnLabel_138
          CFI (cfiCond106) R4 Frame(CFA, -16)
          CFI (cfiCond106) R5 Frame(CFA, -12)
          CFI (cfiCond106) R6 Frame(CFA, -8)
          CFI (cfiCond106) R14 Frame(CFA, -4)
          CFI (cfiCond106) CFA R13+24
??Subroutine39_0:
        LDR.W    R0,??DataTable75
        LDRB     R1,[R0, #+0]
        STRB     R1,[R4, #+7]
        LDRB     R1,[R0, #+0]
        ADDS     R1,R1,#+1
        STRB     R1,[R0, #+0]
        BX       LR
          CFI EndBlock cfiCond103
          CFI EndBlock cfiCond104
          CFI EndBlock cfiPicker105
          CFI EndBlock cfiCond106

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable63:
        DATA32
        DC32     mem

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable63_1:
        DATA32
        DC32     0x40006c04

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable63_2:
        DATA32
        DC32     sync_pending

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock107 Using cfiCommon0
          CFI Function user_send_statusPack
        THUMB
user_send_statusPack:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        SUB      SP,SP,#+32
          CFI CFA R13+48
        MOV      R6,R0
        CMP      R0,#+1
        BNE.N    ??user_send_statusPack_0
        MOVS     R0,#+26
          CFI FunCall user_comm_acquire
        BL       user_comm_acquire
        MOV      R4,R0
        B.N      ??user_send_statusPack_1
??user_send_statusPack_0:
        ADD      R4,SP,#+4
??user_send_statusPack_1:
        CMP      R4,#+0
        BEQ.N    ??user_send_statusPack_2
        LDR.W    R0,??DataTable84
          CFI FunCall user_debug
        BL       user_debug
        BL       ?Subroutine38
??CrossCallReturnLabel_125:
        MOVS     R0,#+18
        BL       ?Subroutine65
??CrossCallReturnLabel_228:
        MOVS     R0,#+15
        BL       ?Subroutine39
??CrossCallReturnLabel_137:
        MOVS     R0,#+2
        STRB     R0,[R4, #+8]
        LDR.W    R5,??DataTable83
        LDR      R0,[R5, #+47]
        STR      R0,[SP, #+0]
        ADD      R0,R4,#+9
        LDR      R1,[SP, #+0]
        STR      R1,[R0, #+0]
        ADD      R3,R5,#+129
        ADD      R0,R4,#+13
        BL       ?Subroutine14
??CrossCallReturnLabel_24:
        LDRH     R0,[R5, #+165]
        BL       ?Subroutine8
??CrossCallReturnLabel_8:
        MOVS     R1,#+7
        ADD      R0,R4,#+17
          CFI FunCall __aeabi_memclr
        BL       __aeabi_memclr
        MOVS     R1,#+20
        BL       ??Subroutine56_0
??CrossCallReturnLabel_199:
        STRB     R0,[R4, #+24]
        LSRS     R0,R0,#+8
        STRB     R0,[R4, #+25]
        CBZ.N    R6,??user_send_statusPack_3
        CMP      R6,#+1
        BEQ.N    ??user_send_statusPack_4
        B.N      ??user_send_statusPack_2
??user_send_statusPack_3:
        MOVS     R1,#+26
        BL       ??Subroutine22_0
??CrossCallReturnLabel_65:
        B.N      ??user_send_statusPack_2
??user_send_statusPack_4:
        LDRB     R0,[R5, #+136]
        CMP      R0,#+2
        BNE.N    ??user_send_statusPack_5
        LDRB     R0,[R5, #+137]
        CBNZ.N   R0,??user_send_statusPack_6
??user_send_statusPack_5:
        MOVS     R0,#+2
        STRB     R0,[R5, #+136]
        MOVS     R0,#+15
        STRB     R0,[R5, #+137]
??user_send_statusPack_6:
        LDRB     R0,[R4, #+7]
          CFI FunCall comm_server_status_sent
        BL       comm_server_status_sent
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        MOV      R1,R0
        LDRB     R0,[R4, #+13]
        LDRB     R2,[R4, #+14]
        ORR      R0,R0,R2, LSL #+8
          CFI FunCall status_event_sent
        BL       status_event_sent
        MOVS     R0,#+26
          CFI FunCall comm_tx_commit
        BL       comm_tx_commit
??user_send_statusPack_2:
        ADD      SP,SP,#+32
          CFI CFA R13+16
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock107

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable64:
        DATA32
        DC32     store_runtime

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable64_1:
        DATA32
        DC32     ?_36

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable64_2:
        DATA32
        DC32     sync_quiet

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock108 Using cfiCommon0
          CFI Function user_send_statusPackSecond
        THUMB
user_send_statusPackSecond:
        PUSH     {R3-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+20
        SUB      SP,SP,#+36
          CFI CFA R13+56
        MOV      R5,R0
        MOV      R6,R1
        CMP      R0,#+1
        BNE.N    ??user_send_statusPackSecond_0
        MOVS     R0,#+32
          CFI FunCall user_comm_acquire
        BL       user_comm_acquire
        MOV      R4,R0
        B.N      ??user_send_statusPackSecond_1
??user_send_statusPackSecond_0:
        ADD      R4,SP,#+4
??user_send_statusPackSecond_1:
        CMP      R4,#+0
        BEQ.N    ??CrossCallReturnLabel_64
        LDR.W    R0,??DataTable90
          CFI FunCall user_debug
        BL       user_debug
        BL       ?Subroutine38
??CrossCallReturnLabel_126:
        MOVS     R0,#+24
        BL       ?Subroutine6
??CrossCallReturnLabel_2:
        MOVS     R0,#+3
        BL       ?Subroutine10
??CrossCallReturnLabel_14:
        LDR      R3,[SP, #+0]
        STR      R3,[R1, #+0]
        LDRB     R1,[R0, #+74]
        STRB     R1,[R4, #+13]
        LDRH     R1,[R0, #+77]
        STR      R1,[SP, #+0]
        ADD      R1,R4,#+14
        LDRB     R6,[SP, #+0]
        STRB     R6,[R1, #+0]
        LDRB     R6,[SP, #+1]
        STRB     R6,[R1, #+1]
        LDRH     R1,[R0, #+20]
        STRB     R1,[R4, #+16]
        LDR      R1,[R0, #+23]
        STR      R1,[SP, #+0]
        ADD      R1,R4,#+17
        LDR      R3,[SP, #+0]
        STR      R3,[R1, #+0]
        LDRB     R1,[R0, #+22]
        STRB     R1,[R4, #+21]
        ADD      R3,R0,#+117
        ADD      R0,R4,#+22
        LDR      R2,[R3, #+0]
        STR      R2,[R0, #+0]
        LDR      R2,[R3, #+4]
        STR      R2,[R0, #+4]
        MOVS     R1,#+26
        BL       ??Subroutine56_0
??CrossCallReturnLabel_198:
        STRB     R0,[R4, #+30]
        LSRS     R0,R0,#+8
        STRB     R0,[R4, #+31]
        CBZ.N    R5,??user_send_statusPackSecond_2
        CMP      R5,#+1
          CFI FunCall comm_tx_commit
        ITT      EQ
        MOVEQ    R0,#+32
        BLEQ     comm_tx_commit
        B.N      ??CrossCallReturnLabel_64
??user_send_statusPackSecond_2:
        MOVS     R1,#+32
        BL       ??Subroutine22_0
??CrossCallReturnLabel_64:
        ADD      SP,SP,#+40
          CFI CFA R13+16
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock108

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
fw_status_pending:
        DS8 4

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock109 Using cfiCommon0
          CFI Function fw_status_poll
        THUMB
fw_status_poll:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        LDR.W    R5,??DataTable90_1
        LDRB     R0,[R5, #+0]
        CBZ.N    R0,??fw_status_poll_0
          CFI FunCall comm_server_online
        BL       comm_server_online
        CBZ.N    R0,??fw_status_poll_0
        MOV      R0,SP
          CFI FunCall comm_tx_acquire
        BL       comm_tx_acquire
        MOVS     R4,R0
        BEQ.N    ??fw_status_poll_0
        LDR.W    R2,??DataTable90_2
        LDR      R1,[R2, #+0]
        STR      R1,[R4, #+0]
        BL       ?Subroutine7
??CrossCallReturnLabel_4:
        LDRB     R0,[R5, #+3]
        STRB     R0,[R4, #+7]
        MOVS     R0,#+4
        STRB     R0,[R4, #+8]
        LDRB     R0,[R5, #+1]
        STRB     R0,[R4, #+9]
        LDRB     R0,[R5, #+2]
        STRB     R0,[R4, #+10]
        BL       ?Subroutine47
??CrossCallReturnLabel_171:
        STRB     R0,[R4, #+11]
        LSRS     R0,R0,#+8
        STRB     R0,[R4, #+12]
        MOVS     R0,#+13
          CFI FunCall comm_tx_commit
        BL       comm_tx_commit
        CBZ.N    R0,??fw_status_poll_0
        MOVS     R0,#+0
        STRB     R0,[R5, #+0]
??fw_status_poll_0:
        POP      {R0,R4,R5,PC}
          CFI EndBlock cfiBlock109

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond110 Using cfiCommon0
          CFI Function fw_status_poll
          CFI Conditional ??CrossCallReturnLabel_4
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
          CFI Block cfiCond111 Using cfiCommon0
          CFI (cfiCond111) Function user_send_statusUpdateFW
          CFI (cfiCond111) Conditional ??CrossCallReturnLabel_5
          CFI (cfiCond111) R4 Frame(CFA, -20)
          CFI (cfiCond111) R5 Frame(CFA, -16)
          CFI (cfiCond111) R6 Frame(CFA, -12)
          CFI (cfiCond111) R7 Frame(CFA, -8)
          CFI (cfiCond111) R14 Frame(CFA, -4)
          CFI (cfiCond111) CFA R13+40
          CFI Block cfiPicker112 Using cfiCommon1
          CFI (cfiPicker112) NoFunction
          CFI (cfiPicker112) Picker
        THUMB
?Subroutine7:
        MOVS     R0,#+5
          CFI Block cfiCond113 Using cfiCommon0
          CFI (cfiCond113) Function user_send_timeshtamp
          CFI (cfiCond113) Conditional ??CrossCallReturnLabel_6
          CFI (cfiCond113) R4 Frame(CFA, -20)
          CFI (cfiCond113) R5 Frame(CFA, -16)
          CFI (cfiCond113) R6 Frame(CFA, -12)
          CFI (cfiCond113) R7 Frame(CFA, -8)
          CFI (cfiCond113) R14 Frame(CFA, -4)
          CFI (cfiCond113) CFA R13+48
??Subroutine7_0:
        STRB     R0,[R4, #+4]
          CFI Block cfiCond114 Using cfiCommon0
          CFI (cfiCond114) Function user_send_crcMemoryID
          CFI (cfiCond114) Conditional ??CrossCallReturnLabel_7
          CFI (cfiCond114) R4 Frame(CFA, -32)
          CFI (cfiCond114) R5 Frame(CFA, -28)
          CFI (cfiCond114) R6 Frame(CFA, -24)
          CFI (cfiCond114) R7 Frame(CFA, -20)
          CFI (cfiCond114) R8 Frame(CFA, -16)
          CFI (cfiCond114) R9 Frame(CFA, -12)
          CFI (cfiCond114) R10 Frame(CFA, -8)
          CFI (cfiCond114) R14 Frame(CFA, -4)
          CFI (cfiCond114) CFA R13+40
??Subroutine7_1:
        MOVS     R0,#+0
        STRB     R0,[R4, #+5]
        MOVS     R0,#+255
        STRB     R0,[R4, #+6]
        BX       LR
          CFI EndBlock cfiCond110
          CFI EndBlock cfiCond111
          CFI EndBlock cfiPicker112
          CFI EndBlock cfiCond113
          CFI EndBlock cfiCond114

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock115 Using cfiCommon0
          CFI Function user_send_statusUpdateFW
        THUMB
user_send_statusUpdateFW:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        SUB      SP,SP,#+16
          CFI CFA R13+40
        MOV      R5,R1
        MOV      R6,R2
        MOV      R7,R3
        CMP      R6,#+1
        BNE.N    ??user_send_statusUpdateFW_0
        MOVS     R0,#+13
          CFI FunCall user_comm_acquire
        BL       user_comm_acquire
        MOV      R4,R0
        B.N      ??user_send_statusUpdateFW_1
??user_send_statusUpdateFW_0:
        MOV      R4,SP
??user_send_statusUpdateFW_1:
        CBNZ.N   R4,??user_send_statusUpdateFW_2
        CMP      R6,#+1
        BNE.N    ??CrossCallReturnLabel_63
        LDR.W    R0,??DataTable90_1
        MOVS     R1,#+1
        STRB     R1,[R0, #+0]
        LDR.W    R1,??DataTable83
        LDRB     R1,[R1, #+150]
        STRB     R1,[R0, #+1]
        STRB     R5,[R0, #+2]
        STRB     R7,[R0, #+3]
        B.N      ??CrossCallReturnLabel_63
??user_send_statusUpdateFW_2:
        LDR.W    R0,??DataTable91
          CFI FunCall user_debug
        BL       user_debug
        BL       ?Subroutine38
??CrossCallReturnLabel_127:
        BL       ?Subroutine7
??CrossCallReturnLabel_5:
        STRB     R7,[R4, #+7]
        MOVS     R0,#+4
        STRB     R0,[R4, #+8]
        LDR.W    R0,??DataTable83
        LDRB     R0,[R0, #+150]
        STRB     R0,[R4, #+9]
        STRB     R5,[R4, #+10]
        BL       ?Subroutine47
??CrossCallReturnLabel_170:
        STRB     R0,[R4, #+11]
        LSRS     R0,R0,#+8
        STRB     R0,[R4, #+12]
        CBZ.N    R6,??user_send_statusUpdateFW_3
        CMP      R6,#+1
          CFI FunCall comm_tx_commit
        ITT      EQ
        MOVEQ    R0,#+13
        BLEQ     comm_tx_commit
        B.N      ??CrossCallReturnLabel_63
??user_send_statusUpdateFW_3:
        MOVS     R1,#+13
        BL       ??Subroutine22_0
??CrossCallReturnLabel_63:
        ADD      SP,SP,#+20
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI EndBlock cfiBlock115

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable67:
        DATA32
        DC32     0x40010c10

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable67_1:
        DATA32
        DC32     0x40010c14

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable67_2:
        DATA32
        DC32     ?_43

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable67_3:
        DATA32
        DC32     0x40010c08

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond116 Using cfiCommon0
          CFI Function fw_status_poll
          CFI Conditional ??CrossCallReturnLabel_171
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
          CFI Block cfiCond117 Using cfiCommon0
          CFI (cfiCond117) Function user_send_statusUpdateFW
          CFI (cfiCond117) Conditional ??CrossCallReturnLabel_170
          CFI (cfiCond117) R4 Frame(CFA, -20)
          CFI (cfiCond117) R5 Frame(CFA, -16)
          CFI (cfiCond117) R6 Frame(CFA, -12)
          CFI (cfiCond117) R7 Frame(CFA, -8)
          CFI (cfiCond117) R14 Frame(CFA, -4)
          CFI (cfiCond117) CFA R13+40
          CFI Block cfiCond118 Using cfiCommon0
          CFI (cfiCond118) Function user_getFWblockFromServer
          CFI (cfiCond118) Conditional ??CrossCallReturnLabel_169
          CFI (cfiCond118) R4 Frame(CFA, -20)
          CFI (cfiCond118) R5 Frame(CFA, -16)
          CFI (cfiCond118) R6 Frame(CFA, -12)
          CFI (cfiCond118) R7 Frame(CFA, -8)
          CFI (cfiCond118) R14 Frame(CFA, -4)
          CFI (cfiCond118) CFA R13+40
          CFI Block cfiPicker119 Using cfiCommon1
          CFI (cfiPicker119) NoFunction
          CFI (cfiPicker119) Picker
        THUMB
?Subroutine47:
        MOVS     R1,#+7
        ADDS     R0,R4,#+4
          CFI (cfiCond116) FunCall fw_status_poll crc16_calc
          CFI (cfiCond117) FunCall user_send_statusUpdateFW crc16_calc
          CFI (cfiCond118) FunCall user_getFWblockFromServer crc16_calc
        B.W      crc16_calc
          CFI EndBlock cfiCond116
          CFI EndBlock cfiCond117
          CFI EndBlock cfiCond118
          CFI EndBlock cfiPicker119

        SECTION `.text`:CODE:NOROOT(2)
          CFI Block cfiBlock120 Using cfiCommon0
          CFI Function user_send_crcPack
        THUMB
user_send_crcPack:
        LDR R3,=store_runtime
        LDRB R3,[R3]
        CMP R3,#0
          CFI FunCall service_crc_begin_pack
        BNE.W service_crc_begin_pack
        PUSH     {R3-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+20
        SUB      SP,SP,#+28
          CFI CFA R13+48
        MOV      R5,R0
        MOV      R6,R1
        CMP      R0,#+1
        BNE.N    ??user_send_crcPack_0
        MOVS     R0,#+21
          CFI FunCall user_comm_acquire
        BL       user_comm_acquire
        MOV      R4,R0
        B.N      ??user_send_crcPack_1
??user_send_crcPack_0:
        ADD      R4,SP,#+4
??user_send_crcPack_1:
        CMP      R4,#+0
        BEQ.N    ??CrossCallReturnLabel_62
        LDR.W    R0,??DataTable94
          CFI FunCall user_debug
        BL       user_debug
        BL       ?Subroutine38
??CrossCallReturnLabel_128:
        MOVS     R0,#+13
        BL       ?Subroutine6
??CrossCallReturnLabel_3:
        MOVS     R0,#+5
        STRB     R0,[R4, #+8]
        LDR.W    R6,??DataTable95
        LDR      R1,[R6, #+20]
        MOVS     R0,#+0
          CFI FunCall id_crc16
        BL       id_crc16
        STRH     R0,[SP, #+0]
        ADD      R0,R4,#+9
        BL       ?Subroutine17
??CrossCallReturnLabel_33:
        LDR      R1,[R6, #+28]
        LDR      R0,[R6, #+24]
          CFI FunCall user_calcCRC16InFlash
        BL       user_calcCRC16InFlash
        STRH     R0,[SP, #+0]
        ADD      R0,R4,#+11
        BL       ?Subroutine17
??CrossCallReturnLabel_34:
        LDR      R1,[R6, #+36]
        LDR      R0,[R6, #+32]
          CFI FunCall user_calcCRC16InFlash
        BL       user_calcCRC16InFlash
        STRH     R0,[SP, #+0]
        ADD      R0,R4,#+13
        BL       ?Subroutine17
??CrossCallReturnLabel_35:
        LDR      R1,[R6, #+52]
        LDR      R0,[R6, #+48]
          CFI FunCall user_calcCRC16InFlash
        BL       user_calcCRC16InFlash
        STRH     R0,[SP, #+0]
        ADD      R0,R4,#+15
        BL       ?Subroutine17
??CrossCallReturnLabel_36:
        LDR      R1,[R6, #+44]
        LDR      R0,[R6, #+40]
          CFI FunCall user_calcCRC16InFlash
        BL       user_calcCRC16InFlash
        STRH     R0,[SP, #+0]
        ADD      R0,R4,#+17
        LDRB     R1,[SP, #+0]
        STRB     R1,[R0, #+0]
        LDRB     R1,[SP, #+1]
        STRB     R1,[R0, #+1]
        MOVS     R1,#+15
        BL       ??Subroutine56_0
??CrossCallReturnLabel_197:
        STRB     R0,[R4, #+19]
        LSRS     R0,R0,#+8
        STRB     R0,[R4, #+20]
        CBZ.N    R5,??user_send_crcPack_2
        CMP      R5,#+1
          CFI FunCall comm_tx_commit
        ITT      EQ
        MOVEQ    R0,#+21
        BLEQ     comm_tx_commit
        B.N      ??CrossCallReturnLabel_62
??user_send_crcPack_2:
        MOVS     R1,#+21
        BL       ??Subroutine22_0
??CrossCallReturnLabel_62:
        ADD      SP,SP,#+32
          CFI CFA R13+16
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock120

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable68:
        DATA32
        DC32     0x15180

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable68_1:
        DATA32
        DC32     ?_37

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable68_2:
        DATA32
        DC32     ?_38

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond121 Using cfiCommon0
          CFI Function user_send_statusPackSecond
          CFI Conditional ??CrossCallReturnLabel_2
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+56
          CFI Block cfiCond122 Using cfiCommon0
          CFI (cfiCond122) Function user_send_crcPack
          CFI (cfiCond122) Conditional ??CrossCallReturnLabel_3
          CFI (cfiCond122) R4 Frame(CFA, -16)
          CFI (cfiCond122) R5 Frame(CFA, -12)
          CFI (cfiCond122) R6 Frame(CFA, -8)
          CFI (cfiCond122) R14 Frame(CFA, -4)
          CFI (cfiCond122) CFA R13+48
          CFI Block cfiPicker123 Using cfiCommon1
          CFI (cfiPicker123) NoFunction
          CFI (cfiPicker123) Picker
        THUMB
?Subroutine6:
        STRB     R0,[R4, #+4]
        MOVS     R0,#+0
        STRB     R0,[R4, #+5]
        MOVS     R0,#+255
        STRB     R0,[R4, #+6]
        STRB     R6,[R4, #+7]
        BX       LR
          CFI EndBlock cfiCond121
          CFI EndBlock cfiCond122
          CFI EndBlock cfiPicker123

        SECTION `.text`:CODE:NOROOT(2)
          CFI Block cfiBlock124 Using cfiCommon0
          CFI Function user_send_crcMemoryID
        THUMB
user_send_crcMemoryID:
        LDR R3,=store_runtime
        LDRB R3,[R3]
        CMP R3,#0
          CFI FunCall service_crc_begin_id
        BNE.W service_crc_begin_id
        PUSH     {R2-R10,LR}
          CFI R14 Frame(CFA, -4)
          CFI R10 Frame(CFA, -8)
          CFI R9 Frame(CFA, -12)
          CFI R8 Frame(CFA, -16)
          CFI R7 Frame(CFA, -20)
          CFI R6 Frame(CFA, -24)
          CFI R5 Frame(CFA, -28)
          CFI R4 Frame(CFA, -32)
          CFI CFA R13+40
        MOV      R5,R0
        MOV      R10,R2
        LDR.W    R8,??DataTable95
        LDR      R9,[R8, #+16]
        CMP      R0,#+0
        ITE      EQ
        MOVEQ    R7,#+203
        MOVNE    R7,#+6
        MOV      R6,R1
        CMP      R6,#+1
        ADD      R0,R7,#+8
        BNE.N    ??user_send_crcMemoryID_0
          CFI FunCall user_comm_acquire
        BL       user_comm_acquire
        B.N      ??user_send_crcMemoryID_1
??user_send_crcMemoryID_0:
          CFI FunCall malloc
        BL       malloc
??user_send_crcMemoryID_1:
        MOVS     R4,R0
        BEQ.N    ??user_send_crcMemoryID_2
        BL       ?Subroutine37
??CrossCallReturnLabel_121:
        BL       ??Subroutine7_1
??CrossCallReturnLabel_7:
        STRB     R10,[R4, #+7]
        CBNZ.N   R5,??user_send_crcMemoryID_3
        MOVS     R0,#+6
        STRB     R0,[R4, #+8]
        LDR.W    R0,??DataTable97
          CFI FunCall user_debug
        BL       user_debug
??user_send_crcMemoryID_4:
        MOV      R1,#+16000
        LDR      R0,[R8, #+16]
        SUB      R0,R9,R0
          CFI FunCall id_crc16
        BL       id_crc16
        STRH     R0,[SP, #+0]
        UXTH     R0,R5
        ADD      R0,R4,R0, LSL #+1
        ADD      R3,R0,#+9
        LDRB     R1,[SP, #+0]
        STRB     R1,[R3, #+0]
        LDRB     R1,[SP, #+1]
        STRB     R1,[R3, #+1]
        ADD      R9,R9,#+16000
        ADDS     R5,R5,#+1
        UXTH     R0,R5
        CMP      R0,#+100
        BLT.N    ??user_send_crcMemoryID_4
        B.N      ??CrossCallReturnLabel_37
??user_send_crcMemoryID_3:
        MOVS     R0,#+15
        STRB     R0,[R4, #+8]
        LDR.W    R0,??DataTable99
          CFI FunCall user_debug
        BL       user_debug
        MOV      R1,#+16000
        SUBS     R0,R5,#+1
        MULS     R0,R0,R1
          CFI FunCall id_crc16
        BL       id_crc16
        STRH     R0,[SP, #+0]
        STRB     R5,[R4, #+9]
        ADD      R0,R4,#+10
        BL       ?Subroutine17
??CrossCallReturnLabel_37:
        ADDS     R1,R7,#+2
        BL       ??Subroutine56_0
??CrossCallReturnLabel_196:
        BL       ?Subroutine64
??CrossCallReturnLabel_222:
        MOVS     R0,R6
        BEQ.N    ??user_send_crcMemoryID_5
        CMP      R6,#+1
        BEQ.N    ??user_send_crcMemoryID_6
        B.N      ??user_send_crcMemoryID_7
??user_send_crcMemoryID_5:
        ADD      R1,R7,#+8
        BL       ??Subroutine22_0
??CrossCallReturnLabel_61:
        MOV      R0,R4
          CFI FunCall free
        BL       free
??user_send_crcMemoryID_2:
        POP      {R0,R1,R4-R10,PC}
??user_send_crcMemoryID_6:
        ADD      R0,R7,#+8
          CFI FunCall comm_tx_commit
        BL       comm_tx_commit
        B.N      ??user_send_crcMemoryID_8
??user_send_crcMemoryID_7:
        BNE.N    ??CrossCallReturnLabel_61
??user_send_crcMemoryID_8:
          CFI FunCall comm_tx_release
        BL       comm_tx_release
        B.N      ??user_send_crcMemoryID_2
          CFI EndBlock cfiBlock124

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable69:
        DATA32
        DC32     ?_39

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable69_1:
        DATA32
        DC32     ?_40

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond125 Using cfiCommon0
          CFI Function user_send_crcPack
          CFI Conditional ??CrossCallReturnLabel_33
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+48
          CFI Block cfiCond126 Using cfiCommon0
          CFI (cfiCond126) Function user_send_crcPack
          CFI (cfiCond126) Conditional ??CrossCallReturnLabel_34
          CFI (cfiCond126) R4 Frame(CFA, -16)
          CFI (cfiCond126) R5 Frame(CFA, -12)
          CFI (cfiCond126) R6 Frame(CFA, -8)
          CFI (cfiCond126) R14 Frame(CFA, -4)
          CFI (cfiCond126) CFA R13+48
          CFI Block cfiCond127 Using cfiCommon0
          CFI (cfiCond127) Function user_send_crcPack
          CFI (cfiCond127) Conditional ??CrossCallReturnLabel_35
          CFI (cfiCond127) R4 Frame(CFA, -16)
          CFI (cfiCond127) R5 Frame(CFA, -12)
          CFI (cfiCond127) R6 Frame(CFA, -8)
          CFI (cfiCond127) R14 Frame(CFA, -4)
          CFI (cfiCond127) CFA R13+48
          CFI Block cfiCond128 Using cfiCommon0
          CFI (cfiCond128) Function user_send_crcPack
          CFI (cfiCond128) Conditional ??CrossCallReturnLabel_36
          CFI (cfiCond128) R4 Frame(CFA, -16)
          CFI (cfiCond128) R5 Frame(CFA, -12)
          CFI (cfiCond128) R6 Frame(CFA, -8)
          CFI (cfiCond128) R14 Frame(CFA, -4)
          CFI (cfiCond128) CFA R13+48
          CFI Block cfiCond129 Using cfiCommon0
          CFI (cfiCond129) Function user_send_crcMemoryID
          CFI (cfiCond129) Conditional ??CrossCallReturnLabel_37
          CFI (cfiCond129) R4 Frame(CFA, -32)
          CFI (cfiCond129) R5 Frame(CFA, -28)
          CFI (cfiCond129) R6 Frame(CFA, -24)
          CFI (cfiCond129) R7 Frame(CFA, -20)
          CFI (cfiCond129) R8 Frame(CFA, -16)
          CFI (cfiCond129) R9 Frame(CFA, -12)
          CFI (cfiCond129) R10 Frame(CFA, -8)
          CFI (cfiCond129) R14 Frame(CFA, -4)
          CFI (cfiCond129) CFA R13+40
          CFI Block cfiCond130 Using cfiCommon0
          CFI (cfiCond130) Function log_addUpdateEvent
          CFI (cfiCond130) Conditional ??CrossCallReturnLabel_38
          CFI (cfiCond130) R4 Frame(CFA, -16)
          CFI (cfiCond130) R5 Frame(CFA, -12)
          CFI (cfiCond130) R6 Frame(CFA, -8)
          CFI (cfiCond130) R14 Frame(CFA, -4)
          CFI (cfiCond130) CFA R13+56
          CFI Block cfiPicker131 Using cfiCommon1
          CFI (cfiPicker131) NoFunction
          CFI (cfiPicker131) Picker
        THUMB
?Subroutine17:
        LDRB     R2,[SP, #+0]
        STRB     R2,[R0, #+0]
        LDRB     R2,[SP, #+1]
        STRB     R2,[R0, #+1]
        BX       LR
          CFI EndBlock cfiCond125
          CFI EndBlock cfiCond126
          CFI EndBlock cfiCond127
          CFI EndBlock cfiCond128
          CFI EndBlock cfiCond129
          CFI EndBlock cfiCond130
          CFI EndBlock cfiPicker131

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock132 Using cfiCommon0
          CFI Function user_send_smsToServer
        THUMB
user_send_smsToServer:
        PUSH     {R2-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+24
        MOV      R5,R0
          CFI FunCall comm_server_online
        BL       comm_server_online
        CBZ.N    R0,??user_send_smsToServer_0
        MOVS     R6,#+0
        MOV      R0,SP
          CFI FunCall comm_tx_acquire
        BL       comm_tx_acquire
        MOVS     R4,R0
        BEQ.N    ??user_send_smsToServer_0
        CBNZ.N   R5,??user_send_smsToServer_1
??user_send_smsToServer_2:
          CFI FunCall comm_tx_release
        BL       comm_tx_release
??user_send_smsToServer_0:
        MOVS     R0,#+0
        POP      {R1,R2,R4-R6,PC}
??user_send_smsToServer_3:
        ADDS     R6,R6,#+1
        UXTH     R6,R6
??user_send_smsToServer_1:
        LDRH     R1,[SP, #+0]
        ADD      R0,R6,#+11
        CMP      R1,R0
        BCC.N    ??user_send_smsToServer_2
        LDRB     R0,[R5, R6]
        CMP      R0,#+0
        BNE.N    ??user_send_smsToServer_3
        ADDS     R6,R6,#+3
        UXTH     R6,R6
        LDR.W    R0,??DataTable100
          CFI FunCall user_debug
        BL       user_debug
        BL       ?Subroutine30
??CrossCallReturnLabel_94:
        BL       ??Subroutine39_0
??CrossCallReturnLabel_138:
        MOVS     R0,#+7
        STRB     R0,[R4, #+8]
        MOV      R1,R5
        ADD      R0,R4,#+9
          CFI FunCall strcpy
        BL       strcpy
        ADDS     R1,R6,#+2
        BL       ?Subroutine36
??CrossCallReturnLabel_119:
        ADDS     R1,R4,R6
        BL       ??Subroutine64_0
??CrossCallReturnLabel_225:
        ADDS     R6,R6,#+8
        UXTH     R0,R6
          CFI FunCall comm_tx_commit
        BL       comm_tx_commit
        CMP      R0,#+0
        BEQ.N    ??user_send_smsToServer_2
        MOVS     R0,#+1
        POP      {R1,R2,R4-R6,PC}
          CFI EndBlock cfiBlock132

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond133 Using cfiCommon0
          CFI Function user_send_anyData
          CFI Conditional ??CrossCallReturnLabel_120
          CFI R4 Frame(CFA, -36)
          CFI R5 Frame(CFA, -32)
          CFI R6 Frame(CFA, -28)
          CFI R7 Frame(CFA, -24)
          CFI R8 Frame(CFA, -20)
          CFI R9 Frame(CFA, -16)
          CFI R10 Frame(CFA, -12)
          CFI R11 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+40
          CFI Block cfiCond134 Using cfiCommon0
          CFI (cfiCond134) Function user_send_smsToServer
          CFI (cfiCond134) Conditional ??CrossCallReturnLabel_119
          CFI (cfiCond134) R4 Frame(CFA, -16)
          CFI (cfiCond134) R5 Frame(CFA, -12)
          CFI (cfiCond134) R6 Frame(CFA, -8)
          CFI (cfiCond134) R14 Frame(CFA, -4)
          CFI (cfiCond134) CFA R13+24
          CFI Block cfiCond135 Using cfiCommon0
          CFI (cfiCond135) Function user_send_tempIDs
          CFI (cfiCond135) Conditional ??CrossCallReturnLabel_118
          CFI (cfiCond135) R4 Frame(CFA, -36)
          CFI (cfiCond135) R5 Frame(CFA, -32)
          CFI (cfiCond135) R6 Frame(CFA, -28)
          CFI (cfiCond135) R7 Frame(CFA, -24)
          CFI (cfiCond135) R8 Frame(CFA, -20)
          CFI (cfiCond135) R9 Frame(CFA, -16)
          CFI (cfiCond135) R10 Frame(CFA, -12)
          CFI (cfiCond135) R11 Frame(CFA, -8)
          CFI (cfiCond135) R14 Frame(CFA, -4)
          CFI (cfiCond135) CFA R13+48
          CFI Block cfiPicker136 Using cfiCommon1
          CFI (cfiPicker136) NoFunction
          CFI (cfiPicker136) Picker
        THUMB
?Subroutine36:
        UXTH     R1,R1
        ADDS     R0,R4,#+4
          CFI (cfiCond133) FunCall user_send_anyData crc16_calc
          CFI (cfiCond134) FunCall user_send_smsToServer crc16_calc
          CFI (cfiCond135) FunCall user_send_tempIDs crc16_calc
        B.W      crc16_calc
          CFI EndBlock cfiCond133
          CFI EndBlock cfiCond134
          CFI EndBlock cfiCond135
          CFI EndBlock cfiPicker136

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
unreg_sms_pending:
        DS8 12

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock137 Using cfiCommon0
          CFI Function unreg_sms_note
        THUMB
unreg_sms_note:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        MOV      R1,R0
        LDR.W    R4,??DataTable100_1
        MOVS     R2,#+10
        MOV      R0,R4
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        MOVS     R0,#+1
        STRB     R0,[R4, #+10]
        POP      {R4,PC}
          CFI EndBlock cfiBlock137

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable71:
        DATA32
        DC32     ?_41

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock138 Using cfiCommon0
          CFI Function unreg_sms_poll
        THUMB
unreg_sms_poll:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        SUB      SP,SP,#+32
          CFI CFA R13+40
        LDR.W    R4,??DataTable100_1
        LDRB     R0,[R4, #+10]
        CBZ.N    R0,??unreg_sms_poll_0
        MOVS     R2,#+10
        MOV      R1,R4
        MOV      R0,SP
          CFI FunCall __aeabi_memcpy4
        BL       __aeabi_memcpy4
        MOVS     R0,#+0
        STRB     R0,[SP, #+10]
        LDR.W    R1,??DataTable101
        MOV      R0,SP
          CFI FunCall strcat
        BL       strcat
          CFI FunCall user_send_smsToServer
        BL       user_send_smsToServer
        CBZ.N    R0,??unreg_sms_poll_0
        MOVS     R0,#+0
        STRB     R0,[R4, #+10]
??unreg_sms_poll_0:
        ADD      SP,SP,#+32
          CFI CFA R13+8
        POP      {R4,PC}
          CFI EndBlock cfiBlock138

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable72:
        DATA32
        DC32     ?_42

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
reader_out:
        DS8 12

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock139 Using cfiCommon0
          CFI Function user_send_readedIDtoServer
        THUMB
user_send_readedIDtoServer:
        LDR.W    R3,??DataTable101_1
        STRB     R1,[R3, #+0]
        CBNZ.N   R0,??user_send_readedIDtoServer_0
        MOVS     R1,#+10
        ADDS     R0,R3,#+1
          CFI FunCall __aeabi_memclr
        B.W      __aeabi_memclr
??user_send_readedIDtoServer_0:
        MOVS     R2,#+10
        MOV      R1,R0
        ADDS     R0,R3,#+1
          CFI FunCall __aeabi_memcpy
        B.W      __aeabi_memcpy
          CFI EndBlock cfiBlock139

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock140 Using cfiCommon0
          CFI Function reader_reply_poll
        THUMB
reader_reply_poll:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        LDR.W    R5,??DataTable101_1
        LDRB     R0,[R5, #+0]
        CBZ.N    R0,??reader_reply_poll_0
        MOVS     R0,#+22
          CFI FunCall user_comm_acquire
        BL       user_comm_acquire
        MOVS     R4,R0
        BEQ.N    ??reader_reply_poll_0
        BL       ?Subroutine38
??CrossCallReturnLabel_129:
        MOVS     R0,#+14
        BL       ?Subroutine5
??CrossCallReturnLabel_0:
        MOVS     R0,#+8
        STRB     R0,[R4, #+8]
        LDRB     R0,[R5, #+0]
        STRB     R0,[R4, #+9]
        MOVS     R2,#+10
        ADDS     R1,R5,#+1
        ADD      R0,R4,#+10
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        MOVS     R1,#+16
        BL       ??Subroutine56_0
??CrossCallReturnLabel_195:
        STRB     R0,[R4, #+20]
        LSRS     R0,R0,#+8
        STRB     R0,[R4, #+21]
        MOVS     R0,#+22
          CFI FunCall comm_tx_commit
        BL       comm_tx_commit
        CBZ.N    R0,??reader_reply_poll_0
        MOVS     R0,#+0
        STRB     R0,[R5, #+0]
??reader_reply_poll_0:
        POP      {R0,R4,R5,PC}
          CFI EndBlock cfiBlock140

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable74:
        DATA32
        DC32     ?_44

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond141 Using cfiCommon0
          CFI Function user_send_anyData
          CFI Conditional ??CrossCallReturnLabel_123
          CFI R4 Frame(CFA, -36)
          CFI R5 Frame(CFA, -32)
          CFI R6 Frame(CFA, -28)
          CFI R7 Frame(CFA, -24)
          CFI R8 Frame(CFA, -20)
          CFI R9 Frame(CFA, -16)
          CFI R10 Frame(CFA, -12)
          CFI R11 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+40
          CFI Block cfiCond142 Using cfiCommon0
          CFI (cfiCond142) Function user_send_registrationData
          CFI (cfiCond142) Conditional ??CrossCallReturnLabel_124
          CFI (cfiCond142) R4 Frame(CFA, -16)
          CFI (cfiCond142) R5 Frame(CFA, -12)
          CFI (cfiCond142) R6 Frame(CFA, -8)
          CFI (cfiCond142) R14 Frame(CFA, -4)
          CFI (cfiCond142) CFA R13+72
          CFI Block cfiCond143 Using cfiCommon0
          CFI (cfiCond143) Function user_send_statusPack
          CFI (cfiCond143) Conditional ??CrossCallReturnLabel_125
          CFI (cfiCond143) R4 Frame(CFA, -16)
          CFI (cfiCond143) R5 Frame(CFA, -12)
          CFI (cfiCond143) R6 Frame(CFA, -8)
          CFI (cfiCond143) R14 Frame(CFA, -4)
          CFI (cfiCond143) CFA R13+48
          CFI Block cfiCond144 Using cfiCommon0
          CFI (cfiCond144) Function user_send_statusPackSecond
          CFI (cfiCond144) Conditional ??CrossCallReturnLabel_126
          CFI (cfiCond144) R4 Frame(CFA, -16)
          CFI (cfiCond144) R5 Frame(CFA, -12)
          CFI (cfiCond144) R6 Frame(CFA, -8)
          CFI (cfiCond144) R14 Frame(CFA, -4)
          CFI (cfiCond144) CFA R13+56
          CFI Block cfiCond145 Using cfiCommon0
          CFI (cfiCond145) Function user_send_statusUpdateFW
          CFI (cfiCond145) Conditional ??CrossCallReturnLabel_127
          CFI (cfiCond145) R4 Frame(CFA, -20)
          CFI (cfiCond145) R5 Frame(CFA, -16)
          CFI (cfiCond145) R6 Frame(CFA, -12)
          CFI (cfiCond145) R7 Frame(CFA, -8)
          CFI (cfiCond145) R14 Frame(CFA, -4)
          CFI (cfiCond145) CFA R13+40
          CFI Block cfiCond146 Using cfiCommon0
          CFI (cfiCond146) Function user_send_crcPack
          CFI (cfiCond146) Conditional ??CrossCallReturnLabel_128
          CFI (cfiCond146) R4 Frame(CFA, -16)
          CFI (cfiCond146) R5 Frame(CFA, -12)
          CFI (cfiCond146) R6 Frame(CFA, -8)
          CFI (cfiCond146) R14 Frame(CFA, -4)
          CFI (cfiCond146) CFA R13+48
          CFI Block cfiCond147 Using cfiCommon0
          CFI (cfiCond147) Function reader_reply_poll
          CFI (cfiCond147) Conditional ??CrossCallReturnLabel_129
          CFI (cfiCond147) R4 Frame(CFA, -12)
          CFI (cfiCond147) R5 Frame(CFA, -8)
          CFI (cfiCond147) R14 Frame(CFA, -4)
          CFI (cfiCond147) CFA R13+16
          CFI Block cfiCond148 Using cfiCommon0
          CFI (cfiCond148) Function user_send_answer_comand
          CFI (cfiCond148) Conditional ??CrossCallReturnLabel_130
          CFI (cfiCond148) R4 Frame(CFA, -24)
          CFI (cfiCond148) R5 Frame(CFA, -20)
          CFI (cfiCond148) R6 Frame(CFA, -16)
          CFI (cfiCond148) R7 Frame(CFA, -12)
          CFI (cfiCond148) R8 Frame(CFA, -8)
          CFI (cfiCond148) R14 Frame(CFA, -4)
          CFI (cfiCond148) CFA R13+48
          CFI Block cfiCond149 Using cfiCommon0
          CFI (cfiCond149) Function user_send_timeshtamp
          CFI (cfiCond149) Conditional ??CrossCallReturnLabel_131
          CFI (cfiCond149) R4 Frame(CFA, -20)
          CFI (cfiCond149) R5 Frame(CFA, -16)
          CFI (cfiCond149) R6 Frame(CFA, -12)
          CFI (cfiCond149) R7 Frame(CFA, -8)
          CFI (cfiCond149) R14 Frame(CFA, -4)
          CFI (cfiCond149) CFA R13+48
          CFI Block cfiCond150 Using cfiCommon0
          CFI (cfiCond150) Function user_send_identPackToConfigurator
          CFI (cfiCond150) Conditional ??CrossCallReturnLabel_132
          CFI (cfiCond150) R4 Frame(CFA, -12)
          CFI (cfiCond150) R5 Frame(CFA, -8)
          CFI (cfiCond150) R14 Frame(CFA, -4)
          CFI (cfiCond150) CFA R13+40
          CFI Block cfiCond151 Using cfiCommon0
          CFI (cfiCond151) Function user_send_statusPackToConfigurator
          CFI (cfiCond151) Conditional ??CrossCallReturnLabel_133
          CFI (cfiCond151) R4 Frame(CFA, -20)
          CFI (cfiCond151) R5 Frame(CFA, -16)
          CFI (cfiCond151) R6 Frame(CFA, -12)
          CFI (cfiCond151) R7 Frame(CFA, -8)
          CFI (cfiCond151) R14 Frame(CFA, -4)
          CFI (cfiCond151) CFA R13+120
          CFI Block cfiCond152 Using cfiCommon0
          CFI (cfiCond152) Function user_send_configToConfigurator
          CFI (cfiCond152) Conditional ??CrossCallReturnLabel_134
          CFI (cfiCond152) R4 Frame(CFA, -12)
          CFI (cfiCond152) R5 Frame(CFA, -8)
          CFI (cfiCond152) R14 Frame(CFA, -4)
          CFI (cfiCond152) CFA R13+16
          CFI Block cfiCond153 Using cfiCommon0
          CFI (cfiCond153) Function user_getFWblockFromServer
          CFI (cfiCond153) Conditional ??CrossCallReturnLabel_135
          CFI (cfiCond153) R4 Frame(CFA, -20)
          CFI (cfiCond153) R5 Frame(CFA, -16)
          CFI (cfiCond153) R6 Frame(CFA, -12)
          CFI (cfiCond153) R7 Frame(CFA, -8)
          CFI (cfiCond153) R14 Frame(CFA, -4)
          CFI (cfiCond153) CFA R13+40
          CFI Block cfiPicker154 Using cfiCommon1
          CFI (cfiPicker154) NoFunction
          CFI (cfiPicker154) Picker
        THUMB
?Subroutine38:
        MOVS     R0,#+83
        STRB     R0,[R4, #+0]
        MOVS     R0,#+76
        STRB     R0,[R4, #+1]
        MOVS     R0,#+68
        STRB     R0,[R4, #+2]
        MOVS     R0,#+78
        STRB     R0,[R4, #+3]
        BX       LR
          CFI EndBlock cfiCond141
          CFI EndBlock cfiCond142
          CFI EndBlock cfiCond143
          CFI EndBlock cfiCond144
          CFI EndBlock cfiCond145
          CFI EndBlock cfiCond146
          CFI EndBlock cfiCond147
          CFI EndBlock cfiCond148
          CFI EndBlock cfiCond149
          CFI EndBlock cfiCond150
          CFI EndBlock cfiCond151
          CFI EndBlock cfiCond152
          CFI EndBlock cfiCond153
          CFI EndBlock cfiPicker154

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock155 Using cfiCommon0
          CFI Function user_send_answer_comand
        THUMB
user_send_answer_comand:
        PUSH     {R4-R8,LR}
          CFI R14 Frame(CFA, -4)
          CFI R8 Frame(CFA, -8)
          CFI R7 Frame(CFA, -12)
          CFI R6 Frame(CFA, -16)
          CFI R5 Frame(CFA, -20)
          CFI R4 Frame(CFA, -24)
          CFI CFA R13+24
        SUB      SP,SP,#+24
          CFI CFA R13+48
        MOV      R7,R0
        MOV      R6,R1
        MOV      R5,R2
        MOV      R8,R3
        CMP      R5,#+1
        BNE.N    ??user_send_answer_comand_0
        MOVS     R0,#+19
          CFI FunCall user_comm_acquire
        BL       user_comm_acquire
        MOV      R4,R0
        B.N      ??user_send_answer_comand_1
??user_send_answer_comand_0:
        ADD      R4,SP,#+4
??user_send_answer_comand_1:
        CMP      R4,#+0
        BEQ.N    ??CrossCallReturnLabel_60
        LDR.W    R0,??DataTable101_2
          CFI FunCall user_debug
        BL       user_debug
        BL       ?Subroutine38
??CrossCallReturnLabel_130:
        MOVS     R0,#+11
        BL       ?Subroutine65
??CrossCallReturnLabel_229:
        CMP      R7,#+7
        ITET     NE
        CMPNE    R7,#+21
        MOVEQ    R0,#+5
        MOVNE    R0,#+255
        STRB     R0,[R4, #+6]
        STRB     R8,[R4, #+7]
        MOVS     R0,#+11
        STRB     R0,[R4, #+8]
        STRB     R7,[R4, #+9]
        STRB     R6,[R4, #+10]
        LDR.W    R0,??DataTable102
        LDR      R1,[R0, #+47]
        STR      R1,[SP, #+0]
        ADD      R1,R4,#+11
        LDR      R3,[SP, #+0]
        STR      R3,[R1, #+0]
        LDRH     R0,[R0, #+165]
        BL       ?Subroutine8
??CrossCallReturnLabel_9:
        MOVS     R1,#+13
        BL       ??Subroutine56_0
??CrossCallReturnLabel_194:
        STRB     R0,[R4, #+17]
        LSRS     R0,R0,#+8
        STRB     R0,[R4, #+18]
        CBZ.N    R5,??user_send_answer_comand_2
        CMP      R5,#+1
          CFI FunCall comm_tx_commit
        ITT      EQ
        MOVEQ    R0,#+19
        BLEQ     comm_tx_commit
        B.N      ??CrossCallReturnLabel_60
??user_send_answer_comand_2:
        MOVS     R1,#+19
        BL       ??Subroutine22_0
??CrossCallReturnLabel_60:
        ADD      SP,SP,#+24
          CFI CFA R13+24
        POP      {R4-R8,PC}
          CFI EndBlock cfiBlock155

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable75:
        DATA32
        DC32     uniqIDCnt

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond156 Using cfiCommon0
          CFI Function user_send_registrationData
          CFI Conditional ??CrossCallReturnLabel_227
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+72
          CFI Block cfiCond157 Using cfiCommon0
          CFI (cfiCond157) Function user_send_statusPack
          CFI (cfiCond157) Conditional ??CrossCallReturnLabel_228
          CFI (cfiCond157) R4 Frame(CFA, -16)
          CFI (cfiCond157) R5 Frame(CFA, -12)
          CFI (cfiCond157) R6 Frame(CFA, -8)
          CFI (cfiCond157) R14 Frame(CFA, -4)
          CFI (cfiCond157) CFA R13+48
          CFI Block cfiCond158 Using cfiCommon0
          CFI (cfiCond158) Function user_send_answer_comand
          CFI (cfiCond158) Conditional ??CrossCallReturnLabel_229
          CFI (cfiCond158) R4 Frame(CFA, -24)
          CFI (cfiCond158) R5 Frame(CFA, -20)
          CFI (cfiCond158) R6 Frame(CFA, -16)
          CFI (cfiCond158) R7 Frame(CFA, -12)
          CFI (cfiCond158) R8 Frame(CFA, -8)
          CFI (cfiCond158) R14 Frame(CFA, -4)
          CFI (cfiCond158) CFA R13+48
          CFI Block cfiCond159 Using cfiCommon0
          CFI (cfiCond159) Function user_send_identPackToConfigurator
          CFI (cfiCond159) Conditional ??CrossCallReturnLabel_230
          CFI (cfiCond159) R4 Frame(CFA, -12)
          CFI (cfiCond159) R5 Frame(CFA, -8)
          CFI (cfiCond159) R14 Frame(CFA, -4)
          CFI (cfiCond159) CFA R13+40
          CFI Block cfiPicker160 Using cfiCommon1
          CFI (cfiPicker160) NoFunction
          CFI (cfiPicker160) Picker
        THUMB
?Subroutine65:
        STRB     R0,[R4, #+4]
        MOVS     R0,#+0
        STRB     R0,[R4, #+5]
        BX       LR
          CFI EndBlock cfiCond156
          CFI EndBlock cfiCond157
          CFI EndBlock cfiCond158
          CFI EndBlock cfiCond159
          CFI EndBlock cfiPicker160

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond161 Using cfiCommon0
          CFI Function user_send_statusPackToConfigurator
          CFI Conditional ??CrossCallReturnLabel_202
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+120
          CFI Block cfiCond162 Using cfiCommon0
          CFI (cfiCond162) Function user_send_logRecords
          CFI (cfiCond162) Conditional ??CrossCallReturnLabel_201
          CFI (cfiCond162) R4 Frame(CFA, -36)
          CFI (cfiCond162) R5 Frame(CFA, -32)
          CFI (cfiCond162) R6 Frame(CFA, -28)
          CFI (cfiCond162) R7 Frame(CFA, -24)
          CFI (cfiCond162) R8 Frame(CFA, -20)
          CFI (cfiCond162) R9 Frame(CFA, -16)
          CFI (cfiCond162) R10 Frame(CFA, -12)
          CFI (cfiCond162) R11 Frame(CFA, -8)
          CFI (cfiCond162) R14 Frame(CFA, -4)
          CFI (cfiCond162) CFA R13+48
          CFI Block cfiPicker163 Using cfiCommon1
          CFI (cfiPicker163) NoFunction
          CFI (cfiPicker163) Picker
        THUMB
?Subroutine56:
        ADDS     R1,R6,#+2
          CFI Block cfiCond164 Using cfiCommon0
          CFI (cfiCond164) Function user_send_registrationData
          CFI (cfiCond164) Conditional ??CrossCallReturnLabel_200
          CFI (cfiCond164) R4 Frame(CFA, -16)
          CFI (cfiCond164) R5 Frame(CFA, -12)
          CFI (cfiCond164) R6 Frame(CFA, -8)
          CFI (cfiCond164) R14 Frame(CFA, -4)
          CFI (cfiCond164) CFA R13+72
          CFI Block cfiCond165 Using cfiCommon0
          CFI (cfiCond165) Function user_send_statusPack
          CFI (cfiCond165) Conditional ??CrossCallReturnLabel_199
          CFI (cfiCond165) R4 Frame(CFA, -16)
          CFI (cfiCond165) R5 Frame(CFA, -12)
          CFI (cfiCond165) R6 Frame(CFA, -8)
          CFI (cfiCond165) R14 Frame(CFA, -4)
          CFI (cfiCond165) CFA R13+48
          CFI Block cfiCond166 Using cfiCommon0
          CFI (cfiCond166) Function user_send_statusPackSecond
          CFI (cfiCond166) Conditional ??CrossCallReturnLabel_198
          CFI (cfiCond166) R4 Frame(CFA, -16)
          CFI (cfiCond166) R5 Frame(CFA, -12)
          CFI (cfiCond166) R6 Frame(CFA, -8)
          CFI (cfiCond166) R14 Frame(CFA, -4)
          CFI (cfiCond166) CFA R13+56
          CFI Block cfiCond167 Using cfiCommon0
          CFI (cfiCond167) Function user_send_crcPack
          CFI (cfiCond167) Conditional ??CrossCallReturnLabel_197
          CFI (cfiCond167) R4 Frame(CFA, -16)
          CFI (cfiCond167) R5 Frame(CFA, -12)
          CFI (cfiCond167) R6 Frame(CFA, -8)
          CFI (cfiCond167) R14 Frame(CFA, -4)
          CFI (cfiCond167) CFA R13+48
          CFI Block cfiCond168 Using cfiCommon0
          CFI (cfiCond168) Function user_send_crcMemoryID
          CFI (cfiCond168) Conditional ??CrossCallReturnLabel_196
          CFI (cfiCond168) R4 Frame(CFA, -32)
          CFI (cfiCond168) R5 Frame(CFA, -28)
          CFI (cfiCond168) R6 Frame(CFA, -24)
          CFI (cfiCond168) R7 Frame(CFA, -20)
          CFI (cfiCond168) R8 Frame(CFA, -16)
          CFI (cfiCond168) R9 Frame(CFA, -12)
          CFI (cfiCond168) R10 Frame(CFA, -8)
          CFI (cfiCond168) R14 Frame(CFA, -4)
          CFI (cfiCond168) CFA R13+40
          CFI Block cfiCond169 Using cfiCommon0
          CFI (cfiCond169) Function reader_reply_poll
          CFI (cfiCond169) Conditional ??CrossCallReturnLabel_195
          CFI (cfiCond169) R4 Frame(CFA, -12)
          CFI (cfiCond169) R5 Frame(CFA, -8)
          CFI (cfiCond169) R14 Frame(CFA, -4)
          CFI (cfiCond169) CFA R13+16
          CFI Block cfiCond170 Using cfiCommon0
          CFI (cfiCond170) Function user_send_answer_comand
          CFI (cfiCond170) Conditional ??CrossCallReturnLabel_194
          CFI (cfiCond170) R4 Frame(CFA, -24)
          CFI (cfiCond170) R5 Frame(CFA, -20)
          CFI (cfiCond170) R6 Frame(CFA, -16)
          CFI (cfiCond170) R7 Frame(CFA, -12)
          CFI (cfiCond170) R8 Frame(CFA, -8)
          CFI (cfiCond170) R14 Frame(CFA, -4)
          CFI (cfiCond170) CFA R13+48
          CFI Block cfiCond171 Using cfiCommon0
          CFI (cfiCond171) Function user_send_timeshtamp
          CFI (cfiCond171) Conditional ??CrossCallReturnLabel_193
          CFI (cfiCond171) R4 Frame(CFA, -20)
          CFI (cfiCond171) R5 Frame(CFA, -16)
          CFI (cfiCond171) R6 Frame(CFA, -12)
          CFI (cfiCond171) R7 Frame(CFA, -8)
          CFI (cfiCond171) R14 Frame(CFA, -4)
          CFI (cfiCond171) CFA R13+48
          CFI Block cfiCond172 Using cfiCommon0
          CFI (cfiCond172) Function user_send_identPackToConfigurator
          CFI (cfiCond172) Conditional ??CrossCallReturnLabel_192
          CFI (cfiCond172) R4 Frame(CFA, -12)
          CFI (cfiCond172) R5 Frame(CFA, -8)
          CFI (cfiCond172) R14 Frame(CFA, -4)
          CFI (cfiCond172) CFA R13+40
          CFI Block cfiCond173 Using cfiCommon0
          CFI (cfiCond173) Function user_send_configToConfigurator
          CFI (cfiCond173) Conditional ??CrossCallReturnLabel_191
          CFI (cfiCond173) R4 Frame(CFA, -12)
          CFI (cfiCond173) R5 Frame(CFA, -8)
          CFI (cfiCond173) R14 Frame(CFA, -4)
          CFI (cfiCond173) CFA R13+16
??Subroutine56_0:
        ADDS     R0,R4,#+4
          CFI (cfiCond161) FunCall user_send_statusPackToConfigurator crc16_calc
          CFI (cfiCond162) FunCall user_send_logRecords crc16_calc
          CFI (cfiCond164) FunCall user_send_registrationData crc16_calc
          CFI (cfiCond165) FunCall user_send_statusPack crc16_calc
          CFI (cfiCond166) FunCall user_send_statusPackSecond crc16_calc
          CFI (cfiCond167) FunCall user_send_crcPack crc16_calc
          CFI (cfiCond168) FunCall user_send_crcMemoryID crc16_calc
          CFI (cfiCond169) FunCall reader_reply_poll crc16_calc
          CFI (cfiCond170) FunCall user_send_answer_comand crc16_calc
          CFI (cfiCond171) FunCall user_send_timeshtamp crc16_calc
          CFI (cfiCond172) FunCall user_send_identPackToConfigurator crc16_calc
          CFI (cfiCond173) FunCall user_send_configToConfigurator crc16_calc
        B.W      crc16_calc
          CFI EndBlock cfiCond161
          CFI EndBlock cfiCond162
          CFI EndBlock cfiPicker163
          CFI EndBlock cfiCond164
          CFI EndBlock cfiCond165
          CFI EndBlock cfiCond166
          CFI EndBlock cfiCond167
          CFI EndBlock cfiCond168
          CFI EndBlock cfiCond169
          CFI EndBlock cfiCond170
          CFI EndBlock cfiCond171
          CFI EndBlock cfiCond172
          CFI EndBlock cfiCond173

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond174 Using cfiCommon0
          CFI Function user_send_statusPackToConfigurator
          CFI Conditional ??CrossCallReturnLabel_69
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+120
          CFI Block cfiCond175 Using cfiCommon0
          CFI (cfiCond175) Function user_send_logRecords
          CFI (cfiCond175) Conditional ??CrossCallReturnLabel_68
          CFI (cfiCond175) R4 Frame(CFA, -36)
          CFI (cfiCond175) R5 Frame(CFA, -32)
          CFI (cfiCond175) R6 Frame(CFA, -28)
          CFI (cfiCond175) R7 Frame(CFA, -24)
          CFI (cfiCond175) R8 Frame(CFA, -20)
          CFI (cfiCond175) R9 Frame(CFA, -16)
          CFI (cfiCond175) R10 Frame(CFA, -12)
          CFI (cfiCond175) R11 Frame(CFA, -8)
          CFI (cfiCond175) R14 Frame(CFA, -4)
          CFI (cfiCond175) CFA R13+48
          CFI Block cfiPicker176 Using cfiCommon1
          CFI (cfiPicker176) NoFunction
          CFI (cfiPicker176) Picker
        THUMB
?Subroutine22:
        ADD      R1,R6,#+8
          CFI Block cfiCond177 Using cfiCommon0
          CFI (cfiCond177) Function user_send_anyData
          CFI (cfiCond177) Conditional ??CrossCallReturnLabel_67
          CFI (cfiCond177) R4 Frame(CFA, -36)
          CFI (cfiCond177) R5 Frame(CFA, -32)
          CFI (cfiCond177) R6 Frame(CFA, -28)
          CFI (cfiCond177) R7 Frame(CFA, -24)
          CFI (cfiCond177) R8 Frame(CFA, -20)
          CFI (cfiCond177) R9 Frame(CFA, -16)
          CFI (cfiCond177) R10 Frame(CFA, -12)
          CFI (cfiCond177) R11 Frame(CFA, -8)
          CFI (cfiCond177) R14 Frame(CFA, -4)
          CFI (cfiCond177) CFA R13+40
          CFI Block cfiCond178 Using cfiCommon0
          CFI (cfiCond178) Function user_send_registrationData
          CFI (cfiCond178) Conditional ??CrossCallReturnLabel_66
          CFI (cfiCond178) R4 Frame(CFA, -16)
          CFI (cfiCond178) R5 Frame(CFA, -12)
          CFI (cfiCond178) R6 Frame(CFA, -8)
          CFI (cfiCond178) R14 Frame(CFA, -4)
          CFI (cfiCond178) CFA R13+72
          CFI Block cfiCond179 Using cfiCommon0
          CFI (cfiCond179) Function user_send_statusPack
          CFI (cfiCond179) Conditional ??CrossCallReturnLabel_65
          CFI (cfiCond179) R4 Frame(CFA, -16)
          CFI (cfiCond179) R5 Frame(CFA, -12)
          CFI (cfiCond179) R6 Frame(CFA, -8)
          CFI (cfiCond179) R14 Frame(CFA, -4)
          CFI (cfiCond179) CFA R13+48
          CFI Block cfiCond180 Using cfiCommon0
          CFI (cfiCond180) Function user_send_statusPackSecond
          CFI (cfiCond180) Conditional ??CrossCallReturnLabel_64
          CFI (cfiCond180) R4 Frame(CFA, -16)
          CFI (cfiCond180) R5 Frame(CFA, -12)
          CFI (cfiCond180) R6 Frame(CFA, -8)
          CFI (cfiCond180) R14 Frame(CFA, -4)
          CFI (cfiCond180) CFA R13+56
          CFI Block cfiCond181 Using cfiCommon0
          CFI (cfiCond181) Function user_send_statusUpdateFW
          CFI (cfiCond181) Conditional ??CrossCallReturnLabel_63
          CFI (cfiCond181) R4 Frame(CFA, -20)
          CFI (cfiCond181) R5 Frame(CFA, -16)
          CFI (cfiCond181) R6 Frame(CFA, -12)
          CFI (cfiCond181) R7 Frame(CFA, -8)
          CFI (cfiCond181) R14 Frame(CFA, -4)
          CFI (cfiCond181) CFA R13+40
          CFI Block cfiCond182 Using cfiCommon0
          CFI (cfiCond182) Function user_send_crcPack
          CFI (cfiCond182) Conditional ??CrossCallReturnLabel_62
          CFI (cfiCond182) R4 Frame(CFA, -16)
          CFI (cfiCond182) R5 Frame(CFA, -12)
          CFI (cfiCond182) R6 Frame(CFA, -8)
          CFI (cfiCond182) R14 Frame(CFA, -4)
          CFI (cfiCond182) CFA R13+48
          CFI Block cfiCond183 Using cfiCommon0
          CFI (cfiCond183) Function user_send_crcMemoryID
          CFI (cfiCond183) Conditional ??CrossCallReturnLabel_61
          CFI (cfiCond183) R4 Frame(CFA, -32)
          CFI (cfiCond183) R5 Frame(CFA, -28)
          CFI (cfiCond183) R6 Frame(CFA, -24)
          CFI (cfiCond183) R7 Frame(CFA, -20)
          CFI (cfiCond183) R8 Frame(CFA, -16)
          CFI (cfiCond183) R9 Frame(CFA, -12)
          CFI (cfiCond183) R10 Frame(CFA, -8)
          CFI (cfiCond183) R14 Frame(CFA, -4)
          CFI (cfiCond183) CFA R13+40
          CFI Block cfiCond184 Using cfiCommon0
          CFI (cfiCond184) Function user_send_answer_comand
          CFI (cfiCond184) Conditional ??CrossCallReturnLabel_60
          CFI (cfiCond184) R4 Frame(CFA, -24)
          CFI (cfiCond184) R5 Frame(CFA, -20)
          CFI (cfiCond184) R6 Frame(CFA, -16)
          CFI (cfiCond184) R7 Frame(CFA, -12)
          CFI (cfiCond184) R8 Frame(CFA, -8)
          CFI (cfiCond184) R14 Frame(CFA, -4)
          CFI (cfiCond184) CFA R13+48
          CFI Block cfiCond185 Using cfiCommon0
          CFI (cfiCond185) Function user_send_timeshtamp
          CFI (cfiCond185) Conditional ??CrossCallReturnLabel_59
          CFI (cfiCond185) R4 Frame(CFA, -20)
          CFI (cfiCond185) R5 Frame(CFA, -16)
          CFI (cfiCond185) R6 Frame(CFA, -12)
          CFI (cfiCond185) R7 Frame(CFA, -8)
          CFI (cfiCond185) R14 Frame(CFA, -4)
          CFI (cfiCond185) CFA R13+48
          CFI Block cfiCond186 Using cfiCommon0
          CFI (cfiCond186) Function user_send_identPackToConfigurator
          CFI (cfiCond186) Conditional ??CrossCallReturnLabel_58
          CFI (cfiCond186) R4 Frame(CFA, -12)
          CFI (cfiCond186) R5 Frame(CFA, -8)
          CFI (cfiCond186) R14 Frame(CFA, -4)
          CFI (cfiCond186) CFA R13+40
          CFI Block cfiCond187 Using cfiCommon0
          CFI (cfiCond187) Function user_send_configToConfigurator
          CFI (cfiCond187) Conditional ??CrossCallReturnLabel_57
          CFI (cfiCond187) R4 Frame(CFA, -12)
          CFI (cfiCond187) R5 Frame(CFA, -8)
          CFI (cfiCond187) R14 Frame(CFA, -4)
          CFI (cfiCond187) CFA R13+16
          CFI Block cfiCond188 Using cfiCommon0
          CFI (cfiCond188) Function user_send_tempIDs
          CFI (cfiCond188) Conditional ??CrossCallReturnLabel_56
          CFI (cfiCond188) R4 Frame(CFA, -36)
          CFI (cfiCond188) R5 Frame(CFA, -32)
          CFI (cfiCond188) R6 Frame(CFA, -28)
          CFI (cfiCond188) R7 Frame(CFA, -24)
          CFI (cfiCond188) R8 Frame(CFA, -20)
          CFI (cfiCond188) R9 Frame(CFA, -16)
          CFI (cfiCond188) R10 Frame(CFA, -12)
          CFI (cfiCond188) R11 Frame(CFA, -8)
          CFI (cfiCond188) R14 Frame(CFA, -4)
          CFI (cfiCond188) CFA R13+48
          CFI Block cfiCond189 Using cfiCommon0
          CFI (cfiCond189) Function user_getFWblockFromServer
          CFI (cfiCond189) Conditional ??CrossCallReturnLabel_55
          CFI (cfiCond189) R4 Frame(CFA, -20)
          CFI (cfiCond189) R5 Frame(CFA, -16)
          CFI (cfiCond189) R6 Frame(CFA, -12)
          CFI (cfiCond189) R7 Frame(CFA, -8)
          CFI (cfiCond189) R14 Frame(CFA, -4)
          CFI (cfiCond189) CFA R13+40
??Subroutine22_0:
        MOV      R0,R4
          CFI (cfiCond174) FunCall user_send_statusPackToConfigurator usb_send
          CFI (cfiCond175) FunCall user_send_logRecords usb_send
          CFI (cfiCond177) FunCall user_send_anyData usb_send
          CFI (cfiCond178) FunCall user_send_registrationData usb_send
          CFI (cfiCond179) FunCall user_send_statusPack usb_send
          CFI (cfiCond180) FunCall user_send_statusPackSecond usb_send
          CFI (cfiCond181) FunCall user_send_statusUpdateFW usb_send
          CFI (cfiCond182) FunCall user_send_crcPack usb_send
          CFI (cfiCond183) FunCall user_send_crcMemoryID usb_send
          CFI (cfiCond184) FunCall user_send_answer_comand usb_send
          CFI (cfiCond185) FunCall user_send_timeshtamp usb_send
          CFI (cfiCond186) FunCall user_send_identPackToConfigurator usb_send
          CFI (cfiCond187) FunCall user_send_configToConfigurator usb_send
          CFI (cfiCond188) FunCall user_send_tempIDs usb_send
          CFI (cfiCond189) FunCall user_getFWblockFromServer usb_send
        B.W      usb_send
          CFI EndBlock cfiCond174
          CFI EndBlock cfiCond175
          CFI EndBlock cfiPicker176
          CFI EndBlock cfiCond177
          CFI EndBlock cfiCond178
          CFI EndBlock cfiCond179
          CFI EndBlock cfiCond180
          CFI EndBlock cfiCond181
          CFI EndBlock cfiCond182
          CFI EndBlock cfiCond183
          CFI EndBlock cfiCond184
          CFI EndBlock cfiCond185
          CFI EndBlock cfiCond186
          CFI EndBlock cfiCond187
          CFI EndBlock cfiCond188
          CFI EndBlock cfiCond189

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond190 Using cfiCommon0
          CFI Function user_send_statusPack
          CFI Conditional ??CrossCallReturnLabel_8
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+48
          CFI Block cfiCond191 Using cfiCommon0
          CFI (cfiCond191) Function user_send_answer_comand
          CFI (cfiCond191) Conditional ??CrossCallReturnLabel_9
          CFI (cfiCond191) R4 Frame(CFA, -24)
          CFI (cfiCond191) R5 Frame(CFA, -20)
          CFI (cfiCond191) R6 Frame(CFA, -16)
          CFI (cfiCond191) R7 Frame(CFA, -12)
          CFI (cfiCond191) R8 Frame(CFA, -8)
          CFI (cfiCond191) R14 Frame(CFA, -4)
          CFI (cfiCond191) CFA R13+48
          CFI Block cfiPicker192 Using cfiCommon1
          CFI (cfiPicker192) NoFunction
          CFI (cfiPicker192) Picker
        THUMB
?Subroutine8:
        STR      R0,[SP, #+0]
        ADD      R0,R4,#+15
        LDRB     R2,[SP, #+0]
        STRB     R2,[R0, #+0]
        LDRB     R2,[SP, #+1]
        STRB     R2,[R0, #+1]
        BX       LR
          CFI EndBlock cfiCond190
          CFI EndBlock cfiCond191
          CFI EndBlock cfiPicker192

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock193 Using cfiCommon0
          CFI Function user_send_timeshtamp
        THUMB
user_send_timeshtamp:
        PUSH     {R1,R2,R4-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+28
        SUB      SP,SP,#+20
          CFI CFA R13+48
        MOV      R7,R0
        MOV      R6,R3
        CMP      R6,#+1
        BNE.N    ??user_send_timeshtamp_0
        MOVS     R0,#+18
          CFI FunCall user_comm_acquire
        BL       user_comm_acquire
        MOV      R4,R0
        B.N      ??user_send_timeshtamp_1
??user_send_timeshtamp_0:
        MOV      R4,SP
??user_send_timeshtamp_1:
        CBZ.N    R4,??CrossCallReturnLabel_59
        LDR      R5,[SP, #+48]
        LDR.W    R0,??DataTable102_1
          CFI FunCall user_debug
        BL       user_debug
        BL       ?Subroutine38
??CrossCallReturnLabel_131:
        MOVS     R0,#+10
        BL       ??Subroutine7_0
??CrossCallReturnLabel_6:
        STRB     R5,[R4, #+7]
        MOVS     R0,#+12
        STRB     R0,[R4, #+8]
        STRB     R7,[R4, #+9]
        ADD      R3,SP,#+20
        ADD      R0,R4,#+10
        LDR      R2,[R3, #+0]
        STR      R2,[R0, #+0]
        ADD      R3,SP,#+24
        ADD      R0,R4,#+14
        BL       ?Subroutine14
??CrossCallReturnLabel_25:
        MOVS     R1,#+12
        BL       ??Subroutine56_0
??CrossCallReturnLabel_193:
        STRB     R0,[R4, #+16]
        LSRS     R0,R0,#+8
        STRB     R0,[R4, #+17]
        CBZ.N    R6,??user_send_timeshtamp_2
        CMP      R6,#+1
          CFI FunCall comm_tx_commit
        ITT      EQ
        MOVEQ    R0,#+18
        BLEQ     comm_tx_commit
        B.N      ??CrossCallReturnLabel_59
??user_send_timeshtamp_2:
        MOVS     R1,#+18
        BL       ??Subroutine22_0
??CrossCallReturnLabel_59:
        ADD      SP,SP,#+28
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI EndBlock cfiBlock193

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable76:
        DATA32
        DC32     ?_45

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock194 Using cfiCommon0
          CFI Function user_send_identPackToConfigurator
        THUMB
user_send_identPackToConfigurator:
        PUSH     {R4,R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+12
        SUB      SP,SP,#+28
          CFI CFA R13+40
        MOV      R5,R0
        CMP      R0,#+1
        BNE.N    ??user_send_identPackToConfigurator_0
        MOVS     R0,#+27
          CFI FunCall user_comm_acquire
        BL       user_comm_acquire
        MOV      R4,R0
        B.N      ??user_send_identPackToConfigurator_1
??user_send_identPackToConfigurator_0:
        MOV      R4,SP
??user_send_identPackToConfigurator_1:
        CBZ.N    R4,??CrossCallReturnLabel_58
        BL       ?Subroutine38
??CrossCallReturnLabel_132:
        MOVS     R0,#+19
        BL       ?Subroutine65
??CrossCallReturnLabel_230:
        STRB     R0,[R4, #+6]
        STRB     R0,[R4, #+7]
        MOVS     R0,#+100
        STRB     R0,[R4, #+8]
        MOVS     R2,#+15
        LDR.W    R1,??DataTable83
        BL       ?Subroutine40
??CrossCallReturnLabel_141:
        LDR.W    R0,??DataTable102_2
        LDR      R1,[R0, #+4]
        LDR.N    R0,??DataTable77_2
        CMP      R1,R0
        ITE      EQ
        MOVEQ    R0,#+1
        MOVNE    R0,#+0
        STRB     R0,[R4, #+24]
        MOVS     R1,#+21
        BL       ??Subroutine56_0
??CrossCallReturnLabel_192:
        STRB     R0,[R4, #+25]
        LSRS     R0,R0,#+8
        STRB     R0,[R4, #+26]
        CBZ.N    R5,??user_send_identPackToConfigurator_2
        CMP      R5,#+1
          CFI FunCall comm_tx_commit
        ITT      EQ
        MOVEQ    R0,#+27
        BLEQ     comm_tx_commit
        B.N      ??CrossCallReturnLabel_58
??user_send_identPackToConfigurator_2:
        MOVS     R1,#+27
        BL       ??Subroutine22_0
??CrossCallReturnLabel_58:
        ADD      SP,SP,#+28
          CFI CFA R13+12
        POP      {R4,R5,PC}
          CFI EndBlock cfiBlock194

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable77:
        DATA32
        DC32     store_save_requested

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable77_1:
        DATA32
        DC32     ?_46

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable77_2:
        DATA32
        DC32     0x31323334

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable77_3:
        DATA32
        DC32     ?_47

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable77_4:
        DATA32
        DC32     ?_48

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable77_5:
        DATA32
        DC32     ?_50

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable77_6:
        DATA32
        DC32     ?_51

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond195 Using cfiCommon0
          CFI Function user_send_anyData
          CFI Conditional ??CrossCallReturnLabel_139
          CFI R4 Frame(CFA, -36)
          CFI R5 Frame(CFA, -32)
          CFI R6 Frame(CFA, -28)
          CFI R7 Frame(CFA, -24)
          CFI R8 Frame(CFA, -20)
          CFI R9 Frame(CFA, -16)
          CFI R10 Frame(CFA, -12)
          CFI R11 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+40
          CFI Block cfiCond196 Using cfiCommon0
          CFI (cfiCond196) Function user_send_registrationData
          CFI (cfiCond196) Conditional ??CrossCallReturnLabel_140
          CFI (cfiCond196) R4 Frame(CFA, -16)
          CFI (cfiCond196) R5 Frame(CFA, -12)
          CFI (cfiCond196) R6 Frame(CFA, -8)
          CFI (cfiCond196) R14 Frame(CFA, -4)
          CFI (cfiCond196) CFA R13+72
          CFI Block cfiCond197 Using cfiCommon0
          CFI (cfiCond197) Function user_send_identPackToConfigurator
          CFI (cfiCond197) Conditional ??CrossCallReturnLabel_141
          CFI (cfiCond197) R4 Frame(CFA, -12)
          CFI (cfiCond197) R5 Frame(CFA, -8)
          CFI (cfiCond197) R14 Frame(CFA, -4)
          CFI (cfiCond197) CFA R13+40
          CFI Block cfiCond198 Using cfiCommon0
          CFI (cfiCond198) Function user_send_configToConfigurator
          CFI (cfiCond198) Conditional ??CrossCallReturnLabel_142
          CFI (cfiCond198) R4 Frame(CFA, -12)
          CFI (cfiCond198) R5 Frame(CFA, -8)
          CFI (cfiCond198) R14 Frame(CFA, -4)
          CFI (cfiCond198) CFA R13+16
          CFI Block cfiPicker199 Using cfiCommon1
          CFI (cfiPicker199) NoFunction
          CFI (cfiPicker199) Picker
        THUMB
?Subroutine40:
        ADD      R0,R4,#+9
          CFI (cfiCond195) FunCall user_send_anyData __aeabi_memcpy
          CFI (cfiCond196) FunCall user_send_registrationData __aeabi_memcpy
          CFI (cfiCond197) FunCall user_send_identPackToConfigurator __aeabi_memcpy
          CFI (cfiCond198) FunCall user_send_configToConfigurator __aeabi_memcpy
        B.W      __aeabi_memcpy
          CFI EndBlock cfiCond195
          CFI EndBlock cfiCond196
          CFI EndBlock cfiCond197
          CFI EndBlock cfiCond198
          CFI EndBlock cfiPicker199

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock200 Using cfiCommon0
          CFI Function user_send_statusPackToConfigurator
        THUMB
user_send_statusPackToConfigurator:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        SUB      SP,SP,#+96
          CFI CFA R13+120
        MOV      R5,R0
        MOVS     R6,#+34
        CMP      R0,#+1
        BNE.N    ??user_send_statusPackToConfigurator_0
        MOVS     R0,#+90
          CFI FunCall user_comm_acquire
        BL       user_comm_acquire
        MOV      R4,R0
        B.N      ??user_send_statusPackToConfigurator_1
??user_send_statusPackToConfigurator_0:
        ADD      R4,SP,#+4
??user_send_statusPackToConfigurator_1:
        CMP      R4,#+0
        BEQ.N    ??CrossCallReturnLabel_69
        BL       ?Subroutine38
??CrossCallReturnLabel_133:
        STRB     R6,[R4, #+4]
        MOVS     R0,#+0
        STRB     R0,[R4, #+5]
        STRB     R0,[R4, #+6]
        STRB     R0,[R4, #+7]
        MOVS     R0,#+101
        BL       ?Subroutine10
??CrossCallReturnLabel_15:
        LDR      R2,[SP, #+0]
        STR      R2,[R1, #+0]
        ADD      R3,R0,#+129
        ADD      R1,R4,#+13
        LDRB     R2,[R3, #+0]
        STRB     R2,[R1, #+0]
        LDRB     R2,[R3, #+1]
        STRB     R2,[R1, #+1]
        LDRH     R1,[R0, #+20]
        STRB     R1,[R4, #+15]
        LDR      R1,[R0, #+23]
        STR      R1,[SP, #+0]
        ADD      R1,R4,#+16
        LDR      R2,[SP, #+0]
        STR      R2,[R1, #+0]
        LDRB     R1,[R0, #+22]
        STRB     R1,[R4, #+20]
        LDRB     R1,[R0, #+19]
        STRB     R1,[R4, #+21]
        LDRB     R1,[R0, #+74]
        STRB     R1,[R4, #+22]
        LDRH     R1,[R0, #+77]
        STR      R1,[SP, #+0]
        ADD      R1,R4,#+23
        LDRB     R3,[SP, #+0]
        STRB     R3,[R1, #+0]
        LDRB     R3,[SP, #+1]
        STRB     R3,[R1, #+1]
        LDRB     R1,[R0, #+75]
        STRB     R1,[R4, #+25]
        LDRB     R1,[R0, #+76]
        STRB     R1,[R4, #+26]
        LDRB     R1,[R0, #+80]
        STRB     R1,[R4, #+27]
        LDRB     R1,[R0, #+79]
        STRB     R1,[R4, #+28]
        ADD      R3,R0,#+117
        ADD      R7,R4,#+29
        LDR      R2,[R3, #+0]
        STR      R2,[R7, #+0]
        LDR      R2,[R3, #+4]
        STR      R2,[R7, #+4]
        LDRH     R1,[R0, #+81]
        STRB     R1,[R4, #+37]
        LDRH     R1,[R0, #+81]
        LSRS     R1,R1,#+8
        STRB     R1,[R4, #+38]
        LDRB     R0,[R0, #+169]
        STRB     R0,[R4, #+39]
        LDR.W    R0,??DataTable105
        LDR      R2,[R0, #+0]
        LDR.W    R1,??DataTable105_1
        CMP      R2,R1
        BNE.N    ??user_send_statusPackToConfigurator_2
        MOVS     R3,#+0
??user_send_statusPackToConfigurator_3:
        LDRB     R2,[R0, R3]
        ADDS     R1,R4,R3
        STRB     R2,[R1, #+40]
        ADDS     R3,R3,#+1
        CMP      R3,#+48
        BCC.N    ??user_send_statusPackToConfigurator_3
        MOVS     R6,#+82
        STRB     R6,[R4, #+4]
        MOVS     R0,#+0
        STRB     R0,[R4, #+5]
??user_send_statusPackToConfigurator_2:
        BL       ?Subroutine56
??CrossCallReturnLabel_202:
        ADDS     R1,R4,R6
        STRB     R0,[R1, #+6]
        LSRS     R0,R0,#+8
        STRB     R0,[R1, #+7]
        CBZ.N    R5,??user_send_statusPackToConfigurator_4
        CMP      R5,#+1
          CFI FunCall comm_tx_commit
        ITT      EQ
        ADDEQ    R0,R6,#+8
        BLEQ     comm_tx_commit
        B.N      ??CrossCallReturnLabel_69
??user_send_statusPackToConfigurator_4:
        BL       ?Subroutine22
??CrossCallReturnLabel_69:
        ADD      SP,SP,#+100
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI EndBlock cfiBlock200

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond201 Using cfiCommon0
          CFI Function user_send_statusPackSecond
          CFI Conditional ??CrossCallReturnLabel_14
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+56
          CFI Block cfiCond202 Using cfiCommon0
          CFI (cfiCond202) Function user_send_statusPackToConfigurator
          CFI (cfiCond202) Conditional ??CrossCallReturnLabel_15
          CFI (cfiCond202) R4 Frame(CFA, -20)
          CFI (cfiCond202) R5 Frame(CFA, -16)
          CFI (cfiCond202) R6 Frame(CFA, -12)
          CFI (cfiCond202) R7 Frame(CFA, -8)
          CFI (cfiCond202) R14 Frame(CFA, -4)
          CFI (cfiCond202) CFA R13+120
          CFI Block cfiPicker203 Using cfiCommon1
          CFI (cfiPicker203) NoFunction
          CFI (cfiPicker203) Picker
        THUMB
?Subroutine10:
        STRB     R0,[R4, #+8]
        LDR.W    R0,??DataTable102
        LDR      R1,[R0, #+47]
        STR      R1,[SP, #+0]
        ADD      R1,R4,#+9
        BX       LR
          CFI EndBlock cfiCond201
          CFI EndBlock cfiCond202
          CFI EndBlock cfiPicker203

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock204 Using cfiCommon0
          CFI Function user_send_configToConfigurator
        THUMB
user_send_configToConfigurator:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R5,R0
        CMP      R0,#+1
        MOVW     R0,#+523
        BNE.N    ??user_send_configToConfigurator_0
          CFI FunCall user_comm_acquire
        BL       user_comm_acquire
        B.N      ??user_send_configToConfigurator_1
??user_send_configToConfigurator_0:
          CFI FunCall malloc
        BL       malloc
??user_send_configToConfigurator_1:
        MOVS     R4,R0
        BEQ.N    ??user_send_configToConfigurator_2
        LDR.W    R0,??DataTable100
          CFI FunCall user_debug
        BL       user_debug
        BL       ?Subroutine38
??CrossCallReturnLabel_134:
        MOVS     R0,#+3
        STRB     R0,[R4, #+4]
        MOVS     R0,#+2
        STRB     R0,[R4, #+5]
        MOVS     R0,#+255
        STRB     R0,[R4, #+6]
        MOVS     R0,#+0
        STRB     R0,[R4, #+7]
        MOVS     R0,#+106
        STRB     R0,[R4, #+8]
        MOV      R2,#+512
        LDR.W    R1,??DataTable102_2
        BL       ?Subroutine40
??CrossCallReturnLabel_142:
        MOVW     R1,#+517
        BL       ??Subroutine56_0
??CrossCallReturnLabel_191:
        STRB     R0,[R4, #+521]
        LSRS     R0,R0,#+8
        STRB     R0,[R4, #+522]
        MOVS     R0,R5
        BEQ.N    ??user_send_configToConfigurator_3
        CMP      R5,#+1
        BEQ.N    ??user_send_configToConfigurator_4
        B.N      ??user_send_configToConfigurator_5
??user_send_configToConfigurator_3:
        MOVW     R1,#+523
        BL       ??Subroutine22_0
??CrossCallReturnLabel_57:
        MOV      R0,R4
        POP      {R1,R4,R5,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall free
        B.W      free
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
??user_send_configToConfigurator_2:
        POP      {R0,R4,R5,PC}
??user_send_configToConfigurator_4:
        MOVW     R0,#+523
          CFI FunCall comm_tx_commit
        BL       comm_tx_commit
        B.N      ??user_send_configToConfigurator_6
??user_send_configToConfigurator_5:
        BNE.N    ??CrossCallReturnLabel_57
??user_send_configToConfigurator_6:
        POP      {R0,R4,R5,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall comm_tx_release
        B.W      comm_tx_release
          CFI EndBlock cfiBlock204

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable80:
        DATA32
        DC32     store_reset_requested

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock205 Using cfiCommon0
          CFI Function user_send_logRecords
        THUMB
user_send_logRecords:
        PUSH     {R1-R11,LR}
          CFI R14 Frame(CFA, -4)
          CFI R11 Frame(CFA, -8)
          CFI R10 Frame(CFA, -12)
          CFI R9 Frame(CFA, -16)
          CFI R8 Frame(CFA, -20)
          CFI R7 Frame(CFA, -24)
          CFI R6 Frame(CFA, -28)
          CFI R5 Frame(CFA, -32)
          CFI R4 Frame(CFA, -36)
          CFI CFA R13+48
        MOVS     R9,R0
        MOV      R8,R1
        IT       NE
        CMPNE    R0,#+1
        BNE.N    ??user_send_logRecords_0
        BL       ?Subroutine25
??CrossCallReturnLabel_80:
        CBZ.N    R0,??user_send_logRecords_1
        MOV      R2,R8
        MOV      R1,R9
        MOVS     R0,#+10
          CFI FunCall store_upload_request
        BL       store_upload_request
        CMP      R0,#+0
        BEQ.N    ??user_send_logRecords_2
??user_send_logRecords_1:
        MOV      R1,R9
        MOVS     R0,#+10
          CFI FunCall upload_ack
        BL       upload_ack
        CMP      R0,#+0
        BEQ.N    ??user_send_logRecords_2
        MOV      R1,R9
        MOVS     R0,#+10
          CFI FunCall upload_index
        BL       upload_index
        MOV      R11,R0
        LDR.W    R10,??DataTable83
          CFI FunCall log_getRecordsCount
        BL       log_getRecordsCount
        STRH     R0,[R10, #+89]
        BL       ?Subroutine42
??CrossCallReturnLabel_147:
        BEQ.N    ??user_send_logRecords_2
        MOVS     R1,#+3
        ADD      R0,SP,#+4
          CFI FunCall __aeabi_memclr4
        BL       __aeabi_memclr4
        LDRH     R0,[R10, #+89]
        CBZ.N    R0,??user_send_logRecords_3
        CMP      R11,R0
        BLT.N    ??user_send_logRecords_4
??user_send_logRecords_3:
        MOVS     R0,#+10
          CFI FunCall upload_finish
        BL       upload_finish
        CMP      R0,#+0
??user_send_logRecords_2:
        BEQ.N    ??user_send_logRecords_5
        STR      R8,[SP, #+0]
        MOV      R3,R9
        MOVS     R2,#+3
        ADD      R1,SP,#+4
        MOVS     R0,#+10
          CFI FunCall user_send_anyData
        BL       user_send_anyData
??user_send_logRecords_0:
        B.N      ??user_send_logRecords_5
??user_send_logRecords_4:
        SUB      R5,R0,R11
        UXTH     R5,R5
        CMP      R5,#+33
        IT       GE
        MOVGE    R5,#+32
        CMP      R9,#+1
        BNE.N    ??user_send_logRecords_6
        LSLS     R0,R5,#+5
        BL       ?Subroutine63
??CrossCallReturnLabel_221:
        B.N      ??user_send_logRecords_7
??user_send_logRecords_6:
        LSLS     R0,R5,#+5
        ADDS     R0,R0,#+16
          CFI FunCall malloc
        BL       malloc
??user_send_logRecords_7:
        MOV      R4,R0
        LSLS     R6,R5,#+5
        ADDS     R6,R6,#+6
        UXTH     R6,R6
        LDRH     R7,[R10, #+89]
        SUB      R7,R7,R11
        SUBS     R7,R7,R5
        CMP      R4,#+0
        BEQ.N    ??user_send_logRecords_5
        LDR.W    R0,??DataTable108
          CFI FunCall user_debug
        BL       user_debug
        BL       ?Subroutine30
??CrossCallReturnLabel_95:
        STRB     R8,[R4, #+7]
        MOVS     R0,#+10
        STRB     R0,[R4, #+8]
        STRB     R7,[R4, #+9]
        LSLS     R7,R7,#+16
        LSRS     R7,R7,#+24
        STRB     R7,[R4, #+10]
        STRB     R5,[R4, #+11]
        MOV      R2,R5
        ADD      R1,R4,#+12
        MOV      R0,R11
          CFI FunCall log_readRecords
        BL       log_readRecords
        CBNZ.N   R0,??user_send_logRecords_8
        CMP      R9,#+1
        BNE.N    ??user_send_logRecords_9
        B.N      ??user_send_logRecords_10
??user_send_logRecords_8:
        BL       ?Subroutine56
??CrossCallReturnLabel_201:
        ADDS     R1,R4,R6
        BL       ??Subroutine64_0
??CrossCallReturnLabel_226:
        MOVS     R0,R9
        BEQ.N    ??user_send_logRecords_11
        CMP      R9,#+1
        BEQ.N    ??user_send_logRecords_12
        B.N      ??user_send_logRecords_13
??user_send_logRecords_11:
        BL       ?Subroutine22
??CrossCallReturnLabel_68:
        CBNZ.N   R0,??user_send_logRecords_14
??user_send_logRecords_15:
        MOVS     R1,#+0
??user_send_logRecords_16:
        MOV      R11,#+0
??user_send_logRecords_17:
        LDR.W    R0,??DataTable109
        STRH     R11,[R0, R1]
        MOVS     R0,R9
        BEQ.N    ??user_send_logRecords_9
        LDRH     R0,[R10, #+129]
        ORR      R0,R0,#0x1000
        STRH     R0,[R10, #+129]
        CMP      R9,#+1
        BNE.N    ??user_send_logRecords_9
??user_send_logRecords_10:
          CFI FunCall comm_tx_release
        BL       comm_tx_release
        B.N      ??user_send_logRecords_5
??user_send_logRecords_14:
        MOVS     R1,#+0
??user_send_logRecords_18:
        ADD      R11,R5,R11
        B.N      ??user_send_logRecords_17
??user_send_logRecords_12:
        ADD      R0,R6,#+8
          CFI FunCall comm_tx_commit
        BL       comm_tx_commit
        MOVS     R1,#+2
        CMP      R0,#+0
        BNE.N    ??user_send_logRecords_18
        B.N      ??user_send_logRecords_16
??user_send_logRecords_13:
        MOVS     R0,R9
        BEQ.N    ??user_send_logRecords_15
        MOVS     R1,#+2
        B.N      ??user_send_logRecords_16
??user_send_logRecords_9:
        MOV      R0,R4
          CFI FunCall free
        BL       free
??user_send_logRecords_5:
        POP      {R0-R2,R4-R11,PC}
          CFI EndBlock cfiBlock205

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond206 Using cfiCommon0
          CFI Function user_send_crcMemoryID
          CFI Conditional ??CrossCallReturnLabel_222
          CFI R4 Frame(CFA, -32)
          CFI R5 Frame(CFA, -28)
          CFI R6 Frame(CFA, -24)
          CFI R7 Frame(CFA, -20)
          CFI R8 Frame(CFA, -16)
          CFI R9 Frame(CFA, -12)
          CFI R10 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+40
          CFI Block cfiCond207 Using cfiCommon0
          CFI (cfiCond207) Function user_send_tempIDs
          CFI (cfiCond207) Conditional ??CrossCallReturnLabel_223
          CFI (cfiCond207) R4 Frame(CFA, -36)
          CFI (cfiCond207) R5 Frame(CFA, -32)
          CFI (cfiCond207) R6 Frame(CFA, -28)
          CFI (cfiCond207) R7 Frame(CFA, -24)
          CFI (cfiCond207) R8 Frame(CFA, -20)
          CFI (cfiCond207) R9 Frame(CFA, -16)
          CFI (cfiCond207) R10 Frame(CFA, -12)
          CFI (cfiCond207) R11 Frame(CFA, -8)
          CFI (cfiCond207) R14 Frame(CFA, -4)
          CFI (cfiCond207) CFA R13+48
          CFI Block cfiPicker208 Using cfiCommon1
          CFI (cfiPicker208) NoFunction
          CFI (cfiPicker208) Picker
        THUMB
?Subroutine64:
        ADDS     R1,R4,R7
          CFI Block cfiCond209 Using cfiCommon0
          CFI (cfiCond209) Function user_send_anyData
          CFI (cfiCond209) Conditional ??CrossCallReturnLabel_224
          CFI (cfiCond209) R4 Frame(CFA, -36)
          CFI (cfiCond209) R5 Frame(CFA, -32)
          CFI (cfiCond209) R6 Frame(CFA, -28)
          CFI (cfiCond209) R7 Frame(CFA, -24)
          CFI (cfiCond209) R8 Frame(CFA, -20)
          CFI (cfiCond209) R9 Frame(CFA, -16)
          CFI (cfiCond209) R10 Frame(CFA, -12)
          CFI (cfiCond209) R11 Frame(CFA, -8)
          CFI (cfiCond209) R14 Frame(CFA, -4)
          CFI (cfiCond209) CFA R13+40
          CFI Block cfiCond210 Using cfiCommon0
          CFI (cfiCond210) Function user_send_smsToServer
          CFI (cfiCond210) Conditional ??CrossCallReturnLabel_225
          CFI (cfiCond210) R4 Frame(CFA, -16)
          CFI (cfiCond210) R5 Frame(CFA, -12)
          CFI (cfiCond210) R6 Frame(CFA, -8)
          CFI (cfiCond210) R14 Frame(CFA, -4)
          CFI (cfiCond210) CFA R13+24
          CFI Block cfiCond211 Using cfiCommon0
          CFI (cfiCond211) Function user_send_logRecords
          CFI (cfiCond211) Conditional ??CrossCallReturnLabel_226
          CFI (cfiCond211) R4 Frame(CFA, -36)
          CFI (cfiCond211) R5 Frame(CFA, -32)
          CFI (cfiCond211) R6 Frame(CFA, -28)
          CFI (cfiCond211) R7 Frame(CFA, -24)
          CFI (cfiCond211) R8 Frame(CFA, -20)
          CFI (cfiCond211) R9 Frame(CFA, -16)
          CFI (cfiCond211) R10 Frame(CFA, -12)
          CFI (cfiCond211) R11 Frame(CFA, -8)
          CFI (cfiCond211) R14 Frame(CFA, -4)
          CFI (cfiCond211) CFA R13+48
??Subroutine64_0:
        STRB     R0,[R1, #+6]
        LSRS     R0,R0,#+8
        STRB     R0,[R1, #+7]
        BX       LR
          CFI EndBlock cfiCond206
          CFI EndBlock cfiCond207
          CFI EndBlock cfiPicker208
          CFI EndBlock cfiCond209
          CFI EndBlock cfiCond210
          CFI EndBlock cfiCond211

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond212 Using cfiCommon0
          CFI Function user_send_logRecords
          CFI Conditional ??CrossCallReturnLabel_221
          CFI R4 Frame(CFA, -36)
          CFI R5 Frame(CFA, -32)
          CFI R6 Frame(CFA, -28)
          CFI R7 Frame(CFA, -24)
          CFI R8 Frame(CFA, -20)
          CFI R9 Frame(CFA, -16)
          CFI R10 Frame(CFA, -12)
          CFI R11 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+48
          CFI Block cfiCond213 Using cfiCommon0
          CFI (cfiCond213) Function user_send_tempIDs
          CFI (cfiCond213) Conditional ??CrossCallReturnLabel_220
          CFI (cfiCond213) R4 Frame(CFA, -36)
          CFI (cfiCond213) R5 Frame(CFA, -32)
          CFI (cfiCond213) R6 Frame(CFA, -28)
          CFI (cfiCond213) R7 Frame(CFA, -24)
          CFI (cfiCond213) R8 Frame(CFA, -20)
          CFI (cfiCond213) R9 Frame(CFA, -16)
          CFI (cfiCond213) R10 Frame(CFA, -12)
          CFI (cfiCond213) R11 Frame(CFA, -8)
          CFI (cfiCond213) R14 Frame(CFA, -4)
          CFI (cfiCond213) CFA R13+48
          CFI Block cfiPicker214 Using cfiCommon1
          CFI (cfiPicker214) NoFunction
          CFI (cfiPicker214) Picker
        THUMB
?Subroutine63:
        ADDS     R0,R0,#+16
          CFI Block cfiCond215 Using cfiCommon0
          CFI (cfiCond215) Function user_send_anyData
          CFI (cfiCond215) Conditional ??CrossCallReturnLabel_219
          CFI (cfiCond215) R4 Frame(CFA, -36)
          CFI (cfiCond215) R5 Frame(CFA, -32)
          CFI (cfiCond215) R6 Frame(CFA, -28)
          CFI (cfiCond215) R7 Frame(CFA, -24)
          CFI (cfiCond215) R8 Frame(CFA, -20)
          CFI (cfiCond215) R9 Frame(CFA, -16)
          CFI (cfiCond215) R10 Frame(CFA, -12)
          CFI (cfiCond215) R11 Frame(CFA, -8)
          CFI (cfiCond215) R14 Frame(CFA, -4)
          CFI (cfiCond215) CFA R13+40
??Subroutine63_0:
        UXTH     R0,R0
          CFI (cfiCond212) FunCall user_send_logRecords user_comm_acquire
          CFI (cfiCond213) FunCall user_send_tempIDs user_comm_acquire
          CFI (cfiCond215) FunCall user_send_anyData user_comm_acquire
        B.W      user_comm_acquire
          CFI EndBlock cfiCond212
          CFI EndBlock cfiCond213
          CFI EndBlock cfiPicker214
          CFI EndBlock cfiCond215

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond216 Using cfiCommon0
          CFI Function user_send_smsToServer
          CFI Conditional ??CrossCallReturnLabel_94
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
          CFI Block cfiCond217 Using cfiCommon0
          CFI (cfiCond217) Function user_send_logRecords
          CFI (cfiCond217) Conditional ??CrossCallReturnLabel_95
          CFI (cfiCond217) R4 Frame(CFA, -36)
          CFI (cfiCond217) R5 Frame(CFA, -32)
          CFI (cfiCond217) R6 Frame(CFA, -28)
          CFI (cfiCond217) R7 Frame(CFA, -24)
          CFI (cfiCond217) R8 Frame(CFA, -20)
          CFI (cfiCond217) R9 Frame(CFA, -16)
          CFI (cfiCond217) R10 Frame(CFA, -12)
          CFI (cfiCond217) R11 Frame(CFA, -8)
          CFI (cfiCond217) R14 Frame(CFA, -4)
          CFI (cfiCond217) CFA R13+48
          CFI Block cfiPicker218 Using cfiCommon1
          CFI (cfiPicker218) NoFunction
          CFI (cfiPicker218) Picker
        THUMB
?Subroutine30:
        MOVS     R0,#+83
        STRB     R0,[R4, #+0]
        MOVS     R0,#+76
        STRB     R0,[R4, #+1]
        MOVS     R0,#+68
        STRB     R0,[R4, #+2]
        MOVS     R0,#+78
        STRB     R0,[R4, #+3]
        STRB     R6,[R4, #+4]
        LSRS     R0,R6,#+8
        STRB     R0,[R4, #+5]
        MOVS     R0,#+255
        STRB     R0,[R4, #+6]
        BX       LR
          CFI EndBlock cfiCond216
          CFI EndBlock cfiCond217
          CFI EndBlock cfiPicker218

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond219 Using cfiCommon0
          CFI Function upload_ack
          CFI Conditional ??CrossCallReturnLabel_76
          CFI R4 Frame(CFA, -24)
          CFI R5 Frame(CFA, -20)
          CFI R6 Frame(CFA, -16)
          CFI R7 Frame(CFA, -12)
          CFI R8 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
          CFI Block cfiCond220 Using cfiCommon0
          CFI (cfiCond220) Function upload_finish
          CFI (cfiCond220) Conditional ??CrossCallReturnLabel_77
          CFI (cfiCond220) R4 Frame(CFA, -20)
          CFI (cfiCond220) R5 Frame(CFA, -16)
          CFI (cfiCond220) R6 Frame(CFA, -12)
          CFI (cfiCond220) R7 Frame(CFA, -8)
          CFI (cfiCond220) R14 Frame(CFA, -4)
          CFI (cfiCond220) CFA R13+24
          CFI Block cfiCond221 Using cfiCommon0
          CFI (cfiCond221) Function user_writeconfig
          CFI (cfiCond221) Conditional ??CrossCallReturnLabel_78
          CFI (cfiCond221) R4 Frame(CFA, -8)
          CFI (cfiCond221) R14 Frame(CFA, -4)
          CFI (cfiCond221) CFA R13+8
          CFI Block cfiCond222 Using cfiCommon0
          CFI (cfiCond222) Function user_savereset
          CFI (cfiCond222) Conditional ??CrossCallReturnLabel_79
          CFI (cfiCond222) R14 Frame(CFA, -4)
          CFI (cfiCond222) CFA R13+8
          CFI Block cfiCond223 Using cfiCommon0
          CFI (cfiCond223) Function user_send_logRecords
          CFI (cfiCond223) Conditional ??CrossCallReturnLabel_80
          CFI (cfiCond223) R4 Frame(CFA, -36)
          CFI (cfiCond223) R5 Frame(CFA, -32)
          CFI (cfiCond223) R6 Frame(CFA, -28)
          CFI (cfiCond223) R7 Frame(CFA, -24)
          CFI (cfiCond223) R8 Frame(CFA, -20)
          CFI (cfiCond223) R9 Frame(CFA, -16)
          CFI (cfiCond223) R10 Frame(CFA, -12)
          CFI (cfiCond223) R11 Frame(CFA, -8)
          CFI (cfiCond223) R14 Frame(CFA, -4)
          CFI (cfiCond223) CFA R13+48
          CFI Block cfiCond224 Using cfiCommon0
          CFI (cfiCond224) Function user_send_tempIDs
          CFI (cfiCond224) Conditional ??CrossCallReturnLabel_81
          CFI (cfiCond224) R4 Frame(CFA, -36)
          CFI (cfiCond224) R5 Frame(CFA, -32)
          CFI (cfiCond224) R6 Frame(CFA, -28)
          CFI (cfiCond224) R7 Frame(CFA, -24)
          CFI (cfiCond224) R8 Frame(CFA, -20)
          CFI (cfiCond224) R9 Frame(CFA, -16)
          CFI (cfiCond224) R10 Frame(CFA, -12)
          CFI (cfiCond224) R11 Frame(CFA, -8)
          CFI (cfiCond224) R14 Frame(CFA, -4)
          CFI (cfiCond224) CFA R13+48
          CFI Block cfiCond225 Using cfiCommon0
          CFI (cfiCond225) Function fw_take_block
          CFI (cfiCond225) Conditional ??CrossCallReturnLabel_82
          CFI (cfiCond225) R4 Frame(CFA, -24)
          CFI (cfiCond225) R5 Frame(CFA, -20)
          CFI (cfiCond225) R6 Frame(CFA, -16)
          CFI (cfiCond225) R7 Frame(CFA, -12)
          CFI (cfiCond225) R8 Frame(CFA, -8)
          CFI (cfiCond225) R14 Frame(CFA, -4)
          CFI (cfiCond225) CFA R13+64
          CFI Block cfiCond226 Using cfiCommon0
          CFI (cfiCond226) Function log_flushPending
          CFI (cfiCond226) Conditional ??CrossCallReturnLabel_83
          CFI (cfiCond226) R4 Frame(CFA, -12)
          CFI (cfiCond226) R5 Frame(CFA, -8)
          CFI (cfiCond226) R14 Frame(CFA, -4)
          CFI (cfiCond226) CFA R13+16
          CFI Block cfiPicker227 Using cfiCommon1
          CFI (cfiPicker227) NoFunction
          CFI (cfiPicker227) Picker
        THUMB
?Subroutine25:
        LDR.W    R0,??DataTable110
        LDRB     R0,[R0, #+0]
        BX       LR
          CFI EndBlock cfiCond219
          CFI EndBlock cfiCond220
          CFI EndBlock cfiCond221
          CFI EndBlock cfiCond222
          CFI EndBlock cfiCond223
          CFI EndBlock cfiCond224
          CFI EndBlock cfiCond225
          CFI EndBlock cfiCond226
          CFI EndBlock cfiPicker227

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable82:
        DATA32
        DC32     id_used_bytes

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable82_1:
        DATA32
        DC32     _A_tempStrBuffer

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable82_2:
        DATA32
        DC32     ?_52

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable82_3:
        DATA32
        DC32     ?_53

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock228 Using cfiCommon0
          CFI Function user_send_tempIDs
        THUMB
user_send_tempIDs:
        PUSH     {R1-R11,LR}
          CFI R14 Frame(CFA, -4)
          CFI R11 Frame(CFA, -8)
          CFI R10 Frame(CFA, -12)
          CFI R9 Frame(CFA, -16)
          CFI R8 Frame(CFA, -20)
          CFI R7 Frame(CFA, -24)
          CFI R6 Frame(CFA, -28)
          CFI R5 Frame(CFA, -32)
          CFI R4 Frame(CFA, -36)
          CFI CFA R13+48
        MOV      R10,R0
        MOV      R8,R1
        CMP      R0,#+0
        IT       NE
        CMPNE    R0,#+1
        BNE.N    ??user_send_tempIDs_0
        BL       ?Subroutine25
??CrossCallReturnLabel_81:
        CBZ.N    R0,??user_send_tempIDs_1
        MOV      R2,R8
        MOV      R1,R10
        MOVS     R0,#+9
          CFI FunCall store_upload_request
        BL       store_upload_request
        CMP      R0,#+0
        BEQ.N    ??user_send_tempIDs_0
??user_send_tempIDs_1:
        MOV      R1,R10
        MOVS     R0,#+9
          CFI FunCall upload_ack
        BL       upload_ack
        CMP      R0,#+0
        BEQ.N    ??user_send_tempIDs_0
        MOV      R1,R10
        MOVS     R0,#+9
          CFI FunCall upload_index
        BL       upload_index
        MOV      R9,R0
        LDR.W    R11,??DataTable83
          CFI FunCall user_getTempIDRecordsCount
        BL       user_getTempIDRecordsCount
        STRH     R0,[R11, #+83]
        BL       ?Subroutine42
??CrossCallReturnLabel_148:
        BEQ.N    ??user_send_tempIDs_0
        CBZ.N    R1,??user_send_tempIDs_2
        CMP      R9,R1
        BLT.N    ??user_send_tempIDs_3
??user_send_tempIDs_2:
        MOVS     R0,#+9
          CFI FunCall upload_finish
        BL       upload_finish
        CMP      R0,#+0
        BEQ.N    ??user_send_tempIDs_0
        MOVS     R0,#+0
        STRH     R0,[SP, #+4]
        STR      R8,[SP, #+0]
        MOV      R3,R10
        MOVS     R2,#+2
        ADD      R1,SP,#+4
        MOVS     R0,#+9
          CFI FunCall user_send_anyData
        BL       user_send_anyData
??user_send_tempIDs_0:
        B.N      ??user_send_tempIDs_4
??user_send_tempIDs_3:
        LDR.W    R0,??DataTable95
        LDR      R0,[R0, #+64]
        ADD      R0,R0,R9, LSL #+5
        STR      R0,[SP, #+0]
        LDRH     R5,[R11, #+83]
        SUB      R5,R5,R9
        UXTH     R5,R5
        CMP      R5,#+101
        IT       GE
        MOVGE    R5,#+100
        CMP      R10,#+1
        BNE.N    ??user_send_tempIDs_5
        MOVS     R0,#+10
        MULS     R0,R0,R5
        BL       ?Subroutine63
??CrossCallReturnLabel_220:
        B.N      ??user_send_tempIDs_6
??user_send_tempIDs_5:
        MOVS     R0,#+10
        MULS     R0,R0,R5
        ADDS     R0,R0,#+16
          CFI FunCall malloc
        BL       malloc
??user_send_tempIDs_6:
        MOV      R4,R0
        MOVS     R7,#+10
        MULS     R7,R7,R5
        ADDS     R7,R7,#+6
        UXTH     R7,R7
        LDRH     R6,[R11, #+83]
        SUB      R6,R6,R9
        SUBS     R6,R6,R5
        CMP      R4,#+0
        BEQ.N    ??user_send_tempIDs_4
        LDR.W    R0,??DataTable111
          CFI FunCall user_debug
        BL       user_debug
        BL       ?Subroutine37
??CrossCallReturnLabel_122:
        LSRS     R0,R7,#+8
        STRB     R0,[R4, #+5]
        MOVS     R0,#+255
        STRB     R0,[R4, #+6]
        STRB     R8,[R4, #+7]
        MOVS     R0,#+9
        STRB     R0,[R4, #+8]
        STRB     R6,[R4, #+9]
        LSLS     R6,R6,#+16
        LSRS     R6,R6,#+24
        STRB     R6,[R4, #+10]
        STRB     R5,[R4, #+11]
        MOVS     R6,#+0
        LDR      R8,[SP, #+0]
        B.N      ??user_send_tempIDs_7
??user_send_tempIDs_8:
        ADD      R8,R8,#+32
        ADDS     R6,R6,#+1
??user_send_tempIDs_7:
        UXTH     R0,R6
        CMP      R0,R5
        BGE.N    ??user_send_tempIDs_9
        MOVS     R2,#+10
        UXTH     R1,R6
        MULS     R1,R1,R2
        ADDS     R0,R4,R1
        ADD      R1,R0,#+12
        ADD      R0,R8,#+2
          CFI FunCall mem_read
        BL       mem_read
        CMP      R0,#+0
        BNE.N    ??user_send_tempIDs_8
        CMP      R10,#+1
        BNE.N    ??user_send_tempIDs_10
        B.N      ??user_send_tempIDs_11
??user_send_tempIDs_9:
        ADDS     R1,R7,#+2
        BL       ?Subroutine36
??CrossCallReturnLabel_118:
        BL       ?Subroutine64
??CrossCallReturnLabel_223:
        CMP      R10,#+0
        BEQ.N    ??user_send_tempIDs_12
        CMP      R10,#+1
        BEQ.N    ??user_send_tempIDs_13
        B.N      ??user_send_tempIDs_14
??user_send_tempIDs_12:
        ADDS     R7,R7,#+8
        UXTH     R1,R7
        BL       ??Subroutine22_0
??CrossCallReturnLabel_56:
        CBNZ.N   R0,??user_send_tempIDs_15
??user_send_tempIDs_16:
        MOVS     R1,#+0
??user_send_tempIDs_17:
        MOVS     R2,#+0
??user_send_tempIDs_18:
        LDR.W    R0,??DataTable115
        STRH     R2,[R0, R1]
        LDRH     R0,[R11, #+129]
        ORR      R0,R0,#0x4000
        STRH     R0,[R11, #+129]
        CMP      R10,#+1
        BNE.N    ??user_send_tempIDs_10
??user_send_tempIDs_11:
          CFI FunCall comm_tx_release
        BL       comm_tx_release
        B.N      ??user_send_tempIDs_4
??user_send_tempIDs_15:
        MOVS     R1,#+0
??user_send_tempIDs_19:
        ADD      R2,R5,R9
        B.N      ??user_send_tempIDs_18
??user_send_tempIDs_13:
        ADDS     R7,R7,#+8
        UXTH     R0,R7
          CFI FunCall comm_tx_commit
        BL       comm_tx_commit
        MOVS     R1,#+2
        CMP      R0,#+0
        BNE.N    ??user_send_tempIDs_19
        B.N      ??user_send_tempIDs_17
??user_send_tempIDs_14:
        CMP      R10,#+0
        BEQ.N    ??user_send_tempIDs_16
        MOVS     R1,#+2
        B.N      ??user_send_tempIDs_17
??user_send_tempIDs_10:
        MOV      R0,R4
          CFI FunCall free
        BL       free
??user_send_tempIDs_4:
        POP      {R0-R2,R4-R11,PC}
          CFI EndBlock cfiBlock228

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable83:
        DATA32
        DC32     device

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond229 Using cfiCommon0
          CFI Function user_send_logRecords
          CFI Conditional ??CrossCallReturnLabel_147
          CFI R4 Frame(CFA, -36)
          CFI R5 Frame(CFA, -32)
          CFI R6 Frame(CFA, -28)
          CFI R7 Frame(CFA, -24)
          CFI R8 Frame(CFA, -20)
          CFI R9 Frame(CFA, -16)
          CFI R10 Frame(CFA, -12)
          CFI R11 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+48
          CFI Block cfiCond230 Using cfiCommon0
          CFI (cfiCond230) Function user_send_tempIDs
          CFI (cfiCond230) Conditional ??CrossCallReturnLabel_148
          CFI (cfiCond230) R4 Frame(CFA, -36)
          CFI (cfiCond230) R5 Frame(CFA, -32)
          CFI (cfiCond230) R6 Frame(CFA, -28)
          CFI (cfiCond230) R7 Frame(CFA, -24)
          CFI (cfiCond230) R8 Frame(CFA, -20)
          CFI (cfiCond230) R9 Frame(CFA, -16)
          CFI (cfiCond230) R10 Frame(CFA, -12)
          CFI (cfiCond230) R11 Frame(CFA, -8)
          CFI (cfiCond230) R14 Frame(CFA, -4)
          CFI (cfiCond230) CFA R13+48
          CFI Block cfiCond231 Using cfiCommon0
          CFI (cfiCond231) Function fw_resume_after_boot
          CFI (cfiCond231) Conditional ??CrossCallReturnLabel_149
          CFI (cfiCond231) R4 Frame(CFA, -8)
          CFI (cfiCond231) R14 Frame(CFA, -4)
          CFI (cfiCond231) CFA R13+48
          CFI Block cfiPicker232 Using cfiCommon1
          CFI (cfiPicker232) NoFunction
          CFI (cfiPicker232) Picker
        THUMB
?Subroutine42:
        MOV      R1,R0
        MOVW     R0,#+65535
        CMP      R1,R0
        BX       LR
          CFI EndBlock cfiCond229
          CFI EndBlock cfiCond230
          CFI EndBlock cfiCond231
          CFI EndBlock cfiPicker232

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond233 Using cfiCommon0
          CFI Function user_send_crcMemoryID
          CFI Conditional ??CrossCallReturnLabel_121
          CFI R4 Frame(CFA, -32)
          CFI R5 Frame(CFA, -28)
          CFI R6 Frame(CFA, -24)
          CFI R7 Frame(CFA, -20)
          CFI R8 Frame(CFA, -16)
          CFI R9 Frame(CFA, -12)
          CFI R10 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+40
          CFI Block cfiCond234 Using cfiCommon0
          CFI (cfiCond234) Function user_send_tempIDs
          CFI (cfiCond234) Conditional ??CrossCallReturnLabel_122
          CFI (cfiCond234) R4 Frame(CFA, -36)
          CFI (cfiCond234) R5 Frame(CFA, -32)
          CFI (cfiCond234) R6 Frame(CFA, -28)
          CFI (cfiCond234) R7 Frame(CFA, -24)
          CFI (cfiCond234) R8 Frame(CFA, -20)
          CFI (cfiCond234) R9 Frame(CFA, -16)
          CFI (cfiCond234) R10 Frame(CFA, -12)
          CFI (cfiCond234) R11 Frame(CFA, -8)
          CFI (cfiCond234) R14 Frame(CFA, -4)
          CFI (cfiCond234) CFA R13+48
          CFI Block cfiPicker235 Using cfiCommon1
          CFI (cfiPicker235) NoFunction
          CFI (cfiPicker235) Picker
        THUMB
?Subroutine37:
        MOVS     R0,#+83
        STRB     R0,[R4, #+0]
        MOVS     R0,#+76
        STRB     R0,[R4, #+1]
        MOVS     R0,#+68
        STRB     R0,[R4, #+2]
        MOVS     R0,#+78
        STRB     R0,[R4, #+3]
        STRB     R7,[R4, #+4]
        BX       LR
          CFI EndBlock cfiCond233
          CFI EndBlock cfiCond234
          CFI EndBlock cfiPicker235

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock236 Using cfiCommon0
          CFI Function user_getFWblockFromServer
        THUMB
user_getFWblockFromServer:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        SUB      SP,SP,#+16
          CFI CFA R13+40
        MOV      R5,R0
        MOV      R6,R1
          CFI FunCall fw_async_active
        BL       fw_async_active
        CBNZ.N   R0,??user_getFWblockFromServer_0
        CMP      R5,#+1
        BNE.N    ??user_getFWblockFromServer_1
        MOVS     R0,#+16
          CFI FunCall user_comm_acquire
        BL       user_comm_acquire
        MOV      R4,R0
        B.N      ??user_getFWblockFromServer_2
??user_getFWblockFromServer_1:
        MOV      R4,SP
??user_getFWblockFromServer_2:
        CBNZ.N   R4,??user_getFWblockFromServer_3
??user_getFWblockFromServer_0:
        MOVS     R0,#+0
        B.N      ??user_getFWblockFromServer_4
??user_getFWblockFromServer_3:
        MOVS     R0,R5
        BEQ.N    ??user_getFWblockFromServer_5
        LDR.W    R7,??DataTable115_1
        MOV      R2,R6
        LDR.W    R1,??DataTable115_2
        MOV      R0,R7
          CFI FunCall sprintf
        BL       sprintf
        MOV      R0,R7
          CFI FunCall user_debug
        BL       user_debug
??user_getFWblockFromServer_5:
        BL       ?Subroutine38
??CrossCallReturnLabel_135:
        MOVS     R0,#+5
        BL       ?Subroutine5
??CrossCallReturnLabel_1:
        MOVS     R0,#+13
        STRB     R0,[R4, #+8]
        STRB     R6,[R4, #+9]
        LSRS     R6,R6,#+8
        STRB     R6,[R4, #+10]
        BL       ?Subroutine47
??CrossCallReturnLabel_169:
        STRB     R0,[R4, #+11]
        LSRS     R0,R0,#+8
        STRB     R0,[R4, #+12]
        CMP      R5,#+1
        BNE.N    ??user_getFWblockFromServer_6
        MOVS     R0,#+13
          CFI FunCall comm_tx_commit
        BL       comm_tx_commit
        B.N      ??user_getFWblockFromServer_4
??user_getFWblockFromServer_6:
        CBNZ.N   R5,??CrossCallReturnLabel_55
        MOVS     R1,#+13
        BL       ??Subroutine22_0
??CrossCallReturnLabel_55:
        MOVS     R0,#+1
??user_getFWblockFromServer_4:
        ADD      SP,SP,#+20
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI EndBlock cfiBlock236

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable84:
        DATA32
        DC32     ?_54

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond237 Using cfiCommon0
          CFI Function reader_reply_poll
          CFI Conditional ??CrossCallReturnLabel_0
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
          CFI Block cfiCond238 Using cfiCommon0
          CFI (cfiCond238) Function user_getFWblockFromServer
          CFI (cfiCond238) Conditional ??CrossCallReturnLabel_1
          CFI (cfiCond238) R4 Frame(CFA, -20)
          CFI (cfiCond238) R5 Frame(CFA, -16)
          CFI (cfiCond238) R6 Frame(CFA, -12)
          CFI (cfiCond238) R7 Frame(CFA, -8)
          CFI (cfiCond238) R14 Frame(CFA, -4)
          CFI (cfiCond238) CFA R13+40
          CFI Block cfiPicker239 Using cfiCommon1
          CFI (cfiPicker239) NoFunction
          CFI (cfiPicker239) Picker
        THUMB
?Subroutine5:
        STRB     R0,[R4, #+4]
        MOVS     R0,#+0
        STRB     R0,[R4, #+5]
        MOVS     R0,#+255
        STRB     R0,[R4, #+6]
        LDR.W    R0,??DataTable116
        LDRB     R1,[R0, #+0]
        STRB     R1,[R4, #+7]
        LDRB     R1,[R0, #+0]
        ADDS     R1,R1,#+1
        STRB     R1,[R0, #+0]
        BX       LR
          CFI EndBlock cfiCond237
          CFI EndBlock cfiCond238
          CFI EndBlock cfiPicker239

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock240 Using cfiCommon0
          CFI Function user_send_Photos
        THUMB
user_send_Photos:
        MOVS     R3,#+0
        MOVS     R2,#+0
          CFI FunCall photo_request_answer
        B.W      photo_request_answer
          CFI EndBlock cfiBlock240

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
id_nextEnd:
        DS8 4

        SECTION `.data`:DATA:REORDER:NOROOT(0)
        DATA
id_endScan:
        DATA8
        DC8 1

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock241 Using cfiCommon0
          CFI Function id_first_pass
          CFI NoCalls
        THUMB
id_first_pass:
        SUB      SP,SP,#+4
          CFI CFA R13+4
        ADD      R3,R0,#+22
        LDR      R2,[R3, #+0]
        STR      R2,[SP, #+0]
        CMN      R2,#+1
        BEQ.N    ??id_first_pass_0
        LDR.W    R0,??DataTable116_1
        CMP      R2,R0
        BCS.N    ??id_first_pass_1
??id_first_pass_0:
        MOVS     R0,#+0
        B.N      ??id_first_pass_2
??id_first_pass_1:
        MOV      R0,R2
??id_first_pass_2:
        ADD      SP,SP,#+4
          CFI CFA R13+0
        BX       LR
          CFI EndBlock cfiBlock241

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock242 Using cfiCommon0
          CFI Function id_set_first_pass
          CFI NoCalls
        THUMB
id_set_first_pass:
        PUSH     {R1}
          CFI CFA R13+4
        ADD      R3,R0,#+22
        STR      R1,[R3, #+0]
        ADD      SP,SP,#+4
          CFI CFA R13+0
        BX       LR
          CFI EndBlock cfiBlock242

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock243 Using cfiCommon0
          CFI Function id_expired
        THUMB
id_expired:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        MOV      R4,R0
          CFI FunCall id_first_pass
        BL       id_first_pass
        LDRH     R1,[R4, #+17]
        CMP      R1,#+0
        IT       NE
        CMPNE    R0,#+0
        BEQ.N    ??id_expired_0
        LDR.W    R2,??DataTable102
        LDR      R1,[R2, #+47]
        CMP      R1,R0
        BCC.N    ??id_expired_0
        SUBS     R0,R1,R0
        LDRH     R2,[R4, #+17]
        MOVS     R1,#+60
        MULS     R2,R1,R2
        CMP      R0,R2
        SBCS     R0,R0,R0
        MVNS     R0,R0
        LSRS     R0,R0,#+31
        POP      {R4,PC}
??id_expired_0:
        MOVS     R0,#+0
        POP      {R4,PC}
          CFI EndBlock cfiBlock243

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock244 Using cfiCommon0
          CFI Function id_time_end
        THUMB
id_time_end:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        MOV      R4,R0
          CFI FunCall id_first_pass
        BL       id_first_pass
        MOV      R1,R0
        LDRH     R0,[R4, #+17]
        CMP      R0,#+0
        ITTT     NE
        CMPNE    R1,#+0
        LDRBNE   R0,[R4, #+28]
        CMPNE    R0,#+56
        BEQ.N    ??id_time_end_0
        MOVS     R0,#+60
        MOV      R3,#+4294967295
        LDRH     R2,[R4, #+17]
        MLS      R3,R0,R2,R3
        CMP      R3,R1
        BCC.N    ??id_time_end_0
        MLA      R0,R0,R2,R1
        LDR.W    R2,??DataTable117
        UDIV     R2,R0,R2
        LDR.W    R3,??DataTable117
        UDIV     R1,R1,R3
        CMP      R2,R1
        BEQ.N    ??id_time_end_1
??id_time_end_0:
        MOVS     R0,#+0
??id_time_end_1:
        POP      {R4,PC}
          CFI EndBlock cfiBlock244

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock245 Using cfiCommon0
          CFI Function id_watch_end
          CFI NoCalls
        THUMB
id_watch_end:
        CBZ.N    R0,??id_watch_end_0
        LDR.W    R2,??DataTable118
        LDR      R1,[R2, #+0]
        CBZ.N    R1,??id_watch_end_1
        LDR      R1,[R2, #+0]
        CMP      R0,R1
        IT       HI
        MOVHI    R0,R1
??id_watch_end_1:
        STR      R0,[R2, #+0]
??id_watch_end_0:
        BX       LR
          CFI EndBlock cfiBlock245

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock246 Using cfiCommon0
          CFI Function id_today
          CFI NoCalls
        THUMB
id_today:
        LDR.W    R2,??DataTable102
        LDR      R0,[R2, #+47]
        LDR.W    R1,??DataTable116_1
        CMP      R0,R1
        ITTTE    CS
        MOVCS    R1,R0
        LDRCS.W  R0,??DataTable117
        UDIVCS   R0,R1,R0
        MOVCC    R0,#+0
        UXTH     R0,R0
        BX       LR
          CFI EndBlock cfiBlock246

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable90:
        DATA32
        DC32     ?_55

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable90_1:
        DATA32
        DC32     fw_status_pending

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable90_2:
        DATA32
        DC32     ?_0

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock247 Using cfiCommon0
          CFI Function id_pass_day
          CFI NoCalls
        THUMB
id_pass_day:
        SUB      SP,SP,#+4
          CFI CFA R13+4
        ADD      R3,R0,#+26
        LDRB     R2,[R3, #+0]
        STRB     R2,[SP, #+0]
        LDRB     R2,[R3, #+1]
        STRB     R2,[SP, #+1]
        LDRH     R1,[SP, #+0]
        MOVW     R0,#+65535
        CMP      R1,R0
        ITE      EQ
        MOVEQ    R0,#+0
        MOVNE    R0,R1
        ADD      SP,SP,#+4
          CFI CFA R13+0
        BX       LR
          CFI EndBlock cfiBlock247

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock248 Using cfiCommon0
          CFI Function id_set_pass_day
          CFI NoCalls
        THUMB
id_set_pass_day:
        PUSH     {R1}
          CFI CFA R13+4
        ADD      R3,R0,#+26
        LDRB     R0,[SP, #+0]
        STRB     R0,[R3, #+0]
        LDRB     R0,[SP, #+1]
        STRB     R0,[R3, #+1]
        ADD      SP,SP,#+4
          CFI CFA R13+0
        BX       LR
          CFI EndBlock cfiBlock248

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock249 Using cfiCommon0
          CFI Function id_new_day
        THUMB
id_new_day:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R4,R0
          CFI FunCall id_today
        BL       id_today
        MOV      R5,R0
        MOV      R0,R4
          CFI FunCall id_pass_day
        BL       id_pass_day
        MOVS     R1,R5
        IT       NE
        CMPNE    R0,R1
        BNE.N    ??id_new_day_0
        MOVS     R0,#+0
        POP      {R1,R4,R5,PC}
??id_new_day_0:
        CBZ.N    R0,??id_new_day_1
        LDRB     R0,[R4, #+20]
        STRB     R0,[R4, #+21]
??id_new_day_1:
        MOV      R0,R4
          CFI FunCall id_set_pass_day
        BL       id_set_pass_day
        MOVS     R0,#+1
        POP      {R1,R4,R5,PC}
          CFI EndBlock cfiBlock249

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock250 Using cfiCommon0
          CFI Function usb_cmd_without_pin
          CFI NoCalls
        THUMB
usb_cmd_without_pin:
        CMP      R0,#+100
        IT       NE
        CMPNE    R0,#+101
        BEQ.N    ??usb_cmd_without_pin_0
        CMP      R0,#+102
        ITT      NE
        CMPNE    R0,#+111
        CMPNE    R0,#+112
        BNE.N    ??usb_cmd_without_pin_1
??usb_cmd_without_pin_0:
        MOVS     R0,#+1
        BX       LR
??usb_cmd_without_pin_1:
        MOVS     R0,#+0
        BX       LR
          CFI EndBlock cfiBlock250

        SECTION `.bss`:DATA:REORDER:NOROOT(1)
        DATA
status_reported:
        DS8 2

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
status_reported_ms:
        DS8 4

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock251 Using cfiCommon0
          CFI Function status_event_due
          CFI NoCalls
        THUMB
status_event_due:
        LDR.W    R2,??DataTable118_1
        LDRH     R2,[R2, #+0]
        EORS     R0,R2,R0
        MOVW     R2,#+2303
        TST      R0,R2
        BEQ.N    ??status_event_due_0
        LDR.W    R0,??DataTable119
        LDR      R0,[R0, #+0]
        SUBS     R1,R1,R0
        CMP      R1,#+1000
        SBCS     R0,R0,R0
        MVNS     R0,R0
        LSRS     R0,R0,#+31
        BX       LR
??status_event_due_0:
        MOVS     R0,#+0
        BX       LR
          CFI EndBlock cfiBlock251

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable91:
        DATA32
        DC32     ?_56

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock252 Using cfiCommon0
          CFI Function status_event_sent
          CFI NoCalls
        THUMB
status_event_sent:
        LDR.W    R2,??DataTable118_1
        STRH     R0,[R2, #+0]
        LDR.W    R0,??DataTable119
        STR      R1,[R0, #+0]
        BX       LR
          CFI EndBlock cfiBlock252

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock253 Using cfiCommon0
          CFI Function fw_update_quiet
          CFI NoCalls
        THUMB
fw_update_quiet:
        LDR.W    R0,??DataTable102
        LDRH     R0,[R0, #+129]
        LSRS     R0,R0,#+11
        AND      R0,R0,#0x1
        BX       LR
          CFI EndBlock cfiBlock253

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock254 Using cfiCommon0
          CFI Function fw_reason_code
        THUMB
fw_reason_code:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOVS     R4,R0
        BNE.N    ??fw_reason_code_0
        MOVS     R0,#+0
        POP      {R1,R4,R5,PC}
??fw_reason_code_0:
        MOVS     R5,#+0
??fw_reason_code_1:
        LDR.W    R1,??DataTable119_1
        UXTB     R0,R5
        LDR      R1,[R1, R0, LSL #+2]
        MOV      R0,R4
          CFI FunCall strcmp
        BL       strcmp
        CBZ.N    R0,??fw_reason_code_2
        ADDS     R5,R5,#+1
        UXTB     R0,R5
        CMP      R0,#+15
        BCC.N    ??fw_reason_code_1
        B.N      ??fw_reason_code_3
??fw_reason_code_2:
        UXTB     R5,R5
        ADDS     R0,R5,#+1
        POP      {R1,R4,R5,PC}
??fw_reason_code_3:
        MOVS     R2,#+5
        LDR.W    R1,??DataTable119_2
        MOV      R0,R4
          CFI FunCall strncmp
        BL       strncmp
        CBNZ.N   R0,??fw_reason_code_4
        LDRB     R0,[R4, #+5]
        SUBS     R0,R0,#+48
        CMP      R0,#+10
        BCS.N    ??fw_reason_code_4
        LDRB     R0,[R4, #+5]
        SUBS     R0,R0,#+28
        POP      {R1,R4,R5,PC}
??fw_reason_code_4:
        MOVS     R0,#+30
        POP      {R1,R4,R5,PC}
          CFI EndBlock cfiBlock254

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable94:
        DATA32
        DC32     ?_57

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
`fw_reason_code::names`:
        DATA32
        DC32 ?_68, ?_69, ?_70, ?_71, ?_72, ?_73, ?_74, ?_75, ?_76, ?_77, ?_41
        DC32 ?_78, ?_79, ?_80, ?_42

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock255 Using cfiCommon0
          CFI Function fw_log
        THUMB
fw_log:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        MOV      R4,R0
        MOV      R0,R1
          CFI FunCall fw_reason_code
        BL       fw_reason_code
        MOV      R1,R0
        MOV      R0,R4
        POP      {R4,LR}
          CFI R4 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall log_addUpdateEvent
        B.W      log_addUpdateEvent
          CFI EndBlock cfiBlock255

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
fw_write_errors:
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
fw_rollback_pos:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
fw_rollback_end:
        DS8 4

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock256 Using cfiCommon0
          CFI Function fw_abort
        THUMB
fw_abort:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R4,R0
          CFI FunCall fw_async_cancel
        BL       fw_async_cancel
        MOVS     R1,#+0
        LDR.W    R0,??DataTable119_3
        STR      R1,[R0, #+0]
        LDR.N    R5,??DataTable102
        STRB     R1,[R5, #+164]
        BL       ?Subroutine26
??CrossCallReturnLabel_84:
        STRH     R0,[R5, #+148]
        STRH     R0,[R5, #+172]
        BL       ?Subroutine20
??CrossCallReturnLabel_45:
        CBZ.N    R4,??fw_abort_0
        LDR.W    R1,??DataTable119_4
        MOV      R0,R4
          CFI FunCall strcmp
        BL       strcmp
        CMP      R0,#+0
        ITE      EQ
        MOVEQ    R0,#+134
        MOVNE    R0,#+133
        MOV      R1,R4
          CFI FunCall fw_log
        BL       fw_log
??fw_abort_0:
        LDRB     R0,[R5, #+178]
        CBZ.N    R0,??fw_abort_1
        POP      {R0,R4,R5,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall user_saveindex
        B.W      user_saveindex
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
??fw_abort_1:
        POP      {R0,R4,R5,PC}
          CFI EndBlock cfiBlock256

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable95:
        DATA32
        DC32     mem

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock257 Using cfiCommon0
          CFI Function fw_end
        THUMB
fw_end:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        MOV      R4,R1
          CFI FunCall fw_abort
        BL       fw_abort
        LDR.N    R1,??DataTable102
        LDRB     R0,[R1, #+178]
        CBZ.N    R0,??fw_end_0
        LDRB     R0,[R1, #+133]
        CBZ.N    R0,??fw_end_1
??fw_end_0:
        MOV      R3,R4
        LDRB     R2,[R1, #+178]
        MOVS     R1,#+253
        MOVS     R0,#+20
        POP      {R4,LR}
          CFI R4 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall user_send_statusUpdateFW
        B.W      user_send_statusUpdateFW
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
??fw_end_1:
        POP      {R4,PC}
          CFI EndBlock cfiBlock257

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock258 Using cfiCommon0
          CFI Function fw_image_foreign
          CFI NoCalls
        THUMB
fw_image_foreign:
        PUSH     {R3,R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+12
        ADD      R4,R0,#+32
        LDR      R3,[R4, #+0]
        STR      R3,[SP, #+0]
        MOVS     R0,R1
        BEQ.N    ??fw_image_foreign_0
        LSRS     R2,R3,#+8
        LSLS     R2,R2,#+8
        LDR.W    R0,??DataTable119_5
        CMP      R2,R0
        BNE.N    ??fw_image_foreign_0
        UXTB     R0,R3
        CMP      R0,R1
        BEQ.N    ??fw_image_foreign_0
        MOVS     R0,#+1
        POP      {R1,R4,PC}
??fw_image_foreign_0:
        MOVS     R0,#+0
        POP      {R1,R4,PC}
          CFI EndBlock cfiBlock258

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable97:
        DATA32
        DC32     ?_58

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock259 Using cfiCommon0
          CFI Function fw_start_rejected
          CFI NoCalls
        THUMB
fw_start_rejected:
        PUSH     {R4,R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+12
        MOVS     R4,R0
        BEQ.N    ??fw_start_rejected_0
        CMP      R0,#+1
        BNE.N    ??fw_start_rejected_1
        MOVS     R0,#+0
        POP      {R4,R5,PC}
??fw_start_rejected_1:
        LDR.W    R0,??DataTable119_6
        CMP      R2,R0
        BCS.N    ??fw_start_rejected_0
        TST      R2,#0x3
        BNE.N    ??fw_start_rejected_0
        LDR      R4,[SP, #+12]
        ADDW     R0,R2,#+1023
        CMP      R4,R0, LSR #+10
        BCC.N    ??fw_start_rejected_0
        LDR.W    R0,??DataTable119_7
        LDR      R5,[R0, #+4]
        LSLS     R0,R4,#+10
        ADD      R0,R0,#+512
        CMP      R5,R0
        BCC.N    ??fw_start_rejected_0
        CMP      R1,#+0
        IT       NE
        CMPNE    R2,#+0
        BEQ.N    ??fw_start_rejected_0
        CBZ.N    R3,??fw_start_rejected_0
        SUBS     R0,R4,#+1
        SBCS     R0,R0,R0
        LSRS     R0,R0,#+31
        POP      {R4,R5,PC}
??fw_start_rejected_0:
        MOVS     R0,#+1
        POP      {R4,R5,PC}
          CFI EndBlock cfiBlock259

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock260 Using cfiCommon0
          CFI Function fw_start_refused
        THUMB
fw_start_refused:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
        LDR.W    R0,??DataTable119_8
        LDRB     R0,[R0, #+0]
        CMP      R0,#+48
        BGE.N    ??fw_start_refused_0
        LDR.W    R0,??DataTable119_9
        POP      {R1,PC}
??fw_start_refused_0:
          CFI FunCall boot_manual_busy
        BL       boot_manual_busy
        CBZ.N    R0,??fw_start_refused_1
        LDR.W    R0,??DataTable119_10
        POP      {R1,PC}
??fw_start_refused_1:
          CFI FunCall app_trial_active
        BL       app_trial_active
        CBZ.N    R0,??fw_start_refused_2
        LDR.W    R0,??DataTable119_11
        POP      {R1,PC}
??fw_start_refused_2:
        LDR.N    R1,??DataTable102
        LDRB     R0,[R1, #+171]
        CMP      R0,#+0
        ITT      EQ
        LDRBEQ   R0,[R1, #+164]
        CMPEQ    R0,#+0
        BNE.N    ??fw_start_refused_3
        LDR.W    R0,??DataTable119_3
        LDR      R0,[R0, #+0]
        CBZ.N    R0,??fw_start_refused_4
??fw_start_refused_3:
        LDR.W    R0,??DataTable119_12
??fw_start_refused_4:
        POP      {R1,PC}
          CFI EndBlock cfiBlock260

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable99:
        DATA32
        DC32     ?_59

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock261 Using cfiCommon0
          CFI Function fw_same_download
          CFI NoCalls
        THUMB
fw_same_download:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        SUB      SP,SP,#+32
          CFI CFA R13+56
        MOV      R4,R0
        MOV      R5,R1
        MOV      R6,R2
        MOV      R7,R3
        LDR.N    R1,??DataTable102
        LDRB     R0,[R1, #+164]
        CBZ.N    R0,??fw_same_download_0
        LDR      R0,[SP, #+60]
        LDRB     R2,[R1, #+178]
        CMP      R2,R0
        ITTT     EQ
        LDREQ    R0,[SP, #+56]
        LDRHEQ   R1,[R1, #+148]
        CMPEQ    R0,R1
        BNE.N    ??fw_same_download_0
        MOVS     R2,#+32
        MOV      R1,SP
        BL       ?Subroutine51
??CrossCallReturnLabel_182:
        CBZ.N    R0,??fw_same_download_0
        LDRH     R0,[SP, #+5]
        CMP      R0,R4
        ITT      EQ
        LDREQ    R0,[SP, #+20]
        CMPEQ    R0,R5
        BNE.N    ??fw_same_download_0
        LDR      R0,[SP, #+8]
        CMP      R0,R6
        ITTT     EQ
        LDREQ    R0,[SP, #+12]
        CMPEQ    R0,R7
        MOVEQ    R0,#+1
        BEQ.N    ??fw_same_download_1
??fw_same_download_0:
        MOVS     R0,#+0
??fw_same_download_1:
        ADD      SP,SP,#+36
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI EndBlock cfiBlock261

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable100:
        DATA32
        DC32     ?_60

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable100_1:
        DATA32
        DC32     unreg_sms_pending

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock262 Using cfiCommon0
          CFI Function fw_rollback
        THUMB
fw_rollback:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        SUB      SP,SP,#+32
          CFI CFA R13+48
        MOV      R4,R0
        LDR.W    R5,??DataTable119_7
        MOVS     R2,#+32
        MOV      R1,SP
        LDR      R0,[R5, #+8]
          CFI FunCall mem_read
        BL       mem_read
        CBZ.N    R0,??CrossCallReturnLabel_240
        LDR      R0,[SP, #+20]
        CMN      R0,#+1
        ITT      NE
        LDRNE    R0,[SP, #+8]
        CMNNE    R0,#+1
        BEQ.N    ??fw_rollback_0
        LDR      R0,[SP, #+12]
        CMN      R0,#+1
        ITTT     NE
        LDRHNE   R1,[SP, #+5]
        MOVWNE   R0,#+65535
        CMPNE    R1,R0
        BNE.N    ??fw_rollback_1
??fw_rollback_0:
        LDR.W    R1,??DataTable120
        B.N      ??fw_rollback_2
??fw_rollback_1:
        LDR      R1,[SP, #+8]
        LDR.W    R0,??DataTable119_6
        CMP      R1,R0
        BCS.N    ??fw_rollback_3
        LDR      R2,[SP, #+12]
        LDR      R0,[R5, #+8]
        BL       ?Subroutine60
??CrossCallReturnLabel_212:
        CBNZ.N   R0,??fw_rollback_4
??fw_rollback_3:
        LDR.W    R1,??DataTable120_1
??fw_rollback_2:
        BL       ??Subroutine68_0
??CrossCallReturnLabel_240:
        MOVS     R0,#+2
        B.N      ??fw_rollback_5
??fw_rollback_4:
        MOVS     R1,#+0
        LDR.W    R0,??DataTable120_2
        STR      R1,[R0, #+0]
        LDR      R1,[SP, #+8]
        ADD      R1,R1,#+512
        LDR.W    R0,??DataTable119_3
        STR      R1,[R0, #+0]
        LDR.N    R0,??DataTable102
        STRB     R4,[R0, #+178]
        LDRH     R1,[R0, #+129]
        ORR      R1,R1,#0x800
        STRH     R1,[R0, #+129]
        MOVS     R0,#+0
??fw_rollback_5:
        ADD      SP,SP,#+36
          CFI CFA R13+12
        POP      {R4,R5,PC}
          CFI EndBlock cfiBlock262

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable101:
        DATA32
        DC32     ?_61

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable101_1:
        DATA32
        DC32     reader_out

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable101_2:
        DATA32
        DC32     ?_62

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock263 Using cfiCommon0
          CFI Function fw_rollback_poll
        THUMB
fw_rollback_poll:
        PUSH     {R4-R8,LR}
          CFI R14 Frame(CFA, -4)
          CFI R8 Frame(CFA, -8)
          CFI R7 Frame(CFA, -12)
          CFI R6 Frame(CFA, -16)
          CFI R5 Frame(CFA, -20)
          CFI R4 Frame(CFA, -24)
          CFI CFA R13+24
        SUB      SP,SP,#+32
          CFI CFA R13+56
        LDR.W    R4,??DataTable119_3
        LDR      R0,[R4, #+0]
        CMP      R0,#+0
        BEQ.N    ??fw_rollback_poll_0
        LDR.W    R5,??DataTable120_2
        MOV      R1,R0
        LDR      R0,[R5, #+0]
        SUBS     R1,R1,R0
        CMP      R1,#+129
        ITEE     CS
        MOVCS    R6,#+128
        LDRCC    R6,[R4, #+0]
        SUBCC    R6,R6,R0
        LSLS     R0,R0,#+20
        BNE.N    ??fw_rollback_poll_1
        LDR.W    R0,??DataTable119_7
        LDR      R0,[R0, #+0]
        LDR      R1,[R5, #+0]
        ADDS     R0,R1,R0
          CFI FunCall mem_erase_sector
        BL       mem_erase_sector
        CBZ.N    R0,??fw_rollback_poll_2
??fw_rollback_poll_1:
        LDR.W    R8,??DataTable115_1
        LDR.W    R7,??DataTable119_7
        UXTH     R2,R6
        MOV      R1,R8
        LDR      R0,[R7, #+8]
        LDR      R3,[R5, #+0]
        ADDS     R0,R3,R0
          CFI FunCall mem_read
        BL       mem_read
        CBNZ.N   R0,??fw_rollback_poll_3
        MOVS     R1,#+0
        LDR.W    R0,??DataTable120_3
        B.N      ??fw_rollback_poll_4
??fw_rollback_poll_3:
        LDR      R0,[R5, #+0]
        CBNZ.N   R0,??fw_rollback_poll_5
        MOVS     R2,#+255
        MOVS     R1,#+4
        MOV      R0,R8
          CFI FunCall __aeabi_memset4
        BL       __aeabi_memset4
??fw_rollback_poll_5:
        UXTH     R2,R6
        MOV      R1,R8
        LDR      R0,[R7, #+0]
        LDR      R3,[R5, #+0]
        ADDS     R0,R3,R0
          CFI FunCall mem_write
        BL       mem_write
        CBNZ.N   R0,??fw_rollback_poll_6
??fw_rollback_poll_2:
        MOVS     R1,#+0
        LDR.W    R0,??DataTable120_4
        B.N      ??fw_rollback_poll_4
??fw_rollback_poll_6:
        LDR      R0,[R5, #+0]
        UXTH     R6,R6
        ADDS     R6,R6,R0
        STR      R6,[R5, #+0]
          CFI FunCall wdt_reload
        BL       wdt_reload
        LDR      R1,[R5, #+0]
        LDR      R0,[R4, #+0]
        CMP      R1,R0
        BCC.N    ??fw_rollback_poll_0
        MOVS     R0,#+0
        STR      R0,[R4, #+0]
        MOVS     R2,#+32
        MOV      R1,SP
        LDR      R0,[R7, #+0]
          CFI FunCall mem_read
        BL       mem_read
        CBZ.N    R0,??fw_rollback_poll_7
        LDR      R2,[SP, #+12]
        LDR      R1,[SP, #+8]
        LDR      R0,[R7, #+0]
        BL       ?Subroutine60
??CrossCallReturnLabel_211:
        CBNZ.N   R0,??fw_rollback_poll_8
??fw_rollback_poll_7:
        MOVS     R1,#+0
        ADR.N    R0,??DataTable105_2
??fw_rollback_poll_4:
          CFI FunCall fw_end
        BL       fw_end
        B.N      ??fw_rollback_poll_0
??fw_rollback_poll_8:
        MOVS     R1,#+0
        MOVS     R0,#+137
          CFI FunCall fw_log
        BL       fw_log
        MOVS     R1,#+1
        LDR.N    R0,??DataTable102
        STRB     R1,[R0, #+171]
??fw_rollback_poll_0:
        ADD      SP,SP,#+32
          CFI CFA R13+24
        POP      {R4-R8,PC}
          CFI EndBlock cfiBlock263

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable102:
        DATA32
        DC32     device

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable102_1:
        DATA32
        DC32     ?_63

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable102_2:
        DATA32
        DC32     config

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond264 Using cfiCommon0
          CFI Function fw_rollback
          CFI Conditional ??CrossCallReturnLabel_212
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+48
          CFI Block cfiCond265 Using cfiCommon0
          CFI (cfiCond265) Function fw_rollback_poll
          CFI (cfiCond265) Conditional ??CrossCallReturnLabel_211
          CFI (cfiCond265) R4 Frame(CFA, -24)
          CFI (cfiCond265) R5 Frame(CFA, -20)
          CFI (cfiCond265) R6 Frame(CFA, -16)
          CFI (cfiCond265) R7 Frame(CFA, -12)
          CFI (cfiCond265) R8 Frame(CFA, -8)
          CFI (cfiCond265) R14 Frame(CFA, -4)
          CFI (cfiCond265) CFA R13+56
          CFI Block cfiCond266 Using cfiCommon0
          CFI (cfiCond266) Function fw_take_block
          CFI (cfiCond266) Conditional ??CrossCallReturnLabel_210
          CFI (cfiCond266) R4 Frame(CFA, -24)
          CFI (cfiCond266) R5 Frame(CFA, -20)
          CFI (cfiCond266) R6 Frame(CFA, -16)
          CFI (cfiCond266) R7 Frame(CFA, -12)
          CFI (cfiCond266) R8 Frame(CFA, -8)
          CFI (cfiCond266) R14 Frame(CFA, -4)
          CFI (cfiCond266) CFA R13+64
          CFI Block cfiPicker267 Using cfiCommon1
          CFI (cfiPicker267) NoFunction
          CFI (cfiPicker267) Picker
        THUMB
?Subroutine60:
        ADD      R0,R0,#+512
          CFI (cfiCond264) FunCall fw_rollback user_checkFirmware
          CFI (cfiCond265) FunCall fw_rollback_poll user_checkFirmware
          CFI (cfiCond266) FunCall fw_take_block user_checkFirmware
        B.W      user_checkFirmware
          CFI EndBlock cfiCond264
          CFI EndBlock cfiCond265
          CFI EndBlock cfiCond266
          CFI EndBlock cfiPicker267

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock268 Using cfiCommon0
          CFI Function fw_start
        THUMB
fw_start:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        SUB      SP,SP,#+56
          CFI CFA R13+72
        MOV      R4,R1
        ADD      R3,SP,#+8
        LDRB     R1,[R0, #+0]
        STRB     R1,[R3, #+0]
        LDRB     R1,[R0, #+1]
        STRB     R1,[R3, #+1]
        ADDS     R3,R0,#+2
        ADD      R5,SP,#+20
        LDR      R2,[R3, #+0]
        STR      R2,[R5, #+0]
        ADDS     R3,R0,#+6
        ADD      R1,SP,#+16
        LDR      R2,[R3, #+0]
        STR      R2,[R1, #+0]
        ADD      R3,R0,#+10
        ADD      R1,SP,#+12
        LDR      R2,[R3, #+0]
        STR      R2,[R1, #+0]
        ADD      R3,R0,#+14
        ADD      R0,SP,#+10
        BL       ?Subroutine14
??CrossCallReturnLabel_26:
        BL       ?Subroutine11
??CrossCallReturnLabel_16:
          CFI FunCall fw_start_rejected
        BL       fw_start_rejected
        CBZ.N    R0,??fw_start_0
        BL       ?Subroutine68
??CrossCallReturnLabel_242:
        MOVS     R0,#+1
        B.N      ??fw_start_1
??fw_start_0:
        LDRH     R0,[SP, #+8]
        CMP      R0,#+1
        BEQ.N    ??fw_start_2
        STR      R4,[SP, #+4]
        BL       ?Subroutine11
??CrossCallReturnLabel_17:
          CFI FunCall fw_same_download
        BL       fw_same_download
        CBZ.N    R0,??fw_start_2
        MOVS     R0,#+0
        LDR.W    R1,??DataTable120_5
        STR      R0,[R1, #+174]
        B.N      ??fw_start_1
??fw_start_2:
          CFI FunCall fw_start_refused
        BL       fw_start_refused
        CBZ.N    R0,??fw_start_3
        MOV      R1,R0
        BL       ??Subroutine68_0
??CrossCallReturnLabel_239:
        B.N      ??fw_start_4
??fw_start_3:
        LDRH     R0,[SP, #+8]
        CMP      R0,#+1
        BNE.N    ??fw_start_5
        MOV      R0,R4
          CFI FunCall fw_rollback
        BL       fw_rollback
        B.N      ??fw_start_1
??fw_start_5:
        LDR.W    R6,??DataTable119_7
        LDR      R0,[R6, #+0]
          CFI FunCall mem_erase_sector
        BL       mem_erase_sector
        CBZ.N    R0,??fw_start_4
        MOVS     R2,#+255
        MOVS     R1,#+32
        ADD      R0,SP,#+24
          CFI FunCall __aeabi_memset4
        BL       __aeabi_memset4
        LDR.W    R5,??DataTable120_5
        LDRB     R0,[R5, #+19]
        STRB     R0,[SP, #+28]
        LDRH     R0,[SP, #+8]
        STRH     R0,[SP, #+29]
        LDR      R0,[SP, #+16]
        STR      R0,[SP, #+32]
        LDR      R0,[SP, #+12]
        STR      R0,[SP, #+36]
        LDR      R0,[SP, #+20]
        STR      R0,[SP, #+44]
        MOVS     R2,#+32
        ADD      R1,SP,#+24
        LDR      R0,[R6, #+0]
          CFI FunCall mem_write
        BL       mem_write
        CBNZ.N   R0,??fw_start_6
??fw_start_4:
        MOVS     R0,#+2
        B.N      ??fw_start_1
??fw_start_6:
          CFI FunCall wdt_reload
        BL       wdt_reload
        LDRH     R0,[SP, #+10]
        STRH     R0,[R5, #+148]
        MOVS     R0,#+0
        STRH     R0,[R5, #+172]
        STRB     R0,[R5, #+150]
        STR      R0,[R5, #+174]
        STRB     R4,[R5, #+178]
        LDR.W    R1,??DataTable120_6
        STRB     R0,[R1, #+0]
        MOVS     R1,#+0
        MOVS     R0,#+132
          CFI FunCall fw_log
        BL       fw_log
        CBZ.N    R4,??fw_start_7
        MOVS     R0,#+1
        STRB     R0,[R5, #+164]
        MOV      R0,#+600
        STRH     R0,[R5, #+179]
        LDRH     R0,[R5, #+129]
        ORR      R0,R0,#0x800
        STRH     R0,[R5, #+129]
          CFI FunCall user_saveindex
        BL       user_saveindex
??fw_start_7:
        MOVS     R0,#+0
??fw_start_1:
        ADD      SP,SP,#+56
          CFI CFA R13+16
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock268

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond269 Using cfiCommon0
          CFI Function upload_restore
          CFI Conditional ??CrossCallReturnLabel_23
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
          CFI Block cfiCond270 Using cfiCommon0
          CFI (cfiCond270) Function user_send_statusPack
          CFI (cfiCond270) Conditional ??CrossCallReturnLabel_24
          CFI (cfiCond270) R4 Frame(CFA, -16)
          CFI (cfiCond270) R5 Frame(CFA, -12)
          CFI (cfiCond270) R6 Frame(CFA, -8)
          CFI (cfiCond270) R14 Frame(CFA, -4)
          CFI (cfiCond270) CFA R13+48
          CFI Block cfiCond271 Using cfiCommon0
          CFI (cfiCond271) Function user_send_timeshtamp
          CFI (cfiCond271) Conditional ??CrossCallReturnLabel_25
          CFI (cfiCond271) R4 Frame(CFA, -20)
          CFI (cfiCond271) R5 Frame(CFA, -16)
          CFI (cfiCond271) R6 Frame(CFA, -12)
          CFI (cfiCond271) R7 Frame(CFA, -8)
          CFI (cfiCond271) R14 Frame(CFA, -4)
          CFI (cfiCond271) CFA R13+48
          CFI Block cfiCond272 Using cfiCommon0
          CFI (cfiCond272) Function fw_start
          CFI (cfiCond272) Conditional ??CrossCallReturnLabel_26
          CFI (cfiCond272) R4 Frame(CFA, -16)
          CFI (cfiCond272) R5 Frame(CFA, -12)
          CFI (cfiCond272) R6 Frame(CFA, -8)
          CFI (cfiCond272) R14 Frame(CFA, -4)
          CFI (cfiCond272) CFA R13+72
          CFI Block cfiCond273 Using cfiCommon0
          CFI (cfiCond273) Function log_addEvent
          CFI (cfiCond273) Conditional ??CrossCallReturnLabel_27
          CFI (cfiCond273) R4 Frame(CFA, -20)
          CFI (cfiCond273) R5 Frame(CFA, -16)
          CFI (cfiCond273) R6 Frame(CFA, -12)
          CFI (cfiCond273) R7 Frame(CFA, -8)
          CFI (cfiCond273) R14 Frame(CFA, -4)
          CFI (cfiCond273) CFA R13+64
          CFI Block cfiCond274 Using cfiCommon0
          CFI (cfiCond274) Function log_async_step
          CFI (cfiCond274) Conditional ??CrossCallReturnLabel_28
          CFI (cfiCond274) R4 Frame(CFA, -16)
          CFI (cfiCond274) R5 Frame(CFA, -12)
          CFI (cfiCond274) R6 Frame(CFA, -8)
          CFI (cfiCond274) R14 Frame(CFA, -4)
          CFI (cfiCond274) CFA R13+56
          CFI Block cfiPicker275 Using cfiCommon1
          CFI (cfiPicker275) NoFunction
          CFI (cfiPicker275) Picker
        THUMB
?Subroutine14:
        LDRB     R2,[R3, #+0]
        STRB     R2,[R0, #+0]
        LDRB     R2,[R3, #+1]
        STRB     R2,[R0, #+1]
        BX       LR
          CFI EndBlock cfiCond269
          CFI EndBlock cfiCond270
          CFI EndBlock cfiCond271
          CFI EndBlock cfiCond272
          CFI EndBlock cfiCond273
          CFI EndBlock cfiCond274
          CFI EndBlock cfiPicker275

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond276 Using cfiCommon0
          CFI Function fw_start
          CFI Conditional ??CrossCallReturnLabel_16
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+72
          CFI Block cfiCond277 Using cfiCommon0
          CFI (cfiCond277) Function fw_start
          CFI (cfiCond277) Conditional ??CrossCallReturnLabel_17
          CFI (cfiCond277) R4 Frame(CFA, -16)
          CFI (cfiCond277) R5 Frame(CFA, -12)
          CFI (cfiCond277) R6 Frame(CFA, -8)
          CFI (cfiCond277) R14 Frame(CFA, -4)
          CFI (cfiCond277) CFA R13+72
          CFI Block cfiPicker278 Using cfiCommon1
          CFI (cfiPicker278) NoFunction
          CFI (cfiPicker278) Picker
        THUMB
?Subroutine11:
        LDRH     R0,[SP, #+10]
        STR      R0,[SP, #+0]
        LDR      R3,[SP, #+12]
        LDR      R2,[SP, #+16]
        LDR      R1,[SP, #+20]
        LDRH     R0,[SP, #+8]
        BX       LR
          CFI EndBlock cfiCond276
          CFI EndBlock cfiCond277
          CFI EndBlock cfiPicker278

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock279 Using cfiCommon0
          CFI Function fw_block_written
        THUMB
fw_block_written:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        SUB      SP,SP,#+64
          CFI CFA R13+80
        MOV      R6,R0
        MOV      R4,R1
        MOVS     R5,#+0
??fw_block_written_0:
        MOVS     R2,#+64
        MOV      R1,SP
        ADDS     R0,R5,R6
          CFI FunCall mem_read
        BL       mem_read
        CBZ.N    R0,??fw_block_written_1
        MOVS     R2,#+64
        ADDS     R1,R4,R5
        MOV      R0,SP
          CFI FunCall memcmp
        BL       memcmp
        CMP      R0,#+0
        BNE.N    ??fw_block_written_1
        ADDS     R5,R5,#+64
        CMP      R5,#+1024
        BCC.N    ??fw_block_written_0
        B.N      ??fw_block_written_2
??fw_block_written_1:
        MOVS     R0,#+0
        B.N      ??fw_block_written_3
??fw_block_written_2:
        MOVS     R0,#+1
??fw_block_written_3:
        ADD      SP,SP,#+64
          CFI CFA R13+16
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock279

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock280 Using cfiCommon0
          CFI Function fw_blocks_done
        THUMB
fw_blocks_done:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R4,R0
        LDR.W    R5,??DataTable120_5
        MOVS     R0,#+0
        STRB     R0,[R5, #+164]
        BL       ?Subroutine26
??CrossCallReturnLabel_85:
        MOVS     R0,R4
        BEQ.N    ??fw_blocks_done_0
        MOVS     R0,#+1
        STRB     R0,[R5, #+171]
        B.N      ??CrossCallReturnLabel_238
??fw_blocks_done_0:
        ADR.N    R1,??DataTable105_2
        BL       ??Subroutine68_0
??CrossCallReturnLabel_238:
        LDRB     R0,[R5, #+178]
        CBZ.N    R0,??fw_blocks_done_1
          CFI FunCall user_saveindex
        BL       user_saveindex
??fw_blocks_done_1:
        CBZ.N    R4,??fw_blocks_done_2
        MOVS     R0,#+0
        POP      {R1,R4,R5,PC}
??fw_blocks_done_2:
        MOVS     R0,#+253
        POP      {R1,R4,R5,PC}
          CFI EndBlock cfiBlock280

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond281 Using cfiCommon0
          CFI Function fw_start
          CFI Conditional ??CrossCallReturnLabel_242
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+72
          CFI Block cfiCond282 Using cfiCommon0
          CFI (cfiCond282) Function user_parse_comand
          CFI (cfiCond282) Conditional ??CrossCallReturnLabel_241
          CFI (cfiCond282) R4 Frame(CFA, -36)
          CFI (cfiCond282) R5 Frame(CFA, -32)
          CFI (cfiCond282) R6 Frame(CFA, -28)
          CFI (cfiCond282) R7 Frame(CFA, -24)
          CFI (cfiCond282) R8 Frame(CFA, -20)
          CFI (cfiCond282) R9 Frame(CFA, -16)
          CFI (cfiCond282) R10 Frame(CFA, -12)
          CFI (cfiCond282) R11 Frame(CFA, -8)
          CFI (cfiCond282) R14 Frame(CFA, -4)
          CFI (cfiCond282) CFA R13+64
          CFI Block cfiPicker283 Using cfiCommon1
          CFI (cfiPicker283) NoFunction
          CFI (cfiPicker283) Picker
        THUMB
?Subroutine68:
        LDR.W    R1,??DataTable120_7
          CFI Block cfiCond284 Using cfiCommon0
          CFI (cfiCond284) Function fw_rollback
          CFI (cfiCond284) Conditional ??CrossCallReturnLabel_240
          CFI (cfiCond284) R4 Frame(CFA, -12)
          CFI (cfiCond284) R5 Frame(CFA, -8)
          CFI (cfiCond284) R14 Frame(CFA, -4)
          CFI (cfiCond284) CFA R13+48
          CFI Block cfiCond285 Using cfiCommon0
          CFI (cfiCond285) Function fw_start
          CFI (cfiCond285) Conditional ??CrossCallReturnLabel_239
          CFI (cfiCond285) R4 Frame(CFA, -16)
          CFI (cfiCond285) R5 Frame(CFA, -12)
          CFI (cfiCond285) R6 Frame(CFA, -8)
          CFI (cfiCond285) R14 Frame(CFA, -4)
          CFI (cfiCond285) CFA R13+72
          CFI Block cfiCond286 Using cfiCommon0
          CFI (cfiCond286) Function fw_blocks_done
          CFI (cfiCond286) Conditional ??CrossCallReturnLabel_238
          CFI (cfiCond286) R4 Frame(CFA, -12)
          CFI (cfiCond286) R5 Frame(CFA, -8)
          CFI (cfiCond286) R14 Frame(CFA, -4)
          CFI (cfiCond286) CFA R13+16
          CFI Block cfiCond287 Using cfiCommon0
          CFI (cfiCond287) Function fw_take_block
          CFI (cfiCond287) Conditional ??CrossCallReturnLabel_237
          CFI (cfiCond287) R4 Frame(CFA, -24)
          CFI (cfiCond287) R5 Frame(CFA, -20)
          CFI (cfiCond287) R6 Frame(CFA, -16)
          CFI (cfiCond287) R7 Frame(CFA, -12)
          CFI (cfiCond287) R8 Frame(CFA, -8)
          CFI (cfiCond287) R14 Frame(CFA, -4)
          CFI (cfiCond287) CFA R13+64
??Subroutine68_0:
        MOVS     R0,#+133
          CFI (cfiCond281) FunCall fw_start fw_log
          CFI (cfiCond282) FunCall user_parse_comand fw_log
          CFI (cfiCond284) FunCall fw_rollback fw_log
          CFI (cfiCond281) FunCall fw_start fw_log
          CFI (cfiCond286) FunCall fw_blocks_done fw_log
          CFI (cfiCond287) FunCall fw_take_block fw_log
        B.N      fw_log
          CFI EndBlock cfiCond281
          CFI EndBlock cfiCond282
          CFI EndBlock cfiPicker283
          CFI EndBlock cfiCond284
          CFI EndBlock cfiCond285
          CFI EndBlock cfiCond286
          CFI EndBlock cfiCond287

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable105:
        DATA32
        DC32     fault_record

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable105_1:
        DATA32
        DC32     0x31544c46

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable105_2:
        DATA8
        DC8      "CRC"

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond288 Using cfiCommon0
          CFI Function fw_abort
          CFI Conditional ??CrossCallReturnLabel_84
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
          CFI Block cfiCond289 Using cfiCommon0
          CFI (cfiCond289) Function fw_blocks_done
          CFI (cfiCond289) Conditional ??CrossCallReturnLabel_85
          CFI (cfiCond289) R4 Frame(CFA, -12)
          CFI (cfiCond289) R5 Frame(CFA, -8)
          CFI (cfiCond289) R14 Frame(CFA, -4)
          CFI (cfiCond289) CFA R13+16
          CFI Block cfiPicker290 Using cfiCommon1
          CFI (cfiPicker290) NoFunction
          CFI (cfiPicker290) Picker
        THUMB
?Subroutine26:
        LDRH     R1,[R5, #+129]
        MOVW     R0,#+63487
        ANDS     R1,R0,R1
        STRH     R1,[R5, #+129]
        MOVS     R0,#+0
        STRH     R0,[R5, #+179]
        BX       LR
          CFI EndBlock cfiCond288
          CFI EndBlock cfiCond289
          CFI EndBlock cfiPicker290

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock291 Using cfiCommon0
          CFI Function fw_take_block
        THUMB
fw_take_block:
        PUSH     {R3-R8,LR}
          CFI R14 Frame(CFA, -4)
          CFI R8 Frame(CFA, -8)
          CFI R7 Frame(CFA, -12)
          CFI R6 Frame(CFA, -16)
          CFI R5 Frame(CFA, -20)
          CFI R4 Frame(CFA, -24)
          CFI CFA R13+28
        SUB      SP,SP,#+36
          CFI CFA R13+64
        MOV      R6,R0
        MOV      R5,R1
        MOV      R7,R2
          CFI FunCall boot_manual_busy
        BL       boot_manual_busy
        CMP      R0,#+0
        ITTT     EQ
        LDREQ.W  R4,??DataTable120_5
        LDRBEQ   R1,[R4, #+178]
        CMPEQ    R7,R1
        BNE.N    ??fw_take_block_0
        MOVW     R0,#+1026
        CMP      R5,R0
        BEQ.N    ??fw_take_block_1
        BL       ?Subroutine69
??CrossCallReturnLabel_243:
        BCC.N    ??fw_take_block_2
??fw_take_block_0:
        B.N      ??fw_take_block_3
??fw_take_block_1:
        LDRB     R1,[R6, #+0]
        STRB     R1,[SP, #+0]
        LDRB     R1,[R6, #+1]
        STRB     R1,[SP, #+1]
        LDRH     R1,[SP, #+0]
        LDRH     R0,[R4, #+172]
        CMP      R1,R0
        BCC.N    ??fw_take_block_4
        MOV      R1,R0
        LDRH     R0,[R4, #+148]
        CMP      R1,R0
        BCS.N    ??fw_take_block_3
        LDRH     R0,[SP, #+0]
        CMP      R0,R1
        BEQ.N    ??fw_take_block_5
??fw_take_block_2:
        MOVS     R0,#+255
        B.N      ??fw_take_block_6
??fw_take_block_5:
        CBNZ.N   R0,??fw_take_block_7
        LDRB     R1,[R4, #+19]
        ADDS     R0,R6,#+2
          CFI FunCall fw_image_foreign
        BL       fw_image_foreign
        CBZ.N    R0,??fw_take_block_7
        MOVS     R0,#+0
          CFI FunCall fw_abort
        BL       fw_abort
        ADR.N    R1,??DataTable112
        BL       ??Subroutine68_0
??CrossCallReturnLabel_237:
        B.N      ??fw_take_block_3
??fw_take_block_7:
        LDR.W    R5,??DataTable119_7
        LDR      R1,[R5, #+0]
        LDRH     R0,[SP, #+0]
        ADD      R0,R1,R0, LSL #+10
        ADD      R8,R0,#+512
        BL       ?Subroutine25
??CrossCallReturnLabel_82:
        CBZ.N    R0,??fw_take_block_8
        MOV      R2,R7
        MOV      R1,R8
        ADDS     R0,R6,#+2
          CFI FunCall fw_async_begin
        BL       fw_async_begin
        CBZ.N    R0,??fw_take_block_9
        MOVS     R0,#+252
        B.N      ??fw_take_block_6
??fw_take_block_8:
        ADDW     R0,R8,#+4095
        LSRS     R0,R0,#+12
        LSLS     R0,R0,#+12
        ADD      R1,R8,#+1024
        CMP      R0,R1
        BCS.N    ??fw_take_block_10
          CFI FunCall mem_erase_sector
        BL       mem_erase_sector
        CBZ.N    R0,??fw_take_block_11
??fw_take_block_10:
        MOV      R2,#+1024
        ADDS     R1,R6,#+2
        MOV      R0,R8
          CFI FunCall mem_write
        BL       mem_write
        MOVS     R7,R0
        BEQ.N    ??fw_take_block_12
        MOVS     R7,#+1
        B.N      ??fw_take_block_12
??fw_take_block_11:
        MOVS     R7,#+0
??fw_take_block_12:
          CFI FunCall wdt_reload
        BL       wdt_reload
        CBZ.N    R7,??fw_take_block_13
        ADDS     R1,R6,#+2
        MOV      R0,R8
          CFI FunCall fw_block_written
        BL       fw_block_written
        CBNZ.N   R0,??fw_take_block_14
??fw_take_block_13:
        LDR.W    R1,??DataTable120_6
        LDRB     R0,[R1, #+0]
        ADDS     R0,R0,#+1
        STRB     R0,[R1, #+0]
        LDRB     R0,[R1, #+0]
        CMP      R0,#+3
        BCS.N    ??fw_take_block_15
??fw_take_block_9:
        MOVS     R0,#+254
        B.N      ??fw_take_block_6
??fw_take_block_15:
        LDR.W    R0,??DataTable120_4
        B.N      ??fw_take_block_16
??fw_take_block_14:
        BL       ?Subroutine20
??CrossCallReturnLabel_46:
        STR      R1,[R4, #+174]
        LDRH     R0,[R4, #+172]
        ADDS     R0,R0,#+1
        STRH     R0,[R4, #+172]
        BL       ?Subroutine52
??CrossCallReturnLabel_183:
        UDIV     R0,R1,R0
        STRB     R0,[R4, #+150]
        BL       ?Subroutine69
??CrossCallReturnLabel_244:
        BCS.N    ??fw_take_block_17
??fw_take_block_4:
        MOVS     R0,#+0
        B.N      ??fw_take_block_6
??fw_take_block_17:
        BL       ?Subroutine44
??CrossCallReturnLabel_155:
        CBNZ.N   R0,??fw_take_block_18
        LDR.W    R0,??DataTable120_3
??fw_take_block_16:
          CFI FunCall fw_abort
        BL       fw_abort
??fw_take_block_3:
        MOVS     R0,#+253
        B.N      ??fw_take_block_6
??fw_take_block_18:
        LDR      R2,[SP, #+16]
        LDR      R1,[SP, #+12]
        LDR      R0,[R5, #+0]
        BL       ?Subroutine60
??CrossCallReturnLabel_210:
          CFI FunCall fw_blocks_done
        BL       fw_blocks_done
??fw_take_block_6:
        ADD      SP,SP,#+40
          CFI CFA R13+24
        POP      {R4-R8,PC}
          CFI EndBlock cfiBlock291

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond292 Using cfiCommon0
          CFI Function fw_take_block
          CFI Conditional ??CrossCallReturnLabel_243
          CFI R4 Frame(CFA, -24)
          CFI R5 Frame(CFA, -20)
          CFI R6 Frame(CFA, -16)
          CFI R7 Frame(CFA, -12)
          CFI R8 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+64
          CFI Block cfiCond293 Using cfiCommon0
          CFI (cfiCond293) Function fw_take_block
          CFI (cfiCond293) Conditional ??CrossCallReturnLabel_244
          CFI (cfiCond293) R4 Frame(CFA, -24)
          CFI (cfiCond293) R5 Frame(CFA, -20)
          CFI (cfiCond293) R6 Frame(CFA, -16)
          CFI (cfiCond293) R7 Frame(CFA, -12)
          CFI (cfiCond293) R8 Frame(CFA, -8)
          CFI (cfiCond293) R14 Frame(CFA, -4)
          CFI (cfiCond293) CFA R13+64
          CFI Block cfiPicker294 Using cfiCommon1
          CFI (cfiPicker294) NoFunction
          CFI (cfiPicker294) Picker
        THUMB
?Subroutine69:
        LDRH     R0,[R4, #+172]
        LDRH     R1,[R4, #+148]
        CMP      R0,R1
        BX       LR
          CFI EndBlock cfiCond292
          CFI EndBlock cfiCond293
          CFI EndBlock cfiPicker294

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond295 Using cfiCommon0
          CFI Function fw_abort
          CFI Conditional ??CrossCallReturnLabel_45
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
          CFI Block cfiCond296 Using cfiCommon0
          CFI (cfiCond296) Function fw_take_block
          CFI (cfiCond296) Conditional ??CrossCallReturnLabel_46
          CFI (cfiCond296) R4 Frame(CFA, -24)
          CFI (cfiCond296) R5 Frame(CFA, -20)
          CFI (cfiCond296) R6 Frame(CFA, -16)
          CFI (cfiCond296) R7 Frame(CFA, -12)
          CFI (cfiCond296) R8 Frame(CFA, -8)
          CFI (cfiCond296) R14 Frame(CFA, -4)
          CFI (cfiCond296) CFA R13+64
          CFI Block cfiCond297 Using cfiCommon0
          CFI (cfiCond297) Function fw_storage_poll
          CFI (cfiCond297) Conditional ??CrossCallReturnLabel_47
          CFI (cfiCond297) R4 Frame(CFA, -16)
          CFI (cfiCond297) R5 Frame(CFA, -12)
          CFI (cfiCond297) R6 Frame(CFA, -8)
          CFI (cfiCond297) R14 Frame(CFA, -4)
          CFI (cfiCond297) CFA R13+56
          CFI Block cfiPicker298 Using cfiCommon1
          CFI (cfiPicker298) NoFunction
          CFI (cfiPicker298) Picker
        THUMB
?Subroutine20:
        MOVS     R1,#+0
        LDR.W    R0,??DataTable120_6
        STRB     R1,[R0, #+0]
        BX       LR
          CFI EndBlock cfiCond295
          CFI EndBlock cfiCond296
          CFI EndBlock cfiCond297
          CFI EndBlock cfiPicker298

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock299 Using cfiCommon0
          CFI Function fw_resume_block
          CFI NoCalls
        THUMB
fw_resume_block:
        LDR.W    R2,??DataTable120_5
        LDRH     R0,[R2, #+172]
        LDRH     R1,[R2, #+148]
        CMP      R0,R1
        IT       CS
        MOVWCS   R0,#+65535
        BX       LR
          CFI EndBlock cfiBlock299

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable108:
        DATA32
        DC32     ?_64

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock300 Using cfiCommon0
          CFI Function fw_resume_after_boot
        THUMB
fw_resume_after_boot:
        PUSH     {R3,R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+12
        SUB      SP,SP,#+36
          CFI CFA R13+48
        LDR.W    R4,??DataTable120_5
        LDRB     R0,[R4, #+164]
        CBNZ.N   R0,??fw_resume_after_boot_0
        BL       ?Subroutine61
??CrossCallReturnLabel_213:
        B.N      ??fw_resume_after_boot_1
??fw_resume_after_boot_0:
        MOVS     R2,#+32
        ADD      R1,SP,#+4
        BL       ?Subroutine51
??CrossCallReturnLabel_181:
        CBZ.N    R0,??fw_resume_after_boot_2
        LDR.W    R0,??DataTable119_8
        LDRB     R0,[R0, #+0]
        CMP      R0,#+48
        BLT.N    ??fw_resume_after_boot_2
        LDR      R0,[SP, #+4]
        CMN      R0,#+1
        BNE.N    ??fw_resume_after_boot_2
        LDRH     R0,[SP, #+9]
        CMP      R0,#+2
        BLT.N    ??fw_resume_after_boot_2
        LDRH     R0,[R4, #+148]
        STR      R0,[SP, #+0]
        LDR      R3,[SP, #+16]
        LDR      R2,[SP, #+12]
        LDR      R1,[SP, #+24]
        LDRH     R0,[SP, #+9]
          CFI FunCall fw_start_rejected
        BL       fw_start_rejected
        CBZ.N    R0,??fw_resume_after_boot_3
??fw_resume_after_boot_2:
        MOVS     R0,#+0
        STRB     R0,[R4, #+164]
        LDRH     R1,[R4, #+129]
        MOVW     R0,#+63487
        ANDS     R1,R0,R1
        STRH     R1,[R4, #+129]
        MOVS     R0,#+0
        BL       ?Subroutine61
??CrossCallReturnLabel_214:
          CFI FunCall user_saveindex
        BL       user_saveindex
        B.N      ??fw_resume_after_boot_1
??fw_resume_after_boot_3:
        MOVS     R0,#+1
        STRB     R0,[R4, #+178]
          CFI FunCall fw_resume_block
        BL       fw_resume_block
        STRH     R0,[R4, #+172]
        BL       ?Subroutine42
??CrossCallReturnLabel_149:
        BNE.N    ??fw_resume_after_boot_4
        LDR.W    R0,??DataTable120_3
          CFI FunCall fw_abort
        BL       fw_abort
        B.N      ??fw_resume_after_boot_1
??fw_resume_after_boot_4:
        MOVS     R0,#+0
        STR      R0,[R4, #+174]
        LDRH     R0,[R4, #+179]
        CBZ.N    R0,??fw_resume_after_boot_5
        MOV      R1,R0
        MOVW     R0,#+601
        CMP      R1,R0
        BCC.N    ??fw_resume_after_boot_6
??fw_resume_after_boot_5:
        MOV      R0,#+600
        STRH     R0,[R4, #+179]
??fw_resume_after_boot_6:
        LDRH     R0,[R4, #+129]
        ORR      R0,R0,#0x800
        STRH     R0,[R4, #+129]
        BL       ?Subroutine52
??CrossCallReturnLabel_184:
        SDIV     R0,R1,R0
        STRB     R0,[R4, #+150]
??fw_resume_after_boot_1:
        ADD      SP,SP,#+40
          CFI CFA R13+8
        POP      {R4,PC}
          CFI EndBlock cfiBlock300

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable109:
        DATA32
        DC32     log_unacked

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond301 Using cfiCommon0
          CFI Function fw_resume_after_boot
          CFI Conditional ??CrossCallReturnLabel_213
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+48
          CFI Block cfiCond302 Using cfiCommon0
          CFI (cfiCond302) Function fw_resume_after_boot
          CFI (cfiCond302) Conditional ??CrossCallReturnLabel_214
          CFI (cfiCond302) R4 Frame(CFA, -8)
          CFI (cfiCond302) R14 Frame(CFA, -4)
          CFI (cfiCond302) CFA R13+48
          CFI Block cfiPicker303 Using cfiCommon1
          CFI (cfiPicker303) NoFunction
          CFI (cfiPicker303) Picker
        THUMB
?Subroutine61:
        STRH     R0,[R4, #+179]
        STRH     R0,[R4, #+148]
        STRH     R0,[R4, #+172]
        BX       LR
          CFI EndBlock cfiCond301
          CFI EndBlock cfiCond302
          CFI EndBlock cfiPicker303

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond304 Using cfiCommon0
          CFI Function fw_take_block
          CFI Conditional ??CrossCallReturnLabel_183
          CFI R4 Frame(CFA, -24)
          CFI R5 Frame(CFA, -20)
          CFI R6 Frame(CFA, -16)
          CFI R7 Frame(CFA, -12)
          CFI R8 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+64
          CFI Block cfiCond305 Using cfiCommon0
          CFI (cfiCond305) Function fw_resume_after_boot
          CFI (cfiCond305) Conditional ??CrossCallReturnLabel_184
          CFI (cfiCond305) R4 Frame(CFA, -8)
          CFI (cfiCond305) R14 Frame(CFA, -4)
          CFI (cfiCond305) CFA R13+48
          CFI Block cfiPicker306 Using cfiCommon1
          CFI (cfiPicker306) NoFunction
          CFI (cfiPicker306) Picker
        THUMB
?Subroutine52:
        LDRH     R1,[R4, #+172]
        MOVS     R0,#+100
        MULS     R1,R0,R1
        LDRH     R0,[R4, #+148]
        BX       LR
          CFI EndBlock cfiCond304
          CFI EndBlock cfiCond305
          CFI EndBlock cfiPicker306

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond307 Using cfiCommon0
          CFI Function fw_same_download
          CFI Conditional ??CrossCallReturnLabel_182
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+56
          CFI Block cfiCond308 Using cfiCommon0
          CFI (cfiCond308) Function fw_resume_after_boot
          CFI (cfiCond308) Conditional ??CrossCallReturnLabel_181
          CFI (cfiCond308) R4 Frame(CFA, -8)
          CFI (cfiCond308) R14 Frame(CFA, -4)
          CFI (cfiCond308) CFA R13+48
          CFI Block cfiPicker309 Using cfiCommon1
          CFI (cfiPicker309) NoFunction
          CFI (cfiPicker309) Picker
        THUMB
?Subroutine51:
        LDR.W    R0,??DataTable119_7
        LDR      R0,[R0, #+0]
          CFI (cfiCond307) FunCall fw_same_download mem_read
          CFI (cfiCond308) FunCall fw_resume_after_boot mem_read
        B.W      mem_read
          CFI EndBlock cfiCond307
          CFI EndBlock cfiCond308
          CFI EndBlock cfiPicker309

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable110:
        DATA32
        DC32     store_runtime

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock310 Using cfiCommon0
          CFI Function cmd8_event
          CFI NoCalls
        THUMB
cmd8_event:
        CMP      R0,#+1
        BEQ.N    ??cmd8_event_0
        CMP      R0,#+3
        BEQ.N    ??cmd8_event_1
        CMP      R0,#+4
        BEQ.N    ??cmd8_event_2
        B.N      ??cmd8_event_3
??cmd8_event_0:
        MOVS     R0,#+77
        BX       LR
??cmd8_event_1:
        MOVS     R0,#+74
        BX       LR
??cmd8_event_2:
        MOVS     R0,#+76
        BX       LR
??cmd8_event_3:
        MOVS     R0,#+75
        BX       LR
          CFI EndBlock cfiBlock310

        SECTION `.data`:DATA:REORDER:NOROOT(0)
        DATA
server_out_event:
        DATA8
        DC8 75

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock311 Using cfiCommon0
          CFI Function sync_cells_valid
          CFI NoCalls
        THUMB
sync_cells_valid:
        PUSH     {R4-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+20
        MOV      R4,R0
        CMP      R1,#+8
        BLT.N    ??sync_cells_valid_0
        LDRB     R5,[R4, #+9]
        CBZ.N    R5,??sync_cells_valid_0
        SUBS     R1,R1,#+8
        MUL      R0,R3,R5
        CMP      R1,R0
        BNE.N    ??sync_cells_valid_0
        MOVS     R1,#+0
        B.N      ??sync_cells_valid_1
??sync_cells_valid_2:
        ADDS     R1,R1,#+1
??sync_cells_valid_1:
        UXTH     R0,R1
        CMP      R0,R5
        BGE.N    ??sync_cells_valid_3
        UXTH     R6,R1
        MULS     R6,R6,R3
        ADDS     R0,R4,R6
        ADD      R6,R0,#+10
        LDRB     R0,[R6, #+0]
        CMP      R2,#+2
        ITT      EQ
        LDRBEQ   R7,[R6, #+1]
        ORREQ    R0,R0,R7, LSL #+8
        CBZ.N    R0,??sync_cells_valid_0
        LDR.W    R12,??DataTable119_7
        ADD      R7,R12,R2, LSL #+3
        LDR      R12,[R7, #+4]
        MOV      R7,R3
        UDIV     R7,R12,R7
        CMP      R7,R0
        BCC.N    ??sync_cells_valid_0
        CMP      R2,#+2
        ITT      EQ
        LDRBEQ   R0,[R6, #+12]
        CMPEQ    R0,#+0
        BNE.N    ??sync_cells_valid_2
??sync_cells_valid_0:
        MOVS     R0,#+0
        POP      {R4-R7,PC}
??sync_cells_valid_3:
        MOVS     R0,#+1
        POP      {R4-R7,PC}
          CFI EndBlock cfiBlock311

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable111:
        DATA32
        DC32     ?_65

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock312 Using cfiCommon0
          CFI Function command_args_valid
          CFI NoCalls
        THUMB
command_args_valid:
        LDRB     R2,[R0, #+4]
        CMP      R2,#+1
        BEQ.N    ??command_args_valid_0
        CMP      R2,#+2
        IT       NE
        CMPNE    R2,#+3
        BEQ.N    ??command_args_valid_1
        CMP      R2,#+6
        BEQ.N    ??command_args_valid_2
        CMP      R2,#+18
        BEQ.N    ??command_args_valid_1
        CMP      R2,#+23
        IT       NE
        CMPNE    R2,#+102
        BEQ.N    ??command_args_valid_3
        CMP      R2,#+105
        IT       NE
        CMPNE    R2,#+108
        BEQ.N    ??command_args_valid_1
        B.N      ??command_args_valid_4
??command_args_valid_0:
        CMP      R1,#+4
        BLT.N    ??command_args_valid_5
        LDRB     R0,[R0, #+5]
        CBNZ.N   R0,??command_args_valid_4
        CMP      R1,#+8
        BLT.N    ??command_args_valid_6
        B.N      ??command_args_valid_4
??command_args_valid_1:
        CMP      R1,#+4
        B.N      ??command_args_valid_7
??command_args_valid_2:
        CMP      R1,#+6
??command_args_valid_7:
        BLT.N    ??command_args_valid_5
        B.N      ??command_args_valid_4
??command_args_valid_3:
        CMP      R1,#+7
        BGE.N    ??command_args_valid_4
??command_args_valid_5:
        MOVS     R0,#+0
        BX       LR
??command_args_valid_4:
        MOVS     R0,#+1
??command_args_valid_6:
        BX       LR
          CFI EndBlock cfiBlock312

        SECTION `.bss`:DATA:REORDER:NOROOT(1)
        DATA
id_cells_end:
        DS8 2

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
table_job:
        DS8 60

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
table_fault:
        DS8 1

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock313 Using cfiCommon0
          CFI Function table_runtime_active
          CFI NoCalls
        THUMB
table_runtime_active:
        LDR.W    R0,??DataTable120_8
        LDRB     R0,[R0, #+50]
        B.W      ?Subroutine0
          CFI EndBlock cfiBlock313

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable112:
        DATA8
        DC8      0x48, 0x57, 0x00, 0x00

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock314 Using cfiCommon0
          CFI Function table_runtime_blocks_reads
          CFI NoCalls
        THUMB
table_runtime_blocks_reads:
        LDR.W    R0,??DataTable120_8
        LDRB     R1,[R0, #+50]
        LDR.W    R0,??DataTable120_9
        LDRB     R0,[R0, #+0]
        ORRS     R0,R0,R1
        IT       NE
        MOVNE    R0,#+1
        BX       LR
          CFI EndBlock cfiBlock314

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock315 Using cfiCommon0
          CFI Function table_runtime_quiet
          CFI NoCalls
        THUMB
table_runtime_quiet:
        LDR.W    R2,??DataTable120_8
        LDRB     R1,[R2, #+50]
        CBZ.N    R1,??table_runtime_quiet_0
        LDRB     R1,[R2, #+53]
        CMP      R1,R0
        ITT      EQ
        MOVEQ    R0,#+1
        STRBEQ   R0,[R2, #+55]
??table_runtime_quiet_0:
        BX       LR
          CFI EndBlock cfiBlock315

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock316 Using cfiCommon0
          CFI Function table_runtime_cancel
        THUMB
table_runtime_cancel:
        LDR.W    R1,??DataTable120_8
        LDRB     R0,[R1, #+50]
        CBZ.N    R0,??table_runtime_cancel_0
        MOVS     R0,#+1
        STRB     R0,[R1, #+55]
        STRB     R0,[R1, #+56]
          CFI FunCall sj_runtime_cancel
        B.W      sj_runtime_cancel
??table_runtime_cancel_0:
        BX       LR
          CFI EndBlock cfiBlock316

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable115:
        DATA32
        DC32     temp_unacked

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable115_1:
        DATA32
        DC32     _A_tempStrBuffer

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable115_2:
        DATA32
        DC32     ?_66

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock317 Using cfiCommon0
          CFI Function table_chunk
        THUMB
table_chunk:
        PUSH     {R3-R11,LR}
          CFI R14 Frame(CFA, -4)
          CFI R11 Frame(CFA, -8)
          CFI R10 Frame(CFA, -12)
          CFI R9 Frame(CFA, -16)
          CFI R8 Frame(CFA, -20)
          CFI R7 Frame(CFA, -24)
          CFI R6 Frame(CFA, -28)
          CFI R5 Frame(CFA, -32)
          CFI R4 Frame(CFA, -36)
          CFI CFA R13+40
        MOV      R5,R1
        MOV      R10,R2
        LDR.W    R7,??DataTable120_8
        MOVS     R2,#+64
        MOV      R1,R10
        LDR      R0,[R7, #+8]
        ADDS     R0,R5,R0
          CFI FunCall w25_read_data
        BL       w25_read_data
        CMP      R0,#+0
        BEQ.N    ??table_chunk_0
        LDRB     R0,[R7, #+52]
        CMP      R0,#+16
        BNE.N    ??table_chunk_1
        MOVS     R1,#+0
??table_chunk_2:
        ADDS     R0,R1,R5
        LDR.N    R2,??DataTable119_7
        LDR      R2,[R2, #+44]
        CMP      R0,R2
        BCS.N    ??table_chunk_3
        ADDS     R2,R1,R5
        LDRH     R0,[R7, #+40]
        CMP      R2,R0
        ITTTE    LT
        LDRLT    R2,[R7, #+0]
        ADDLT    R0,R1,R5
        LDRBLT   R0,[R2, R0]
        MOVGE    R0,#+255
        STRB     R0,[R10, R1]
        ADDS     R1,R1,#+1
        UXTH     R1,R1
        CMP      R1,#+64
        BLT.N    ??table_chunk_2
??table_chunk_3:
        MOVS     R0,#+1
??table_chunk_4:
        POP      {R1,R4-R11,PC}
??table_chunk_1:
        MOV      R11,#+0
        B.N      ??table_chunk_5
??table_chunk_6:
        MOV      R2,R1
        MOV      R1,R6
        ADD      R0,R10,R3
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        B.N      ??table_chunk_7
??table_chunk_8:
        LDRB     R0,[R7, #+52]
        CMP      R0,#+13
        BNE.N    ??table_chunk_6
        ADD      R8,R10,R3
        LDRB     R9,[R8, #+28]
        LDRH     R1,[R8, #+0]
        BL       ?Subroutine62
??CrossCallReturnLabel_215:
        BEQ.N    ??table_chunk_9
        MOVS     R2,#+10
        ADDS     R1,R6,#+2
        ADD      R0,R8,#+2
          CFI FunCall memcmp
        BL       memcmp
        SUBS     R4,R0,#+1
        SBCS     R4,R4,R4
        LSRS     R4,R4,#+31
??table_chunk_10:
        ADD      R3,R8,#+22
        LDR      R2,[R3, #+0]
        STR      R2,[SP, #+0]
        LDRH     R2,[R7, #+38]
        MOV      R1,R6
        MOV      R0,R8
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        CBZ.N    R4,??table_chunk_11
        ADD      R0,R8,#+22
        LDR      R1,[SP, #+0]
        STR      R1,[R0, #+0]
        STRB     R9,[R8, #+28]
??table_chunk_11:
        LDRH     R1,[R7, #+42]
        MOV      R0,R8
          CFI FunCall id_set_pass_day
        BL       id_set_pass_day
??table_chunk_7:
        ADD      R11,R11,#+1
??table_chunk_5:
        LDRH     R0,[R7, #+36]
        UXTH     R1,R11
        CMP      R1,R0
        BCS.N    ??table_chunk_3
        LDR      R0,[R7, #+0]
        LDRH     R2,[R7, #+34]
        ADDS     R2,R1,R2
        LDRH     R1,[R7, #+38]
        MULS     R2,R1,R2
        ADDS     R6,R0,R2
        LDRB     R1,[R6, #+0]
        LDRB     R0,[R7, #+52]
        CMP      R0,#+13
        ITT      EQ
        LDRBEQ   R0,[R6, #+1]
        ORREQ    R1,R1,R0, LSL #+8
        LDR      R3,[R7, #+12]
        SUBS     R1,R1,#+1
        LDRH     R0,[R7, #+38]
        MLA      R3,R0,R1,R3
        LDR      R1,[R7, #+8]
        ADDS     R1,R5,R1
        CMP      R3,R1
        BCC.N    ??table_chunk_7
        LDR      R1,[R7, #+8]
        ADDS     R1,R5,R1
        ADDS     R1,R1,#+64
        CMP      R3,R1
        BCS.N    ??table_chunk_7
        LDR      R0,[R7, #+8]
        SUBS     R3,R3,R0
        SUBS     R3,R3,R5
        UXTH     R3,R3
        RSB      R0,R3,#+64
        LDRH     R1,[R7, #+38]
        CMP      R0,R1
        BGE.N    ??table_chunk_8
??table_chunk_0:
        MOVS     R0,#+0
        B.N      ??table_chunk_4
??table_chunk_9:
        MOVS     R4,#+0
        B.N      ??table_chunk_10
          CFI EndBlock cfiBlock317

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable116:
        DATA32
        DC32     uniqIDCnt

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable116_1:
        DATA32
        DC32     0x5f5e1000

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock318 Using cfiCommon0
          CFI Function table_runtime_begin
        THUMB
table_runtime_begin:
        PUSH     {R3-R11,LR}
          CFI R14 Frame(CFA, -4)
          CFI R11 Frame(CFA, -8)
          CFI R10 Frame(CFA, -12)
          CFI R9 Frame(CFA, -16)
          CFI R8 Frame(CFA, -20)
          CFI R7 Frame(CFA, -24)
          CFI R6 Frame(CFA, -28)
          CFI R5 Frame(CFA, -32)
          CFI R4 Frame(CFA, -36)
          CFI CFA R13+40
        MOV      R5,R0
        MOV      R10,R1
        MOV      R9,R2
        LDRB     R0,[R5, #+4]
        STR      R0,[SP, #+0]
        CMP      R0,#+13
        BNE.N    ??table_runtime_begin_0
        MOVS     R6,#+2
??table_runtime_begin_1:
        MOVS     R7,#+32
??table_runtime_begin_2:
        LDRB     R0,[R5, #+9]
        MUL      R8,R7,R0
          CFI FunCall user_storage_command_ready
        BL       user_storage_command_ready
        CBZ.N    R0,??table_runtime_begin_3
        LDR.W    R4,??DataTable120_8
        LDRB     R1,[R4, #+50]
        LDR.W    R0,??DataTable120_9
        LDRB     R0,[R0, #+0]
        ORRS     R1,R0,R1
        BNE.N    ??table_runtime_begin_3
          CFI FunCall sj_runtime_blocked
        BL       sj_runtime_blocked
        CBNZ.N   R0,??table_runtime_begin_3
        CMP      R8,#+0
        BEQ.N    ??table_runtime_begin_3
        MOVW     R0,#+2049
        CMP      R8,R0
        BGE.N    ??table_runtime_begin_3
          CFI FunCall mem_tables_ready
        BL       mem_tables_ready
        CBZ.N    R0,??table_runtime_begin_3
        MOV      R0,R8
          CFI FunCall malloc
        BL       malloc
        MOVS     R11,R0
        BNE.N    ??table_runtime_begin_4
??table_runtime_begin_3:
        MOVS     R0,#+0
        B.N      ??table_runtime_begin_5
??table_runtime_begin_0:
        CMP      R0,#+14
        IT       EQ
        MOVEQ    R6,#+4
        BEQ.N    ??table_runtime_begin_1
        CMP      R0,#+15
        ITTEE    NE
        MOVNE    R6,#+5
        MOVNE    R7,#+4
        MOVEQ    R6,#+3
        MOVEQ    R7,#+16
        B.N      ??table_runtime_begin_2
??table_runtime_begin_4:
        MOV      R2,R8
        ADD      R1,R5,#+10
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        MOVS     R1,#+60
        MOV      R0,R4
          CFI FunCall __aeabi_memclr4
        BL       __aeabi_memclr4
        STR      R11,[R4, #+0]
        LDRB     R0,[R5, #+9]
        STRH     R0,[R4, #+32]
        STRH     R7,[R4, #+38]
        STRH     R8,[R4, #+40]
        ADDS     R3,R5,#+5
        ADDS     R0,R4,#+4
        LDR      R2,[R3, #+0]
        STR      R2,[R0, #+0]
        LDR      R0,[SP, #+0]
        STRB     R0,[R4, #+52]
        STRB     R6,[R4, #+51]
        STRB     R10,[R4, #+53]
        STRB     R9,[R4, #+54]
        LDR.N    R0,??DataTable119_7
        LDR      R0,[R0, R6, LSL #+3]
        STR      R0,[R4, #+12]
          CFI FunCall id_today
        BL       id_today
        STRH     R0,[R4, #+42]
        MOVS     R0,#+1
        STRB     R0,[R4, #+50]
        LDR      R0,[SP, #+0]
        CMP      R0,#+13
          CFI FunCall idindex_runtime_suspend
        IT       EQ
        BLEQ     idindex_runtime_suspend
        MOVS     R0,#+1
??table_runtime_begin_5:
        POP      {R1,R4-R11,PC}
          CFI EndBlock cfiBlock318

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable117:
        DATA32
        DC32     0x15180

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock319 Using cfiCommon0
          CFI Function table_scan_begin
          CFI NoCalls
        THUMB
table_scan_begin:
        LDR.W    R0,??DataTable120_8
        MOVS     R1,#+3
        STRB     R1,[R0, #+50]
        MOVS     R1,#+0
        STR      R1,[R0, #+16]
        MOVW     R1,#+65535
        STRH     R1,[R0, #+44]
        MOVS     R1,#+0
        STRH     R1,[R0, #+48]
        STRH     R1,[R0, #+46]
        STR      R1,[R0, #+24]
        STR      R1,[R0, #+20]
        LDRB     R0,[R0, #+52]
        CMP      R0,#+13
        ITT      EQ
        LDREQ.N  R0,??DataTable118
        STREQ    R1,[R0, #+0]
        BX       LR
          CFI EndBlock cfiBlock319

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable118:
        DATA32
        DC32     id_nextEnd

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable118_1:
        DATA32
        DC32     status_reported

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock320 Using cfiCommon0
          CFI Function table_runtime_finish
        THUMB
table_runtime_finish:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        LDR.W    R4,??DataTable120_8
        LDRB     R5,[R4, #+52]
        LDRB     R0,[R4, #+57]
        CMP      R0,#+0
        ITT      EQ
        LDRBEQ   R0,[R4, #+56]
        CMPEQ    R0,#+0
        BNE.N    ??table_runtime_finish_0
        CMP      R5,#+13
        BNE.N    ??table_runtime_finish_1
        LDR      R1,[R4, #+4]
        LDR.N    R0,??DataTable120_5
        STR      R1,[R0, #+31]
        MOVS     R5,#+1
        MOVS     R0,#+1
        B.N      ??table_runtime_finish_2
??table_runtime_finish_1:
        CMP      R5,#+14
        BNE.N    ??table_runtime_finish_3
        LDR      R1,[R4, #+4]
        LDR.N    R0,??DataTable120_5
        STR      R1,[R0, #+35]
        MOVS     R5,#+3
        MOVS     R0,#+8
        B.N      ??table_runtime_finish_2
??table_runtime_finish_3:
        CMP      R5,#+15
        LDR      R1,[R4, #+4]
        LDR.N    R0,??DataTable120_5
        BNE.N    ??table_runtime_finish_4
        STR      R1,[R0, #+39]
        MOVS     R5,#+2
        MOVS     R0,#+4
        B.N      ??table_runtime_finish_2
??table_runtime_finish_4:
        STR      R1,[R0, #+43]
        MOVS     R5,#+5
        MOVS     R0,#+16
??table_runtime_finish_2:
          CFI FunCall sync_packet
        BL       sync_packet
        LDRB     R0,[R4, #+55]
        CBNZ.N   R0,??table_runtime_finish_5
        LDRB     R0,[R4, #+54]
        STR      R0,[SP, #+0]
        LDRB     R3,[R4, #+53]
        LDRH     R2,[R4, #+44]
        LDR      R1,[R4, #+4]
        MOV      R0,R5
          CFI FunCall user_send_timeshtamp
        BL       user_send_timeshtamp
        B.N      ??table_runtime_finish_5
??table_runtime_finish_0:
        LDRB     R0,[R4, #+55]
        CBNZ.N   R0,??table_runtime_finish_5
        LDRB     R3,[R4, #+54]
        LDRB     R2,[R4, #+53]
        MOVS     R1,#+2
        MOV      R0,R5
          CFI FunCall user_send_answer_comand
        BL       user_send_answer_comand
??table_runtime_finish_5:
        LDR      R0,[R4, #+0]
          CFI FunCall free
        BL       free
        MOVS     R0,#+0
        STR      R0,[R4, #+0]
        STRB     R0,[R4, #+50]
        POP      {R0,R4,R5,PC}
          CFI EndBlock cfiBlock320

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable119:
        DATA32
        DC32     status_reported_ms

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable119_1:
        DATA32
        DC32     `fw_reason_code::names`

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable119_2:
        DATA32
        DC32     ?_67

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable119_3:
        DATA32
        DC32     fw_rollback_end

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable119_4:
        DATA32
        DC32     ?_42

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable119_5:
        DATA32
        DC32     0x48570000

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable119_6:
        DATA32
        DC32     0x17e01

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable119_7:
        DATA32
        DC32     mem

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable119_8:
        DATA32
        DC32     0x8007ffc

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable119_9:
        DATA32
        DC32     ?_70

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable119_10:
        DATA32
        DC32     ?_71

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable119_11:
        DATA32
        DC32     ?_72

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable119_12:
        DATA32
        DC32     ?_69

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock321 Using cfiCommon0
          CFI Function table_runtime_poll
        THUMB
table_runtime_poll:
        PUSH     {R4-R8,LR}
          CFI R14 Frame(CFA, -4)
          CFI R8 Frame(CFA, -8)
          CFI R7 Frame(CFA, -12)
          CFI R6 Frame(CFA, -16)
          CFI R5 Frame(CFA, -20)
          CFI R4 Frame(CFA, -24)
          CFI CFA R13+24
        SUB      SP,SP,#+64
          CFI CFA R13+88
        MOV      R5,R0
        LDR.N    R4,??DataTable120_8
        LDRB     R0,[R4, #+50]
        CMP      R0,#+1
        BNE.N    ??table_runtime_poll_0
        LDRB     R0,[R4, #+56]
        CMP      R0,#+0
        BNE.N    ??table_runtime_poll_1
        LDR      R2,[R4, #+0]
        LDRH     R1,[R4, #+34]
        LDRH     R0,[R4, #+38]
        MULS     R1,R0,R1
        LDRB     R0,[R2, R1]
        STRH     R0,[SP, #+0]
        LDRB     R0,[R4, #+52]
        CMP      R0,#+13
        BNE.N    ??table_runtime_poll_2
        LDR      R1,[R4, #+0]
        LDRH     R2,[R4, #+34]
        LDRH     R0,[R4, #+38]
        MULS     R2,R0,R2
        ADDS     R3,R1,R2
        BL       ?Subroutine15
??CrossCallReturnLabel_30:
        LDR      R1,[R4, #+12]
        LDRH     R2,[SP, #+0]
        SUBS     R2,R2,#+1
        LDRH     R0,[R4, #+38]
        MLA      R1,R0,R2,R1
        LSRS     R1,R1,#+12
        LSLS     R1,R1,#+12
??table_runtime_poll_3:
        STR      R1,[R4, #+8]
        MOVS     R0,#+1
        STRH     R0,[R4, #+36]
        LDRB     R0,[R4, #+52]
        CMP      R0,#+16
        BNE.N    ??table_runtime_poll_4
        LDRH     R0,[R4, #+32]
        STRH     R0,[R4, #+36]
        B.N      ??table_runtime_poll_5
??table_runtime_poll_2:
        CMP      R0,#+16
        BNE.N    ??CrossCallReturnLabel_30
        LDR      R1,[R4, #+12]
        B.N      ??table_runtime_poll_3
??table_runtime_poll_6:
        LDRH     R0,[R4, #+36]
        ADDS     R0,R0,#+1
        STRH     R0,[R4, #+36]
??table_runtime_poll_4:
        LDRH     R1,[R4, #+34]
        LDRH     R0,[R4, #+36]
        ADDS     R1,R0,R1
        LDRH     R0,[R4, #+32]
        CMP      R1,R0
        BGE.N    ??table_runtime_poll_5
        LDR      R0,[R4, #+0]
        LDRH     R2,[R4, #+34]
        LDRH     R1,[R4, #+36]
        ADDS     R2,R1,R2
        LDRH     R1,[R4, #+38]
        MULS     R2,R1,R2
        ADD      R0,R0,R2
        LDRB     R1,[R0, #+0]
        STRH     R1,[SP, #+0]
        LDRB     R1,[R4, #+52]
        CMP      R1,#+13
        BNE.N    ??table_runtime_poll_7
        LDRH     R1,[SP, #+0]
        LDRB     R0,[R0, #+1]
        ORR      R1,R1,R0, LSL #+8
        STRH     R1,[SP, #+0]
??table_runtime_poll_7:
        LDR      R2,[R4, #+12]
        LDRH     R1,[SP, #+0]
        SUBS     R1,R1,#+1
        LDRH     R0,[R4, #+38]
        MLA      R2,R0,R1,R2
        LSRS     R2,R2,#+12
        LSLS     R2,R2,#+12
        LDR      R0,[R4, #+8]
        CMP      R2,R0
        BEQ.N    ??table_runtime_poll_6
??table_runtime_poll_5:
        MOVS     R2,#+0
        LDR.W    R1,??DataTable123
        LDR      R0,[R4, #+8]
          CFI FunCall sj_runtime_begin
        BL       sj_runtime_begin
        CBNZ.N   R0,??table_runtime_poll_8
        MOVS     R0,#+1
        STRB     R0,[R4, #+57]
??table_runtime_poll_1:
        B.N      ??table_runtime_poll_9
??table_runtime_poll_8:
        MOVS     R0,#+2
        B.N      ??table_runtime_poll_10
??table_runtime_poll_0:
        CMP      R0,#+2
        BNE.N    ??table_runtime_poll_11
        MOV      R0,R5
          CFI FunCall sj_runtime_poll
        BL       sj_runtime_poll
        MOV      R5,R0
        CMP      R5,#+1
        BEQ.N    ??table_runtime_poll_12
          CFI FunCall sj_runtime_reset
        BL       sj_runtime_reset
        CMP      R5,#+2
        BEQ.N    ??table_runtime_poll_13
        MOVS     R0,#+1
        STRB     R0,[R4, #+57]
          CFI FunCall sj_runtime_blocked
        BL       sj_runtime_blocked
        CBZ.N    R0,??table_runtime_poll_9
        B.N      ??table_runtime_poll_14
??table_runtime_poll_13:
        LDRH     R1,[R4, #+34]
        LDRH     R0,[R4, #+36]
        ADDS     R1,R0,R1
        STRH     R1,[R4, #+34]
        LDRH     R1,[R4, #+34]
        LDRH     R0,[R4, #+32]
        CMP      R1,R0
        BEQ.N    ??table_runtime_poll_9
        LDRB     R0,[R4, #+56]
        CBZ.N    R0,??table_runtime_poll_15
??table_runtime_poll_9:
          CFI FunCall table_scan_begin
        BL       table_scan_begin
??table_runtime_poll_12:
        B.N      ??table_runtime_poll_16
??table_runtime_poll_15:
        MOVS     R0,#+1
        B.N      ??table_runtime_poll_10
??table_runtime_poll_11:
        CMP      R0,#+3
        BNE.N    ??table_runtime_poll_17
        MOVS     R6,#+64
        LDR.W    R5,??DataTable123_1
        LDRB     R0,[R4, #+51]
        ADD      R0,R5,R0, LSL #+3
        LDR      R1,[R0, #+4]
        LDR      R0,[R4, #+16]
        SUBS     R1,R1,R0
        CMP      R1,#+64
        BCS.N    ??table_runtime_poll_18
        LDRB     R0,[R4, #+51]
        ADD      R0,R5,R0, LSL #+3
        LDR      R6,[R0, #+4]
        LDR      R0,[R4, #+16]
        SUBS     R6,R6,R0
        UXTH     R6,R6
??table_runtime_poll_18:
        MOV      R2,R6
        MOV      R1,SP
        LDR      R0,[R4, #+12]
        LDR      R3,[R4, #+16]
        ADDS     R0,R3,R0
          CFI FunCall w25_read_data
        BL       w25_read_data
        CBNZ.N   R0,??table_runtime_poll_19
        MOVS     R0,#+1
        STRB     R0,[R4, #+57]
??table_runtime_poll_14:
        MOVS     R1,#+1
        LDR.N    R0,??DataTable120_9
        STRB     R1,[R0, #+0]
        B.N      ??table_runtime_poll_20
??table_runtime_poll_19:
        LDRH     R2,[R4, #+44]
        MOV      R1,R6
        MOV      R0,SP
          CFI FunCall crc16_calcInit
        BL       crc16_calcInit
        STRH     R0,[R4, #+44]
        LDRB     R0,[R4, #+52]
        CMP      R0,#+13
        BNE.N    ??table_runtime_poll_21
        MOVS     R7,#+0
        B.N      ??table_runtime_poll_22
??table_runtime_poll_23:
        ADD      R8,SP,R7
        MOVS     R1,#+32
        ADD      R0,SP,R7
          CFI FunCall fast_all_ff
        BL       fast_all_ff
        CBNZ.N   R0,??table_runtime_poll_24
        LDR      R0,[R4, #+16]
        ADDS     R0,R7,R0
        ADDS     R0,R0,#+32
        STR      R0,[R4, #+24]
??table_runtime_poll_24:
        LDRH     R0,[R8, #+0]
        MOVW     R1,#+65535
        CMP      R0,R1
        BEQ.N    ??table_runtime_poll_25
        LDR      R0,[R4, #+16]
        ADDS     R0,R7,R0
        LSRS     R0,R0,#+5
        ADDS     R0,R0,#+1
        LDRH     R1,[R4, #+46]
        ADDS     R1,R1,#+1
        STRH     R1,[R4, #+46]
        STRH     R0,[R4, #+48]
        ADD      R1,R8,#+2
        UXTH     R0,R0
          CFI FunCall idindex_hash
        BL       idindex_hash
        LDR      R1,[R4, #+20]
        EORS     R0,R0,R1
        STR      R0,[R4, #+20]
        MOV      R0,R8
          CFI FunCall id_time_end
        BL       id_time_end
          CFI FunCall id_watch_end
        BL       id_watch_end
??table_runtime_poll_25:
        ADDS     R7,R7,#+32
        UXTH     R7,R7
??table_runtime_poll_22:
        CMP      R7,R6
        BLT.N    ??table_runtime_poll_23
??table_runtime_poll_21:
        LDR      R0,[R4, #+16]
        ADDS     R6,R6,R0
        STR      R6,[R4, #+16]
        LDRB     R1,[R4, #+51]
        ADD      R1,R5,R1, LSL #+3
        LDR      R1,[R1, #+4]
        CMP      R6,R1
        BNE.N    ??table_runtime_poll_16
        LDRB     R0,[R4, #+52]
        CMP      R0,#+13
        BNE.N    ??table_runtime_poll_20
        LDRH     R1,[R4, #+46]
        LDR.N    R0,??DataTable120_5
        STRH     R1,[R0, #+81]
        LDRH     R1,[R4, #+48]
        LDR.W    R0,??DataTable125
        STRH     R1,[R0, #+0]
        LDR      R1,[R4, #+24]
        LDR.W    R0,??DataTable126
        STR      R1,[R0, #+0]
        LDRH     R1,[R4, #+48]
        LDR      R0,[R4, #+20]
          CFI FunCall idindex_scanned
        BL       idindex_scanned
??table_runtime_poll_20:
        MOVS     R0,#+4
??table_runtime_poll_10:
        STRB     R0,[R4, #+50]
        B.N      ??table_runtime_poll_16
??table_runtime_poll_17:
        CMP      R0,#+4
        BNE.N    ??table_runtime_poll_16
        LDRB     R0,[R4, #+55]
        CMP      R0,#+0
        ITT      EQ
        LDRBEQ   R0,[R4, #+53]
        CMPEQ    R0,#+1
        BNE.N    ??table_runtime_poll_26
          CFI FunCall comm_tx_busy
        BL       comm_tx_busy
        CBZ.N    R0,??table_runtime_poll_26
        LDRB     R0,[R4, #+58]
        CBNZ.N   R0,??table_runtime_poll_27
        MOVS     R0,#+1
        STRB     R0,[R4, #+58]
        ADD      R0,R5,#+2000
        STR      R0,[R4, #+28]
??table_runtime_poll_27:
        LDR      R0,[R4, #+28]
        SUBS     R5,R5,R0
        BMI.N    ??table_runtime_poll_16
        MOVS     R0,#+1
        STRB     R0,[R4, #+55]
??table_runtime_poll_26:
          CFI FunCall table_runtime_finish
        BL       table_runtime_finish
??table_runtime_poll_16:
        ADD      SP,SP,#+64
          CFI CFA R13+24
        POP      {R4-R8,PC}
          CFI EndBlock cfiBlock321

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable120:
        DATA32
        DC32     ?_73

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable120_1:
        DATA32
        DC32     ?_74

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable120_2:
        DATA32
        DC32     fw_rollback_pos

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable120_3:
        DATA32
        DC32     ?_81

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable120_4:
        DATA32
        DC32     ?_76

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable120_5:
        DATA32
        DC32     device

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable120_6:
        DATA32
        DC32     fw_write_errors

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable120_7:
        DATA32
        DC32     ?_68

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable120_8:
        DATA32
        DC32     table_job

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable120_9:
        DATA32
        DC32     table_fault

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond322 Using cfiCommon0
          CFI Function user_loop
          CFI Conditional ??CrossCallReturnLabel_29
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
          CFI Block cfiCond323 Using cfiCommon0
          CFI (cfiCond323) Function table_runtime_poll
          CFI (cfiCond323) Conditional ??CrossCallReturnLabel_30
          CFI (cfiCond323) R4 Frame(CFA, -24)
          CFI (cfiCond323) R5 Frame(CFA, -20)
          CFI (cfiCond323) R6 Frame(CFA, -16)
          CFI (cfiCond323) R7 Frame(CFA, -12)
          CFI (cfiCond323) R8 Frame(CFA, -8)
          CFI (cfiCond323) R14 Frame(CFA, -4)
          CFI (cfiCond323) CFA R13+88
          CFI Block cfiPicker324 Using cfiCommon1
          CFI (cfiPicker324) NoFunction
          CFI (cfiPicker324) Picker
        THUMB
?Subroutine15:
        LDRB     R2,[R3, #+0]
        STRB     R2,[SP, #+0]
        LDRB     R2,[R3, #+1]
        STRB     R2,[SP, #+1]
        BX       LR
          CFI EndBlock cfiCond322
          CFI EndBlock cfiCond323
          CFI EndBlock cfiPicker324

        SECTION `.text`:CODE:NOROOT(2)
          CFI Block cfiBlock325 Using cfiCommon0
          CFI Function user_parse_comand
        THUMB
user_parse_comand:
        PUSH     {R4-R11,LR}
          CFI R14 Frame(CFA, -4)
          CFI R11 Frame(CFA, -8)
          CFI R10 Frame(CFA, -12)
          CFI R9 Frame(CFA, -16)
          CFI R8 Frame(CFA, -20)
          CFI R7 Frame(CFA, -24)
          CFI R6 Frame(CFA, -28)
          CFI R5 Frame(CFA, -32)
          CFI R4 Frame(CFA, -36)
          CFI CFA R13+36
        SUB      SP,SP,#+28
          CFI CFA R13+64
        MOV      R7,R0
        LDRB     R0,[R7, #+1]
        LDRB     R4,[R7, #+0]
        ORR      R4,R4,R0, LSL #+8
        CMP      R4,#+3
        BLT.W    ??user_parse_comand_1
        MOV      R5,R1
        ADDS     R0,R7,R4
        LDRB     R0,[R0, #+3]
        ADDS     R1,R7,R4
        LDRB     R1,[R1, #+2]
        ORR      R9,R1,R0, LSL #+8
        CMP      R5,#+1
        BNE.N    ??user_parse_comand_2
        MOV      R0,R7
          CFI FunCall server_rx_verified
        BL       server_rx_verified
        CBZ.N    R0,??user_parse_comand_2
        MOV      R8,R9
        B.N      ??user_parse_comand_3
??user_parse_comand_2:
        ADDS     R1,R4,#+2
        UXTH     R1,R1
        MOV      R0,R7
          CFI FunCall crc16_calc
        BL       crc16_calc
        MOV      R8,R0
??user_parse_comand_3:
        LDRB     R6,[R7, #+3]
          CFI FunCall wdt_reload
        BL       wdt_reload
        CMP      R9,R8
        BEQ.N    ??user_parse_comand_4
        MOV      R3,R6
        MOV      R2,R5
        MOVS     R1,#+254
        B.N      ??user_parse_comand_5
??user_parse_comand_4:
        MOV      R1,R4
        MOV      R0,R7
          CFI FunCall command_args_valid
        BL       command_args_valid
        CBNZ.N   R0,??user_parse_comand_6
        BL       ?Subroutine70
??CrossCallReturnLabel_245:
        B.N      ??user_parse_comand_5
??user_parse_comand_6:
        MOVS     R0,R5
        ITTT     EQ
        LDREQ.W  R0,??DataTable127
        LDRBEQ   R0,[R0, #+138]
        CMPEQ    R0,#+0
        BNE.N    ??user_parse_comand_7
        LDRB     R0,[R7, #+4]
          CFI FunCall usb_cmd_without_pin
        BL       usb_cmd_without_pin
        CBNZ.N   R0,??user_parse_comand_7
        MOV      R3,R6
        MOVS     R2,#+0
        MOVS     R1,#+4
??user_parse_comand_5:
        B.N      ??user_parse_comand_8
??user_parse_comand_7:
        LDR.W    R0,??DataTable127_1
        LDRB     R0,[R0, #+0]
        CBZ.N    R0,??user_parse_comand_9
        LDRB     R0,[R7, #+4]
          CFI FunCall store_command_allowed
        BL       store_command_allowed
        CBNZ.N   R0,??user_parse_comand_9
        BL       ?Subroutine71
??CrossCallReturnLabel_249:
        B.N      ??user_parse_comand_8
??user_parse_comand_9:
        LDRB     R0,[R7, #+4]
        CMP      R0,#+1
        BEQ.N    ??user_parse_comand_10
        CMP      R0,#+2
        BEQ.W    ??user_parse_comand_11
        CMP      R0,#+3
        BEQ.W    ??user_parse_comand_12
        CMP      R0,#+5
        BEQ.W    ??user_parse_comand_13
        CMP      R0,#+6
        BEQ.W    ??user_parse_comand_14
        CMP      R0,#+7
        BEQ.W    ??user_parse_comand_15
        CMP      R0,#+8
        BEQ.W    ??user_parse_comand_16
        CMP      R0,#+9
        BEQ.W    ??user_parse_comand_17
        CMP      R0,#+10
        BEQ.W    ??user_parse_comand_18
        CMP      R0,#+11
        BEQ.W    ??user_parse_comand_19
        CMP      R0,#+12
        BEQ.W    ??user_parse_comand_20
        CMP      R0,#+13
        BEQ.W    ??user_parse_comand_21
        CMP      R0,#+14
        BEQ.W    ??user_parse_comand_22
        CMP      R0,#+15
        BEQ.W    ??user_parse_comand_23
        CMP      R0,#+16
        BEQ.W    ??user_parse_comand_24
        CMP      R0,#+17
        BEQ.W    ??user_parse_comand_25
        CMP      R0,#+18
        BEQ.W    ??user_parse_comand_26
        CMP      R0,#+19
        BEQ.W    ??user_parse_comand_27
        CMP      R0,#+20
        BEQ.W    ??user_parse_comand_28
        CMP      R0,#+21
        BEQ.W    ??user_parse_comand_29
        CMP      R0,#+22
        BEQ.W    ??user_parse_comand_30
        CMP      R0,#+23
        BEQ.W    ??user_parse_comand_31
        CMP      R0,#+24
        BEQ.W    ??user_parse_comand_32
        CMP      R0,#+27
        BEQ.W    ??user_parse_comand_33
        CMP      R0,#+100
        BEQ.W    ??user_parse_comand_34
        CMP      R0,#+101
        BEQ.W    ??user_parse_comand_35
        CMP      R0,#+102
        BEQ.W    ??user_parse_comand_36
        CMP      R0,#+103
        BEQ.W    ??user_parse_comand_37
        CMP      R0,#+105
        BEQ.W    ??user_parse_comand_38
        CMP      R0,#+106
        BEQ.W    ??user_parse_comand_39
        CMP      R0,#+107
        BEQ.W    ??user_parse_comand_40
        CMP      R0,#+108
        BEQ.W    ??user_parse_comand_41
        CMP      R0,#+110
        BEQ.W    ??user_parse_comand_42
        CMP      R0,#+111
        BEQ.W    ??user_parse_comand_43
        CMP      R0,#+112
        BEQ.W    ??user_parse_comand_44
        B.N      ??user_parse_comand_45
??user_parse_comand_10:
        LDR.W    R0,??DataTable127_2
          CFI FunCall user_debug
        BL       user_debug
        CMP      R5,#+1
        BNE.N    ??user_parse_comand_46
        LDR.W    R4,??DataTable127
        MOVS     R0,#+0
        STRB     R0,[R4, #+137]
        LDRB     R0,[R7, #+5]
        SUBS     R0,R0,#+1
        SBCS     R0,R0,R0
        LSRS     R0,R0,#+31
          CFI FunCall comm_server_registered
        BL       comm_server_registered
        LDRB     R0,[R7, #+5]
        CMP      R0,#+5
        BHI.N    ??user_parse_comand_46
        TBH      [PC, R0, LSL #+1]
        DATA
??user_parse_comand_0:
        DC16     0x6,0x4B,0x4B,0x4B
        DC16     0x46,0x46
        THUMB
??user_parse_comand_47:
        ADDS     R3,R7,#+6
        ADD      R0,SP,#+4
        LDR      R2,[R3, #+0]
        STR      R2,[R0, #+0]
        MOV      R0,R2
          CFI FunCall log_setTime
        BL       log_setTime
        MOVS     R0,#+1
        STRB     R0,[R4, #+133]
          CFI FunCall app_trial_active
        BL       app_trial_active
        CBZ.N    R0,??user_parse_comand_48
          CFI FunCall app_trial_confirm
        BL       app_trial_confirm
          CFI FunCall app_trial_active
        BL       app_trial_active
        CBNZ.N   R0,??user_parse_comand_48
        MOVS     R1,#+0
        MOVS     R0,#+136
          CFI FunCall fw_log
        BL       fw_log
??user_parse_comand_48:
          CFI FunCall app_boot_error_unreported
        BL       app_boot_error_unreported
        CBZ.N    R0,??user_parse_comand_49
        ADD      R0,SP,#+8
          CFI FunCall app_boot_error_text
        BL       app_boot_error_text
        LDR.W    R1,??DataTable127_3
        ADD      R0,SP,#+8
          CFI FunCall strcmp
        BL       strcmp
        CMP      R0,#+0
        ITE      EQ
        MOVEQ    R0,#+137
        MOVNE    R0,#+133
        ADD      R1,SP,#+8
          CFI FunCall fw_log
        BL       fw_log
          CFI FunCall app_boot_error_reported
        BL       app_boot_error_reported
??user_parse_comand_49:
        MOVS     R0,#+5
        STRB     R0,[R4, #+134]
        MOVS     R0,#+0
        STRB     R0,[R4, #+135]
        ADD      R2,R4,#+129
        LDRB     R3,[R2, #+0]
        STRB     R3,[SP, #+0]
        LDRB     R3,[R2, #+1]
        STRB     R3,[SP, #+1]
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        MOV      R1,R0
        LDRH     R0,[SP, #+0]
          CFI FunCall status_event_sent
        BL       status_event_sent
??user_parse_comand_46:
        B.N      ??user_parse_comand_1
??user_parse_comand_50:
          CFI FunCall app_trial_active
        BL       app_trial_active
        CBNZ.N   R0,??user_parse_comand_51
          CFI FunCall user_factoryReset
        BL       user_factoryReset
??user_parse_comand_51:
        MOVS     R0,#+0
        STRB     R0,[R4, #+133]
          CFI FunCall comm_channel_reopen
        BL       comm_channel_reopen
        B.N      ??user_parse_comand_1
??user_parse_comand_11:
        LDR.W    R0,??DataTable127_4
          CFI FunCall user_debug
        BL       user_debug
        LDR.W    R4,??DataTable127
        LDRB     R0,[R4, #+171]
        CMP      R0,#+0
        BNE.W    ??user_parse_comand_52
        LDRB     R0,[R4, #+164]
        CBZ.N    R0,??user_parse_comand_53
        LDR.W    R0,??DataTable127_5
          CFI FunCall fw_abort
        BL       fw_abort
??user_parse_comand_53:
        LDRB     R0,[R7, #+5]
        CMP      R0,#+0
        MOV      R3,R6
        MOV      R2,R5
        BNE.N    ??user_parse_comand_54
        BL       ??Subroutine18_0
??CrossCallReturnLabel_40:
        MOVS     R0,#+33
          CFI FunCall log_addSystemEvent
        BL       log_addSystemEvent
        MOV      R0,#+1000
          CFI FunCall timers_pause
        BL       timers_pause
        B.N      ??user_parse_comand_55
??user_parse_comand_54:
        BL       ??Subroutine18_0
??CrossCallReturnLabel_39:
        LDRB     R0,[R7, #+5]
        STRB     R0,[R4, #+111]
        B.N      ??user_parse_comand_1
??user_parse_comand_12:
        LDR.W    R0,??DataTable127_6
          CFI FunCall user_debug
        BL       user_debug
        LDRB     R0,[R7, #+5]
        CBNZ.N   R0,??user_parse_comand_56
          CFI FunCall user_factoryReset
        BL       user_factoryReset
        BL       ?Subroutine72
??CrossCallReturnLabel_253:
        B.N      ??user_parse_comand_8
??user_parse_comand_56:
        BL       ?Subroutine18
??CrossCallReturnLabel_42:
        LDRB     R0,[R7, #+5]
        LDR.W    R1,??DataTable127
        STRB     R0,[R1, #+112]
        B.N      ??user_parse_comand_1
??user_parse_comand_13:
        LDR.W    R0,??DataTable127_7
          CFI FunCall user_debug
        BL       user_debug
        CMP      R4,#+19
        BNE.W    ??user_parse_comand_57
        LDR.W    R4,??DataTable127
        MOVS     R2,#+16
        ADDS     R1,R7,#+5
        ADD      R0,R4,#+95
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        LDRH     R0,[R4, #+129]
        UBFX     R1,R0,#+7,#+1
        LDRB     R0,[R4, #+95]
        CMP      R1,R0
        BEQ.N    ??user_parse_comand_58
        CMP      R0,#+0
        ITE      NE
        MOVNE    R0,#+30
        MOVEQ    R0,#+31
          CFI FunCall log_addSystemEvent
        BL       log_addSystemEvent
??user_parse_comand_58:
        LDRH     R0,[R4, #+129]
        UBFX     R1,R0,#+6,#+1
        LDRB     R0,[R4, #+96]
        CMP      R1,R0
        BEQ.N    ??user_parse_comand_59
        CMP      R0,#+0
        ITE      NE
        MOVNE    R0,#+28
        MOVEQ    R0,#+29
          CFI FunCall log_addSystemEvent
        BL       log_addSystemEvent
??user_parse_comand_59:
        LDRH     R0,[R4, #+129]
        UBFX     R1,R0,#+5,#+1
        LDRB     R0,[R4, #+97]
        CMP      R1,R0
        BEQ.N    ??user_parse_comand_60
        CMP      R0,#+0
        ITE      NE
        MOVNE    R0,#+24
        MOVEQ    R0,#+25
          CFI FunCall log_addSystemEvent
        BL       log_addSystemEvent
??user_parse_comand_60:
        LDRH     R0,[R4, #+129]
        UBFX     R1,R0,#+4,#+1
        LDRB     R0,[R4, #+98]
        CMP      R1,R0
        BEQ.N    ??user_parse_comand_61
        CMP      R0,#+0
        ITE      NE
        MOVNE    R0,#+26
        MOVEQ    R0,#+27
          CFI FunCall log_addSystemEvent
        BL       log_addSystemEvent
??user_parse_comand_61:
        LDRB     R1,[R4, #+95]
        LDRH     R0,[R4, #+129]
        BFI      R0,R1,#+7,#+1
        STRH     R0,[R4, #+129]
        LDRB     R1,[R4, #+96]
        BFI      R0,R1,#+6,#+1
        STRH     R0,[R4, #+129]
        LDRB     R1,[R4, #+97]
        BFI      R0,R1,#+5,#+1
        STRH     R0,[R4, #+129]
        LDRB     R1,[R4, #+98]
        BFI      R0,R1,#+4,#+1
        STRH     R0,[R4, #+129]
        MOV      R2,R6
        MOV      R1,R5
        MOVS     R0,#+5
        B.N      ??user_parse_comand_62
??user_parse_comand_14:
        LDR.W    R0,??DataTable128
          CFI FunCall user_debug
        BL       user_debug
        LDRB     R0,[R7, #+5]
        CMP      R0,#+0
        ITT      NE
        LDRBNE   R0,[R7, #+6]
        CMPNE    R0,#+0
        BEQ.N    ??user_parse_comand_63
        CMP      R0,#+3
        ITT      EQ
        LDRBEQ   R0,[R7, #+7]
        CMPEQ    R0,#+0
        BEQ.N    ??user_parse_comand_63
        LDR.W    R0,??DataTable127
        LDRB     R1,[R7, #+5]
        STRB     R1,[R0, #+113]
        LDRB     R1,[R7, #+6]
        STRB     R1,[R0, #+114]
        LDRB     R1,[R7, #+7]
        STRB     R1,[R0, #+115]
        BL       ?Subroutine72
??CrossCallReturnLabel_254:
        B.N      ??user_parse_comand_8
??user_parse_comand_15:
        CMP      R4,#+4
        BNE.N    ??user_parse_comand_64
        LDRB     R0,[R7, #+5]
          CFI FunCall photo_start
        BL       photo_start
        B.N      ??user_parse_comand_65
??user_parse_comand_64:
        MOVS     R0,#+1
??user_parse_comand_65:
        MOV      R3,R6
        MOV      R2,R5
        MOV      R1,R0
        MOVS     R0,#+7
        B.N      ??user_parse_comand_66
??user_parse_comand_16:
        LDR.W    R0,??DataTable128_1
          CFI FunCall user_debug
        BL       user_debug
          CFI FunCall fw_update_quiet
        BL       fw_update_quiet
        CMP      R0,#+0
        BNE.W    ??user_parse_comand_52
        LDRB     R0,[R7, #+5]
        CMP      R0,#+0
??user_parse_comand_63:
        BEQ.W    ??user_parse_comand_57
        SUBS     R0,R4,#+3
        CMP      R0,#+3
        BLT.N    ??user_parse_comand_67
        MOV      R8,#+0
        LDR.W    R9,??DataTable128_2
        MOVS     R1,#+11
        MOV      R0,R9
          CFI FunCall __aeabi_memclr4
        BL       __aeabi_memclr4
        SUBS     R4,R4,#+3
        CMP      R4,#+13
        BLT.N    ??user_parse_comand_68
        MOVS     R2,#+10
        ADD      R1,R7,#+8
        MOV      R0,R9
          CFI FunCall strncpy
        BL       strncpy
??user_parse_comand_68:
        LDRB     R0,[R7, #+6]
        CMP      R0,#+2
        ITTE     LT
        LDRBLT   R0,[R7, #+7]
        CMPLT    R0,#+2
        MOVGE    R8,#+1
        BGE.N    ??user_parse_comand_69
        LDRB     R0,[R7, #+5]
          CFI FunCall cmd8_event
        BL       cmd8_event
        LDR.W    R1,??DataTable128_3
        STRB     R0,[R1, #+0]
        LDRB     R0,[R7, #+6]
        CBNZ.N   R0,??user_parse_comand_70
        LDR.W    R0,??user_activateOut_1
        LDRB     R0,[R0, #+197]
        CBZ.N    R0,??user_parse_comand_70
        MOVS     R0,#+0
        STR      R0,[SP, #+0]
        BL       ??Subroutine49_0
??CrossCallReturnLabel_174:
          CFI FunCall out_load
        BL       out_load
??user_parse_comand_70:
        LDRB     R0,[R7, #+7]
        CBNZ.N   R0,??user_parse_comand_71
        LDR.W    R0,??user_activateOut_1
        LDRB     R0,[R0, #+208]
        CBZ.N    R0,??user_parse_comand_71
        STR      R8,[SP, #+0]
        BL       ??Subroutine49_0
??CrossCallReturnLabel_175:
        MOVS     R0,#+1
          CFI FunCall out_load
        BL       out_load
??user_parse_comand_71:
        LDRB     R0,[R7, #+6]
        CMP      R0,#+1
        LDRB     R0,[R7, #+7]
        BEQ.N    ??user_parse_comand_72
        CMP      R0,#+1
        BNE.N    ??user_parse_comand_69
        B.N      ??user_parse_comand_73
??user_parse_comand_72:
        CMP      R0,#+1
        BNE.N    ??user_parse_comand_74
??user_parse_comand_73:
        LDR.W    R0,??user_activateOut_1
        LDRB     R1,[R0, #+208]
        CBZ.N    R1,??user_parse_comand_75
        MOVS     R1,#+1
        B.N      ??user_parse_comand_75
??user_parse_comand_74:
        MOVS     R1,#+0
??user_parse_comand_75:
        LDRB     R0,[R7, #+6]
        CMP      R0,#+1
        BNE.N    ??user_parse_comand_76
        LDR.W    R0,??user_activateOut_1
        LDRB     R0,[R0, #+197]
        CBZ.N    R0,??user_parse_comand_77
        MOVS     R0,#+1
        B.N      ??user_parse_comand_77
??user_parse_comand_76:
        MOVS     R0,#+0
??user_parse_comand_77:
        MOV      R3,R9
        MOVS     R2,#+6
          CFI FunCall user_activateOut
        BL       user_activateOut
        CBNZ.N   R0,??user_parse_comand_69
        MOV      R8,#+2
??user_parse_comand_69:
        MOV      R3,R6
        MOV      R2,R5
        MOV      R1,R8
        B.N      ??user_parse_comand_8
??user_parse_comand_17:
        LDR.W    R0,??DataTable128_4
          CFI FunCall user_debug
        BL       user_debug
        MOV      R1,R6
        MOV      R0,R5
          CFI FunCall user_send_logRecords
        BL       user_send_logRecords
        B.N      ??user_parse_comand_1
??user_parse_comand_18:
        LDR.W    R0,??DataTable128_5
          CFI FunCall user_debug
        BL       user_debug
        MOV      R1,R6
        MOV      R0,R5
          CFI FunCall user_send_tempIDs
        BL       user_send_tempIDs
        B.N      ??user_parse_comand_1
??user_parse_comand_19:
        LDR.W    R0,??DataTable128_6
          CFI FunCall user_debug
        BL       user_debug
        CMP      R4,#+14
??user_parse_comand_67:
        BLT.W    ??user_parse_comand_57
        LDRB     R0,[R7, #+5]
        CMP      R0,#+0
        ITT      NE
        LDRBNE   R0,[R7, #+15]
        CMPNE    R0,#+0
        BEQ.N    ??user_parse_comand_78
        LDR.W    R10,??DataTable128_7
        MOVS     R2,#+8
        MOVS     R1,#+0
        MOV      R0,R10
          CFI FunCall memchr
        BL       memchr
        CMP      R0,#+0
        BEQ.N    ??user_parse_comand_78
        SUB      R0,R4,#+13
        UXTH     R4,R0
        ADDS     R0,R4,#+1
          CFI FunCall malloc
        BL       malloc
        MOVS     R9,R0
        BNE.N    ??user_parse_comand_79
        BL       ?Subroutine71
??CrossCallReturnLabel_250:
        B.N      ??user_parse_comand_8
??user_parse_comand_79:
        MOV      R1,R10
        ADD      R0,SP,#+8
          CFI FunCall strcpy
        BL       strcpy
        MOVS     R2,#+10
        ADDS     R1,R7,#+5
          CFI FunCall strncat
        BL       strncat
        MOV      R2,R4
        ADD      R1,R7,#+15
        MOV      R0,R9
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        MOVS     R0,#+0
        STRB     R0,[R9, R4]
        BL       ?Subroutine18
??CrossCallReturnLabel_41:
        ADD      R1,SP,#+8
        MOV      R0,R9
          CFI FunCall user_sendsms
        BL       user_sendsms
        MOV      R0,R9
          CFI FunCall free
        BL       free
        B.N      ??user_parse_comand_1
??user_parse_comand_20:
        LDR.W    R0,??DataTable128_8
          CFI FunCall user_debug
        BL       user_debug
        SUBS     R4,R4,#+3
        CMP      R4,#+512
        BNE.N    ??user_parse_comand_57
        ADDS     R1,R7,#+5
        LDRB     R0,[R1, #+8]
        CMP      R0,#+0
        ITT      NE
        LDRBNE   R0,[R1, #+137]
        CMPNE    R0,#+0
??user_parse_comand_78:
        BEQ.N    ??user_parse_comand_57
        LDR.W    R11,??DataTable127
        LDR      R9,[R11, #+27]
        LDRH     R8,[R11, #+510]
        LDRH     R4,[R11, #+129]
        LSRS     R4,R4,#+15
        ADDS     R0,R7,#+5
          CFI FunCall config_exchange
        BL       config_exchange
          CFI FunCall config_terminate_strings
        BL       config_terminate_strings
        LDR.W    R0,??user_activateOut_1
        LDR      R0,[R0, #+0]
        STR      R0,[R11, #+27]
        LDRH     R0,[R11, #+129]
        UBFX     R0,R0,#+0,#+15
        STRH     R0,[R11, #+129]
        LDR.W    R0,??DataTable129
        STRB     R5,[R0, #+22]
        STRB     R6,[R0, #+23]
        MOVS     R1,#+12
        STRB     R1,[R0, #+21]
        MOVS     R1,#+0
        STRB     R1,[R0, #+24]
          CFI FunCall store_config_start
        BL       store_config_start
        MOV      R10,R0
        ADDS     R0,R7,#+5
          CFI FunCall config_exchange
        BL       config_exchange
        STR      R9,[R11, #+27]
        STRH     R8,[R11, #+510]
        LDRH     R0,[R11, #+129]
        BFI      R0,R4,#+15,#+1
        STRH     R0,[R11, #+129]
        CMP      R10,#+0
        B.N      ??user_parse_comand_80
??user_parse_comand_21:
        MOVS     R3,#+32
        MOVS     R2,#+2
        B.N      ??user_parse_comand_81
??user_parse_comand_22:
        MOVS     R3,#+32
        MOVS     R2,#+4
??user_parse_comand_81:
        MOV      R1,R4
        MOV      R0,R7
          CFI FunCall sync_cells_valid
        BL       sync_cells_valid
        CMP      R0,#+0
        BEQ.N    ??user_parse_comand_57
        B.N      ??user_parse_comand_82
??user_parse_comand_23:
        MOVS     R3,#+16
        MOVS     R2,#+3
        MOV      R1,R4
        MOV      R0,R7
          CFI FunCall sync_cells_valid
        BL       sync_cells_valid
        CBZ.N    R0,??user_parse_comand_57
        B.N      ??user_parse_comand_82
??user_parse_comand_24:
        CMP      R4,#+8
        BLT.N    ??user_parse_comand_57
        LDRB     R0,[R7, #+9]
        CBZ.N    R0,??user_parse_comand_57
        SUBS     R4,R4,#+8
        CMP      R4,R0, LSL #+2
        BNE.N    ??user_parse_comand_57
??user_parse_comand_82:
          CFI FunCall mem_tables_ready
        BL       mem_tables_ready
        CMP      R0,#+0
        BEQ.N    ??user_parse_comand_83
        MOV      R2,R6
        MOV      R1,R5
        MOV      R0,R7
          CFI FunCall table_runtime_begin
        BL       table_runtime_begin
        CMP      R0,#+0
??user_parse_comand_80:
        BNE.N    ??user_parse_comand_84
??user_parse_comand_83:
        B.N      ??user_parse_comand_52
??user_parse_comand_25:
        LDR.W    R0,??DataTable131
          CFI FunCall user_debug
        BL       user_debug
        MOV      R1,R6
        MOV      R0,R5
          CFI FunCall user_send_crcPack
        BL       user_send_crcPack
??user_parse_comand_84:
        B.N      ??user_parse_comand_1
??user_parse_comand_26:
        LDR.W    R0,??DataTable131_1
          CFI FunCall user_debug
        BL       user_debug
        LDRB     R0,[R7, #+5]
        CMP      R0,#+101
        BLT.N    ??user_parse_comand_85
??user_parse_comand_57:
        BL       ?Subroutine70
??CrossCallReturnLabel_246:
        B.N      ??user_parse_comand_8
??user_parse_comand_85:
        MOV      R2,R6
        MOV      R1,R5
          CFI FunCall user_send_crcMemoryID
        BL       user_send_crcMemoryID
        B.N      ??user_parse_comand_1
??user_parse_comand_27:
        MOVS     R0,R5
          CFI FunCall user_debug
        ITT      NE
        LDRNE.W  R0,??DataTable131_2
        BLNE     user_debug
        CMP      R4,#+19
        BNE.N    ??user_parse_comand_86
        MOV      R1,R5
        ADDS     R0,R7,#+5
          CFI FunCall fw_start
        BL       fw_start
        MOVS     R1,R0
        BNE.N    ??user_parse_comand_87
        LDR.W    R1,??DataTable132
        LDRB     R1,[R1, #+0]
        CBZ.N    R1,??user_parse_comand_87
        MOV      R2,R6
        MOV      R1,R5
        MOVS     R0,#+19
??user_parse_comand_62:
          CFI FunCall store_command_start
        BL       store_command_start
        B.N      ??user_parse_comand_1
??user_parse_comand_87:
        MOV      R3,R6
        MOV      R2,R5
        MOV      R1,R0
        B.N      ??user_parse_comand_8
??user_parse_comand_86:
        BL       ?Subroutine68
??CrossCallReturnLabel_241:
        B.N      ??user_parse_comand_57
??user_parse_comand_28:
        MOVS     R0,R5
        BEQ.N    ??user_parse_comand_88
        CMP      R4,#+5
        BLT.N    ??user_parse_comand_88
        LDR.W    R9,??DataTable128_2
        LDRB     R2,[R7, #+5]
        LDRB     R0,[R7, #+6]
        ORR      R2,R2,R0, LSL #+8
        LDR.W    R1,??DataTable133
        MOV      R0,R9
          CFI FunCall sprintf
        BL       sprintf
        MOV      R0,R9
          CFI FunCall user_debug
        BL       user_debug
??user_parse_comand_88:
        MOV      R2,R5
        SUBS     R1,R4,#+3
        UXTH     R1,R1
        ADDS     R0,R7,#+5
          CFI FunCall fw_take_block
        BL       fw_take_block
        MOV      R1,R0
        CMP      R1,#+252
        BNE.N    ??user_parse_comand_89
        MOV      R0,R6
          CFI FunCall fw_async_reply
        BL       fw_async_reply
        B.N      ??user_parse_comand_1
??user_parse_comand_89:
        MOV      R3,R6
        MOV      R2,R5
        LDRB     R0,[R7, #+4]
          CFI FunCall user_send_statusUpdateFW
        BL       user_send_statusUpdateFW
        B.N      ??user_parse_comand_1
??user_parse_comand_29:
        LDR.W    R0,??DataTable133_1
          CFI FunCall user_debug
        BL       user_debug
        CMP      R4,#+3
        IT       NE
        CMPNE    R4,#+9
        BNE.N    ??user_parse_comand_90
        SUBS     R3,R4,#+3
        UXTB     R3,R3
        ADDS     R2,R7,#+5
        MOV      R1,R6
        MOV      R0,R5
          CFI FunCall photo_request_answer
        BL       photo_request_answer
        B.N      ??user_parse_comand_1
??user_parse_comand_90:
        BL       ?Subroutine70
??CrossCallReturnLabel_247:
        MOVS     R0,#+21
        B.N      ??user_parse_comand_66
??user_parse_comand_30:
        LDR.W    R0,??DataTable133_2
          CFI FunCall user_debug
        BL       user_debug
        MOV      R1,R6
        MOV      R0,R5
          CFI FunCall user_send_statusPackSecond
        BL       user_send_statusPackSecond
        B.N      ??user_parse_comand_1
??user_parse_comand_31:
        LDR.W    R0,??DataTable133_3
          CFI FunCall user_debug
        BL       user_debug
        ADDS     R2,R7,#+5
        ADD      R3,SP,#+4
        LDR      R1,[R2, #+0]
        STR      R1,[R3, #+0]
        MOV      R0,R1
          CFI FunCall log_setTime
        BL       log_setTime
        BL       ?Subroutine72
??CrossCallReturnLabel_255:
        B.N      ??user_parse_comand_8
??user_parse_comand_32:
        LDR.W    R0,??DataTable133_4
          CFI FunCall user_debug
        BL       user_debug
        CMP      R5,#+1
          CFI FunCall comm_server_status_ack
        ITT      EQ
        LDRBEQ   R0,[R7, #+5]
        BLEQ     comm_server_status_ack
        LDR.W    R0,??DataTable127
        LDRB     R1,[R0, #+136]
        CMP      R1,#+2
        BNE.N    ??user_parse_comand_91
        MOVS     R1,#+0
        STRB     R1,[R0, #+137]
        STRB     R1,[R0, #+135]
??user_parse_comand_91:
        B.N      ??user_parse_comand_1
??user_parse_comand_33:
        LDR.W    R0,??DataTable134
          CFI FunCall user_debug
        BL       user_debug
        BL       ?Subroutine72
??CrossCallReturnLabel_256:
        B.N      ??user_parse_comand_8
??user_parse_comand_34:
        CMP      R5,#+0
        BNE.N    ??user_parse_comand_92
        MOVS     R0,#+0
          CFI FunCall user_send_identPackToConfigurator
        BL       user_send_identPackToConfigurator
??user_parse_comand_92:
        B.N      ??user_parse_comand_1
??user_parse_comand_35:
        CMP      R5,#+0
        BNE.N    ??user_parse_comand_92
        MOVS     R0,#+0
          CFI FunCall user_send_statusPackToConfigurator
        BL       user_send_statusPackToConfigurator
??user_parse_comand_93:
        B.N      ??user_parse_comand_1
??user_parse_comand_36:
        CMP      R5,#+0
        BNE.N    ??user_parse_comand_93
        LDR      R0,[R7, #+5]
        LDR.W    R1,??user_activateOut_1
        LDR      R1,[R1, #+4]
        CMP      R0,R1
        ITE      EQ
        MOVEQ    R1,#+1
        MOVNE    R1,#+0
        LDR.W    R0,??DataTable127
        STRB     R1,[R0, #+138]
        STR      R6,[SP, #+0]
        MOVS     R3,#+0
        MOVS     R2,#+1
        ADD      R1,R0,#+138
        MOVS     R0,#+102
        B.N      ??CrossCallReturnLabel_150
??user_parse_comand_37:
        MOVS     R0,R5
        BNE.N    ??user_parse_comand_1
        MOVS     R0,#+2
          CFI FunCall user_clearMemBlock
        BL       user_clearMemBlock
        CBNZ.N   R0,??user_parse_comand_94
??user_parse_comand_52:
        BL       ?Subroutine71
??CrossCallReturnLabel_251:
        B.N      ??user_parse_comand_8
??user_parse_comand_94:
          CFI FunCall user_getNumbersCount
        BL       user_getNumbersCount
        LDR.W    R1,??DataTable127
        STRH     R0,[R1, #+81]
        BL       ?Subroutine49
??CrossCallReturnLabel_176:
        MOVS     R0,#+103
        B.N      ??CrossCallReturnLabel_150
??user_parse_comand_38:
        CMP      R5,#+0
        BNE.N    ??user_parse_comand_1
        BL       ?Subroutine49
??CrossCallReturnLabel_177:
          CFI FunCall user_send_anyData
        BL       user_send_anyData
        LDRB     R1,[R7, #+5]
        LDR.W    R0,??DataTable127
        STRB     R1,[R0, #+169]
??user_parse_comand_55:
          CFI FunCall user_savereset
        BL       user_savereset
        B.N      ??user_parse_comand_1
??user_parse_comand_39:
        CMP      R5,#+0
        BNE.N    ??user_parse_comand_1
        MOVS     R0,#+0
          CFI FunCall user_send_configToConfigurator
        BL       user_send_configToConfigurator
        B.N      ??user_parse_comand_1
??user_parse_comand_40:
        BL       ?Subroutine43
??CrossCallReturnLabel_150:
          CFI FunCall user_send_anyData
        BL       user_send_anyData
        B.N      ??user_parse_comand_1
??user_parse_comand_43:
        CMP      R5,#+0
        BNE.N    ??user_parse_comand_1
        MOV      R0,R6
          CFI FunCall user_send_usbDiag
        BL       user_send_usbDiag
        B.N      ??user_parse_comand_1
??user_parse_comand_44:
        CMP      R5,#+0
        BNE.N    ??user_parse_comand_1
        MOV      R0,R6
          CFI FunCall user_send_commDiag
        BL       user_send_commDiag
        B.N      ??user_parse_comand_1
??user_parse_comand_42:
        MOVS     R0,R5
        BNE.N    ??user_parse_comand_95
        CMP      R4,#+4
        BGE.N    ??user_parse_comand_96
??user_parse_comand_95:
        BL       ?Subroutine70
??CrossCallReturnLabel_248:
        B.N      ??user_parse_comand_8
??user_parse_comand_96:
        LDR.W    R1,??DataTable127
        LDRB     R0,[R1, #+164]
        CMP      R0,#+0
        ITT      EQ
        LDRBEQ   R0,[R1, #+171]
        CMPEQ    R0,#+0
        BNE.N    ??user_parse_comand_97
          CFI FunCall app_trial_active
        BL       app_trial_active
        CBZ.N    R0,??user_parse_comand_98
??user_parse_comand_97:
        MOV      R3,R6
        MOVS     R2,#+0
        MOVS     R1,#+5
??user_parse_comand_8:
        LDRB     R0,[R7, #+4]
        B.N      ??user_parse_comand_66
??user_parse_comand_98:
        ADD      R3,SP,#+8
        SUBS     R2,R4,#+3
        UXTH     R2,R2
        ADDS     R1,R7,#+5
        MOVS     R0,#+1
          CFI FunCall boot_manual_command
        BL       boot_manual_command
        BL       ?Subroutine48
??CrossCallReturnLabel_173:
        LDRB     R0,[SP, #+8]
        CMP      R0,#+2
        ITTT     EQ
        ADDEQ    R4,SP,#+8
        LDRBEQ   R0,[R4, #+1]
        CMPEQ    R0,#+0
        BNE.N    ??user_parse_comand_1
        MOVS     R0,#+200
          CFI FunCall timers_pause
        BL       timers_pause
          CFI FunCall boot_manual_apply
        BL       boot_manual_apply
        STRB     R0,[R4, #+1]
        BL       ?Subroutine48
??CrossCallReturnLabel_172:
        B.N      ??user_parse_comand_1
??user_parse_comand_41:
        BL       ?Subroutine43
??CrossCallReturnLabel_151:
          CFI FunCall user_send_anyData
        BL       user_send_anyData
        CBNZ.N   R5,??user_parse_comand_1
        LDRB     R0,[R7, #+5]
        LDR.W    R1,??DataTable127
        STRB     R0,[R1, #+170]
        B.N      ??user_parse_comand_1
??user_parse_comand_45:
        MOV      R3,R6
        MOV      R2,R5
        MOVS     R1,#+255
??user_parse_comand_66:
          CFI FunCall user_send_answer_comand
        BL       user_send_answer_comand
??user_parse_comand_1:
        ADD      SP,SP,#+28
          CFI CFA R13+36
        POP      {R4-R11,PC}
          CFI EndBlock cfiBlock325

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond326 Using cfiCommon0
          CFI Function user_parse_comand
          CFI Conditional ??CrossCallReturnLabel_253
          CFI R4 Frame(CFA, -36)
          CFI R5 Frame(CFA, -32)
          CFI R6 Frame(CFA, -28)
          CFI R7 Frame(CFA, -24)
          CFI R8 Frame(CFA, -20)
          CFI R9 Frame(CFA, -16)
          CFI R10 Frame(CFA, -12)
          CFI R11 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+64
          CFI Block cfiCond327 Using cfiCommon0
          CFI (cfiCond327) Function user_parse_comand
          CFI (cfiCond327) Conditional ??CrossCallReturnLabel_254
          CFI (cfiCond327) R4 Frame(CFA, -36)
          CFI (cfiCond327) R5 Frame(CFA, -32)
          CFI (cfiCond327) R6 Frame(CFA, -28)
          CFI (cfiCond327) R7 Frame(CFA, -24)
          CFI (cfiCond327) R8 Frame(CFA, -20)
          CFI (cfiCond327) R9 Frame(CFA, -16)
          CFI (cfiCond327) R10 Frame(CFA, -12)
          CFI (cfiCond327) R11 Frame(CFA, -8)
          CFI (cfiCond327) R14 Frame(CFA, -4)
          CFI (cfiCond327) CFA R13+64
          CFI Block cfiCond328 Using cfiCommon0
          CFI (cfiCond328) Function user_parse_comand
          CFI (cfiCond328) Conditional ??CrossCallReturnLabel_255
          CFI (cfiCond328) R4 Frame(CFA, -36)
          CFI (cfiCond328) R5 Frame(CFA, -32)
          CFI (cfiCond328) R6 Frame(CFA, -28)
          CFI (cfiCond328) R7 Frame(CFA, -24)
          CFI (cfiCond328) R8 Frame(CFA, -20)
          CFI (cfiCond328) R9 Frame(CFA, -16)
          CFI (cfiCond328) R10 Frame(CFA, -12)
          CFI (cfiCond328) R11 Frame(CFA, -8)
          CFI (cfiCond328) R14 Frame(CFA, -4)
          CFI (cfiCond328) CFA R13+64
          CFI Block cfiCond329 Using cfiCommon0
          CFI (cfiCond329) Function user_parse_comand
          CFI (cfiCond329) Conditional ??CrossCallReturnLabel_256
          CFI (cfiCond329) R4 Frame(CFA, -36)
          CFI (cfiCond329) R5 Frame(CFA, -32)
          CFI (cfiCond329) R6 Frame(CFA, -28)
          CFI (cfiCond329) R7 Frame(CFA, -24)
          CFI (cfiCond329) R8 Frame(CFA, -20)
          CFI (cfiCond329) R9 Frame(CFA, -16)
          CFI (cfiCond329) R10 Frame(CFA, -12)
          CFI (cfiCond329) R11 Frame(CFA, -8)
          CFI (cfiCond329) R14 Frame(CFA, -4)
          CFI (cfiCond329) CFA R13+64
          CFI Block cfiPicker330 Using cfiCommon1
          CFI (cfiPicker330) NoFunction
          CFI (cfiPicker330) Picker
        THUMB
?Subroutine72:
        MOV      R3,R6
        MOV      R2,R5
        MOVS     R1,#+0
        BX       LR
          CFI EndBlock cfiCond326
          CFI EndBlock cfiCond327
          CFI EndBlock cfiCond328
          CFI EndBlock cfiCond329
          CFI EndBlock cfiPicker330

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond331 Using cfiCommon0
          CFI Function user_parse_comand
          CFI Conditional ??CrossCallReturnLabel_245
          CFI R4 Frame(CFA, -36)
          CFI R5 Frame(CFA, -32)
          CFI R6 Frame(CFA, -28)
          CFI R7 Frame(CFA, -24)
          CFI R8 Frame(CFA, -20)
          CFI R9 Frame(CFA, -16)
          CFI R10 Frame(CFA, -12)
          CFI R11 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+64
          CFI Block cfiCond332 Using cfiCommon0
          CFI (cfiCond332) Function user_parse_comand
          CFI (cfiCond332) Conditional ??CrossCallReturnLabel_246
          CFI (cfiCond332) R4 Frame(CFA, -36)
          CFI (cfiCond332) R5 Frame(CFA, -32)
          CFI (cfiCond332) R6 Frame(CFA, -28)
          CFI (cfiCond332) R7 Frame(CFA, -24)
          CFI (cfiCond332) R8 Frame(CFA, -20)
          CFI (cfiCond332) R9 Frame(CFA, -16)
          CFI (cfiCond332) R10 Frame(CFA, -12)
          CFI (cfiCond332) R11 Frame(CFA, -8)
          CFI (cfiCond332) R14 Frame(CFA, -4)
          CFI (cfiCond332) CFA R13+64
          CFI Block cfiCond333 Using cfiCommon0
          CFI (cfiCond333) Function user_parse_comand
          CFI (cfiCond333) Conditional ??CrossCallReturnLabel_247
          CFI (cfiCond333) R4 Frame(CFA, -36)
          CFI (cfiCond333) R5 Frame(CFA, -32)
          CFI (cfiCond333) R6 Frame(CFA, -28)
          CFI (cfiCond333) R7 Frame(CFA, -24)
          CFI (cfiCond333) R8 Frame(CFA, -20)
          CFI (cfiCond333) R9 Frame(CFA, -16)
          CFI (cfiCond333) R10 Frame(CFA, -12)
          CFI (cfiCond333) R11 Frame(CFA, -8)
          CFI (cfiCond333) R14 Frame(CFA, -4)
          CFI (cfiCond333) CFA R13+64
          CFI Block cfiCond334 Using cfiCommon0
          CFI (cfiCond334) Function user_parse_comand
          CFI (cfiCond334) Conditional ??CrossCallReturnLabel_248
          CFI (cfiCond334) R4 Frame(CFA, -36)
          CFI (cfiCond334) R5 Frame(CFA, -32)
          CFI (cfiCond334) R6 Frame(CFA, -28)
          CFI (cfiCond334) R7 Frame(CFA, -24)
          CFI (cfiCond334) R8 Frame(CFA, -20)
          CFI (cfiCond334) R9 Frame(CFA, -16)
          CFI (cfiCond334) R10 Frame(CFA, -12)
          CFI (cfiCond334) R11 Frame(CFA, -8)
          CFI (cfiCond334) R14 Frame(CFA, -4)
          CFI (cfiCond334) CFA R13+64
          CFI Block cfiPicker335 Using cfiCommon1
          CFI (cfiPicker335) NoFunction
          CFI (cfiPicker335) Picker
        THUMB
?Subroutine70:
        MOV      R3,R6
        MOV      R2,R5
        MOVS     R1,#+1
        BX       LR
          CFI EndBlock cfiCond331
          CFI EndBlock cfiCond332
          CFI EndBlock cfiCond333
          CFI EndBlock cfiCond334
          CFI EndBlock cfiPicker335

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond336 Using cfiCommon0
          CFI Function user_parse_comand
          CFI Conditional ??CrossCallReturnLabel_176
          CFI R4 Frame(CFA, -36)
          CFI R5 Frame(CFA, -32)
          CFI R6 Frame(CFA, -28)
          CFI R7 Frame(CFA, -24)
          CFI R8 Frame(CFA, -20)
          CFI R9 Frame(CFA, -16)
          CFI R10 Frame(CFA, -12)
          CFI R11 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+64
          CFI Block cfiCond337 Using cfiCommon0
          CFI (cfiCond337) Function user_parse_comand
          CFI (cfiCond337) Conditional ??CrossCallReturnLabel_177
          CFI (cfiCond337) R4 Frame(CFA, -36)
          CFI (cfiCond337) R5 Frame(CFA, -32)
          CFI (cfiCond337) R6 Frame(CFA, -28)
          CFI (cfiCond337) R7 Frame(CFA, -24)
          CFI (cfiCond337) R8 Frame(CFA, -20)
          CFI (cfiCond337) R9 Frame(CFA, -16)
          CFI (cfiCond337) R10 Frame(CFA, -12)
          CFI (cfiCond337) R11 Frame(CFA, -8)
          CFI (cfiCond337) R14 Frame(CFA, -4)
          CFI (cfiCond337) CFA R13+64
          CFI Block cfiPicker338 Using cfiCommon1
          CFI (cfiPicker338) NoFunction
          CFI (cfiPicker338) Picker
        THUMB
?Subroutine49:
        STR      R6,[SP, #+0]
          CFI Block cfiCond339 Using cfiCommon0
          CFI (cfiCond339) Function user_parse_comand
          CFI (cfiCond339) Conditional ??CrossCallReturnLabel_174
          CFI (cfiCond339) R4 Frame(CFA, -36)
          CFI (cfiCond339) R5 Frame(CFA, -32)
          CFI (cfiCond339) R6 Frame(CFA, -28)
          CFI (cfiCond339) R7 Frame(CFA, -24)
          CFI (cfiCond339) R8 Frame(CFA, -20)
          CFI (cfiCond339) R9 Frame(CFA, -16)
          CFI (cfiCond339) R10 Frame(CFA, -12)
          CFI (cfiCond339) R11 Frame(CFA, -8)
          CFI (cfiCond339) R14 Frame(CFA, -4)
          CFI (cfiCond339) CFA R13+64
          CFI Block cfiCond340 Using cfiCommon0
          CFI (cfiCond340) Function user_parse_comand
          CFI (cfiCond340) Conditional ??CrossCallReturnLabel_175
          CFI (cfiCond340) R4 Frame(CFA, -36)
          CFI (cfiCond340) R5 Frame(CFA, -32)
          CFI (cfiCond340) R6 Frame(CFA, -28)
          CFI (cfiCond340) R7 Frame(CFA, -24)
          CFI (cfiCond340) R8 Frame(CFA, -20)
          CFI (cfiCond340) R9 Frame(CFA, -16)
          CFI (cfiCond340) R10 Frame(CFA, -12)
          CFI (cfiCond340) R11 Frame(CFA, -8)
          CFI (cfiCond340) R14 Frame(CFA, -4)
          CFI (cfiCond340) CFA R13+64
??Subroutine49_0:
        MOVS     R3,#+0
        MOVS     R2,#+0
        MOVS     R1,#+0
        BX       LR
          CFI EndBlock cfiCond336
          CFI EndBlock cfiCond337
          CFI EndBlock cfiPicker338
          CFI EndBlock cfiCond339
          CFI EndBlock cfiCond340

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond341 Using cfiCommon0
          CFI Function user_parse_comand
          CFI Conditional ??CrossCallReturnLabel_173
          CFI R4 Frame(CFA, -36)
          CFI R5 Frame(CFA, -32)
          CFI R6 Frame(CFA, -28)
          CFI R7 Frame(CFA, -24)
          CFI R8 Frame(CFA, -20)
          CFI R9 Frame(CFA, -16)
          CFI R10 Frame(CFA, -12)
          CFI R11 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+64
          CFI Block cfiCond342 Using cfiCommon0
          CFI (cfiCond342) Function user_parse_comand
          CFI (cfiCond342) Conditional ??CrossCallReturnLabel_172
          CFI (cfiCond342) R4 Frame(CFA, -36)
          CFI (cfiCond342) R5 Frame(CFA, -32)
          CFI (cfiCond342) R6 Frame(CFA, -28)
          CFI (cfiCond342) R7 Frame(CFA, -24)
          CFI (cfiCond342) R8 Frame(CFA, -20)
          CFI (cfiCond342) R9 Frame(CFA, -16)
          CFI (cfiCond342) R10 Frame(CFA, -12)
          CFI (cfiCond342) R11 Frame(CFA, -8)
          CFI (cfiCond342) R14 Frame(CFA, -4)
          CFI (cfiCond342) CFA R13+64
          CFI Block cfiPicker343 Using cfiCommon1
          CFI (cfiPicker343) NoFunction
          CFI (cfiPicker343) Picker
        THUMB
?Subroutine48:
        STR      R6,[SP, #+0]
        MOVS     R3,#+0
        MOVS     R2,#+8
        ADD      R1,SP,#+8
        MOVS     R0,#+110
          CFI (cfiCond341) FunCall user_parse_comand user_send_anyData
          CFI (cfiCond341) FunCall user_parse_comand user_send_anyData
        B.W      user_send_anyData
          CFI EndBlock cfiCond341
          CFI EndBlock cfiCond342
          CFI EndBlock cfiPicker343

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond344 Using cfiCommon0
          CFI Function user_parse_comand
          CFI Conditional ??CrossCallReturnLabel_42
          CFI R4 Frame(CFA, -36)
          CFI R5 Frame(CFA, -32)
          CFI R6 Frame(CFA, -28)
          CFI R7 Frame(CFA, -24)
          CFI R8 Frame(CFA, -20)
          CFI R9 Frame(CFA, -16)
          CFI R10 Frame(CFA, -12)
          CFI R11 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+64
          CFI Block cfiCond345 Using cfiCommon0
          CFI (cfiCond345) Function user_parse_comand
          CFI (cfiCond345) Conditional ??CrossCallReturnLabel_41
          CFI (cfiCond345) R4 Frame(CFA, -36)
          CFI (cfiCond345) R5 Frame(CFA, -32)
          CFI (cfiCond345) R6 Frame(CFA, -28)
          CFI (cfiCond345) R7 Frame(CFA, -24)
          CFI (cfiCond345) R8 Frame(CFA, -20)
          CFI (cfiCond345) R9 Frame(CFA, -16)
          CFI (cfiCond345) R10 Frame(CFA, -12)
          CFI (cfiCond345) R11 Frame(CFA, -8)
          CFI (cfiCond345) R14 Frame(CFA, -4)
          CFI (cfiCond345) CFA R13+64
          CFI Block cfiPicker346 Using cfiCommon1
          CFI (cfiPicker346) NoFunction
          CFI (cfiPicker346) Picker
        THUMB
?Subroutine18:
        MOV      R3,R6
        MOV      R2,R5
          CFI Block cfiCond347 Using cfiCommon0
          CFI (cfiCond347) Function user_parse_comand
          CFI (cfiCond347) Conditional ??CrossCallReturnLabel_40
          CFI (cfiCond347) R4 Frame(CFA, -36)
          CFI (cfiCond347) R5 Frame(CFA, -32)
          CFI (cfiCond347) R6 Frame(CFA, -28)
          CFI (cfiCond347) R7 Frame(CFA, -24)
          CFI (cfiCond347) R8 Frame(CFA, -20)
          CFI (cfiCond347) R9 Frame(CFA, -16)
          CFI (cfiCond347) R10 Frame(CFA, -12)
          CFI (cfiCond347) R11 Frame(CFA, -8)
          CFI (cfiCond347) R14 Frame(CFA, -4)
          CFI (cfiCond347) CFA R13+64
          CFI Block cfiCond348 Using cfiCommon0
          CFI (cfiCond348) Function user_parse_comand
          CFI (cfiCond348) Conditional ??CrossCallReturnLabel_39
          CFI (cfiCond348) R4 Frame(CFA, -36)
          CFI (cfiCond348) R5 Frame(CFA, -32)
          CFI (cfiCond348) R6 Frame(CFA, -28)
          CFI (cfiCond348) R7 Frame(CFA, -24)
          CFI (cfiCond348) R8 Frame(CFA, -20)
          CFI (cfiCond348) R9 Frame(CFA, -16)
          CFI (cfiCond348) R10 Frame(CFA, -12)
          CFI (cfiCond348) R11 Frame(CFA, -8)
          CFI (cfiCond348) R14 Frame(CFA, -4)
          CFI (cfiCond348) CFA R13+64
??Subroutine18_0:
        MOVS     R1,#+0
        LDRB     R0,[R7, #+4]
          CFI (cfiCond344) FunCall user_parse_comand user_send_answer_comand
          CFI (cfiCond344) FunCall user_parse_comand user_send_answer_comand
          CFI (cfiCond344) FunCall user_parse_comand user_send_answer_comand
          CFI (cfiCond344) FunCall user_parse_comand user_send_answer_comand
        B.W      user_send_answer_comand
          CFI EndBlock cfiCond344
          CFI EndBlock cfiCond345
          CFI EndBlock cfiPicker346
          CFI EndBlock cfiCond347
          CFI EndBlock cfiCond348

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock349 Using cfiCommon0
          CFI Function user_indication
        THUMB
user_indication:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI FunCall comm_server_online
        BL       comm_server_online
        CBZ.N    R0,??user_indication_0
        MOVW     R1,#+65535
        B.N      ??user_indication_1
??user_indication_0:
          CFI FunCall comm_modem_ready
        BL       comm_modem_ready
        CBZ.N    R0,??user_indication_2
        MOVS     R1,#+1
        B.N      ??user_indication_1
??user_indication_2:
        MOVS     R1,#+5
??user_indication_1:
        MOVS     R0,#+1
        POP      {R2,LR}
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall led_load
        B.W      led_load
          CFI EndBlock cfiBlock349

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock350 Using cfiCommon0
          CFI Function user_readinputs
        THUMB
user_readinputs:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
          CFI FunCall adc_startInjectedConvertion
        BL       adc_startInjectedConvertion
        CMP      R0,#+0
        BEQ.N    ??user_readinputs_0
        MOVW     R5,#+3001
        MOVS     R0,#+8
          CFI FunCall adc_getinjectedvalue_mV
        BL       adc_getinjectedvalue_mV
        CMP      R0,R5
        ITE      GE
        MOVGE    R2,#+8
        MOVLT    R2,#+0
        LDR.N    R4,??DataTable127
        LDRH     R1,[R4, #+129]
        MOVW     R0,#+65527
        ANDS     R1,R0,R1
        ORRS     R2,R2,R1
        STRH     R2,[R4, #+129]
        MOVS     R0,#+9
          CFI FunCall adc_getinjectedvalue_mV
        BL       adc_getinjectedvalue_mV
        CMP      R0,R5
        ITE      GE
        MOVGE    R2,#+4
        MOVLT    R2,#+0
        LDRH     R1,[R4, #+129]
        MOVW     R0,#+65531
        ANDS     R1,R0,R1
        ORRS     R2,R2,R1
        STRH     R2,[R4, #+129]
        LDRH     R0,[R4, #+131]
        UBFX     R0,R0,#+3,#+1
        UBFX     R1,R2,#+3,#+1
        CMP      R0,R1
        BEQ.N    ??user_readinputs_1
        UBFX     R0,R2,#+3,#+1
        CMP      R0,#+0
        ITE      NE
        MOVNE    R0,#+20
        MOVEQ    R0,#+21
          CFI FunCall log_addSystemEvent
        BL       log_addSystemEvent
        LDRH     R1,[R4, #+129]
        LSLS     R1,R1,#+24
        LSRS     R1,R1,#+27
        LDRH     R0,[R4, #+131]
        BFI      R0,R1,#+3,#+1
        STRH     R0,[R4, #+131]
??user_readinputs_1:
        LDRH     R0,[R4, #+131]
        UBFX     R0,R0,#+2,#+1
        LDRH     R1,[R4, #+129]
        UBFX     R1,R1,#+2,#+1
        CMP      R0,R1
        BEQ.N    ??user_readinputs_0
        LDRH     R0,[R4, #+129]
        UBFX     R0,R0,#+2,#+1
        CMP      R0,#+0
        ITE      NE
        MOVNE    R0,#+22
        MOVEQ    R0,#+23
          CFI FunCall log_addSystemEvent
        BL       log_addSystemEvent
        LDRH     R1,[R4, #+129]
        LSLS     R1,R1,#+24
        LSRS     R1,R1,#+26
        LDRH     R0,[R4, #+131]
        BFI      R0,R1,#+2,#+1
        STRH     R0,[R4, #+131]
??user_readinputs_0:
        POP      {R0,R4,R5,PC}
          CFI EndBlock cfiBlock350

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock351 Using cfiCommon0
          CFI Function user_last10digitsnumber
        THUMB
user_last10digitsnumber:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        MOV      R4,R0
          CFI FunCall strlen
        BL       strlen
        UXTH     R0,R0
        CMP      R0,#+11
        ITT      GE
        ADDGE    R0,R4,R0
        SUBGE    R4,R0,#+10
        MOV      R0,R4
        POP      {R4,PC}
          CFI EndBlock cfiBlock351

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock352 Using cfiCommon0
          CFI Function user_getweakday
          CFI NoCalls
        THUMB
user_getweakday:
        LDR.W    R1,??DataTable139
        UDIV     R0,R0,R1
        MOVS     R1,#+7
        UDIV     R1,R0,R1
        RSB      R1,R1,R1, LSL #+3
        SUBS     R0,R0,R1
        ADDS     R0,R0,#+3
        CMP      R0,#+7
        IT       GE
        SUBGE    R0,R0,#+7
        UXTB     R0,R0
        BX       LR
          CFI EndBlock cfiBlock352

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable123:
        DATA32
        DC32     table_chunk

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable123_1:
        DATA32
        DC32     mem

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock353 Using cfiCommon0
          CFI Function user_findID
        THUMB
user_findID:
        PUSH     {R3-R9,LR}
          CFI R14 Frame(CFA, -4)
          CFI R9 Frame(CFA, -8)
          CFI R8 Frame(CFA, -12)
          CFI R7 Frame(CFA, -16)
          CFI R6 Frame(CFA, -20)
          CFI R5 Frame(CFA, -24)
          CFI R4 Frame(CFA, -28)
          CFI CFA R13+32
        SUB      SP,SP,#+1024
          CFI CFA R13+1056
        MOV      R9,R1
        MOVS     R1,#+0
        MOVS     R2,#+0
        MOV      R4,R0
??user_findID_0:
        UXTB     R0,R2
        LDRB     R0,[R4, R0]
        ADDS     R1,R0,R1
        ADDS     R2,R2,#+1
        UXTB     R0,R2
        CMP      R0,#+10
        BLT.N    ??user_findID_0
        MOVW     R0,#+2550
        UXTH     R2,R1
        CMP      R2,R0
        ITT      NE
        UXTHNE   R1,R1
        CMPNE    R1,#+0
        BEQ.N    ??user_findID_1
        LDR.W    R7,??DataTable138
        MOV      R2,SP
        MOV      R1,R4
        LDR      R0,[R7, #+16]
          CFI FunCall idindex_find
        BL       idindex_find
        MOVS     R6,R0
        BMI.N    ??user_findID_2
        BEQ.N    ??user_findID_1
        MOVS     R2,#+32
        MOV      R1,SP
        MOV      R0,R9
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        CMP      R6,#+1
        BNE.N    ??user_findID_1
        B.N      ??user_findID_3
??user_findID_2:
        LDR      R1,[R7, #+16]
        LDR.N    R0,??DataTable125
        LDRH     R0,[R0, #+0]
        ADD      R8,R1,R0, LSL #+5
        LDR      R6,[R7, #+16]
        B.N      ??user_findID_4
??user_findID_5:
        ADDS     R7,R7,#+32
        UXTH     R7,R7
        CMP      R7,#+1024
        BLT.N    ??user_findID_6
??user_findID_7:
        ADD      R6,R6,#+1024
??user_findID_4:
        CMP      R6,R8
        BCS.N    ??user_findID_1
        BL       ?Subroutine33
??CrossCallReturnLabel_111:
        CBZ.N    R0,??user_findID_1
        MOVS     R7,#+0
??user_findID_6:
        ADDS     R0,R7,R6
        CMP      R0,R8
        BCS.N    ??user_findID_7
        ADD      R5,SP,R7
        LDRH     R1,[R5, #+0]
        BL       ?Subroutine62
??CrossCallReturnLabel_216:
        BEQ.N    ??user_findID_5
        MOVS     R2,#+10
        MOV      R1,R4
        ADDS     R0,R5,#+2
          CFI FunCall memcmp
        BL       memcmp
        CMP      R0,#+0
        BNE.N    ??user_findID_5
        MOVS     R2,#+32
        MOV      R1,R5
        MOV      R0,R9
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
??user_findID_3:
        MOVS     R0,#+1
        B.N      ??user_findID_8
??user_findID_1:
        MOVS     R0,#+0
??user_findID_8:
        B.W      ?Subroutine3
          CFI EndBlock cfiBlock353

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock354 Using cfiCommon0
          CFI Function user_findTempID
        THUMB
user_findTempID:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        SUB      SP,SP,#+32
          CFI CFA R13+48
        MOV      R4,R0
        LDR.N    R0,??DataTable127
        LDR      R0,[R0, #+85]
        LDR.W    R1,??DataTable138
        LDR      R5,[R1, #+64]
        CMP      R5,R0
        IT       CC
        MOVCC    R5,R0
        LDR      R6,[R1, #+64]
        B.N      ??user_findTempID_0
??user_findTempID_1:
        ADDS     R6,R6,#+32
??user_findTempID_0:
        CMP      R6,R5
        BCS.N    ??user_findTempID_2
        MOVS     R2,#+32
        BL       ??Subroutine33_0
??CrossCallReturnLabel_109:
        CBZ.N    R0,??user_findTempID_2
        MOVS     R2,#+10
        MOV      R1,R4
        ADD      R0,SP,#+2
          CFI FunCall memcmp
        BL       memcmp
        CMP      R0,#+0
        BNE.N    ??user_findTempID_1
        MOVS     R0,#+1
        B.N      ??user_findTempID_3
??user_findTempID_2:
        MOVS     R0,#+0
??user_findTempID_3:
        ADD      SP,SP,#+32
          CFI CFA R13+16
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock354

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable125:
        DATA32
        DC32     id_cells_end

        SECTION `.text`:CODE:NOROOT(2)
          CFI Block cfiBlock355 Using cfiCommon0
          CFI Function user_addIdToTempBuffer
        THUMB
user_addIdToTempBuffer:
        PUSH     {R3-R9,LR}
          CFI R14 Frame(CFA, -4)
          CFI R9 Frame(CFA, -8)
          CFI R8 Frame(CFA, -12)
          CFI R7 Frame(CFA, -16)
          CFI R6 Frame(CFA, -20)
          CFI R5 Frame(CFA, -24)
          CFI R4 Frame(CFA, -28)
          CFI CFA R13+32
        SUB      SP,SP,#+32
          CFI CFA R13+64
        MOV      R9,R0
        MOV      R6,R1
        LDR.W    R0,??DataTable138
        LDR      R1,[R0, #+64]
        LDR      R5,[R0, #+68]
        ADDS     R5,R5,R1
        LDR.W    R8,??DataTable139_1
        LDR      R1,[R8, #+85]
        LDR      R4,[R0, #+64]
        CMP      R4,R1
        IT       CC
        MOVCC    R4,R1
        LDR      R7,[R0, #+64]
        B.N      ??user_addIdToTempBuffer_1
??user_addIdToTempBuffer_2:
        MOVS     R2,#+10
        MOV      R1,R9
        ADD      R0,SP,#+2
          CFI FunCall memcmp
        BL       memcmp
        CBZ.N    R0,??user_addIdToTempBuffer_3
        ADDS     R7,R7,#+32
??user_addIdToTempBuffer_1:
        CMP      R7,R4
        BCS.N    ??user_addIdToTempBuffer_4
        MOVS     R2,#+32
        MOV      R1,SP
        MOV      R0,R7
          CFI FunCall mem_read
        BL       mem_read
        CMP      R0,#+0
        BNE.N    ??user_addIdToTempBuffer_2
??user_addIdToTempBuffer_3:
        ADD      SP,SP,#+36
          CFI CFA R13+28
        POP      {R4-R9,PC}
          CFI CFA R13+64
??user_addIdToTempBuffer_5:
        LDRH     R1,[SP, #+0]
        BL       ?Subroutine62
??CrossCallReturnLabel_217:
        BEQ.N    ??user_addIdToTempBuffer_6
        ADDS     R4,R4,#+32
??user_addIdToTempBuffer_4:
        CMP      R4,R5
        BCS.N    ??user_addIdToTempBuffer_3
        MOVS     R2,#+32
        MOV      R1,SP
        MOV      R0,R4
          CFI FunCall mem_read
        BL       mem_read
        CMP      R0,#+0
        BNE.N    ??user_addIdToTempBuffer_5
        B.N      ??user_addIdToTempBuffer_3
??user_addIdToTempBuffer_6:
        BL       ?Subroutine31
??CrossCallReturnLabel_96:
        MOVS     R2,#+10
        MOV      R1,R9
        ADD      R0,SP,#+2
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        MOVS     R0,#+0
        STRB     R0,[SP, #+12]
        STR      R0,[SP, #+13]
        SUBS     R6,R6,#+1
        CMP      R6,#+4
        BHI.N    ??user_addIdToTempBuffer_7
        TBB      [PC, R6]
        DATA
??user_addIdToTempBuffer_0:
        DC8      0x3,0x17,0x12,0x8
        DC8      0xD,0x0
        THUMB
??user_addIdToTempBuffer_8:
        LDRB     R0,[SP, #+19]
        ORR      R0,R0,#0x1
        B.N      ??user_addIdToTempBuffer_9
??user_addIdToTempBuffer_10:
        LDRB     R0,[SP, #+19]
        ORR      R0,R0,#0x2
        B.N      ??user_addIdToTempBuffer_9
??user_addIdToTempBuffer_11:
        LDRB     R0,[SP, #+19]
        ORR      R0,R0,#0x4
        B.N      ??user_addIdToTempBuffer_9
??user_addIdToTempBuffer_12:
        LDRB     R0,[SP, #+19]
        ORR      R0,R0,#0x8
        B.N      ??user_addIdToTempBuffer_9
??user_addIdToTempBuffer_13:
        LDRB     R0,[SP, #+19]
        ORR      R0,R0,#0x10
??user_addIdToTempBuffer_9:
        STRB     R0,[SP, #+19]
??user_addIdToTempBuffer_7:
        MOVS     R2,#+32
        MOV      R1,SP
        MOV      R0,R4
          CFI FunCall mem_write
        BL       mem_write
        CMP      R0,#+0
        BEQ.N    ??user_addIdToTempBuffer_3
        ADDS     R4,R4,#+32
        STR      R4,[R8, #+85]
        LDRH     R0,[R8, #+129]
        ORR      R0,R0,#0x4000
        STRH     R0,[R8, #+129]
        B.N      ??user_addIdToTempBuffer_3
          CFI EndBlock cfiBlock355

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable126:
        DATA32
        DC32     id_used_bytes

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond356 Using cfiCommon0
          CFI Function table_chunk
          CFI Conditional ??CrossCallReturnLabel_215
          CFI R4 Frame(CFA, -36)
          CFI R5 Frame(CFA, -32)
          CFI R6 Frame(CFA, -28)
          CFI R7 Frame(CFA, -24)
          CFI R8 Frame(CFA, -20)
          CFI R9 Frame(CFA, -16)
          CFI R10 Frame(CFA, -12)
          CFI R11 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+40
          CFI Block cfiCond357 Using cfiCommon0
          CFI (cfiCond357) Function user_findID
          CFI (cfiCond357) Conditional ??CrossCallReturnLabel_216
          CFI (cfiCond357) R4 Frame(CFA, -28)
          CFI (cfiCond357) R5 Frame(CFA, -24)
          CFI (cfiCond357) R6 Frame(CFA, -20)
          CFI (cfiCond357) R7 Frame(CFA, -16)
          CFI (cfiCond357) R8 Frame(CFA, -12)
          CFI (cfiCond357) R9 Frame(CFA, -8)
          CFI (cfiCond357) R14 Frame(CFA, -4)
          CFI (cfiCond357) CFA R13+1056
          CFI Block cfiCond358 Using cfiCommon0
          CFI (cfiCond358) Function user_addIdToTempBuffer
          CFI (cfiCond358) Conditional ??CrossCallReturnLabel_217
          CFI (cfiCond358) R4 Frame(CFA, -28)
          CFI (cfiCond358) R5 Frame(CFA, -24)
          CFI (cfiCond358) R6 Frame(CFA, -20)
          CFI (cfiCond358) R7 Frame(CFA, -16)
          CFI (cfiCond358) R8 Frame(CFA, -12)
          CFI (cfiCond358) R9 Frame(CFA, -8)
          CFI (cfiCond358) R14 Frame(CFA, -4)
          CFI (cfiCond358) CFA R13+64
          CFI Block cfiCond359 Using cfiCommon0
          CFI (cfiCond359) Function user_checkIdTimeEnd
          CFI (cfiCond359) Conditional ??CrossCallReturnLabel_218
          CFI (cfiCond359) R4 Frame(CFA, -28)
          CFI (cfiCond359) R5 Frame(CFA, -24)
          CFI (cfiCond359) R6 Frame(CFA, -20)
          CFI (cfiCond359) R7 Frame(CFA, -16)
          CFI (cfiCond359) R8 Frame(CFA, -12)
          CFI (cfiCond359) R9 Frame(CFA, -8)
          CFI (cfiCond359) R14 Frame(CFA, -4)
          CFI (cfiCond359) CFA R13+1056
          CFI Block cfiPicker360 Using cfiCommon1
          CFI (cfiPicker360) NoFunction
          CFI (cfiPicker360) Picker
        THUMB
?Subroutine62:
        MOVW     R0,#+65535
        CMP      R1,R0
        BX       LR
          CFI EndBlock cfiCond356
          CFI EndBlock cfiCond357
          CFI EndBlock cfiCond358
          CFI EndBlock cfiCond359
          CFI EndBlock cfiPicker360

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock361 Using cfiCommon0
          CFI Function shedule_time_ok
          CFI NoCalls
        THUMB
shedule_time_ok:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        MOVS     R1,#+60
        LDR.W    R2,??DataTable139_1
        LDRB     R3,[R2, #+56]
        LDRB     R2,[R2, #+57]
        MLA      R2,R1,R3,R2
        LDRB     R4,[R0, #+1]
        LDRB     R3,[R0, #+2]
        MLA      R3,R1,R4,R3
        LDRB     R4,[R0, #+3]
        LDRB     R0,[R0, #+4]
        MLA      R0,R1,R4,R0
        CMP      R0,R3
        BLT.N    ??shedule_time_ok_0
        CMP      R2,R3
        BGE.N    ??shedule_time_ok_1
??shedule_time_ok_2:
        MOVS     R0,#+0
        POP      {R4,PC}
??shedule_time_ok_0:
        CMP      R2,R3
        BGE.N    ??shedule_time_ok_3
??shedule_time_ok_1:
        CMP      R0,R2
        BLT.N    ??shedule_time_ok_2
??shedule_time_ok_3:
        MOVS     R0,#+1
        POP      {R4,PC}
          CFI EndBlock cfiBlock361

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable127:
        DATA32
        DC32     device

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable127_1:
        DATA32
        DC32     store_runtime

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable127_2:
        DATA32
        DC32     ?_82

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable127_3:
        DATA32
        DC32     ?_79

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable127_4:
        DATA32
        DC32     ?_83

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable127_5:
        DATA32
        DC32     ?_77

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable127_6:
        DATA32
        DC32     ?_84

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable127_7:
        DATA32
        DC32     ?_85

        SECTION `.text`:CODE:NOROOT(2)
          CFI Block cfiBlock362 Using cfiCommon0
          CFI Function user_checkIdAndControl
        THUMB
user_checkIdAndControl:
        PUSH     {R3-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+20
        SUB      SP,SP,#+84
          CFI CFA R13+104
        MOV      R6,R0
        MOV      R4,R1
          CFI FunCall mem_tables_ready
        BL       mem_tables_ready
        CMP      R0,#+0
        BEQ.W    ??user_checkIdAndControl_3
        SUBS     R0,R4,#+1
        CMP      R0,#+5
        BHI.N    ??user_checkIdAndControl_4
        TBB      [PC, R0]
        DATA
??user_checkIdAndControl_0:
        DC8      0x3,0x6,0x9,0xC
        DC8      0xF,0x12
        THUMB
??user_checkIdAndControl_5:
        LDR.W    R0,??DataTable142
        B.N      ??user_checkIdAndControl_6
??user_checkIdAndControl_7:
        LDR.W    R0,??DataTable142_1
        B.N      ??user_checkIdAndControl_6
??user_checkIdAndControl_8:
        LDR.W    R0,??DataTable142_2
        B.N      ??user_checkIdAndControl_6
??user_checkIdAndControl_9:
        LDR.W    R0,??DataTable142_3
        B.N      ??user_checkIdAndControl_6
??user_checkIdAndControl_10:
        LDR.W    R0,??DataTable142_4
        B.N      ??user_checkIdAndControl_6
??user_checkIdAndControl_11:
        LDR.W    R0,??DataTable142_5
??user_checkIdAndControl_6:
          CFI FunCall user_debug
        BL       user_debug
??user_checkIdAndControl_4:
        LDR.W    R5,??DataTable139_1
        LDRH     R0,[R5, #+129]
        UBFX     R0,R0,#+7,#+1
        CBZ.N    R0,??user_checkIdAndControl_12
        LDR.W    R0,??DataTable142_6
        B.N      ??user_checkIdAndControl_13
??user_checkIdAndControl_12:
        LDRH     R0,[R5, #+129]
        UBFX     R0,R0,#+6,#+1
        CBZ.N    R0,??user_checkIdAndControl_14
        LDR.W    R0,??DataTable142_7
        B.N      ??user_checkIdAndControl_13
??user_checkIdAndControl_14:
        STRH     R0,[SP, #+0]
        MOVS     R1,#+0
        LDR.W    R3,??user_activateOut_1
??user_checkIdAndControl_15:
        MOVS     R2,#+11
        MUL      R0,R2,R1
        ADD      R0,R3,R0
        LDRB     R0,[R0, #+195]
        CMP      R0,#+0
        BNE.N    ??user_checkIdAndControl_16
        SUBS     R0,R4,#+1
        CMP      R0,#+5
        BHI.N    ??user_checkIdAndControl_17
        TBB      [PC, R0]
        DATA
??user_checkIdAndControl_1:
        DC8      0x12,0x3,0x8,0xD
        DC8      0x16,0x1B
        THUMB
??user_checkIdAndControl_18:
        MULS     R2,R2,R1
        ADDS     R0,R3,R2
        LDRB     R0,[R0, #+198]
        B.N      ??user_checkIdAndControl_19
??user_checkIdAndControl_20:
        MULS     R2,R2,R1
        ADDS     R0,R3,R2
        LDRB     R0,[R0, #+202]
        B.N      ??user_checkIdAndControl_19
??user_checkIdAndControl_21:
        MULS     R2,R2,R1
        ADDS     R0,R3,R2
        LDRB     R0,[R0, #+200]
??user_checkIdAndControl_19:
        CBZ.N    R0,??user_checkIdAndControl_16
??user_checkIdAndControl_22:
        MOVS     R2,#+1
        STRB     R2,[SP, R1]
        B.N      ??user_checkIdAndControl_16
??user_checkIdAndControl_23:
        MULS     R2,R2,R1
        ADDS     R0,R3,R2
        LDRB     R0,[R0, #+199]
        B.N      ??user_checkIdAndControl_24
??user_checkIdAndControl_25:
        MULS     R2,R2,R1
        ADDS     R0,R3,R2
        LDRB     R0,[R0, #+197]
        B.N      ??user_checkIdAndControl_24
??user_checkIdAndControl_17:
        MULS     R2,R2,R1
        ADDS     R0,R3,R2
        LDRB     R0,[R0, #+201]
??user_checkIdAndControl_24:
        CBZ.N    R0,??user_checkIdAndControl_16
        MOVS     R0,#+1
        STRB     R0,[SP, R1]
??user_checkIdAndControl_16:
        ADDS     R1,R1,#+1
        CMP      R1,#+2
        BLT.N    ??user_checkIdAndControl_15
        MOV      R0,R6
          CFI FunCall user_findTempID
        BL       user_findTempID
        CBZ.N    R0,??user_checkIdAndControl_26
        LDR.W    R0,??DataTable143
        B.N      ??user_checkIdAndControl_27
??user_checkIdAndControl_26:
        LDRH     R0,[R5, #+129]
        UBFX     R0,R0,#+5,#+1
        CBZ.N    R0,??user_checkIdAndControl_28
        MOV      R1,R4
        MOV      R0,R6
          CFI FunCall user_addIdToTempBuffer
        BL       user_addIdToTempBuffer
        MOVS     R2,#+0
        MOV      R1,R6
        MOVS     R0,#+68
          CFI FunCall log_addControlEvent
        BL       log_addControlEvent
??user_checkIdAndControl_28:
        LDRH     R0,[R5, #+129]
        UBFX     R0,R0,#+4,#+1
        CBZ.N    R0,??user_checkIdAndControl_29
        LDR.W    R0,??DataTable144
        B.N      ??user_checkIdAndControl_27
??user_checkIdAndControl_29:
        LDRB     R0,[R5, #+102]
        CBZ.N    R0,??user_checkIdAndControl_30
        LDR.W    R0,??DataTable144_1
          CFI FunCall user_debug
        BL       user_debug
        LDRB     R0,[R5, #+102]
          CFI FunCall user_checkShedule
        BL       user_checkShedule
        CMP      R0,#+1
        BNE.N    ??user_checkIdAndControl_31
        B.N      ??user_checkIdAndControl_32
??user_checkIdAndControl_30:
        LDRB     R0,[SP, #+0]
        CMP      R0,#+0
        ITT      EQ
        LDRBEQ   R0,[SP, #+1]
        CMPEQ    R0,#+0
        BEQ.W    ??user_checkIdAndControl_33
        ADD      R1,SP,#+52
        MOV      R0,R6
          CFI FunCall user_findID
        BL       user_findID
        CMP      R0,#+0
        BEQ.W    ??user_checkIdAndControl_34
        LDR      R0,[SP, #+65]
        CBZ.N    R0,??user_checkIdAndControl_35
        MOV      R1,R0
        LDR      R0,[R5, #+47]
        CMP      R1,R0
        IT       CC
        LDRCC.W  R0,??DataTable145
        BCC.N    ??user_checkIdAndControl_36
??user_checkIdAndControl_35:
        ADD      R0,SP,#+52
          CFI FunCall id_expired
        BL       id_expired
        CBZ.N    R0,??user_checkIdAndControl_37
        LDR.W    R0,??DataTable146
??user_checkIdAndControl_36:
        B.N      ??user_checkIdAndControl_13
??user_checkIdAndControl_37:
        ADD      R1,SP,#+4
        LDRB     R0,[SP, #+64]
          CFI FunCall user_getGroup
        BL       user_getGroup
        CBNZ.N   R0,??user_checkIdAndControl_38
        LDR.W    R0,??DataTable146_1
        B.N      ??user_checkIdAndControl_13
??user_checkIdAndControl_38:
        LDRB     R0,[SP, #+6]
        CBZ.N    R0,??user_checkIdAndControl_39
        LDR.W    R0,??DataTable147
        B.N      ??user_checkIdAndControl_40
??user_checkIdAndControl_39:
        SUBS     R0,R4,#+1
        CMP      R0,#+4
        BHI.N    ??user_checkIdAndControl_41
        TBB      [PC, R0]
        DATA
??user_checkIdAndControl_2:
        DC8      0x3,0x6,0x9,0xC
        DC8      0x10,0x0
        THUMB
??user_checkIdAndControl_42:
        LDRB     R0,[SP, #+7]
        B.N      ??user_checkIdAndControl_43
??user_checkIdAndControl_44:
        LDRB     R0,[SP, #+10]
        B.N      ??user_checkIdAndControl_43
??user_checkIdAndControl_45:
        LDRB     R0,[SP, #+8]
        B.N      ??user_checkIdAndControl_43
??user_checkIdAndControl_46:
        LDRB     R0,[SP, #+9]
??user_checkIdAndControl_43:
        CBNZ.N   R0,??user_checkIdAndControl_41
??user_checkIdAndControl_31:
        B.N      ??user_checkIdAndControl_3
??user_checkIdAndControl_47:
        LDRB     R0,[SP, #+11]
        CMP      R0,#+0
        BEQ.N    ??user_checkIdAndControl_3
??user_checkIdAndControl_41:
        LDRB     R0,[SP, #+5]
        CBNZ.N   R0,??user_checkIdAndControl_48
        LDR.W    R0,??DataTable149
??user_checkIdAndControl_27:
          CFI FunCall user_debug
        BL       user_debug
        B.N      ??user_checkIdAndControl_32
??user_checkIdAndControl_48:
        ADD      R1,SP,#+20
          CFI FunCall user_getShedule
        BL       user_getShedule
        CBNZ.N   R0,??user_checkIdAndControl_49
        LDR.W    R0,??DataTable149_1
        B.N      ??user_checkIdAndControl_13
??user_checkIdAndControl_49:
        LDRB     R0,[SP, #+25]
        CBZ.N    R0,??user_checkIdAndControl_50
        LDR.W    R0,??DataTable149_2
??user_checkIdAndControl_40:
          CFI FunCall user_debug
        BL       user_debug
        MOVS     R2,#+0
        MOV      R1,R6
        MOVS     R0,#+66
          CFI FunCall log_addControlEvent
        BL       log_addControlEvent
        B.N      ??user_checkIdAndControl_3
??user_checkIdAndControl_50:
        ADD      R0,SP,#+20
        LDRB     R1,[R5, #+55]
        ADD      R0,R0,R1
        LDRB     R0,[R0, #+6]
        CBNZ.N   R0,??user_checkIdAndControl_51
        LDR.W    R0,??DataTable149_3
        B.N      ??user_checkIdAndControl_13
??user_checkIdAndControl_51:
        ADD      R0,SP,#+20
          CFI FunCall shedule_time_ok
        BL       shedule_time_ok
        CBNZ.N   R0,??user_checkIdAndControl_52
        LDR.W    R0,??DataTable149_4
        B.N      ??user_checkIdAndControl_13
??user_checkIdAndControl_52:
        MOVS     R0,#+0
        LDRB     R1,[SP, #+41]
        CMP      R1,#+1
        BEQ.N    ??user_checkIdAndControl_32
        BCC.N    ??user_checkIdAndControl_53
        CMP      R1,#+3
        BEQ.N    ??user_checkIdAndControl_54
          CFI FunCall user_dayIsHoliday
        IT       CC
        BLCC     user_dayIsHoliday
??user_checkIdAndControl_53:
        CBNZ.N   R0,??user_checkIdAndControl_32
??user_checkIdAndControl_55:
        LDR.W    R0,??DataTable149_5
        B.N      ??user_checkIdAndControl_13
??user_checkIdAndControl_54:
          CFI FunCall user_dayIsHoliday
        BL       user_dayIsHoliday
        CMP      R0,#+0
        BNE.N    ??user_checkIdAndControl_55
??user_checkIdAndControl_32:
        MOV      R3,R6
        MOV      R2,R4
        LDRB     R1,[SP, #+1]
        LDRB     R0,[SP, #+0]
          CFI FunCall user_activateOut
        BL       user_activateOut
        B.N      ??user_checkIdAndControl_3
??user_checkIdAndControl_34:
        LDR.W    R0,??DataTable150
          CFI FunCall user_debug
        BL       user_debug
        MOVS     R2,#+0
        MOV      R1,R6
        MOVS     R0,#+65
          CFI FunCall log_addControlEvent
        BL       log_addControlEvent
        LDRB     R0,[R5, #+100]
        CBZ.N    R0,??user_checkIdAndControl_3
        MOV      R0,R6
          CFI FunCall unreg_sms_note
        BL       unreg_sms_note
        B.N      ??user_checkIdAndControl_3
??user_checkIdAndControl_33:
        LDR.W    R0,??DataTable151
??user_checkIdAndControl_13:
          CFI FunCall user_debug
        BL       user_debug
??user_checkIdAndControl_3:
        ADD      SP,SP,#+88
          CFI CFA R13+16
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock362

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable128:
        DATA32
        DC32     ?_86

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable128_1:
        DATA32
        DC32     ?_87

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable128_2:
        DATA32
        DC32     _A_tempStrBuffer

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable128_3:
        DATA32
        DC32     server_out_event

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable128_4:
        DATA32
        DC32     ?_88

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable128_5:
        DATA32
        DC32     ?_89

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable128_6:
        DATA32
        DC32     ?_90

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable128_7:
        DATA32
        DC32     config+137

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable128_8:
        DATA32
        DC32     ?_91

        SECTION `.text`:CODE:NOROOT(2)
          CFI Block cfiBlock363 Using cfiCommon0
          CFI Function user_activateOut
        THUMB
user_activateOut:
        PUSH     {R3-R9,LR}
          CFI R14 Frame(CFA, -4)
          CFI R9 Frame(CFA, -8)
          CFI R8 Frame(CFA, -12)
          CFI R7 Frame(CFA, -16)
          CFI R6 Frame(CFA, -20)
          CFI R5 Frame(CFA, -24)
          CFI R4 Frame(CFA, -28)
          CFI CFA R13+32
        SUB      SP,SP,#+40
          CFI CFA R13+72
        MOV      R4,R1
        MOV      R8,R2
        MOV      R9,R3
        CBZ.N    R0,??user_activateOut_2
        LDR.N    R0,??user_activateOut_1
        LDRB     R0,[R0, #+195]
        CBNZ.N   R0,??user_activateOut_2
          CFI FunCall out_getmodestate
        BL       out_getmodestate
        SUBS     R0,R0,#+1
        SBCS     R0,R0,R0
        LSRS     R0,R0,#+31
        B.N      ??user_activateOut_3
??user_activateOut_2:
        MOVS     R0,#+0
??user_activateOut_3:
        STRB     R0,[SP, #+4]
        CBZ.N    R4,??user_activateOut_4
        LDR.N    R0,??user_activateOut_1
        LDRB     R0,[R0, #+206]
        CBNZ.N   R0,??user_activateOut_4
        MOVS     R0,#+1
          CFI FunCall out_getmodestate
        BL       out_getmodestate
        SUBS     R0,R0,#+1
        SBCS     R0,R0,R0
        LSRS     R0,R0,#+31
        B.N      ??user_activateOut_5
??user_activateOut_4:
        MOVS     R0,#+0
??user_activateOut_5:
        ADD      R5,SP,#+4
        STRB     R0,[R5, #+1]
        LDRB     R0,[SP, #+4]
        CMP      R0,#+0
        ITT      EQ
        LDRBEQ   R0,[R5, #+1]
        CMPEQ    R0,#+0
        BNE.N    ??user_activateOut_6
        LDR.W    R0,??DataTable151_1
          CFI FunCall user_debug
        BL       user_debug
        B.N      ??user_activateOut_7
??user_activateOut_6:
        MOVS     R6,#+255
        MOVS     R7,#+0
        CMP      R9,#+0
        ITT      NE
        LDRBNE   R0,[R9, #+0]
        CMPNE    R0,#+0
        BEQ.N    ??user_activateOut_8
        ADD      R1,SP,#+8
        MOV      R0,R9
          CFI FunCall user_findID
        BL       user_findID
        CMP      R0,#+0
        BEQ.N    ??user_activateOut_8
        MOVS     R4,#+0
        LDRB     R0,[SP, #+28]
        CMP      R0,#+255
        BEQ.N    ??user_activateOut_9
        ADD      R0,SP,#+8
          CFI FunCall id_new_day
        BL       id_new_day
        CBZ.N    R0,??user_activateOut_10
        MOVS     R4,#+1
??user_activateOut_10:
        LDRB     R0,[SP, #+29]
        CBZ.N    R0,??user_activateOut_11
        SUBS     R0,R0,#+1
        STRB     R0,[SP, #+29]
        LDRB     R6,[SP, #+29]
        MOVS     R4,#+1
??user_activateOut_9:
        LDRH     R0,[SP, #+25]
        CBZ.N    R0,??user_activateOut_12
        ADD      R0,SP,#+8
          CFI FunCall id_first_pass
        BL       id_first_pass
        CBNZ.N   R0,??user_activateOut_12
        LDR.W    R2,??DataTable139_1
        LDR      R0,[R2, #+47]
        LDR.W    R1,??DataTable151_2
        CMP      R0,R1
        BCC.N    ??user_activateOut_12
        MOV      R1,R0
        ADD      R0,SP,#+8
          CFI FunCall id_set_first_pass
        BL       id_set_first_pass
        ADD      R0,SP,#+8
          CFI FunCall id_time_end
        BL       id_time_end
          CFI FunCall id_watch_end
        BL       id_watch_end
        MOVS     R7,#+1
        B.N      ??user_activateOut_13
??user_activateOut_11:
        CBZ.N    R4,??user_activateOut_7
        ADD      R0,SP,#+8
          CFI FunCall user_updateID
        BL       user_updateID
        B.N      ??user_activateOut_7
??user_activateOut_12:
        CBZ.N    R4,??user_activateOut_8
??user_activateOut_13:
        ADD      R0,SP,#+8
          CFI FunCall user_updateID
        BL       user_updateID
        CBNZ.N   R0,??user_activateOut_8
??user_activateOut_7:
        MOVS     R0,#+0
        B.N      ??user_activateOut_14
??user_activateOut_8:
        MOVS     R4,#+0
??user_activateOut_15:
        LDRB     R0,[R5, R4]
        CBZ.N    R0,??user_activateOut_16
        CMP      R4,#+0
        ITE      EQ
        LDREQ.W  R0,??DataTable151_3
        LDRNE.W  R0,??DataTable151_4
          CFI FunCall user_debug
        BL       user_debug
        MOVS     R1,#+11
        LDR.N    R2,??user_activateOut_1
        MUL      R0,R1,R4
        ADD      R0,R2,R0
        LDRB     R0,[R0, #+196]
        CBNZ.N   R0,??user_activateOut_17
        MUL      R0,R1,R4
        ADD      R0,R2,R0
        LDRB     R0,[R0, #+205]
        STR      R0,[SP, #+0]
        MUL      R0,R1,R4
        ADD      R0,R2,R0
        LDRB     R3,[R0, #+204]
        LDRB     R2,[R0, #+203]
        MOVS     R1,#+3
        B.N      ??user_activateOut_18
        Nop
        DATA
??user_activateOut_1:
        DATA32
        DC32     config
        THUMB
??user_activateOut_17:
        CMP      R8,#+6
        ITE      EQ
        MOVEQ    R1,#+1
        MOVNE    R1,#+2
        MOVS     R0,#+0
        STR      R0,[SP, #+0]
        MOVS     R3,#+0
        MOVS     R2,#+0
??user_activateOut_18:
        UXTB     R0,R4
          CFI FunCall out_load
        BL       out_load
??user_activateOut_16:
        ADDS     R4,R4,#+1
        CMP      R4,#+2
        BLT.N    ??user_activateOut_15
        SUB      R0,R8,#+1
        CMP      R0,#+5
        BHI.N    ??user_activateOut_19
        TBB      [PC, R0]
        DATA
??user_activateOut_0:
        DC8      0x3,0x7,0xB,0xF
        DC8      0x13,0x17
        THUMB
??user_activateOut_20:
        MOV      R2,R6
        MOV      R1,R9
        MOVS     R0,#+69
        B.N      ??user_activateOut_21
??user_activateOut_22:
        MOV      R2,R6
        MOV      R1,R9
        MOVS     R0,#+71
        B.N      ??user_activateOut_21
??user_activateOut_23:
        MOV      R2,R6
        MOV      R1,R9
        MOVS     R0,#+72
        B.N      ??user_activateOut_21
??user_activateOut_24:
        MOV      R2,R6
        MOV      R1,R9
        MOVS     R0,#+70
        B.N      ??user_activateOut_21
??user_activateOut_25:
        MOV      R2,R6
        MOV      R1,R9
        MOVS     R0,#+73
        B.N      ??user_activateOut_21
??user_activateOut_26:
        MOV      R2,R6
        MOV      R1,R9
        LDR.W    R0,??DataTable154
        LDRB     R0,[R0, #+0]
??user_activateOut_21:
          CFI FunCall log_addControlEvent
        BL       log_addControlEvent
??user_activateOut_19:
        CMP      R6,#+255
        BEQ.N    ??user_activateOut_27
        MOV      R2,R6
        MOV      R1,R9
        MOVS     R0,#+131
          CFI FunCall log_addControlEvent
        BL       log_addControlEvent
        CBNZ.N   R6,??user_activateOut_27
        MOVS     R2,#+0
        MOV      R1,R9
        MOVS     R0,#+79
          CFI FunCall log_addControlEvent
        BL       log_addControlEvent
??user_activateOut_27:
        CBZ.N    R7,??user_activateOut_28
        MOVS     R2,#+255
        MOV      R1,R9
        MOVS     R0,#+139
          CFI FunCall log_addControlEvent
        BL       log_addControlEvent
??user_activateOut_28:
        MOVS     R0,#+1
??user_activateOut_14:
        ADD      SP,SP,#+44
          CFI CFA R13+28
        POP      {R4-R9,PC}
          CFI EndBlock cfiBlock363

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable129:
        DATA32
        DC32     store

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock364 Using cfiCommon0
          CFI Function user_getGroup
        THUMB
user_getGroup:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        SUB      SP,SP,#+1024
          CFI CFA R13+1048
        MOV      R4,R0
        MOV      R5,R1
        LDR.W    R1,??DataTable138
        LDR      R0,[R1, #+24]
        LDR      R6,[R1, #+28]
        ADDS     R6,R6,R0
        MOV      R7,R0
        B.N      ??user_getGroup_0
??user_getGroup_1:
        ADDS     R2,R2,#+16
        CMP      R2,#+1024
        BLT.N    ??user_getGroup_2
        ADD      R7,R7,#+1024
??user_getGroup_0:
        CMP      R7,R6
        BCS.N    ??user_getGroup_3
        BL       ?Subroutine50
??CrossCallReturnLabel_180:
        MOVS     R2,#+0
??user_getGroup_2:
        ADD      R1,SP,R2
        LDRB     R3,[R1, #+0]
        CMP      R3,R4
        BNE.N    ??user_getGroup_1
        MOVS     R2,#+16
        MOV      R0,R5
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        MOVS     R0,#+1
        B.N      ??user_getGroup_4
??user_getGroup_3:
        MOVS     R0,#+0
??user_getGroup_4:
          CFI EndBlock cfiBlock364
        REQUIRE ?Subroutine2

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock365 Using cfiCommon0
          CFI NoFunction
          CFI CFA R13+1048
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
        THUMB
?Subroutine2:
        ADD      SP,SP,#+1024
          CFI CFA R13+24
        ADD      SP,SP,#+4
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI EndBlock cfiBlock365

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock366 Using cfiCommon0
          CFI Function user_getShedule
        THUMB
user_getShedule:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        SUB      SP,SP,#+1024
          CFI CFA R13+1048
        MOV      R4,R0
        MOV      R5,R1
        LDR.W    R1,??DataTable138
        LDR      R0,[R1, #+32]
        LDR      R6,[R1, #+36]
        ADDS     R6,R6,R0
        MOV      R7,R0
        B.N      ??user_getShedule_0
??user_getShedule_1:
        ADDS     R2,R2,#+32
        CMP      R2,#+1024
        BLT.N    ??user_getShedule_2
        ADD      R7,R7,#+1024
??user_getShedule_0:
        CMP      R7,R6
        BCS.N    ??user_getShedule_3
        BL       ?Subroutine50
??CrossCallReturnLabel_179:
        MOVS     R2,#+0
??user_getShedule_2:
        ADD      R1,SP,R2
        LDRB     R3,[R1, #+0]
        CMP      R3,R4
        BNE.N    ??user_getShedule_1
        MOVS     R2,#+32
        MOV      R0,R5
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        MOVS     R0,#+1
        B.N      ??user_getShedule_4
??user_getShedule_3:
        MOVS     R0,#+0
??user_getShedule_4:
        B.N      ?Subroutine2
          CFI EndBlock cfiBlock366

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable131:
        DATA32
        DC32     ?_92

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable131_1:
        DATA32
        DC32     ?_93

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable131_2:
        DATA32
        DC32     ?_94

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond367 Using cfiCommon0
          CFI Function user_getGroup
          CFI Conditional ??CrossCallReturnLabel_180
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+1048
          CFI Block cfiCond368 Using cfiCommon0
          CFI (cfiCond368) Function user_getShedule
          CFI (cfiCond368) Conditional ??CrossCallReturnLabel_179
          CFI (cfiCond368) R4 Frame(CFA, -20)
          CFI (cfiCond368) R5 Frame(CFA, -16)
          CFI (cfiCond368) R6 Frame(CFA, -12)
          CFI (cfiCond368) R7 Frame(CFA, -8)
          CFI (cfiCond368) R14 Frame(CFA, -4)
          CFI (cfiCond368) CFA R13+1048
          CFI Block cfiCond369 Using cfiCommon0
          CFI (cfiCond369) Function user_checkIdTimeEnd
          CFI (cfiCond369) Conditional ??CrossCallReturnLabel_178
          CFI (cfiCond369) R4 Frame(CFA, -28)
          CFI (cfiCond369) R5 Frame(CFA, -24)
          CFI (cfiCond369) R6 Frame(CFA, -20)
          CFI (cfiCond369) R7 Frame(CFA, -16)
          CFI (cfiCond369) R8 Frame(CFA, -12)
          CFI (cfiCond369) R9 Frame(CFA, -8)
          CFI (cfiCond369) R14 Frame(CFA, -4)
          CFI (cfiCond369) CFA R13+1056
          CFI Block cfiPicker370 Using cfiCommon1
          CFI (cfiPicker370) NoFunction
          CFI (cfiPicker370) Picker
        THUMB
?Subroutine50:
        MOV      R2,#+1024
        MOV      R1,SP
        MOV      R0,R7
          CFI (cfiCond367) FunCall user_getGroup mem_read
          CFI (cfiCond368) FunCall user_getShedule mem_read
          CFI (cfiCond369) FunCall user_checkIdTimeEnd mem_read
        B.W      mem_read
          CFI EndBlock cfiCond367
          CFI EndBlock cfiCond368
          CFI EndBlock cfiCond369
          CFI EndBlock cfiPicker370

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock371 Using cfiCommon0
          CFI Function user_dayIsHoliday
        THUMB
user_dayIsHoliday:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
        SUB      SP,SP,#+1024
          CFI CFA R13+1032
        MOV      R2,#+1024
        MOV      R1,SP
        LDR.W    R0,??DataTable138
        LDR      R0,[R0, #+40]
          CFI FunCall mem_read
        BL       mem_read
        MOVS     R2,#+0
??user_dayIsHoliday_0:
        LDR      R1,[SP, R2]
        LDR.W    R0,??DataTable139
        UDIV     R1,R1,R0
        LDR.W    R0,??DataTable139_1
        LDR      R3,[R0, #+47]
        LDR.W    R0,??DataTable139
        UDIV     R0,R3,R0
        CMP      R1,R0
        BEQ.N    ??user_dayIsHoliday_1
        ADDS     R2,R2,#+4
        CMP      R2,#+1024
        BLT.N    ??user_dayIsHoliday_0
        B.N      ??user_dayIsHoliday_2
??user_dayIsHoliday_1:
        MOVS     R0,#+1
??user_dayIsHoliday_3:
        ADD      SP,SP,#+1024
          CFI CFA R13+8
        ADD      SP,SP,#+4
          CFI CFA R13+4
        POP      {PC}
          CFI CFA R13+1032
??user_dayIsHoliday_2:
        MOVS     R0,#+0
        B.N      ??user_dayIsHoliday_3
          CFI EndBlock cfiBlock371

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable132:
        DATA32
        DC32     store_save_requested

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock372 Using cfiCommon0
          CFI Function user_clearTempIdBuffer
        THUMB
user_clearTempIdBuffer:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        LDR.W    R4,??DataTable138
        LDR      R0,[R4, #+64]
        LDR      R5,[R4, #+68]
        ADDS     R5,R5,R0
        MOV      R6,R0
        B.N      ??user_clearTempIdBuffer_0
??user_clearTempIdBuffer_1:
          CFI FunCall wdt_reload
        BL       wdt_reload
        ADD      R6,R6,#+4096
??user_clearTempIdBuffer_0:
        CMP      R6,R5
        BCS.N    ??user_clearTempIdBuffer_2
        MOV      R0,R6
          CFI FunCall mem_erase_sector
        BL       mem_erase_sector
        CMP      R0,#+0
        BNE.N    ??user_clearTempIdBuffer_1
        POP      {R4-R6,PC}
??user_clearTempIdBuffer_2:
        LDR.W    R5,??DataTable139_1
        LDRH     R1,[R5, #+129]
        MOVW     R0,#+49151
        ANDS     R1,R0,R1
        STRH     R1,[R5, #+129]
        MOVS     R0,#+0
        STRH     R0,[R5, #+83]
        MOVS     R0,#+9
          CFI FunCall upload_reset
        BL       upload_reset
        LDR      R0,[R4, #+64]
        STR      R0,[R5, #+85]
        MOVS     R0,#+1
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock372

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable133:
        DATA32
        DC32     ?_95

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable133_1:
        DATA32
        DC32     ?_96

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable133_2:
        DATA32
        DC32     ?_97

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable133_3:
        DATA32
        DC32     ?_98

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable133_4:
        DATA32
        DC32     ?_99

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock373 Using cfiCommon0
          CFI Function user_getNumbersCount
        THUMB
user_getNumbersCount:
        PUSH     {R3-R11,LR}
          CFI R14 Frame(CFA, -4)
          CFI R11 Frame(CFA, -8)
          CFI R10 Frame(CFA, -12)
          CFI R9 Frame(CFA, -16)
          CFI R8 Frame(CFA, -20)
          CFI R7 Frame(CFA, -24)
          CFI R6 Frame(CFA, -28)
          CFI R5 Frame(CFA, -32)
          CFI R4 Frame(CFA, -36)
          CFI CFA R13+40
        SUB      SP,SP,#+1024
          CFI CFA R13+1064
        LDR.N    R0,??DataTable138
        LDR      R1,[R0, #+16]
        LDR      R11,[R0, #+20]
        ADD      R11,R11,R1
        MOVS     R4,#+0
        MOVS     R5,#+0
        MOVS     R6,#+0
        MOV      R9,R4
        LDR.W    R7,??DataTable154_1
        STRH     R4,[R7, #+0]
        LDR      R8,[R0, #+16]
        B.N      ??user_getNumbersCount_0
??user_getNumbersCount_1:
          CFI FunCall wdt_reload
        BL       wdt_reload
        ADD      R8,R8,#+1024
??user_getNumbersCount_0:
        CMP      R8,R11
        BCS.N    ??user_getNumbersCount_2
        MOV      R2,#+1024
        MOV      R1,SP
        MOV      R0,R8
          CFI FunCall mem_read
        BL       mem_read
        MOV      R10,#+0
??user_getNumbersCount_3:
        ADD      R0,SP,R10
        ADDS     R5,R5,#+1
        MOVS     R1,#+0
??user_getNumbersCount_4:
        UXTB     R2,R1
        ADD      R2,R2,R10
        LDRB     R2,[SP, R2]
        CMP      R2,#+255
        BNE.N    ??user_getNumbersCount_5
        ADDS     R1,R1,#+1
        UXTB     R2,R1
        CMP      R2,#+32
        BCC.N    ??user_getNumbersCount_4
        B.N      ??user_getNumbersCount_6
??user_getNumbersCount_5:
        MOV      R6,R5
??user_getNumbersCount_6:
        LDRH     R2,[R0, #+0]
        MOVW     R1,#+65535
        CMP      R2,R1
        BEQ.N    ??user_getNumbersCount_7
        ADDS     R4,R4,#+1
        STRH     R5,[R7, #+0]
        ADDS     R1,R0,#+2
        UXTH     R0,R5
          CFI FunCall idindex_hash
        BL       idindex_hash
        EOR      R9,R0,R9
??user_getNumbersCount_7:
        ADD      R10,R10,#+32
        CMP      R10,#+1024
        BLT.N    ??user_getNumbersCount_3
        B.N      ??user_getNumbersCount_1
??user_getNumbersCount_2:
        LSLS     R6,R6,#+16
        LSRS     R6,R6,#+11
        LDR.W    R0,??DataTable155
        STR      R6,[R0, #+0]
        LDRH     R1,[R7, #+0]
        MOV      R0,R9
          CFI FunCall idindex_scanned
        BL       idindex_scanned
        UXTH     R0,R4
        ADD      SP,SP,#+1024
          CFI CFA R13+40
        ADD      SP,SP,#+4
          CFI CFA R13+36
        POP      {R4-R11,PC}
          CFI EndBlock cfiBlock373

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable134:
        DATA32
        DC32     ?_100

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock374 Using cfiCommon0
          CFI Function user_checkIdTimeEnd
        THUMB
user_checkIdTimeEnd:
; Hand-written runtime continuation. Historical body below is unreachable.
          CFI FunCall expiry_runtime_begin
        B.W      expiry_runtime_begin
        PUSH     {R3-R9,LR}
          CFI R14 Frame(CFA, -4)
          CFI R9 Frame(CFA, -8)
          CFI R8 Frame(CFA, -12)
          CFI R7 Frame(CFA, -16)
          CFI R6 Frame(CFA, -20)
          CFI R5 Frame(CFA, -24)
          CFI R4 Frame(CFA, -28)
          CFI CFA R13+32
        SUB      SP,SP,#+1024
          CFI CFA R13+1056
          CFI FunCall mem_tables_ready
        BL       mem_tables_ready
        CMP      R0,#+0
        BEQ.N    ??user_checkIdTimeEnd_0
        LDR.N    R1,??DataTable138
        LDR      R2,[R1, #+16]
        LDR.W    R0,??DataTable154_1
        LDRH     R0,[R0, #+0]
        ADD      R8,R2,R0, LSL #+5
        LDR.W    R9,??DataTable139_1
        LDR      R2,[R9, #+47]
        LDR.W    R0,??DataTable151_2
        CMP      R2,R0
        BCC.N    ??user_checkIdTimeEnd_0
        LDR.W    R6,??DataTable154_2
        LDRB     R0,[R6, #+0]
        CBNZ.N   R0,??user_checkIdTimeEnd_1
        LDR.W    R2,??DataTable154_3
        LDR      R0,[R2, #+0]
        CMP      R0,#+0
        BEQ.N    ??user_checkIdTimeEnd_0
        LDR      R0,[R9, #+47]
        LDR      R2,[R2, #+0]
        CMP      R0,R2
        BCC.N    ??user_checkIdTimeEnd_0
??user_checkIdTimeEnd_1:
        MOVS     R0,#+0
        STRB     R0,[R6, #+0]
        MOVS     R2,#+0
        LDR.W    R0,??DataTable154_3
        STR      R2,[R0, #+0]
        LDR      R7,[R1, #+16]
        B.N      ??user_checkIdTimeEnd_2
??user_checkIdTimeEnd_3:
        MOVS     R0,#+1
        STRB     R0,[R6, #+0]
        B.N      ??user_checkIdTimeEnd_4
??user_checkIdTimeEnd_5:
          CFI FunCall wdt_reload
        BL       wdt_reload
        ADD      R7,R7,#+1024
??user_checkIdTimeEnd_2:
        CMP      R7,R8
        BCS.N    ??user_checkIdTimeEnd_0
        BL       ?Subroutine50
??CrossCallReturnLabel_178:
        MOVS     R5,#+0
??user_checkIdTimeEnd_6:
        ADDS     R0,R5,R7
        CMP      R0,R8
        BCS.N    ??user_checkIdTimeEnd_5
        ADD      R4,SP,R5
        LDRH     R1,[R4, #+0]
        BL       ?Subroutine62
??CrossCallReturnLabel_218:
        BEQ.N    ??user_checkIdTimeEnd_4
        MOV      R0,R4
          CFI FunCall id_time_end
        BL       id_time_end
        CBZ.N    R0,??user_checkIdTimeEnd_4
        LDR      R1,[R9, #+47]
        CMP      R1,R0
        BCS.N    ??user_checkIdTimeEnd_7
          CFI FunCall id_watch_end
        BL       id_watch_end
        B.N      ??user_checkIdTimeEnd_4
??user_checkIdTimeEnd_7:
        LDR.N    R1,??DataTable139
        UDIV     R0,R0,R1
        LDR      R2,[R9, #+47]
        UDIV     R1,R2,R1
        CMP      R0,R1
        BNE.N    ??user_checkIdTimeEnd_4
        MOVS     R0,#+56
        STRB     R0,[R4, #+28]
        MOV      R0,R4
          CFI FunCall user_updateID
        BL       user_updateID
        CMP      R0,#+0
        BEQ.N    ??user_checkIdTimeEnd_3
        MOVS     R2,#+255
        ADDS     R1,R4,#+2
        MOVS     R0,#+138
          CFI FunCall log_addControlEvent
        BL       log_addControlEvent
??user_checkIdTimeEnd_4:
        ADDS     R5,R5,#+32
        UXTH     R5,R5
        CMP      R5,#+1024
        BLT.N    ??user_checkIdTimeEnd_6
        B.N      ??user_checkIdTimeEnd_5
??user_checkIdTimeEnd_0:
          CFI EndBlock cfiBlock374
        REQUIRE ?Subroutine3

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock375 Using cfiCommon0
          CFI NoFunction
          CFI CFA R13+1056
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
        THUMB
?Subroutine3:
        ADD      SP,SP,#+1024
          CFI CFA R13+32
        ADD      SP,SP,#+4
          CFI CFA R13+28
        POP      {R4-R9,PC}
          CFI EndBlock cfiBlock375

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock376 Using cfiCommon0
          CFI Function user_getTempIDRecordsCount
          CFI NoCalls
        THUMB
user_getTempIDRecordsCount:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        SUB      SP,SP,#+1024
          CFI CFA R13+1040
        LDR.N    R0,??DataTable138
        LDR      R1,[R0, #+64]
        LDR      R4,[R0, #+68]
        ADDS     R4,R4,R1
        MOVS     R5,#+0
        MOV      R6,R1
        B.N      ??user_getTempIDRecordsCount_0
??user_getTempIDRecordsCount_1:
        ADD      R6,R6,#+1024
        MOV      R5,R0
??user_getTempIDRecordsCount_0:
        CMP      R6,R4
        BCS.N    ??user_getTempIDRecordsCount_2
        BL       ?Subroutine33
??CrossCallReturnLabel_110:
        CBZ.N    R0,??user_getTempIDRecordsCount_3
        MOVS     R1,#+0
        MOV      R0,R5
        B.N      ??user_getTempIDRecordsCount_4
??user_getTempIDRecordsCount_3:
        MOVW     R0,#+65535
        B.N      ??user_getTempIDRecordsCount_5
??user_getTempIDRecordsCount_6:
        ADDS     R0,R0,#+1
        ADDS     R1,R1,#+32
        CMP      R1,#+1024
        BGE.N    ??user_getTempIDRecordsCount_1
??user_getTempIDRecordsCount_4:
        LDRH     R3,[SP, R1]
        MOVW     R2,#+65535
        CMP      R3,R2
        BNE.N    ??user_getTempIDRecordsCount_6
        ADDS     R6,R1,R6
        LDR.N    R1,??DataTable139_1
        STR      R6,[R1, #+85]
        UXTH     R0,R0
        B.N      ??user_getTempIDRecordsCount_5
??user_getTempIDRecordsCount_2:
        LDR.N    R0,??DataTable139_1
        STR      R4,[R0, #+85]
        UXTH     R0,R5
??user_getTempIDRecordsCount_5:
        ADD      SP,SP,#+1024
          CFI CFA R13+16
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock376

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock377 Using cfiCommon0
          CFI Function user_checkShedule
        THUMB
user_checkShedule:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        SUB      SP,SP,#+32
          CFI CFA R13+40
        MOV      R4,R0
          CFI FunCall mem_tables_ready
        BL       mem_tables_ready
        CMP      R0,#+0
        BEQ.N    ??user_checkShedule_0
        MOV      R1,SP
        MOV      R0,R4
          CFI FunCall user_getShedule
        BL       user_getShedule
        CBNZ.N   R0,??user_checkShedule_1
        LDR.W    R0,??DataTable157
        B.N      ??user_checkShedule_2
??user_checkShedule_1:
        LDRB     R0,[SP, #+5]
        CBZ.N    R0,??user_checkShedule_3
        LDR.W    R0,??DataTable157_1
        B.N      ??user_checkShedule_2
??user_checkShedule_3:
        LDR.N    R1,??DataTable139_1
        LDRB     R1,[R1, #+55]
        ADD      R0,SP,R1
        LDRB     R0,[R0, #+6]
        CBNZ.N   R0,??user_checkShedule_4
        LDR.W    R0,??DataTable158
        B.N      ??user_checkShedule_2
??user_checkShedule_4:
        MOV      R0,SP
          CFI FunCall shedule_time_ok
        BL       shedule_time_ok
        CBNZ.N   R0,??user_checkShedule_5
        LDR.W    R0,??DataTable158_1
        B.N      ??user_checkShedule_2
??user_checkShedule_5:
        MOVS     R0,#+0
        LDRB     R1,[SP, #+21]
        CMP      R1,#+1
        BEQ.N    ??user_checkShedule_6
        BCC.N    ??user_checkShedule_7
        CMP      R1,#+3
        BEQ.N    ??user_checkShedule_8
          CFI FunCall user_dayIsHoliday
        IT       CC
        BLCC     user_dayIsHoliday
??user_checkShedule_7:
        CBNZ.N   R0,??user_checkShedule_6
??user_checkShedule_9:
        LDR.W    R0,??DataTable160
??user_checkShedule_2:
          CFI FunCall user_debug
        BL       user_debug
        MOVS     R0,#+0
        B.N      ??user_checkShedule_0
??user_checkShedule_8:
          CFI FunCall user_dayIsHoliday
        BL       user_dayIsHoliday
        CMP      R0,#+0
        BNE.N    ??user_checkShedule_9
??user_checkShedule_6:
        MOVS     R0,#+1
??user_checkShedule_0:
        ADD      SP,SP,#+32
          CFI CFA R13+8
        POP      {R4,PC}
          CFI EndBlock cfiBlock377

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock378 Using cfiCommon0
          CFI Function user_updateID
        THUMB
user_updateID:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        MOV      R4,R0
        LDR.W    R5,??DataTable160_1
        LDR      R0,[R5, #+16]
        LDR      R6,[R5, #+20]
        ADDS     R6,R6,R0
        LDRH     R0,[R4, #+0]
        CMP      R0,#+0
        ITTT     NE
        MOVNE    R1,R0
        MOVWNE   R0,#+65535
        CMPNE    R1,R0
        BEQ.N    ??user_updateID_0
        LDR      R7,[R5, #+16]
        SUBS     R0,R1,#+1
        ADD      R7,R7,R0, LSL #+5
        ADD      R0,R7,#+32
        CMP      R6,R0
        BCC.N    ??user_updateID_0
        MOVS     R2,#+2
        MOV      R1,SP
        MOV      R0,R7
          CFI FunCall mem_read
        BL       mem_read
        CBZ.N    R0,??user_updateID_1
        LDRH     R1,[SP, #+0]
        LDRH     R0,[R4, #+0]
        CMP      R1,R0
        BNE.N    ??user_updateID_0
        MOVS     R2,#+32
        MOV      R1,R4
        MOV      R0,R7
        B.N      ??user_updateID_2
??user_updateID_0:
        LDR      R5,[R5, #+16]
        B.N      ??user_updateID_3
??user_updateID_4:
        ADDS     R5,R5,#+32
??user_updateID_3:
        ADD      R0,R5,#+32
        CMP      R6,R0
        BCC.N    ??user_updateID_1
        MOVS     R2,#+2
        BL       ?Subroutine59
??CrossCallReturnLabel_208:
        CBZ.N    R0,??user_updateID_1
        LDRH     R1,[SP, #+0]
        LDRH     R0,[R4, #+0]
        CMP      R1,R0
        BNE.N    ??user_updateID_4
        MOVS     R2,#+32
        MOV      R1,R4
        MOV      R0,R5
??user_updateID_2:
          CFI FunCall mem_patch
        BL       mem_patch
        POP      {R1,R4-R7,PC}
??user_updateID_1:
        MOVS     R0,#+0
        POP      {R1,R4-R7,PC}
          CFI EndBlock cfiBlock378

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable138:
        DATA32
        DC32     mem

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond379 Using cfiCommon0
          CFI Function user_calcCRC16InFlash
          CFI Conditional ??CrossCallReturnLabel_209
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+1048
          CFI Block cfiCond380 Using cfiCommon0
          CFI (cfiCond380) Function user_updateID
          CFI (cfiCond380) Conditional ??CrossCallReturnLabel_208
          CFI (cfiCond380) R4 Frame(CFA, -20)
          CFI (cfiCond380) R5 Frame(CFA, -16)
          CFI (cfiCond380) R6 Frame(CFA, -12)
          CFI (cfiCond380) R7 Frame(CFA, -8)
          CFI (cfiCond380) R14 Frame(CFA, -4)
          CFI (cfiCond380) CFA R13+24
          CFI Block cfiCond381 Using cfiCommon0
          CFI (cfiCond381) Function user_getPhotoCount
          CFI (cfiCond381) Conditional ??CrossCallReturnLabel_207
          CFI (cfiCond381) R4 Frame(CFA, -16)
          CFI (cfiCond381) R5 Frame(CFA, -12)
          CFI (cfiCond381) R6 Frame(CFA, -8)
          CFI (cfiCond381) R14 Frame(CFA, -4)
          CFI (cfiCond381) CFA R13+24
          CFI Block cfiPicker382 Using cfiCommon1
          CFI (cfiPicker382) NoFunction
          CFI (cfiPicker382) Picker
        THUMB
?Subroutine59:
        MOV      R1,SP
        MOV      R0,R5
          CFI (cfiCond379) FunCall user_calcCRC16InFlash mem_read
          CFI (cfiCond380) FunCall user_updateID mem_read
          CFI (cfiCond381) FunCall user_getPhotoCount mem_read
        B.W      mem_read
          CFI EndBlock cfiCond379
          CFI EndBlock cfiCond380
          CFI EndBlock cfiCond381
          CFI EndBlock cfiPicker382

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock383 Using cfiCommon0
          CFI Function log_clearLogBuffer
        THUMB
log_clearLogBuffer:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        LDR.W    R4,??DataTable160_1
        LDR      R0,[R4, #+56]
        LDR      R6,[R4, #+60]
        ADDS     R6,R6,R0
        LDR.N    R5,??DataTable139_1
        MOV      R1,R0
        LDR      R0,[R5, #+91]
        CMP      R1,R0
        BCS.N    ??log_clearLogBuffer_0
        LDR      R0,[R5, #+91]
        CMP      R0,R6
        IT       LS
        MOVLS    R6,R0
??log_clearLogBuffer_0:
        LSRS     R7,R1,#+12
        LSLS     R7,R7,#+12
        B.N      ??log_clearLogBuffer_1
??log_clearLogBuffer_2:
          CFI FunCall wdt_reload
        BL       wdt_reload
        ADD      R7,R7,#+4096
??log_clearLogBuffer_1:
        CMP      R7,R6
        BCS.N    ??log_clearLogBuffer_3
        MOV      R0,R7
          CFI FunCall mem_erase_sector
        BL       mem_erase_sector
        CMP      R0,#+0
        BNE.N    ??log_clearLogBuffer_2
        POP      {R1,R4-R7,PC}
??log_clearLogBuffer_3:
        MOVS     R0,#+0
        STRH     R0,[R5, #+151]
        MOVS     R0,#+10
          CFI FunCall upload_reset
        BL       upload_reset
        MOVS     R0,#+0
        STRH     R0,[R5, #+89]
        LDRH     R1,[R5, #+129]
        MOVW     R0,#+61439
        ANDS     R1,R0,R1
        STRH     R1,[R5, #+129]
        LDR      R0,[R4, #+56]
        STR      R0,[R5, #+91]
        MOVS     R0,#+1
        POP      {R1,R4-R7,PC}
          CFI EndBlock cfiBlock383

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable139:
        DATA32
        DC32     0x15180

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable139_1:
        DATA32
        DC32     device

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock384 Using cfiCommon0
          CFI Function log_blank
          CFI NoCalls
        THUMB
log_blank:
        MOVS     R1,#+0
??log_blank_0:
        UXTH     R2,R1
        LDRB     R2,[R0, R2]
        CMP      R2,#+255
        BNE.N    ??log_blank_1
        ADDS     R1,R1,#+1
        UXTH     R2,R1
        CMP      R2,#+32
        BCC.N    ??log_blank_0
        B.N      ??log_blank_2
??log_blank_1:
        MOVS     R0,#+0
        BX       LR
??log_blank_2:
        MOVS     R0,#+1
        BX       LR
          CFI EndBlock cfiBlock384

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock385 Using cfiCommon0
          CFI Function log_valid
        THUMB
log_valid:
        PUSH     {R2-R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+16
        LDR      R1,[R0, #+0]
        CMN      R1,#+1
        BEQ.N    ??log_valid_0
        ADD      R4,R0,#+28
        ADD      R1,SP,#+4
        LDR      R3,[R4, #+0]
        STR      R3,[R1, #+0]
        MOVS     R1,R3
        BNE.N    ??log_valid_1
        MOVS     R2,#+0
??log_valid_2:
        UXTB     R1,R2
        ADD      R1,R0,R1
        LDRB     R1,[R1, #+24]
        CBNZ.N   R1,??log_valid_0
        ADDS     R2,R2,#+1
        UXTB     R1,R2
        CMP      R1,#+8
        BCC.N    ??log_valid_2
        B.N      ??log_valid_3
??log_valid_1:
        LDR.W    R1,??DataTable163
        CMP      R3,R1
        BNE.N    ??log_valid_0
        ADD      R4,R0,#+24
        LDRB     R3,[R4, #+0]
        STRB     R3,[SP, #+0]
        LDRB     R3,[R4, #+1]
        STRB     R3,[SP, #+1]
        ADD      R4,R0,#+26
        ADD      R1,SP,#+2
        BL       ?Subroutine13
??CrossCallReturnLabel_22:
        LDRH     R2,[SP, #+2]
        LDRH     R1,[SP, #+0]
        MVNS     R1,R1
        UXTH     R1,R1
        CMP      R2,R1
        BNE.N    ??log_valid_0
        LDRH     R4,[SP, #+0]
        MOVS     R1,#+24
          CFI FunCall crc16_calc
        BL       crc16_calc
        CMP      R4,R0
        BNE.N    ??log_valid_0
??log_valid_3:
        MOVS     R0,#+1
        POP      {R1,R2,R4,PC}
??log_valid_0:
        MOVS     R0,#+0
        POP      {R1,R2,R4,PC}
          CFI EndBlock cfiBlock385

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
log_lastIndex:
        DS8 4

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock386 Using cfiCommon0
          CFI Function log_uniqueIndex
        THUMB
log_uniqueIndex:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        SUB      SP,SP,#+32
          CFI CFA R13+48
        MOV      R5,R0
        MOV      R0,R1
        LDR.W    R1,??DataTable164
        LDR      R4,[R1, #+0]
        LDR.W    R1,??DataTable160_1
        LDR      R1,[R1, #+56]
        ADDS     R1,R1,#+32
        CMP      R0,R1
        BCC.N    ??log_uniqueIndex_0
        MOVS     R2,#+32
        MOV      R1,SP
        SUBS     R0,R0,#+32
          CFI FunCall mem_read
        BL       mem_read
        CBZ.N    R0,??log_uniqueIndex_0
        MOV      R0,SP
          CFI FunCall log_valid
        BL       log_valid
        CBZ.N    R0,??log_uniqueIndex_0
        LDR      R0,[SP, #+0]
        CMP      R0,R4
        IT       CS
        MOVCS    R4,R0
??log_uniqueIndex_0:
        CBZ.N    R4,??log_uniqueIndex_1
        LDR      R0,[R5, #+0]
        CMP      R4,R0
        ITT      CS
        ADDCS    R4,R4,#+1
        STRCS    R4,[R5, #+0]
??log_uniqueIndex_1:
        ADD      SP,SP,#+36
          CFI CFA R13+12
        POP      {R4,R5,PC}
          CFI EndBlock cfiBlock386

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock387 Using cfiCommon0
          CFI Function log_addEvent
        THUMB
log_addEvent:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        SUB      SP,SP,#+40
          CFI CFA R13+64
        MOV      R4,R0
        LDR.W    R0,??DataTable163_1
        LDRB     R0,[R0, #+19]
        CMP      R0,#+10
        BEQ.N    ??log_addEvent_0
          CFI FunCall user_storage_flash_ready
        BL       user_storage_flash_ready
        CMP      R0,#+0
        BEQ.N    ??log_addEvent_0
        LDR.W    R1,??DataTable160_1
        LDR      R0,[R1, #+56]
        LDR      R7,[R1, #+60]
        ADDS     R7,R7,R0
        LDR.W    R5,??DataTable163_2
        LDR      R0,[R5, #+91]
        LDR      R6,[R1, #+56]
        CMP      R6,R0
        BCS.N    ??log_addEvent_1
        MOV      R6,R0
        B.N      ??log_addEvent_1
??log_addEvent_2:
        ADDS     R6,R6,#+32
??log_addEvent_1:
        ADD      R0,R6,#+32
        CMP      R7,R0
        BCC.N    ??log_addEvent_0
          CFI FunCall wdt_reload
        BL       wdt_reload
        MOVS     R2,#+32
        ADD      R1,SP,#+8
        BL       ??Subroutine33_1
??CrossCallReturnLabel_106:
        CMP      R0,#+0
        BEQ.N    ??log_addEvent_0
        ADD      R0,SP,#+8
          CFI FunCall log_blank
        BL       log_blank
        CMP      R0,#+0
        BEQ.N    ??log_addEvent_2
        MOV      R1,R6
        MOV      R0,R4
          CFI FunCall log_uniqueIndex
        BL       log_uniqueIndex
        ADD      R0,SP,#+8
        MOV      R1,R4
        MOVS     R2,#+32
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        MOVS     R1,#+24
        ADD      R0,SP,#+8
          CFI FunCall crc16_calc
        BL       crc16_calc
        BL       ?Subroutine66
??CrossCallReturnLabel_231:
        LDR.W    R0,??DataTable163
        STR      R0,[SP, #+4]
        ADD      R0,SP,#+32
        BL       ?Subroutine16
??CrossCallReturnLabel_31:
        ADD      R0,SP,#+34
        BL       ?Subroutine14
??CrossCallReturnLabel_27:
        MOVS     R2,#+255
        MOVS     R1,#+4
        ADD      R0,SP,#+36
          CFI FunCall __aeabi_memset4
        BL       __aeabi_memset4
        MOVS     R2,#+28
        ADD      R1,SP,#+8
        MOV      R0,R6
          CFI FunCall mem_write
        BL       mem_write
        CBZ.N    R0,??log_addEvent_0
        MOVS     R2,#+4
        ADD      R1,SP,#+4
        ADD      R0,R6,#+28
          CFI FunCall mem_write
        BL       mem_write
        CBZ.N    R0,??log_addEvent_0
        MOVS     R2,#+32
        ADD      R1,SP,#+8
        BL       ??Subroutine33_1
??CrossCallReturnLabel_105:
        CBZ.N    R0,??log_addEvent_0
        ADD      R0,SP,#+8
          CFI FunCall log_valid
        BL       log_valid
        CBZ.N    R0,??log_addEvent_0
        LDR      R1,[R4, #+0]
        LDR.W    R0,??DataTable164
        STR      R1,[R0, #+0]
        ADDS     R6,R6,#+32
        STR      R6,[R5, #+91]
        LDRH     R0,[R5, #+129]
        ORR      R0,R0,#0x1000
        STRH     R0,[R5, #+129]
        MOVS     R0,#+1
        B.N      ??log_addEvent_3
??log_addEvent_0:
        MOVS     R0,#+0
??log_addEvent_3:
        ADD      SP,SP,#+44
          CFI CFA R13+20
        POP      {R4-R7,PC}
          CFI EndBlock cfiBlock387

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable142:
        DATA32
        DC32     ?_101

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable142_1:
        DATA32
        DC32     ?_102

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable142_2:
        DATA32
        DC32     ?_103

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable142_3:
        DATA32
        DC32     ?_104

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable142_4:
        DATA32
        DC32     ?_105

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable142_5:
        DATA32
        DC32     ?_106

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable142_6:
        DATA32
        DC32     ?_107

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable142_7:
        DATA32
        DC32     ?_108

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock388 Using cfiCommon0
          CFI Function log_getRecordsCount
        THUMB
log_getRecordsCount:
        PUSH     {R3-R9,LR}
          CFI R14 Frame(CFA, -4)
          CFI R9 Frame(CFA, -8)
          CFI R8 Frame(CFA, -12)
          CFI R7 Frame(CFA, -16)
          CFI R6 Frame(CFA, -20)
          CFI R5 Frame(CFA, -24)
          CFI R4 Frame(CFA, -28)
          CFI CFA R13+32
        SUB      SP,SP,#+1024
          CFI CFA R13+1056
        LDR.W    R0,??DataTable160_1
        LDR      R1,[R0, #+56]
        LDR      R4,[R0, #+60]
        ADDS     R4,R4,R1
        MOVS     R5,#+0
        MOV      R6,R1
        B.N      ??log_getRecordsCount_0
??log_getRecordsCount_1:
        ADD      R6,R6,#+1024
??log_getRecordsCount_0:
        CMP      R6,R4
        BCS.N    ??log_getRecordsCount_2
        SUBS     R1,R4,R6
        MOVW     R0,#+1025
        CMP      R1,R0
        ITEE     CS
        MOVCS    R9,#+1024
        SUBCC    R0,R4,R6
        UXTHCC   R9,R0
          CFI FunCall wdt_reload
        BL       wdt_reload
        MOV      R2,R9
        BL       ??Subroutine33_0
??CrossCallReturnLabel_108:
        CBZ.N    R0,??log_getRecordsCount_3
        MOVS     R7,#+0
        B.N      ??log_getRecordsCount_4
??log_getRecordsCount_3:
        MOVW     R0,#+65535
        B.N      ??log_getRecordsCount_5
??log_getRecordsCount_6:
        MOV      R0,R8
          CFI FunCall log_valid
        BL       log_valid
        CBZ.N    R0,??log_getRecordsCount_7
        ADDS     R5,R5,#+1
        LDR.W    R0,??DataTable164
        LDR      R2,[R0, #+0]
        LDR      R1,[R8, #+0]
        CMP      R2,R1
        IT       CC
        STRCC    R1,[R0, #+0]
??log_getRecordsCount_7:
        ADDS     R7,R7,#+32
        UXTH     R7,R7
??log_getRecordsCount_4:
        ADD      R0,R7,#+32
        CMP      R9,R0
        BCC.N    ??log_getRecordsCount_1
        ADD      R8,SP,R7
        MOV      R0,R8
          CFI FunCall log_blank
        BL       log_blank
        CMP      R0,#+0
        BEQ.N    ??log_getRecordsCount_6
        ADDS     R6,R7,R6
        LDR.W    R0,??DataTable163_2
        STR      R6,[R0, #+91]
        B.N      ??log_getRecordsCount_8
??log_getRecordsCount_2:
        LDR.W    R0,??DataTable163_2
        STR      R4,[R0, #+91]
??log_getRecordsCount_8:
        UXTH     R0,R5
??log_getRecordsCount_5:
        B.N      ?Subroutine3
          CFI EndBlock cfiBlock388

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable143:
        DATA32
        DC32     ?_109

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock389 Using cfiCommon0
          CFI Function log_readRecords
        THUMB
log_readRecords:
        PUSH     {R4-R10,LR}
          CFI R14 Frame(CFA, -4)
          CFI R10 Frame(CFA, -8)
          CFI R9 Frame(CFA, -12)
          CFI R8 Frame(CFA, -16)
          CFI R7 Frame(CFA, -20)
          CFI R6 Frame(CFA, -24)
          CFI R5 Frame(CFA, -28)
          CFI R4 Frame(CFA, -32)
          CFI CFA R13+32
        SUB      SP,SP,#+32
          CFI CFA R13+64
        MOV      R8,R0
        MOV      R9,R1
        MOV      R10,R2
        MOVS     R7,#+0
        MOVS     R4,#+0
        LDR.W    R1,??DataTable160_1
        LDR      R0,[R1, #+56]
        LDR      R5,[R1, #+60]
        ADDS     R5,R5,R0
        MOV      R6,R0
        B.N      ??log_readRecords_0
??log_readRecords_1:
        MOV      R0,SP
          CFI FunCall log_blank
        BL       log_blank
        CBNZ.N   R0,??log_readRecords_2
        MOV      R0,SP
          CFI FunCall log_valid
        BL       log_valid
        CBZ.N    R0,??log_readRecords_3
        MOV      R1,R7
        ADDS     R7,R1,#+1
        UXTH     R1,R1
        CMP      R1,R8
        BCC.N    ??log_readRecords_3
        ADD      R2,SP,#+24
        MOVS     R0,#+0
        STRD     R0,R0,[R2, #+0]
        MOVS     R2,#+32
        MOV      R1,SP
        LSLS     R0,R4,#+16
        ADD      R0,R9,R0, LSR #+11
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        ADDS     R4,R4,#+1
??log_readRecords_3:
        ADDS     R6,R6,#+32
??log_readRecords_0:
        ADD      R0,R6,#+32
        CMP      R5,R0
        BCC.N    ??log_readRecords_4
        UXTH     R1,R4
        CMP      R1,R10
        BCS.N    ??log_readRecords_4
          CFI FunCall wdt_reload
        BL       wdt_reload
        MOVS     R2,#+32
        BL       ??Subroutine33_0
??CrossCallReturnLabel_107:
        CMP      R0,#+0
        BNE.N    ??log_readRecords_1
        B.N      ??log_readRecords_5
??log_readRecords_4:
        UXTH     R4,R4
        CMP      R4,R10
        IT       EQ
        MOVEQ    R0,#+1
        BEQ.N    ??log_readRecords_5
??log_readRecords_2:
        MOVS     R0,#+0
??log_readRecords_5:
        ADD      SP,SP,#+32
          CFI CFA R13+32
        POP      {R4-R10,PC}
          CFI EndBlock cfiBlock389

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable144:
        DATA32
        DC32     ?_110

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable144_1:
        DATA32
        DC32     ?_111

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond390 Using cfiCommon0
          CFI Function user_findID
          CFI Conditional ??CrossCallReturnLabel_111
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+1056
          CFI Block cfiCond391 Using cfiCommon0
          CFI (cfiCond391) Function user_getTempIDRecordsCount
          CFI (cfiCond391) Conditional ??CrossCallReturnLabel_110
          CFI (cfiCond391) R4 Frame(CFA, -16)
          CFI (cfiCond391) R5 Frame(CFA, -12)
          CFI (cfiCond391) R6 Frame(CFA, -8)
          CFI (cfiCond391) R14 Frame(CFA, -4)
          CFI (cfiCond391) CFA R13+1040
          CFI Block cfiPicker392 Using cfiCommon1
          CFI (cfiPicker392) NoFunction
          CFI (cfiPicker392) Picker
        THUMB
?Subroutine33:
        MOV      R2,#+1024
          CFI Block cfiCond393 Using cfiCommon0
          CFI (cfiCond393) Function user_findTempID
          CFI (cfiCond393) Conditional ??CrossCallReturnLabel_109
          CFI (cfiCond393) R4 Frame(CFA, -16)
          CFI (cfiCond393) R5 Frame(CFA, -12)
          CFI (cfiCond393) R6 Frame(CFA, -8)
          CFI (cfiCond393) R14 Frame(CFA, -4)
          CFI (cfiCond393) CFA R13+48
          CFI Block cfiCond394 Using cfiCommon0
          CFI (cfiCond394) Function log_getRecordsCount
          CFI (cfiCond394) Conditional ??CrossCallReturnLabel_108
          CFI (cfiCond394) R4 Frame(CFA, -28)
          CFI (cfiCond394) R5 Frame(CFA, -24)
          CFI (cfiCond394) R6 Frame(CFA, -20)
          CFI (cfiCond394) R7 Frame(CFA, -16)
          CFI (cfiCond394) R8 Frame(CFA, -12)
          CFI (cfiCond394) R9 Frame(CFA, -8)
          CFI (cfiCond394) R14 Frame(CFA, -4)
          CFI (cfiCond394) CFA R13+1056
          CFI Block cfiCond395 Using cfiCommon0
          CFI (cfiCond395) Function log_readRecords
          CFI (cfiCond395) Conditional ??CrossCallReturnLabel_107
          CFI (cfiCond395) R4 Frame(CFA, -32)
          CFI (cfiCond395) R5 Frame(CFA, -28)
          CFI (cfiCond395) R6 Frame(CFA, -24)
          CFI (cfiCond395) R7 Frame(CFA, -20)
          CFI (cfiCond395) R8 Frame(CFA, -16)
          CFI (cfiCond395) R9 Frame(CFA, -12)
          CFI (cfiCond395) R10 Frame(CFA, -8)
          CFI (cfiCond395) R14 Frame(CFA, -4)
          CFI (cfiCond395) CFA R13+64
??Subroutine33_0:
        MOV      R1,SP
          CFI Block cfiCond396 Using cfiCommon0
          CFI (cfiCond396) Function log_addEvent
          CFI (cfiCond396) Conditional ??CrossCallReturnLabel_106
          CFI (cfiCond396) R4 Frame(CFA, -20)
          CFI (cfiCond396) R5 Frame(CFA, -16)
          CFI (cfiCond396) R6 Frame(CFA, -12)
          CFI (cfiCond396) R7 Frame(CFA, -8)
          CFI (cfiCond396) R14 Frame(CFA, -4)
          CFI (cfiCond396) CFA R13+64
          CFI Block cfiCond397 Using cfiCommon0
          CFI (cfiCond397) Function log_addEvent
          CFI (cfiCond397) Conditional ??CrossCallReturnLabel_105
          CFI (cfiCond397) R4 Frame(CFA, -20)
          CFI (cfiCond397) R5 Frame(CFA, -16)
          CFI (cfiCond397) R6 Frame(CFA, -12)
          CFI (cfiCond397) R7 Frame(CFA, -8)
          CFI (cfiCond397) R14 Frame(CFA, -4)
          CFI (cfiCond397) CFA R13+64
??Subroutine33_1:
        MOV      R0,R6
          CFI (cfiCond390) FunCall user_findID mem_read
          CFI (cfiCond391) FunCall user_getTempIDRecordsCount mem_read
          CFI (cfiCond393) FunCall user_findTempID mem_read
          CFI (cfiCond394) FunCall log_getRecordsCount mem_read
          CFI (cfiCond395) FunCall log_readRecords mem_read
          CFI (cfiCond396) FunCall log_addEvent mem_read
          CFI (cfiCond396) FunCall log_addEvent mem_read
        B.W      mem_read
          CFI EndBlock cfiCond390
          CFI EndBlock cfiCond391
          CFI EndBlock cfiPicker392
          CFI EndBlock cfiCond393
          CFI EndBlock cfiCond394
          CFI EndBlock cfiCond395
          CFI EndBlock cfiCond396
          CFI EndBlock cfiCond397

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
log_pending:
        DS8 512

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
log_pendingCount:
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
log_timeKnown:
        DS8 1

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock398 Using cfiCommon0
          CFI Function user_storage_background_ready
        THUMB
user_storage_background_ready:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI FunCall user_storage_command_ready
        BL       user_storage_command_ready
        CBZ.N    R0,??user_storage_background_ready_0
        LDR.W    R0,??DataTable166_1
        LDRB     R0,[R0, #+0]
        CBNZ.N   R0,??user_storage_background_ready_0
        LDR.W    R0,??DataTable166_2
        LDRB     R0,[R0, #+0]
        CBZ.N    R0,??user_storage_background_ready_1
        LDR.W    R0,??DataTable170
        LDRB     R0,[R0, #+0]
        CBNZ.N   R0,??user_storage_background_ready_0
        BL       ?Subroutine12
??CrossCallReturnLabel_18:
        SBCS     R0,R0,R0
        LSRS     R0,R0,#+31
        POP      {R1,PC}
??user_storage_background_ready_1:
        MOVS     R0,#+1
        POP      {R1,PC}
??user_storage_background_ready_0:
        MOVS     R0,#+0
        POP      {R1,PC}
          CFI EndBlock cfiBlock398

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable145:
        DATA32
        DC32     ?_112

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
fw_job:
        DS8 40

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock399 Using cfiCommon0
          CFI Function fw_async_active
          CFI NoCalls
        THUMB
fw_async_active:
        LDR.W    R0,??DataTable166_3
        LDRB     R0,[R0, #+32]
          CFI EndBlock cfiBlock399
        REQUIRE ?Subroutine0

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock400 Using cfiCommon0
          CFI NoFunction
        THUMB
?Subroutine0:
        CBZ.N    R0,??Subroutine0_0
        MOVS     R0,#+1
??Subroutine0_0:
        BX       LR
          CFI EndBlock cfiBlock400

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable146:
        DATA32
        DC32     ?_113

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable146_1:
        DATA32
        DC32     ?_114

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock401 Using cfiCommon0
          CFI Function fw_async_cancel
        THUMB
fw_async_cancel:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI FunCall fw_async_active
        BL       fw_async_active
        CBZ.N    R0,??fw_async_cancel_0
        LDR.W    R0,??DataTable166_3
        MOVS     R1,#+1
        STRB     R1,[R0, #+36]
        STRB     R1,[R0, #+37]
??fw_async_cancel_0:
        POP      {R0,PC}
          CFI EndBlock cfiBlock401

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable147:
        DATA32
        DC32     ?_115

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock402 Using cfiCommon0
          CFI Function fw_async_quiet
        THUMB
fw_async_quiet:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        MOV      R4,R0
          CFI FunCall fw_async_active
        BL       fw_async_active
        CBZ.N    R0,??fw_async_quiet_0
        LDR.W    R0,??DataTable166_3
        LDRB     R1,[R0, #+34]
        CMP      R1,R4
        ITT      EQ
        MOVEQ    R1,#+1
        STRBEQ   R1,[R0, #+36]
??fw_async_quiet_0:
        POP      {R4,PC}
          CFI EndBlock cfiBlock402

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock403 Using cfiCommon0
          CFI Function fw_async_reply
          CFI NoCalls
        THUMB
fw_async_reply:
        LDR.W    R1,??DataTable166_3
        STRB     R0,[R1, #+35]
        BX       LR
          CFI EndBlock cfiBlock403

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable149:
        DATA32
        DC32     ?_116

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable149_1:
        DATA32
        DC32     ?_117

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable149_2:
        DATA32
        DC32     ?_118

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable149_3:
        DATA32
        DC32     ?_119

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable149_4:
        DATA32
        DC32     ?_120

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable149_5:
        DATA32
        DC32     ?_121

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock404 Using cfiCommon0
          CFI Function fw_async_begin
        THUMB
fw_async_begin:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        MOV      R4,R0
        MOV      R6,R1
        MOV      R5,R2
          CFI FunCall photo_storage_busy
        BL       photo_storage_busy
        CBNZ.N   R0,??fw_async_begin_0
          CFI FunCall table_runtime_active
        BL       table_runtime_active
        CBNZ.N   R0,??fw_async_begin_0
          CFI FunCall fw_async_active
        BL       fw_async_active
        CBNZ.N   R0,??fw_async_begin_0
          CFI FunCall log_async_active
        BL       log_async_active
        CMP      R0,#+0
        ITTT     EQ
        LDREQ.W  R0,??DataTable163_1
        LDRBEQ   R0,[R0, #+18]
        CMPEQ    R0,#+0
        BNE.N    ??fw_async_begin_0
          CFI FunCall mem_config_result
        BL       mem_config_result
        CBNZ.N   R0,??fw_async_begin_0
          CFI FunCall w25_async_active
        BL       w25_async_active
        CBNZ.N   R0,??fw_async_begin_0
        LDR.W    R0,??DataTable160_1
        LDR      R1,[R0, #+0]
        ADD      R1,R1,#+512
        CMP      R6,R1
        BCC.N    ??fw_async_begin_0
        LDR      R1,[R0, #+0]
        LDR      R0,[R0, #+4]
        ADDS     R1,R0,R1
        SUB      R1,R1,#+1024
        CMP      R1,R6
        BCC.N    ??fw_async_begin_0
        LDR.W    R7,??DataTable166_3
        MOV      R0,#+1024
          CFI FunCall malloc
        BL       malloc
        STR      R0,[R7, #+0]
        CBNZ.N   R0,??fw_async_begin_1
??fw_async_begin_0:
        MOVS     R0,#+0
        POP      {R1,R4-R7,PC}
??fw_async_begin_1:
        MOV      R2,#+1024
        MOV      R1,R4
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        STR      R6,[R7, #+4]
        LDR.W    R0,??DataTable163_2
        LDRH     R0,[R0, #+172]
        STRH     R0,[R7, #+30]
        MOVS     R0,#+0
        STRH     R0,[R7, #+28]
        STRH     R0,[R7, #+36]
        STRB     R0,[R7, #+33]
        STRB     R5,[R7, #+34]
        LDR.W    R0,??DataTable175
        LDR      R0,[R0, #+0]
        STR      R0,[R7, #+24]
        ADDW     R0,R6,#+4095
        LSRS     R0,R0,#+12
        LSLS     R0,R0,#+12
        ADD      R6,R6,#+1024
        CMP      R0,R6
        ITE      CC
        MOVCC    R0,#+1
        MOVCS    R0,#+2
        STRB     R0,[R7, #+32]
        MOVS     R0,#+1
        POP      {R1,R4-R7,PC}
          CFI EndBlock cfiBlock404

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable150:
        DATA32
        DC32     ?_122

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock405 Using cfiCommon0
          CFI Function fw_async_complete
        THUMB
fw_async_complete:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R4,R0
        LDR.W    R5,??DataTable166_3
        LDR      R0,[R5, #+0]
          CFI FunCall free
        BL       free
        MOVS     R0,#+0
        STR      R0,[R5, #+0]
        STRB     R4,[R5, #+38]
        MOVS     R0,#+5
        STRB     R0,[R5, #+32]
        POP      {R0,R4,R5,PC}
          CFI EndBlock cfiBlock405

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable151:
        DATA32
        DC32     ?_123

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable151_1:
        DATA32
        DC32     ?_124

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable151_2:
        DATA32
        DC32     0x5f5e1000

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable151_3:
        DATA32
        DC32     ?_125

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable151_4:
        DATA32
        DC32     ?_126

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock406 Using cfiCommon0
          CFI Function fw_storage_poll
        THUMB
fw_storage_poll:
        PUSH     {R3-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+20
        SUB      SP,SP,#+36
          CFI CFA R13+56
        MOV      R5,R0
        LDR.W    R4,??DataTable166_3
        LDRB     R0,[R4, #+34]
        CBNZ.N   R0,??fw_storage_poll_0
        LDR      R0,[R4, #+24]
        LDR.W    R1,??DataTable175
        LDR      R1,[R1, #+0]
        CMP      R0,R1
        ITT      NE
        MOVNE    R0,#+1
        STRBNE   R0,[R4, #+36]
??fw_storage_poll_0:
        LDRB     R0,[R4, #+37]
        CBZ.N    R0,??fw_storage_poll_1
        LDRB     R0,[R4, #+33]
        CBZ.N    R0,??fw_storage_poll_2
        BL       ?Subroutine41
??CrossCallReturnLabel_146:
        CMP      R0,#+1
        BEQ.N    ??fw_storage_poll_3
??fw_storage_poll_2:
        LDR      R0,[R4, #+0]
          CFI FunCall free
        BL       free
        MOVS     R0,#+0
        STR      R0,[R4, #+0]
        STRH     R0,[R4, #+32]
??fw_storage_poll_3:
        B.N      ??fw_storage_poll_4
??fw_storage_poll_1:
        LDRB     R0,[R4, #+32]
        CMP      R0,#+5
        BNE.N    ??fw_storage_poll_5
        LDRB     R0,[R4, #+36]
        CMP      R0,#+0
        ITT      EQ
        LDRBEQ   R0,[R4, #+34]
        CMPEQ    R0,#+1
        BNE.N    ??fw_storage_poll_6
          CFI FunCall comm_tx_busy
        BL       comm_tx_busy
        CMP      R0,#+0
        BNE.N    ??fw_storage_poll_3
??fw_storage_poll_6:
        MOVS     R0,#+0
        STRB     R0,[R4, #+32]
        LDRB     R0,[R4, #+36]
        CMP      R0,#+0
        BNE.N    ??fw_storage_poll_7
        LDRB     R3,[R4, #+35]
        LDRB     R2,[R4, #+34]
        LDRB     R1,[R4, #+38]
        MOVS     R0,#+20
          CFI FunCall user_send_statusUpdateFW
        BL       user_send_statusUpdateFW
??fw_storage_poll_7:
        B.N      ??fw_storage_poll_4
??fw_storage_poll_5:
        LDRB     R0,[R4, #+33]
        CBZ.N    R0,??fw_storage_poll_8
        BL       ?Subroutine41
??CrossCallReturnLabel_145:
        CMP      R0,#+1
        BEQ.N    ??fw_storage_poll_7
        MOVS     R1,#+0
        STRB     R1,[R4, #+33]
        CMP      R0,#+2
        BNE.W    ??fw_storage_poll_9
        LDRB     R0,[R4, #+32]
        CMP      R0,#+1
        IT       EQ
        MOVEQ    R0,#+2
        BEQ.N    ??fw_storage_poll_10
        LDRH     R0,[R4, #+28]
        ADDS     R0,R0,#+64
        STRH     R0,[R4, #+28]
??fw_storage_poll_11:
        B.N      ??fw_storage_poll_4
??fw_storage_poll_8:
          CFI FunCall w25_read_available
        BL       w25_read_available
        CMP      R0,#+0
        BEQ.N    ??fw_storage_poll_11
        LDRB     R0,[R4, #+32]
        CMP      R0,#+1
        BNE.N    ??fw_storage_poll_12
        BL       ??Subroutine43_0
??CrossCallReturnLabel_152:
        LDR      R0,[R4, #+4]
        ADDW     R0,R0,#+4095
        LSRS     R0,R0,#+12
        LSLS     R0,R0,#+12
        B.N      ??fw_storage_poll_13
??fw_storage_poll_12:
        CMP      R0,#+2
        BNE.N    ??fw_storage_poll_14
        LDRH     R0,[R4, #+28]
        CMP      R0,#+1024
        BCS.N    ??fw_storage_poll_15
        MOV      R3,R5
        MOVS     R2,#+64
        LDR      R1,[R4, #+0]
        ADD      R1,R1,R0
        LDR      R0,[R4, #+4]
        LDRH     R5,[R4, #+28]
        ADDS     R0,R5,R0
??fw_storage_poll_13:
          CFI FunCall w25_async_start
        BL       w25_async_start
        STRB     R0,[R4, #+33]
        CMP      R0,#+0
        BEQ.N    ??fw_storage_poll_9
        B.N      ??fw_storage_poll_4
??fw_storage_poll_15:
        BL       ?Subroutine20
??CrossCallReturnLabel_47:
        LDR.W    R1,??DataTable163_2
        MOVS     R0,#+0
        STR      R0,[R1, #+174]
        LDRH     R0,[R4, #+30]
        ADDS     R0,R0,#+1
        STRH     R0,[R1, #+172]
        LDRH     R2,[R1, #+172]
        MOVS     R0,#+100
        MULS     R2,R0,R2
        LDRH     R0,[R1, #+148]
        UDIV     R0,R2,R0
        STRB     R0,[R1, #+150]
        LDRH     R0,[R1, #+172]
        LDRH     R1,[R1, #+148]
        CMP      R0,R1
        IT       CC
        MOVCC    R0,#+0
        BCC.N    ??fw_storage_poll_16
        MOVS     R0,#+3
??fw_storage_poll_10:
        B.N      ??fw_storage_poll_17
??fw_storage_poll_14:
        CMP      R0,#+3
        BNE.N    ??fw_storage_poll_18
        LDR.W    R5,??DataTable160_1
        BL       ?Subroutine44
??CrossCallReturnLabel_154:
        CMP      R0,#+0
        BEQ.N    ??fw_storage_poll_19
        LDR      R0,[SP, #+12]
        CBZ.N    R0,??fw_storage_poll_20
        LDRB     R0,[SP, #+12]
        TST      R0,#0x3
        BNE.N    ??fw_storage_poll_20
        LDR.W    R0,??DataTable163_2
        LDRH     R1,[R0, #+148]
        LSLS     R1,R1,#+10
        LDR      R0,[SP, #+12]
        CMP      R1,R0
        ITTTT    CS
        LDRCS    R0,[R5, #+4]
        SUBCS    R0,R0,#+512
        LDRCS    R1,[SP, #+12]
        CMPCS    R0,R1
        BCS.N    ??fw_storage_poll_21
??fw_storage_poll_20:
        MOVS     R0,#+0
        B.N      ??fw_storage_poll_22
??fw_storage_poll_21:
        STR      R1,[R4, #+12]
        LDR      R0,[SP, #+16]
        STR      R0,[R4, #+20]
        LDR      R0,[R5, #+0]
        ADD      R0,R0,#+512
        STR      R0,[R4, #+8]
        MOV      R0,#+4294967295
        STR      R0,[R4, #+16]
        MOVS     R0,#+4
        B.N      ??fw_storage_poll_17
??fw_storage_poll_18:
        CMP      R0,#+4
        BNE.N    ??fw_storage_poll_4
        LDR      R0,[R4, #+12]
        CMP      R0,#+65
        ITE      CS
        MOVCS    R5,#+64
        LDRHCC   R5,[R4, #+12]
        MOV      R2,R5
        LDR      R1,[R4, #+0]
        LDR      R0,[R4, #+8]
          CFI FunCall mem_read
        BL       mem_read
        CMP      R0,#+0
        BEQ.N    ??fw_storage_poll_19
        MOVS     R6,#+0
        B.N      ??fw_storage_poll_23
??fw_storage_poll_24:
        LDR      R0,[R4, #+0]
        LDR      R3,[R0, R6]
        STR      R3,[SP, #+0]
        MOV      R1,R3
        LDR      R0,[R4, #+16]
          CFI FunCall CRC32
        BL       CRC32
        STR      R0,[R4, #+16]
        ADDS     R6,R6,#+4
        UXTH     R6,R6
??fw_storage_poll_23:
        CMP      R6,R5
        BLT.N    ??fw_storage_poll_24
        LDR      R0,[R4, #+8]
        ADDS     R0,R5,R0
        STR      R0,[R4, #+8]
        LDR      R0,[R4, #+12]
        SUBS     R5,R0,R5
        STR      R5,[R4, #+12]
        BNE.N    ??fw_storage_poll_4
        LDR      R0,[R4, #+16]
        LDR      R1,[R4, #+20]
        CMP      R0,R1
        ITE      EQ
        MOVEQ    R0,#+1
        MOVNE    R0,#+0
??fw_storage_poll_22:
          CFI FunCall fw_blocks_done
        BL       fw_blocks_done
??fw_storage_poll_16:
          CFI FunCall fw_async_complete
        BL       fw_async_complete
        B.N      ??fw_storage_poll_4
??fw_storage_poll_9:
        LDR.W    R0,??DataTable180
        LDRB     R1,[R0, #+0]
        ADDS     R1,R1,#+1
        UXTB     R2,R1
        CMP      R2,#+3
        ITE      CC
        MOVCC    R5,#+254
        MOVCS    R5,#+253
        STRB     R1,[R0, #+0]
        MOV      R0,R5
          CFI FunCall fw_async_complete
        BL       fw_async_complete
        CMP      R5,#+253
        BNE.N    ??fw_storage_poll_4
        MOVS     R0,#+0
        STRB     R0,[R4, #+32]
        LDR.W    R0,??DataTable181
        B.N      ??fw_storage_poll_25
??fw_storage_poll_19:
        MOVS     R0,#+253
          CFI FunCall fw_async_complete
        BL       fw_async_complete
        MOVS     R0,#+0
        STRB     R0,[R4, #+32]
        LDR.W    R0,??DataTable185
??fw_storage_poll_25:
          CFI FunCall fw_abort
        BL       fw_abort
        MOVS     R0,#+5
??fw_storage_poll_17:
        STRB     R0,[R4, #+32]
??fw_storage_poll_4:
        ADD      SP,SP,#+40
          CFI CFA R13+16
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock406

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond407 Using cfiCommon0
          CFI Function fw_take_block
          CFI Conditional ??CrossCallReturnLabel_155
          CFI R4 Frame(CFA, -24)
          CFI R5 Frame(CFA, -20)
          CFI R6 Frame(CFA, -16)
          CFI R7 Frame(CFA, -12)
          CFI R8 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+64
          CFI Block cfiCond408 Using cfiCommon0
          CFI (cfiCond408) Function fw_storage_poll
          CFI (cfiCond408) Conditional ??CrossCallReturnLabel_154
          CFI (cfiCond408) R4 Frame(CFA, -16)
          CFI (cfiCond408) R5 Frame(CFA, -12)
          CFI (cfiCond408) R6 Frame(CFA, -8)
          CFI (cfiCond408) R14 Frame(CFA, -4)
          CFI (cfiCond408) CFA R13+56
          CFI Block cfiPicker409 Using cfiCommon1
          CFI (cfiPicker409) NoFunction
          CFI (cfiPicker409) Picker
        THUMB
?Subroutine44:
        MOVS     R2,#+32
        ADD      R1,SP,#+4
        LDR      R0,[R5, #+0]
          CFI (cfiCond407) FunCall fw_take_block mem_read
          CFI (cfiCond408) FunCall fw_storage_poll mem_read
        B.W      mem_read
          CFI EndBlock cfiCond407
          CFI EndBlock cfiCond408
          CFI EndBlock cfiPicker409

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
log_job:
        DS8 48

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock410 Using cfiCommon0
          CFI Function log_async_active
          CFI NoCalls
        THUMB
log_async_active:
        LDR.W    R0,??DataTable182
        LDRB     R0,[R0, #+44]
        B.N      ?Subroutine0
          CFI EndBlock cfiBlock410

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock411 Using cfiCommon0
          CFI Function log_async_begin
        THUMB
log_async_begin:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        MOV      R1,R0
        LDR.W    R4,??DataTable182
        MOV      R0,R4
        MOVS     R2,#+32
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        LDR.W    R0,??DataTable163_2
        LDR      R0,[R0, #+91]
        LDR.W    R1,??DataTable160_1
        LDR      R1,[R1, #+56]
        CMP      R1,R0
        IT       CC
        MOVCC    R1,R0
        STR      R1,[R4, #+32]
        LDR.W    R0,??DataTable165
        LDR      R0,[R0, #+0]
        STR      R0,[R4, #+36]
        MOVS     R0,#+1
        STRB     R0,[R4, #+44]
        MOVS     R0,#+0
        STRB     R0,[R4, #+45]
        MOVS     R0,#+1
        STRB     R0,[R4, #+46]
        POP      {R4,PC}
          CFI EndBlock cfiBlock411

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable154:
        DATA32
        DC32     server_out_event

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable154_1:
        DATA32
        DC32     id_cells_end

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable154_2:
        DATA32
        DC32     id_endScan

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable154_3:
        DATA32
        DC32     id_nextEnd

        SECTION `.text`:CODE:NOROOT(2)
          CFI Block cfiBlock412 Using cfiCommon0
          CFI Function log_async_step
        THUMB
log_async_step:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        SUB      SP,SP,#+40
          CFI CFA R13+56
        MOV      R5,R0
        LDR.W    R0,??DataTable163
        STR      R0,[SP, #+36]
        LDR.W    R4,??DataTable182
        LDRB     R0,[R4, #+44]
        CBZ.N    R0,??log_async_step_1
        LDRB     R0,[R4, #+46]
        CMP      R0,#+3
        BNE.N    ??log_async_step_2
        LDR      R0,[R4, #+40]
        SUBS     R0,R5,R0
        BPL.N    ??log_async_step_2
        MOVS     R0,#+3
??log_async_step_3:
        ADD      SP,SP,#+40
          CFI CFA R13+16
        POP      {R4-R6,PC}
          CFI CFA R13+56
??log_async_step_2:
        LDRB     R0,[R4, #+45]
        CBZ.N    R0,??log_async_step_4
        BL       ?Subroutine41
??CrossCallReturnLabel_144:
        CMP      R0,#+1
        BEQ.N    ??log_async_step_3
        MOVS     R1,#+0
        STRB     R1,[R4, #+45]
        CMP      R0,#+2
        BNE.W    ??log_async_step_5
        LDRB     R0,[R4, #+44]
        ADDS     R0,R0,#+1
??log_async_step_6:
        STRB     R0,[R4, #+44]
??log_async_step_7:
        MOVS     R0,#+1
??log_async_step_8:
        STRB     R0,[R4, #+46]
        B.N      ??log_async_step_1
??log_async_step_4:
          CFI FunCall w25_read_available
        BL       w25_read_available
        CBNZ.N   R0,??log_async_step_9
??log_async_step_1:
        LDRB     R0,[R4, #+46]
        B.N      ??log_async_step_3
??log_async_step_9:
        LDR.W    R1,??DataTable160_1
        LDR      R0,[R4, #+32]
        LDR      R2,[R1, #+56]
        CMP      R0,R2
        BCC.N    ??log_async_step_10
        LDR      R0,[R1, #+60]
        ADDS     R2,R0,R2
        SUBS     R2,R2,#+32
        LDR      R0,[R4, #+32]
        CMP      R2,R0
??log_async_step_10:
        BCC.W    ??log_async_step_11
        LDRB     R0,[R4, #+44]
        SUBS     R0,R0,#+1
        CMP      R0,#+5
        BHI.N    ??log_async_step_7
        TBB      [PC, R0]
        DATA
??log_async_step_0:
        DC8      0x4,0x1B,0x4A,0x4A
        DC8      0x5E,0x5E
        THUMB
??log_async_step_12:
        B.N      ??log_async_step_7
??log_async_step_13:
        LDR      R0,[R4, #+32]
        LDR      R1,[R1, #+56]
        ADDS     R1,R1,#+32
        CMP      R0,R1
        BCC.N    ??log_async_step_14
        MOVS     R2,#+32
        ADD      R1,SP,#+4
        SUBS     R0,R0,#+32
          CFI FunCall mem_read
        BL       mem_read
        CMP      R0,#+0
        BEQ.N    ??log_async_step_15
        BL       ?Subroutine67
??CrossCallReturnLabel_236:
        CBZ.N    R0,??log_async_step_14
        LDR      R0,[SP, #+4]
        LDR      R1,[R4, #+36]
        CMP      R0,R1
        IT       CC
        MOVCC    R0,R1
        STR      R0,[R4, #+36]
??log_async_step_14:
        MOVS     R0,#+2
        B.N      ??log_async_step_16
??log_async_step_17:
        BL       ?Subroutine45
??CrossCallReturnLabel_157:
        CMP      R0,#+0
        BEQ.N    ??log_async_step_15
        ADD      R0,SP,#+4
          CFI FunCall log_blank
        BL       log_blank
        CBNZ.N   R0,??log_async_step_18
        BL       ?Subroutine67
??CrossCallReturnLabel_235:
        CBZ.N    R0,??log_async_step_19
        LDR      R0,[SP, #+4]
        LDR      R1,[R4, #+36]
        CMP      R0,R1
        IT       CC
        MOVCC    R0,R1
        STR      R0,[R4, #+36]
??log_async_step_19:
        LDR      R0,[R4, #+32]
        ADDS     R0,R0,#+32
        STR      R0,[R4, #+32]
??log_async_step_20:
        B.N      ??log_async_step_7
??log_async_step_18:
        LDR      R0,[R4, #+36]
        CBZ.N    R0,??log_async_step_21
        LDR      R1,[R4, #+0]
        CMP      R0,R1
        ITT      CS
        ADDCS    R0,R0,#+1
        STRCS    R0,[R4, #+0]
??log_async_step_21:
        MOVS     R1,#+24
        BL       ??Subroutine73_0
??CrossCallReturnLabel_257:
        BL       ?Subroutine66
??CrossCallReturnLabel_232:
        ADD      R0,R4,#+24
        BL       ?Subroutine16
??CrossCallReturnLabel_32:
        ADD      R0,R4,#+26
        BL       ?Subroutine14
??CrossCallReturnLabel_28:
        ADD      R2,SP,#+36
        ADD      R0,R4,#+28
        LDR      R1,[R2, #+0]
        STR      R1,[R0, #+0]
        B.N      ??log_async_step_22
??log_async_step_23:
        LDRB     R0,[R4, #+44]
        CMP      R0,#+3
        ITTEE    EQ
        MOVEQ    R2,#+28
        MOVEQ    R1,#+0
        MOVNE    R2,#+4
        MOVNE    R1,#+28
        MOV      R0,R1
        MOV      R3,R5
        ADD      R1,R4,R1
        LDR      R6,[R4, #+32]
        ADDS     R0,R0,R6
          CFI FunCall w25_async_start
        BL       w25_async_start
        STRB     R0,[R4, #+45]
        CMP      R0,#+0
        BNE.N    ??log_async_step_20
        B.N      ??log_async_step_5
??log_async_step_24:
        BL       ?Subroutine45
??CrossCallReturnLabel_156:
        CMP      R0,#+0
??log_async_step_15:
        BEQ.N    ??log_async_step_25
        MOVS     R2,#+32
        MOV      R1,R4
        ADD      R0,SP,#+4
          CFI FunCall memcmp
        BL       memcmp
        CBNZ.N   R0,??log_async_step_26
        BL       ?Subroutine67
??CrossCallReturnLabel_234:
        CBZ.N    R0,??log_async_step_26
        LDR      R0,[R4, #+0]
        LDR.W    R1,??DataTable164
        STR      R0,[R1, #+0]
        LDR.W    R0,??DataTable163_2
        LDR      R1,[R4, #+32]
        ADDS     R1,R1,#+32
        STR      R1,[R0, #+91]
        LDRH     R1,[R0, #+129]
        ORR      R1,R1,#0x1000
        STRH     R1,[R0, #+129]
        MOVS     R0,#+0
        STRB     R0,[R4, #+44]
        MOVS     R0,#+2
        B.N      ??log_async_step_8
??log_async_step_26:
        LDRB     R0,[R4, #+44]
        CMP      R0,#+5
        BNE.N    ??log_async_step_27
??log_async_step_5:
        MOVS     R0,#+6
        B.N      ??log_async_step_28
??log_async_step_27:
        MOVS     R0,#+0
??log_async_step_29:
        ADD      R1,SP,#+4
        LDRB     R2,[R1, R0]
        LDRB     R1,[R4, R0]
        ANDS     R2,R1,R2
        CMP      R2,R1
        BNE.N    ??log_async_step_30
        ADDS     R0,R0,#+1
        CMP      R0,#+32
        BCC.N    ??log_async_step_29
        B.N      ??log_async_step_22
??log_async_step_30:
        BL       ?Subroutine67
??CrossCallReturnLabel_233:
        CBZ.N    R0,??log_async_step_31
        LDR      R0,[SP, #+4]
        LDR      R1,[R4, #+36]
        CMP      R0,R1
        IT       CC
        MOVCC    R0,R1
        STR      R0,[R4, #+36]
??log_async_step_31:
        LDR      R0,[R4, #+32]
        ADDS     R0,R0,#+32
        STR      R0,[R4, #+32]
        MOVS     R0,#+2
        B.N      ??log_async_step_6
??log_async_step_22:
        MOVS     R0,#+3
??log_async_step_16:
        STRB     R0,[R4, #+44]
        B.N      ??log_async_step_7
??log_async_step_25:
        LDRB     R0,[R4, #+44]
        CMP      R0,#+1
        IT       NE
        CMPNE    R0,#+2
        BNE.N    ??log_async_step_32
??log_async_step_11:
        MOVS     R0,#+0
??log_async_step_28:
        STRB     R0,[R4, #+44]
??log_async_step_32:
        ADD      R5,R5,#+1000
        STR      R5,[R4, #+40]
        MOVS     R0,#+3
        B.N      ??log_async_step_8
          CFI EndBlock cfiBlock412

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable155:
        DATA32
        DC32     id_used_bytes

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond413 Using cfiCommon0
          CFI Function log_async_step
          CFI Conditional ??CrossCallReturnLabel_236
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+56
          CFI Block cfiCond414 Using cfiCommon0
          CFI (cfiCond414) Function log_async_step
          CFI (cfiCond414) Conditional ??CrossCallReturnLabel_235
          CFI (cfiCond414) R4 Frame(CFA, -16)
          CFI (cfiCond414) R5 Frame(CFA, -12)
          CFI (cfiCond414) R6 Frame(CFA, -8)
          CFI (cfiCond414) R14 Frame(CFA, -4)
          CFI (cfiCond414) CFA R13+56
          CFI Block cfiCond415 Using cfiCommon0
          CFI (cfiCond415) Function log_async_step
          CFI (cfiCond415) Conditional ??CrossCallReturnLabel_234
          CFI (cfiCond415) R4 Frame(CFA, -16)
          CFI (cfiCond415) R5 Frame(CFA, -12)
          CFI (cfiCond415) R6 Frame(CFA, -8)
          CFI (cfiCond415) R14 Frame(CFA, -4)
          CFI (cfiCond415) CFA R13+56
          CFI Block cfiCond416 Using cfiCommon0
          CFI (cfiCond416) Function log_async_step
          CFI (cfiCond416) Conditional ??CrossCallReturnLabel_233
          CFI (cfiCond416) R4 Frame(CFA, -16)
          CFI (cfiCond416) R5 Frame(CFA, -12)
          CFI (cfiCond416) R6 Frame(CFA, -8)
          CFI (cfiCond416) R14 Frame(CFA, -4)
          CFI (cfiCond416) CFA R13+56
          CFI Block cfiPicker417 Using cfiCommon1
          CFI (cfiPicker417) NoFunction
          CFI (cfiPicker417) Picker
        THUMB
?Subroutine67:
        ADD      R0,SP,#+4
          CFI (cfiCond413) FunCall log_async_step log_valid
          CFI (cfiCond413) FunCall log_async_step log_valid
          CFI (cfiCond413) FunCall log_async_step log_valid
          CFI (cfiCond413) FunCall log_async_step log_valid
        B.W      log_valid
          CFI EndBlock cfiCond413
          CFI EndBlock cfiCond414
          CFI EndBlock cfiCond415
          CFI EndBlock cfiCond416
          CFI EndBlock cfiPicker417

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond418 Using cfiCommon0
          CFI Function log_addEvent
          CFI Conditional ??CrossCallReturnLabel_231
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+64
          CFI Block cfiCond419 Using cfiCommon0
          CFI (cfiCond419) Function log_async_step
          CFI (cfiCond419) Conditional ??CrossCallReturnLabel_232
          CFI (cfiCond419) R4 Frame(CFA, -16)
          CFI (cfiCond419) R5 Frame(CFA, -12)
          CFI (cfiCond419) R6 Frame(CFA, -8)
          CFI (cfiCond419) R14 Frame(CFA, -4)
          CFI (cfiCond419) CFA R13+56
          CFI Block cfiPicker420 Using cfiCommon1
          CFI (cfiPicker420) NoFunction
          CFI (cfiPicker420) Picker
        THUMB
?Subroutine66:
        STRH     R0,[SP, #+0]
        MVNS     R0,R0
        STRH     R0,[SP, #+2]
        BX       LR
          CFI EndBlock cfiCond418
          CFI EndBlock cfiCond419
          CFI EndBlock cfiPicker420

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond421 Using cfiCommon0
          CFI Function log_async_step
          CFI Conditional ??CrossCallReturnLabel_157
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+56
          CFI Block cfiCond422 Using cfiCommon0
          CFI (cfiCond422) Function log_async_step
          CFI (cfiCond422) Conditional ??CrossCallReturnLabel_156
          CFI (cfiCond422) R4 Frame(CFA, -16)
          CFI (cfiCond422) R5 Frame(CFA, -12)
          CFI (cfiCond422) R6 Frame(CFA, -8)
          CFI (cfiCond422) R14 Frame(CFA, -4)
          CFI (cfiCond422) CFA R13+56
          CFI Block cfiPicker423 Using cfiCommon1
          CFI (cfiPicker423) NoFunction
          CFI (cfiPicker423) Picker
        THUMB
?Subroutine45:
        MOVS     R2,#+32
        ADD      R1,SP,#+4
        LDR      R0,[R4, #+32]
          CFI (cfiCond421) FunCall log_async_step mem_read
          CFI (cfiCond421) FunCall log_async_step mem_read
        B.W      mem_read
          CFI EndBlock cfiCond421
          CFI EndBlock cfiCond422
          CFI EndBlock cfiPicker423

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond424 Using cfiCommon0
          CFI Function fw_storage_poll
          CFI Conditional ??CrossCallReturnLabel_146
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+56
          CFI Block cfiCond425 Using cfiCommon0
          CFI (cfiCond425) Function fw_storage_poll
          CFI (cfiCond425) Conditional ??CrossCallReturnLabel_145
          CFI (cfiCond425) R4 Frame(CFA, -16)
          CFI (cfiCond425) R5 Frame(CFA, -12)
          CFI (cfiCond425) R6 Frame(CFA, -8)
          CFI (cfiCond425) R14 Frame(CFA, -4)
          CFI (cfiCond425) CFA R13+56
          CFI Block cfiCond426 Using cfiCommon0
          CFI (cfiCond426) Function log_async_step
          CFI (cfiCond426) Conditional ??CrossCallReturnLabel_144
          CFI (cfiCond426) R4 Frame(CFA, -16)
          CFI (cfiCond426) R5 Frame(CFA, -12)
          CFI (cfiCond426) R6 Frame(CFA, -8)
          CFI (cfiCond426) R14 Frame(CFA, -4)
          CFI (cfiCond426) CFA R13+56
          CFI Block cfiCond427 Using cfiCommon0
          CFI (cfiCond427) Function user_storage_poll
          CFI (cfiCond427) Conditional ??CrossCallReturnLabel_143
          CFI (cfiCond427) R4 Frame(CFA, -16)
          CFI (cfiCond427) R5 Frame(CFA, -12)
          CFI (cfiCond427) R6 Frame(CFA, -8)
          CFI (cfiCond427) R14 Frame(CFA, -4)
          CFI (cfiCond427) CFA R13+80
          CFI Block cfiPicker428 Using cfiCommon1
          CFI (cfiPicker428) NoFunction
          CFI (cfiPicker428) Picker
        THUMB
?Subroutine41:
        MOV      R0,R5
          CFI (cfiCond424) FunCall fw_storage_poll w25_async_poll
          CFI (cfiCond424) FunCall fw_storage_poll w25_async_poll
          CFI (cfiCond426) FunCall log_async_step w25_async_poll
          CFI (cfiCond427) FunCall user_storage_poll w25_async_poll
        B.W      w25_async_poll
          CFI EndBlock cfiCond424
          CFI EndBlock cfiCond425
          CFI EndBlock cfiCond426
          CFI EndBlock cfiCond427
          CFI EndBlock cfiPicker428

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond429 Using cfiCommon0
          CFI Function log_addEvent
          CFI Conditional ??CrossCallReturnLabel_31
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+64
          CFI Block cfiCond430 Using cfiCommon0
          CFI (cfiCond430) Function log_async_step
          CFI (cfiCond430) Conditional ??CrossCallReturnLabel_32
          CFI (cfiCond430) R4 Frame(CFA, -16)
          CFI (cfiCond430) R5 Frame(CFA, -12)
          CFI (cfiCond430) R6 Frame(CFA, -8)
          CFI (cfiCond430) R14 Frame(CFA, -4)
          CFI (cfiCond430) CFA R13+56
          CFI Block cfiPicker431 Using cfiCommon1
          CFI (cfiPicker431) NoFunction
          CFI (cfiPicker431) Picker
        THUMB
?Subroutine16:
        LDRB     R2,[SP, #+0]
        STRB     R2,[R0, #+0]
        LDRB     R2,[SP, #+1]
        STRB     R2,[R0, #+1]
        ADD      R3,SP,#+2
        BX       LR
          CFI EndBlock cfiCond429
          CFI EndBlock cfiCond430
          CFI EndBlock cfiPicker431

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock432 Using cfiCommon0
          CFI Function log_storage_poll
        THUMB
log_storage_poll:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R4,R0
          CFI FunCall log_async_active
        BL       log_async_active
        CBNZ.N   R0,??log_storage_poll_0
        LDR.W    R0,??DataTable166_2
        LDRB     R0,[R0, #+0]
        CBZ.N    R0,??log_storage_poll_1
        LDR.W    R1,??DataTable182
        LDRB     R0,[R1, #+46]
        CMP      R0,#+3
        BNE.N    ??log_storage_poll_2
        LDR      R0,[R1, #+40]
        SUBS     R4,R4,R0
        BMI.N    ??log_storage_poll_1
??log_storage_poll_2:
        LDR.W    R0,??DataTable191
        POP      {R1,R4,R5,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall log_async_begin
        B.N      log_async_begin
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
??log_storage_poll_0:
        MOV      R0,R4
          CFI FunCall log_async_step
        BL       log_async_step
        CMP      R0,#+2
        BNE.N    ??log_storage_poll_1
        LDR.W    R4,??DataTable166_2
        LDRB     R0,[R4, #+0]
        SUBS     R5,R0,#+1
        LDR.W    R0,??DataTable191
        LSLS     R2,R5,#+24
        LSRS     R2,R2,#+19
        ADD      R1,R0,#+32
          CFI FunCall __aeabi_memmove4
        BL       __aeabi_memmove4
        STRB     R5,[R4, #+0]
??log_storage_poll_1:
        POP      {R0,R4,R5,PC}
          CFI EndBlock cfiBlock432

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock433 Using cfiCommon0
          CFI Function log_flushPending
        THUMB
log_flushPending:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        BL       ?Subroutine25
??CrossCallReturnLabel_83:
        CMP      R0,#+0
        LDR.W    R4,??DataTable166_2
        LDR.W    R5,??DataTable191
        BEQ.N    ??log_flushPending_0
??log_flushPending_1:
        POP      {R0,R4,R5,PC}
??log_flushPending_2:
        LDRB     R0,[R4, #+0]
        SUBS     R0,R0,#+1
        STRB     R0,[R4, #+0]
        LDRB     R2,[R4, #+0]
        LSLS     R2,R2,#+5
        ADD      R1,R5,#+32
        MOV      R0,R5
          CFI FunCall __aeabi_memmove4
        BL       __aeabi_memmove4
??log_flushPending_0:
        LDRB     R0,[R4, #+0]
        CMP      R0,#+0
        BEQ.N    ??log_flushPending_1
        MOV      R0,R5
          CFI FunCall log_addEvent
        BL       log_addEvent
        CMP      R0,#+0
        BNE.N    ??log_flushPending_2
        POP      {R0,R4,R5,PC}
          CFI EndBlock cfiBlock433

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable157:
        DATA32
        DC32     ?_127

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable157_1:
        DATA32
        DC32     ?_128

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock434 Using cfiCommon0
          CFI Function log_setTime
        THUMB
log_setTime:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        MOV      R6,R0
        LDR.N    R4,??DataTable163_2
        STR      R6,[R4, #+47]
        MOVS     R1,#+1
        LDR.W    R0,??DataTable190
        STRB     R1,[R0, #+0]
        LDR.W    R5,??DataTable170
        LDRB     R0,[R5, #+0]
        CBNZ.N   R0,??log_setTime_0
          CFI FunCall log_async_active
        BL       log_async_active
        MOVS     R2,R0
        BEQ.N    ??log_setTime_1
        MOVS     R2,#+1
        B.N      ??log_setTime_1
??log_setTime_2:
        LDR.W    R1,??DataTable191
        LDR      R3,[R4, #+51]
        SUBS     R3,R6,R3
        ADD      R0,R1,R2, LSL #+5
        LDR      R0,[R0, #+4]
        ADDS     R3,R0,R3
        ADD      R0,R1,R2, LSL #+5
        STR      R3,[R0, #+4]
        LSLS     R0,R2,#+5
        STR      R3,[R1, R0]
        ADDS     R2,R2,#+1
        UXTB     R2,R2
??log_setTime_1:
        LDR.W    R0,??DataTable166_2
        LDRB     R0,[R0, #+0]
        CMP      R2,R0
        BLT.N    ??log_setTime_2
??log_setTime_0:
        MOVS     R0,#+1
        STRB     R0,[R5, #+0]
        POP      {R4-R6,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall log_flushPending
        B.N      log_flushPending
          CFI EndBlock cfiBlock434

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable158:
        DATA32
        DC32     ?_129

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable158_1:
        DATA32
        DC32     ?_130

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock435 Using cfiCommon0
          CFI Function log_checkPending
        THUMB
log_checkPending:
        LDR.W    R0,??DataTable166_2
        LDRB     R0,[R0, #+0]
        CBZ.N    R0,??log_checkPending_0
        LDR.W    R0,??DataTable170
        LDRB     R0,[R0, #+0]
        CBNZ.N   R0,??log_checkPending_1
        LDR.N    R0,??DataTable163_2
        LDR      R0,[R0, #+51]
        CMP      R0,#+600
        BCC.N    ??log_checkPending_0
??log_checkPending_1:
          CFI FunCall log_flushPending
        B.N      log_flushPending
??log_checkPending_0:
        BX       LR
          CFI EndBlock cfiBlock435

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock436 Using cfiCommon0
          CFI Function log_put
        THUMB
log_put:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        MOV      R4,R0
        LDR.W    R5,??DataTable192
        LDRB     R0,[R5, #+0]
        CBNZ.N   R0,??log_put_0
        LDR.W    R0,??DataTable170
        LDRB     R0,[R0, #+0]
        CBNZ.N   R0,??log_put_1
        LDR.N    R0,??DataTable163_2
        LDR      R0,[R0, #+51]
        CMP      R0,#+600
        BCC.N    ??log_put_2
??log_put_1:
          CFI FunCall log_flushPending
        BL       log_flushPending
        LDR.W    R0,??DataTable166_2
        LDRB     R0,[R0, #+0]
        CBNZ.N   R0,??log_put_0
        MOV      R0,R4
          CFI FunCall log_addEvent
        BL       log_addEvent
        CBNZ.N   R0,??log_put_3
??log_put_0:
        LDRB     R0,[R5, #+0]
        CBNZ.N   R0,??log_put_4
??log_put_2:
        LDR.W    R0,??DataTable166_2
        LDRB     R0,[R0, #+0]
        CMP      R0,#+16
          CFI FunCall log_flushPending
        IT       EQ
        BLEQ     log_flushPending
??log_put_4:
        LDR.N    R5,??DataTable166_2
        LDRB     R0,[R5, #+0]
        CMP      R0,#+16
        BNE.N    ??log_put_5
        MOVS     R0,#+0
        POP      {R4-R6,PC}
??log_put_5:
        LDR.W    R6,??DataTable191
        ADD      R0,R6,R0, LSL #+5
        MOV      R1,R4
        MOVS     R2,#+32
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        LDR.W    R0,??DataTable170
        LDRB     R0,[R0, #+0]
        CBNZ.N   R0,??log_put_6
        LDRB     R0,[R5, #+0]
        ADD      R0,R6,R0, LSL #+5
        LDR.N    R1,??DataTable163_2
        LDR      R1,[R1, #+51]
        STR      R1,[R0, #+4]
??log_put_6:
        LDRB     R0,[R5, #+0]
        ADDS     R0,R0,#+1
        STRB     R0,[R5, #+0]
??log_put_3:
        MOVS     R0,#+1
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock436

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable160:
        DATA32
        DC32     ?_131

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable160_1:
        DATA32
        DC32     mem

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock437 Using cfiCommon0
          CFI Function log_addSystemEvent
          CFI NoCalls
        THUMB
log_addSystemEvent:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        SUB      SP,SP,#+32
          CFI CFA R13+40
        MOV      R4,R0
        BL       ?Subroutine31
??CrossCallReturnLabel_97:
        BL       ?Subroutine9
??CrossCallReturnLabel_10:
        STRB     R4,[SP, #+8]
        BL       ?Subroutine32
??CrossCallReturnLabel_104:
        ADD      SP,SP,#+32
          CFI CFA R13+8
        POP      {R4,PC}
          CFI EndBlock cfiBlock437

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock438 Using cfiCommon0
          CFI Function log_addSystemValue
        THUMB
log_addSystemValue:
        PUSH     {R1-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+24
        MOV      R4,R0
        MOV      R2,R1
        MOV      R5,SP
        MOVS     R0,#+0
        MOVS     R1,#+0
        MOVS     R3,#+0
        STM      R5,{R0,R1,R3}
        ADR.N    R1,??DataTable166
        MOV      R0,SP
          CFI FunCall sprintf
        BL       sprintf
        MOV      R1,SP
        MOV      R0,R4
          CFI FunCall log_addSystemEventID
        BL       log_addSystemEventID
        POP      {R1-R5,PC}
          CFI EndBlock cfiBlock438

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock439 Using cfiCommon0
          CFI Function log_addSystemEventID
          CFI NoCalls
        THUMB
log_addSystemEventID:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        SUB      SP,SP,#+32
          CFI CFA R13+48
        MOV      R5,R0
        MOV      R4,R1
        BL       ?Subroutine31
??CrossCallReturnLabel_98:
        BL       ?Subroutine9
??CrossCallReturnLabel_11:
        STRB     R5,[SP, #+8]
        CBZ.N    R4,??CrossCallReturnLabel_89
        BL       ?Subroutine28
??CrossCallReturnLabel_89:
        BL       ?Subroutine32
??CrossCallReturnLabel_103:
        ADD      SP,SP,#+36
          CFI CFA R13+12
        POP      {R4,R5,PC}
          CFI EndBlock cfiBlock439

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond440 Using cfiCommon0
          CFI Function user_addIdToTempBuffer
          CFI Conditional ??CrossCallReturnLabel_96
          CFI R4 Frame(CFA, -28)
          CFI R5 Frame(CFA, -24)
          CFI R6 Frame(CFA, -20)
          CFI R7 Frame(CFA, -16)
          CFI R8 Frame(CFA, -12)
          CFI R9 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+64
          CFI Block cfiCond441 Using cfiCommon0
          CFI (cfiCond441) Function log_addSystemEvent
          CFI (cfiCond441) Conditional ??CrossCallReturnLabel_97
          CFI (cfiCond441) R4 Frame(CFA, -8)
          CFI (cfiCond441) R14 Frame(CFA, -4)
          CFI (cfiCond441) CFA R13+40
          CFI Block cfiCond442 Using cfiCommon0
          CFI (cfiCond442) Function log_addSystemEventID
          CFI (cfiCond442) Conditional ??CrossCallReturnLabel_98
          CFI (cfiCond442) R4 Frame(CFA, -12)
          CFI (cfiCond442) R5 Frame(CFA, -8)
          CFI (cfiCond442) R14 Frame(CFA, -4)
          CFI (cfiCond442) CFA R13+48
          CFI Block cfiCond443 Using cfiCommon0
          CFI (cfiCond443) Function log_addControlEvent
          CFI (cfiCond443) Conditional ??CrossCallReturnLabel_99
          CFI (cfiCond443) R4 Frame(CFA, -16)
          CFI (cfiCond443) R5 Frame(CFA, -12)
          CFI (cfiCond443) R6 Frame(CFA, -8)
          CFI (cfiCond443) R14 Frame(CFA, -4)
          CFI (cfiCond443) CFA R13+48
          CFI Block cfiCond444 Using cfiCommon0
          CFI (cfiCond444) Function log_addPhotoEvent
          CFI (cfiCond444) Conditional ??CrossCallReturnLabel_100
          CFI (cfiCond444) R14 Frame(CFA, -4)
          CFI (cfiCond444) CFA R13+40
          CFI Block cfiPicker445 Using cfiCommon1
          CFI (cfiPicker445) NoFunction
          CFI (cfiPicker445) Picker
        THUMB
?Subroutine31:
        MOVS     R1,#+32
        MOV      R0,SP
          CFI (cfiCond440) FunCall user_addIdToTempBuffer __aeabi_memclr4
          CFI (cfiCond441) FunCall log_addSystemEvent __aeabi_memclr4
          CFI (cfiCond442) FunCall log_addSystemEventID __aeabi_memclr4
          CFI (cfiCond443) FunCall log_addControlEvent __aeabi_memclr4
          CFI (cfiCond444) FunCall log_addPhotoEvent __aeabi_memclr4
        B.W      __aeabi_memclr4
          CFI EndBlock cfiCond440
          CFI EndBlock cfiCond441
          CFI EndBlock cfiCond442
          CFI EndBlock cfiCond443
          CFI EndBlock cfiCond444
          CFI EndBlock cfiPicker445

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock446 Using cfiCommon0
          CFI Function log_addControlEvent
          CFI NoCalls
        THUMB
log_addControlEvent:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        SUB      SP,SP,#+32
          CFI CFA R13+48
        MOV      R6,R0
        MOV      R4,R1
        MOV      R5,R2
        BL       ?Subroutine31
??CrossCallReturnLabel_99:
        BL       ?Subroutine9
??CrossCallReturnLabel_12:
        STRB     R6,[SP, #+8]
        STRB     R5,[SP, #+23]
        CBZ.N    R4,??CrossCallReturnLabel_90
        BL       ?Subroutine28
??CrossCallReturnLabel_90:
        BL       ?Subroutine32
??CrossCallReturnLabel_102:
        ADD      SP,SP,#+32
          CFI CFA R13+16
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock446

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond447 Using cfiCommon0
          CFI Function log_addSystemEvent
          CFI Conditional ??CrossCallReturnLabel_104
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+40
          CFI Block cfiCond448 Using cfiCommon0
          CFI (cfiCond448) Function log_addSystemEventID
          CFI (cfiCond448) Conditional ??CrossCallReturnLabel_103
          CFI (cfiCond448) R4 Frame(CFA, -12)
          CFI (cfiCond448) R5 Frame(CFA, -8)
          CFI (cfiCond448) R14 Frame(CFA, -4)
          CFI (cfiCond448) CFA R13+48
          CFI Block cfiCond449 Using cfiCommon0
          CFI (cfiCond449) Function log_addControlEvent
          CFI (cfiCond449) Conditional ??CrossCallReturnLabel_102
          CFI (cfiCond449) R4 Frame(CFA, -16)
          CFI (cfiCond449) R5 Frame(CFA, -12)
          CFI (cfiCond449) R6 Frame(CFA, -8)
          CFI (cfiCond449) R14 Frame(CFA, -4)
          CFI (cfiCond449) CFA R13+48
          CFI Block cfiCond450 Using cfiCommon0
          CFI (cfiCond450) Function log_addPhotoEvent
          CFI (cfiCond450) Conditional ??CrossCallReturnLabel_101
          CFI (cfiCond450) R14 Frame(CFA, -4)
          CFI (cfiCond450) CFA R13+40
          CFI Block cfiPicker451 Using cfiCommon1
          CFI (cfiPicker451) NoFunction
          CFI (cfiPicker451) Picker
        THUMB
?Subroutine32:
        MOV      R0,SP
          CFI (cfiCond447) FunCall log_addSystemEvent log_put
          CFI (cfiCond448) FunCall log_addSystemEventID log_put
          CFI (cfiCond449) FunCall log_addControlEvent log_put
          CFI (cfiCond450) FunCall log_addPhotoEvent log_put
        B.N      log_put
          CFI EndBlock cfiCond447
          CFI EndBlock cfiCond448
          CFI EndBlock cfiCond449
          CFI EndBlock cfiCond450
          CFI EndBlock cfiPicker451

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond452 Using cfiCommon0
          CFI Function log_addSystemEventID
          CFI Conditional ??CrossCallReturnLabel_89
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+48
          CFI Block cfiCond453 Using cfiCommon0
          CFI (cfiCond453) Function log_addControlEvent
          CFI (cfiCond453) Conditional ??CrossCallReturnLabel_90
          CFI (cfiCond453) R4 Frame(CFA, -16)
          CFI (cfiCond453) R5 Frame(CFA, -12)
          CFI (cfiCond453) R6 Frame(CFA, -8)
          CFI (cfiCond453) R14 Frame(CFA, -4)
          CFI (cfiCond453) CFA R13+48
          CFI Block cfiPicker454 Using cfiCommon1
          CFI (cfiPicker454) NoFunction
          CFI (cfiPicker454) Picker
        THUMB
?Subroutine28:
        MOVS     R2,#+10
        MOV      R1,R4
        ADD      R0,SP,#+9
          CFI (cfiCond452) FunCall log_addSystemEventID __aeabi_memcpy
          CFI (cfiCond453) FunCall log_addControlEvent __aeabi_memcpy
        B.W      __aeabi_memcpy
          CFI EndBlock cfiCond452
          CFI EndBlock cfiCond453
          CFI EndBlock cfiPicker454

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond455 Using cfiCommon0
          CFI Function log_addSystemEvent
          CFI Conditional ??CrossCallReturnLabel_10
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+40
          CFI Block cfiCond456 Using cfiCommon0
          CFI (cfiCond456) Function log_addSystemEventID
          CFI (cfiCond456) Conditional ??CrossCallReturnLabel_11
          CFI (cfiCond456) R4 Frame(CFA, -12)
          CFI (cfiCond456) R5 Frame(CFA, -8)
          CFI (cfiCond456) R14 Frame(CFA, -4)
          CFI (cfiCond456) CFA R13+48
          CFI Block cfiCond457 Using cfiCommon0
          CFI (cfiCond457) Function log_addControlEvent
          CFI (cfiCond457) Conditional ??CrossCallReturnLabel_12
          CFI (cfiCond457) R4 Frame(CFA, -16)
          CFI (cfiCond457) R5 Frame(CFA, -12)
          CFI (cfiCond457) R6 Frame(CFA, -8)
          CFI (cfiCond457) R14 Frame(CFA, -4)
          CFI (cfiCond457) CFA R13+48
          CFI Block cfiCond458 Using cfiCommon0
          CFI (cfiCond458) Function log_addPhotoEvent
          CFI (cfiCond458) Conditional ??CrossCallReturnLabel_13
          CFI (cfiCond458) R14 Frame(CFA, -4)
          CFI (cfiCond458) CFA R13+40
          CFI Block cfiPicker459 Using cfiCommon1
          CFI (cfiPicker459) NoFunction
          CFI (cfiPicker459) Picker
        THUMB
?Subroutine9:
        LDR.N    R1,??DataTable163_2
        LDR      R0,[R1, #+47]
        STR      R0,[SP, #+0]
        LDR      R0,[R1, #+47]
        STR      R0,[SP, #+4]
        BX       LR
          CFI EndBlock cfiCond455
          CFI EndBlock cfiCond456
          CFI EndBlock cfiCond457
          CFI EndBlock cfiCond458
          CFI EndBlock cfiPicker459

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock460 Using cfiCommon0
          CFI Function log_addUpdateEvent
        THUMB
log_addUpdateEvent:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        SUB      SP,SP,#+40
          CFI CFA R13+56
        MOV      R6,R0
        MOV      R4,R1
        LDR.N    R5,??DataTable163_2
        LDRH     R0,[R5, #+20]
        STRH     R0,[SP, #+0]
        LDR      R0,[R5, #+23]
        STR      R0,[SP, #+4]
        MOVS     R1,#+32
        ADD      R0,SP,#+8
          CFI FunCall __aeabi_memclr4
        BL       __aeabi_memclr4
        LDR      R0,[R5, #+47]
        STR      R0,[SP, #+8]
        LDR      R0,[R5, #+47]
        STR      R0,[SP, #+12]
        STRB     R6,[SP, #+16]
        LDRB     R0,[R5, #+19]
        STRB     R0,[SP, #+17]
        ADD      R0,SP,#+18
        BL       ?Subroutine17
??CrossCallReturnLabel_38:
        ADD      R2,SP,#+4
        ADD      R0,SP,#+20
        LDR      R1,[R2, #+0]
        STR      R1,[R0, #+0]
        STR      R4,[SP, #+27]
        ADD      R0,SP,#+8
          CFI FunCall log_put
        BL       log_put
        ADD      SP,SP,#+40
          CFI CFA R13+16
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock460

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable163:
        DATA32
        DC32     0x32474f4c

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable163_1:
        DATA32
        DC32     store

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable163_2:
        DATA32
        DC32     device

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock461 Using cfiCommon0
          CFI Function log_addPhotoEvent
          CFI NoCalls
        THUMB
log_addPhotoEvent:
        PUSH     {R0,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
        SUB      SP,SP,#+32
          CFI CFA R13+40
        BL       ?Subroutine31
??CrossCallReturnLabel_100:
        BL       ?Subroutine9
??CrossCallReturnLabel_13:
        MOVS     R0,#+129
        STRB     R0,[SP, #+8]
        ADD      R3,SP,#+32
        ADD      R0,SP,#+9
        LDR      R2,[R3, #+0]
        STR      R2,[R0, #+0]
        BL       ?Subroutine32
??CrossCallReturnLabel_101:
        ADD      SP,SP,#+36
          CFI CFA R13+4
        POP      {PC}
          CFI EndBlock cfiBlock461

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock462 Using cfiCommon0
          CFI Function user_getPhotoCount
        THUMB
user_getPhotoCount:
        PUSH     {R2-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+24
        LDR.W    R0,??DataTable193
        LDR      R5,[R0, #+72]
          CFI FunCall photo_storage_end
        BL       photo_storage_end
        MOV      R6,R0
        MOVS     R4,#+0
          CFI FunCall photo_cleanup_active
        BL       photo_cleanup_active
        CBZ.N    R0,??user_getPhotoCount_0
        CMP      R0,#+1
        IT       NE
        MOVNE    R5,#+4294967295
        LDR.W    R1,??DataTable193_1
        STR      R5,[R1, #+157]
        B.N      ??user_getPhotoCount_1
??user_getPhotoCount_0:
          CFI FunCall photo_recover
        BL       photo_recover
        CBNZ.N   R0,??user_getPhotoCount_2
        LDR.W    R1,??DataTable193_1
        MOV      R0,#+4294967295
        STR      R0,[R1, #+157]
??user_getPhotoCount_1:
        STRH     R4,[R1, #+155]
        BL       ?Subroutine57
??CrossCallReturnLabel_203:
        STRH     R2,[R1, #+129]
        MOVS     R0,#+0
        POP      {R1,R2,R4-R6,PC}
??user_getPhotoCount_3:
        LDR      R0,[SP, #+0]
        CMN      R0,#+1
        BEQ.N    ??user_getPhotoCount_4
        LDR      R0,[SP, #+4]
        CBZ.N    R0,??user_getPhotoCount_5
        CMP      R0,#+65536
        BCS.N    ??user_getPhotoCount_5
        SUBS     R1,R6,R5
        MOVW     R0,#+1033
        CMP      R1,R0
        ITTT     CS
        SUBWCS   R1,R1,#+1033
        LDRCS    R0,[SP, #+4]
        CMPCS    R1,R0
        BCC.N    ??user_getPhotoCount_5
        UXTH     R1,R0
        MOV      R0,R5
          CFI FunCall photo_confirmed_offset
        BL       photo_confirmed_offset
        LDR      R1,[SP, #+4]
        CMP      R0,R1
        IT       CC
        ADDCC    R4,R4,#+1
        ADDS     R5,R1,R5
        ADDW     R5,R5,#+1033
??user_getPhotoCount_2:
        ADD      R0,R5,#+8
        CMP      R6,R0
        BCC.N    ??user_getPhotoCount_4
        MOVS     R2,#+8
        BL       ?Subroutine59
??CrossCallReturnLabel_207:
        CMP      R0,#+0
        BNE.N    ??user_getPhotoCount_3
        MOV      R5,#+4294967295
        MOVS     R4,#+0
??user_getPhotoCount_4:
        LDR.W    R1,??DataTable193_1
        STR      R5,[R1, #+157]
        STRH     R4,[R1, #+155]
        UXTH     R0,R4
        CMP      R0,#+0
        ITE      NE
        MOVNE    R3,#+8192
        MOVEQ    R3,#+0
        B.N      ??user_getPhotoCount_6
??user_getPhotoCount_5:
        MOV      R5,R6
        B.N      ??user_getPhotoCount_4
??user_getPhotoCount_6:
        BL       ?Subroutine57
??CrossCallReturnLabel_204:
        ORRS     R3,R3,R2
        STRH     R3,[R1, #+129]
        UXTH     R0,R4
        POP      {R1,R2,R4-R6,PC}
          CFI EndBlock cfiBlock462

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable164:
        DATA32
        DC32     log_lastIndex

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond463 Using cfiCommon0
          CFI Function user_getPhotoCount
          CFI Conditional ??CrossCallReturnLabel_203
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
          CFI Block cfiCond464 Using cfiCommon0
          CFI (cfiCond464) Function user_getPhotoCount
          CFI (cfiCond464) Conditional ??CrossCallReturnLabel_204
          CFI (cfiCond464) R4 Frame(CFA, -16)
          CFI (cfiCond464) R5 Frame(CFA, -12)
          CFI (cfiCond464) R6 Frame(CFA, -8)
          CFI (cfiCond464) R14 Frame(CFA, -4)
          CFI (cfiCond464) CFA R13+24
          CFI Block cfiPicker465 Using cfiCommon1
          CFI (cfiPicker465) NoFunction
          CFI (cfiPicker465) Picker
        THUMB
?Subroutine57:
        LDRH     R2,[R1, #+129]
        MOVW     R0,#+57343
        ANDS     R2,R0,R2
        BX       LR
          CFI EndBlock cfiCond463
          CFI EndBlock cfiCond464
          CFI EndBlock cfiPicker465

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock466 Using cfiCommon0
          CFI Function user_checkFirmware
        THUMB
user_checkFirmware:
        PUSH     {R4-R10,LR}
          CFI R14 Frame(CFA, -4)
          CFI R10 Frame(CFA, -8)
          CFI R9 Frame(CFA, -12)
          CFI R8 Frame(CFA, -16)
          CFI R7 Frame(CFA, -20)
          CFI R6 Frame(CFA, -24)
          CFI R5 Frame(CFA, -28)
          CFI R4 Frame(CFA, -32)
          CFI CFA R13+32
        MOV      R10,R0
        MOVS     R4,R1
        MOV      R5,R2
        BEQ.N    ??user_checkFirmware_0
        TST      R4,#0x3
        BNE.N    ??user_checkFirmware_0
        MOV      R8,#+4294967295
        LDR.W    R9,??DataTable195
??user_checkFirmware_1:
        CMP      R4,#+129
        ITE      CS
        MOVCS    R7,#+128
        MOVCC    R7,R4
        UXTH     R2,R7
        MOV      R1,R9
        MOV      R0,R10
          CFI FunCall mem_read
        BL       mem_read
        CBZ.N    R0,??user_checkFirmware_2
        MOVS     R6,#+0
        B.N      ??user_checkFirmware_3
??user_checkFirmware_4:
        LDR      R1,[R9, R6, LSL #+2]
        MOV      R0,R8
          CFI FunCall CRC32
        BL       CRC32
        MOV      R8,R0
        ADDS     R6,R6,#+1
??user_checkFirmware_3:
        CMP      R6,R7, LSR #+2
        BCC.N    ??user_checkFirmware_4
        ADD      R10,R7,R10
        SUBS     R4,R4,R7
        BNE.N    ??user_checkFirmware_1
        CMP      R8,R5
        BEQ.N    ??user_checkFirmware_5
??user_checkFirmware_0:
        MOVS     R0,#+0
        B.N      ??user_checkFirmware_2
??user_checkFirmware_5:
        MOVS     R0,#+1
??user_checkFirmware_2:
        POP      {R4-R10,PC}
          CFI EndBlock cfiBlock466

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable165:
        DATA32
        DC32     log_lastIndex

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock467 Using cfiCommon0
          CFI Function user_send_usbDiag
        THUMB
user_send_usbDiag:
        PUSH     {R4,R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+12
        SUB      SP,SP,#+476
          CFI CFA R13+488
        MOV      R4,R0
        MOVS     R0,#+5
        STR      R0,[SP, #+244]
        ADD      R5,SP,#+244
        LDR.W    R0,??DataTable197
        LDR      R0,[R0, #+51]
        STR      R0,[R5, #+4]
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        STR      R0,[R5, #+8]
        ADD      R0,SP,#+256
          CFI FunCall usb_diag_fill
        BL       usb_diag_fill
        LDR.W    R0,??DataTable193_2
        LDR      R0,[R0, #+0]
        STR      R0,[SP, #+348]
        LDR.W    R0,??DataTable193_3
        LDR      R0,[R0, #+0]
        STR      R0,[SP, #+352]
        LDR.W    R0,??DataTable193_4
        LDR      R0,[R0, #+0]
        STR      R0,[SP, #+356]
        MOVS     R0,#+0
        STR      R0,[SP, #+360]
        STR      R0,[SP, #+364]
        STR      R0,[SP, #+368]
        LDR.W    R0,??DataTable193_5
        LDR      R0,[R0, #+0]
        LDR.W    R1,??DataTable197_1
        CMP      R0,R1
        ITE      EQ
        MOVEQ    R0,#+1
        MOVNE    R0,#+0
        STR      R0,[SP, #+372]
        ADD      R0,SP,#+376
          CFI FunCall usb_diag_fill_regs
        BL       usb_diag_fill_regs
        ADD      R0,SP,#+448
          CFI FunCall usb_diag_fill_errors
        BL       usb_diag_fill_errors
        ADD      R0,SP,#+456
          CFI FunCall usb_diag_fill_addr
        BL       usb_diag_fill_addr
          CFI FunCall stack_free_bytes
        BL       stack_free_bytes
        STR      R0,[SP, #+472]
        MOVS     R0,#+83
        STRB     R0,[SP, #+0]
        MOVS     R1,#+76
        STRB     R1,[SP, #+1]
        MOVS     R1,#+68
        STRB     R1,[SP, #+2]
        MOVS     R1,#+78
        STRB     R1,[SP, #+3]
        MOVS     R1,#+235
        STRB     R1,[SP, #+4]
        MOVS     R1,#+0
        STRB     R1,[SP, #+5]
        STRB     R1,[SP, #+6]
        STRB     R4,[SP, #+7]
        MOVS     R1,#+111
        STRB     R1,[SP, #+8]
        MOVS     R2,#+232
        ADD      R1,SP,#+244
        ADD      R0,SP,#+9
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        MOVS     R1,#+237
        ADD      R0,SP,#+4
          CFI FunCall crc16_calc
        BL       crc16_calc
        STRB     R0,[SP, #+241]
        LSRS     R0,R0,#+8
        STRB     R0,[SP, #+242]
        MOVS     R1,#+243
        MOV      R0,SP
          CFI FunCall usb_send
        BL       usb_send
        ADD      SP,SP,#+476
          CFI CFA R13+12
        POP      {R4,R5,PC}
          CFI EndBlock cfiBlock467

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable166:
        DATA8
        DC8      0x25, 0x64, 0x00, 0x00

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable166_1:
        DATA32
        DC32     store_save_requested

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable166_2:
        DATA32
        DC32     log_pendingCount

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable166_3:
        DATA32
        DC32     fw_job

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock468 Using cfiCommon0
          CFI Function user_send_commDiag
        THUMB
user_send_commDiag:
        PUSH     {R4-R8,LR}
          CFI R14 Frame(CFA, -4)
          CFI R8 Frame(CFA, -8)
          CFI R7 Frame(CFA, -12)
          CFI R6 Frame(CFA, -16)
          CFI R5 Frame(CFA, -20)
          CFI R4 Frame(CFA, -24)
          CFI CFA R13+24
        SUB      SP,SP,#+456
          CFI CFA R13+480
        MOV      R5,R0
          CFI FunCall comm_modem_status
        BL       comm_modem_status
        MOV      R7,R0
          CFI FunCall comm_modem_diag
        BL       comm_modem_diag
        MOV      R4,R0
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        MOV      R8,R0
        MOV      R0,SP
          CFI FunCall comm_uart_stats
        BL       comm_uart_stats
        MOVS     R0,#+1
        STR      R0,[SP, #+240]
        ADD      R6,SP,#+240
        STR      R8,[R6, #+4]
        LDRB     R0,[R7, #+6]
        LDRB     R1,[R7, #+7]
        ORR      R0,R0,R1, LSL #+8
        LDRB     R1,[R7, #+8]
        ORR      R0,R0,R1, LSL #+16
        LDRB     R1,[R7, #+9]
        ORR      R0,R0,R1, LSL #+24
        STR      R0,[R6, #+8]
        LDRH     R0,[R7, #+4]
        LDRB     R1,[R7, #+11]
        ORR      R0,R0,R1, LSL #+16
        STR      R0,[SP, #+252]
        LDR      R0,[R7, #+0]
        STR      R0,[R6, #+16]
          CFI FunCall comm_channel_diag
        BL       comm_channel_diag
        STR      R0,[SP, #+260]
          CFI FunCall comm_server_diag
        BL       comm_server_diag
        STR      R0,[R6, #+24]
        ADD      R1,SP,#+272
        MOV      R0,R8
          CFI FunCall comm_recover_diag
        BL       comm_recover_diag
        STR      R0,[SP, #+268]
        LDRH     R0,[SP, #+0]
        STR      R0,[SP, #+276]
        LDRH     R0,[SP, #+2]
        STR      R0,[SP, #+280]
        LDRH     R0,[SP, #+4]
        STR      R0,[SP, #+284]
        LDRH     R0,[SP, #+6]
        STR      R0,[SP, #+288]
        LDRH     R0,[SP, #+8]
        STR      R0,[SP, #+292]
        LDRB     R0,[R4, #+113]
        LDRB     R1,[R4, #+112]
        ORR      R0,R0,R1, LSL #+8
        LDRH     R1,[R4, #+110]
        ORR      R0,R0,R1, LSL #+16
        STR      R0,[SP, #+296]
        MOVS     R1,#+0
??user_send_commDiag_0:
        ADDS     R0,R4,R1
        LDRB     R0,[R0, #+90]
        ADDS     R2,R4,R1
        LDRB     R2,[R2, #+100]
        ORR      R0,R0,R2, LSL #+8
        ADD      R2,R4,R1, LSL #+1
        LDRH     R2,[R2, #+70]
        ORR      R0,R0,R2, LSL #+16
        ADD      R2,SP,#+240
        ADD      R2,R2,R1, LSL #+2
        STR      R0,[R2, #+60]
        ADDS     R1,R1,#+1
        CMP      R1,#+10
        BCC.N    ??user_send_commDiag_0
        MOVS     R1,#+0
??user_send_commDiag_1:
        LDRH     R0,[R4, R1, LSL #+1]
        ADD      R2,SP,#+240
        ADD      R2,R2,R1, LSL #+2
        STR      R0,[R2, #+100]
        ADDS     R1,R1,#+1
        CMP      R1,#+29
        BLT.N    ??user_send_commDiag_1
        MOVS     R0,#+83
        STRB     R0,[SP, #+12]
        ADD      R0,SP,#+12
        MOVS     R1,#+76
        STRB     R1,[R0, #+1]
        MOVS     R1,#+68
        STRB     R1,[R0, #+2]
        MOVS     R1,#+78
        STRB     R1,[SP, #+15]
        MOVS     R1,#+219
        STRB     R1,[R0, #+4]
        MOVS     R1,#+0
        STRB     R1,[SP, #+17]
        STRB     R1,[R0, #+6]
        STRB     R5,[R0, #+7]
        MOVS     R1,#+112
        STRB     R1,[R0, #+8]
        MOVS     R2,#+216
        ADD      R1,SP,#+240
        ADD      R0,SP,#+21
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        MOVS     R1,#+221
        ADD      R0,SP,#+16
          CFI FunCall crc16_calc
        BL       crc16_calc
        STRB     R0,[SP, #+237]
        LSRS     R0,R0,#+8
        STRB     R0,[SP, #+238]
        MOVS     R1,#+227
        ADD      R0,SP,#+12
          CFI FunCall usb_send
        BL       usb_send
        ADD      SP,SP,#+456
          CFI CFA R13+24
        POP      {R4-R8,PC}
          CFI EndBlock cfiBlock468

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock469 Using cfiCommon0
          CFI Function user_comm_local_time
          CFI NoCalls
        THUMB
user_comm_local_time:
        LDR.W    R2,??DataTable193_1
        LDR      R0,[R2, #+47]
        LDR.W    R1,??DataTable197_2
        CMP      R0,R1
        IT       CC
        MOVCC    R0,#+4294967295
        BX       LR
          CFI EndBlock cfiBlock469

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock470 Using cfiCommon0
          CFI Function user_comm_ussd
          CFI NoCalls
        THUMB
user_comm_ussd:
        LDR.W    R0,??DataTable197_3
        BX       LR
          CFI EndBlock cfiBlock470

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock471 Using cfiCommon0
          CFI Function user_comm_volte
          CFI NoCalls
        THUMB
user_comm_volte:
        LDR.W    R0,??DataTable197_4
        LDRB     R0,[R0, #+171]
        BX       LR
          CFI EndBlock cfiBlock471

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock472 Using cfiCommon0
          CFI Function user_comm_balance
        THUMB
user_comm_balance:
        MOV      R1,R0
        CMP      R1,#+1
        BGE.N    ??user_comm_balance_0
        MOVS     R2,#+1
        LDR.W    R0,??DataTable197
        STRH     R2,[R0, #+77]
        B.N      ??user_comm_balance_1
??user_comm_balance_0:
        MOVW     R0,#+65001
        CMP      R1,R0
        BLT.N    ??user_comm_balance_2
        MOVW     R1,#+65000
        LDR.W    R0,??DataTable197
        STRH     R1,[R0, #+77]
        BX       LR
??user_comm_balance_2:
        LDR.W    R0,??DataTable197
        STRH     R1,[R0, #+77]
        CMP      R1,#+100
        BGE.N    ??user_comm_balance_3
??user_comm_balance_1:
        MOVS     R0,#+37
          CFI FunCall log_addSystemValue
        B.N      log_addSystemValue
??user_comm_balance_3:
        BX       LR
          CFI EndBlock cfiBlock472

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable170:
        DATA32
        DC32     log_timeKnown

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock473 Using cfiCommon0
          CFI Function user_comm_low_signal
        THUMB
user_comm_low_signal:
        MOV      R1,R0
        MOVS     R0,#+38
          CFI FunCall log_addSystemValue
        B.N      log_addSystemValue
          CFI EndBlock cfiBlock473

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock474 Using cfiCommon0
          CFI Function user_comm_link
        THUMB
user_comm_link:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        CMP      R0,#+5
        BCC.N    ??user_comm_link_0
        MOVS     R0,#+1
        POP      {R4,PC}
??user_comm_link_0:
        LDR.W    R3,??DataTable197_5
        LDRB     R4,[R3, R0]
        CMP      R0,#+3
        BGE.N    ??user_comm_link_1
        MOV      R0,R1
          CFI FunCall user_comm_server
        BL       user_comm_server
        ADDS     R4,R0,R4
??user_comm_link_1:
        UXTB     R0,R4
        POP      {R4,LR}
          CFI R4 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall log_addSystemEvent
        B.N      log_addSystemEvent
          CFI EndBlock cfiBlock474

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
`user_comm_link::first`:
        DATA8
        DC8 14, 16, 18, 11, 12
        DATA
        DS8 3

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock475 Using cfiCommon0
          CFI Function user_comm_busy
        THUMB
user_comm_busy:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
        LDR.W    R0,??DataTable197
        LDRB     R0,[R0, #+164]
        CBNZ.N   R0,??user_comm_busy_0
          CFI FunCall comm_call_active
        BL       comm_call_active
        CBZ.N    R0,??user_comm_busy_1
??user_comm_busy_0:
        MOVS     R0,#+1
??user_comm_busy_1:
        POP      {R1,PC}
          CFI EndBlock cfiBlock475

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
comm_host:
        DS8 64

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
comm_port:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
comm_credential:
        DS8 32

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock476 Using cfiCommon0
          CFI Function comm_split
        THUMB
comm_split:
        PUSH     {R4-R8,LR}
          CFI R14 Frame(CFA, -4)
          CFI R8 Frame(CFA, -8)
          CFI R7 Frame(CFA, -12)
          CFI R6 Frame(CFA, -16)
          CFI R5 Frame(CFA, -20)
          CFI R4 Frame(CFA, -24)
          CFI CFA R13+24
        MOV      R5,R0
        MOV      R7,R1
        MOVS     R1,#+58
        MOV      R0,R7
          CFI FunCall strrchr
        BL       strrchr
        MOVS     R4,R0
        IT       NE
        SUBNE    R6,R4,R7
        BNE.N    ??comm_split_0
        MOV      R0,R7
          CFI FunCall strlen
        BL       strlen
        MOV      R6,R0
??comm_split_0:
        CMP      R6,#+32
        IT       CS
        MOVCS    R6,#+31
        LDR.W    R8,??DataTable197_6
        MOV      R2,R6
        MOV      R1,R7
        ADD      R0,R8,R5, LSL #+5
          CFI FunCall __aeabi_memcpy
        BL       __aeabi_memcpy
        ADD      R1,R8,R5, LSL #+5
        MOVS     R0,#+0
        STRB     R0,[R1, R6]
        CBZ.N    R4,??comm_split_1
        ADDS     R0,R4,#+1
          CFI FunCall atoi
        BL       atoi
??comm_split_1:
        LDR.W    R1,??DataTable197_7
        STRH     R0,[R1, R5, LSL #+1]
        POP      {R4-R8,PC}
          CFI EndBlock cfiBlock476

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock477 Using cfiCommon0
          CFI Function user_comm_server
          CFI NoCalls
        THUMB
user_comm_server:
        CBNZ.N   R0,??user_comm_server_0
        LDR.W    R1,??DataTable197_4
        LDRB     R0,[R1, #+219]
        CBNZ.N   R0,??user_comm_server_1
        LDRB     R0,[R1, #+220]
        CBZ.N    R0,??user_comm_server_2
??user_comm_server_0:
        MOVS     R0,#+1
        BX       LR
??user_comm_server_1:
        MOVS     R0,#+0
??user_comm_server_2:
        BX       LR
          CFI EndBlock cfiBlock477

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock478 Using cfiCommon0
          CFI Function comm_endpoint
        THUMB
comm_endpoint:
        MOVS     R1,R0
        BNE.N    ??comm_endpoint_0
        LDR.W    R2,??DataTable197_4
        LDRB     R1,[R2, #+219]
        CBNZ.N   R1,??comm_endpoint_1
        LDRB     R1,[R2, #+220]
        CBZ.N    R1,??comm_endpoint_1
??comm_endpoint_0:
        LDR.W    R1,??DataTable197_8
        B.N      ??comm_endpoint_2
??comm_endpoint_1:
        ADD      R1,R2,#+8
??comm_endpoint_2:
          CFI FunCall comm_split
        B.N      comm_split
          CFI EndBlock cfiBlock478

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable175:
        DATA32
        DC32     usb_bus_resets

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock479 Using cfiCommon0
          CFI Function comm_text
        THUMB
comm_text:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        MOV      R4,R1
        MOVS     R2,R0
        BEQ.N    ??comm_text_0
        SUBS     R2,R0,#+1
        CMP      R2,#+2
        BLS.N    ??comm_text_1
        SUBS     R2,R2,#+6
        BEQ.N    ??comm_text_2
        B.N      ??comm_text_3
??comm_text_0:
        MOVS     R0,#+0
          CFI FunCall comm_endpoint
        BL       comm_endpoint
        MOVS     R0,#+32
        STRH     R0,[R4, #+0]
        LDR.W    R0,??DataTable197_6
        POP      {R4-R6,PC}
??comm_text_2:
        MOVS     R0,#+1
          CFI FunCall comm_endpoint
        BL       comm_endpoint
        MOVS     R0,#+32
        STRH     R0,[R4, #+0]
        LDR.W    R0,??DataTable197_9
        POP      {R4-R6,PC}
??comm_text_1:
        LDR.W    R1,??DataTable197_4
        ADD      R2,R1,#+104
        SUBS     R5,R0,#+1
        MOVS     R3,#+0
        B.N      ??comm_text_4
??comm_text_5:
        LDRB     R0,[R2, #+0]
        CBZ.N    R0,??comm_text_6
        LDRB     R0,[R2], #+1
        CMP      R0,#+44
        BNE.N    ??comm_text_7
        SUBS     R5,R5,#+1
??comm_text_4:
        UXTB     R0,R5
        CBZ.N    R0,??comm_text_8
??comm_text_7:
        ADD      R0,R1,#+136
        CMP      R2,R0
        BCC.N    ??comm_text_5
??comm_text_6:
        MOVS     R0,#+1
        STRH     R0,[R4, #+0]
        LDR.W    R0,??DataTable197_10
        POP      {R4-R6,PC}
??comm_text_9:
        LDRB     R6,[R2], #+1
        LDR.W    R5,??DataTable197_11
        UXTB     R0,R3
        STRB     R6,[R5, R0]
        ADDS     R3,R3,#+1
??comm_text_8:
        ADD      R0,R1,#+136
        CMP      R2,R0
        BCS.N    ??comm_text_10
        LDRB     R0,[R2, #+0]
        CBZ.N    R0,??comm_text_10
        CMP      R0,#+44
        BEQ.N    ??comm_text_10
        UXTB     R0,R3
        ADDS     R0,R0,#+1
        CMP      R0,#+32
        BCC.N    ??comm_text_9
??comm_text_10:
        LDR.W    R0,??DataTable197_11
        MOVS     R1,#+0
        UXTB     R3,R3
        STRB     R1,[R0, R3]
        MOVS     R1,#+32
        STRH     R1,[R4, #+0]
        POP      {R4-R6,PC}
??comm_text_3:
        MOVS     R0,#+0
        STRH     R0,[R4, #+0]
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock479

        SECTION `.rodata`:CONST:REORDER:NOROOT(0)
        DATA
`comm_text::empty`:
        DATA8
        DC8 ""

        SECTION `.text`:CODE:NOROOT(2)
          CFI Block cfiBlock480 Using cfiCommon0
          CFI Function comm_number
        THUMB
comm_number:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
        SUBS     R0,R0,#+4
        CMP      R0,#+5
        BHI.N    ??comm_number_1
        TBB      [PC, R0]
        DATA
??comm_number_0:
        DC8      0x3,0x35,0x11,0x3A
        DC8      0xA,0x1D
        THUMB
??comm_number_2:
        MOVS     R0,#+0
          CFI FunCall comm_endpoint
        BL       comm_endpoint
        LDR.W    R0,??DataTable197_7
        LDRH     R0,[R0, #+0]
        POP      {R1,PC}
??comm_number_3:
        MOVS     R0,#+1
          CFI FunCall comm_endpoint
        BL       comm_endpoint
        LDR.W    R0,??DataTable197_7
        LDRH     R0,[R0, #+2]
        POP      {R1,PC}
??comm_number_4:
        LDR.W    R1,??DataTable197_4
        LDRB     R0,[R1, #+136]
        CMP      R0,#+5
        BCS.N    ??comm_number_1
        LDR.W    R0,??DataTable197_12
        LDRB     R1,[R1, #+136]
        LDRB     R0,[R0, R1]
        POP      {R1,PC}
??comm_number_5:
        MOVS     R0,#+1
          CFI FunCall comm_endpoint
        BL       comm_endpoint
        LDR.W    R0,??DataTable197_4
        LDRB     R0,[R0, #+220]
        CMP      R0,#+0
        ITTT     NE
        LDRNE.W  R0,??DataTable197_6
        LDRBNE   R0,[R0, #+32]
        CMPNE    R0,#+0
        BEQ.N    ??comm_number_6
        LDR.W    R0,??DataTable197_7
        LDRH     R0,[R0, #+2]
        CBZ.N    R0,??comm_number_7
        MOVS     R0,#+1
        B.N      ??comm_number_7
??comm_number_6:
        MOVS     R0,#+0
??comm_number_7:
        UXTB     R0,R0
        POP      {R1,PC}
??comm_number_8:
        LDR.W    R0,??DataTable197
        LDRB     R0,[R0, #+169]
        POP      {R1,PC}
??comm_number_1:
        MOVS     R0,#+0
        POP      {R1,PC}
          CFI EndBlock cfiBlock480

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
`comm_number{1}{2}{3}::rat`:
        DATA8
        DC8 0, 1, 0, 5, 3
        DATA
        DS8 3

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
comm_settings:
        DATA32
        DC32 comm_text, comm_number

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock481 Using cfiCommon0
          CFI Function comm_gpio
          CFI NoCalls
        THUMB
comm_gpio:
        MOVS     R2,#+1
        LSL      R0,R2,R0
        UXTH     R0,R0
        CMP      R1,#+0
        ITE      NE
        LDRNE.W  R1,??DataTable197_13
        LDREQ.W  R1,??DataTable197_14
        STR      R0,[R1, #+0]
        BX       LR
          CFI EndBlock cfiBlock481

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock482 Using cfiCommon0
          CFI Function comm_type
          CFI NoCalls
        THUMB
comm_type:
        LDR.W    R1,??DataTable197
        STRB     R0,[R1, #+169]
        BX       LR
          CFI EndBlock cfiBlock482

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock483 Using cfiCommon0
          CFI Function comm_imei
        THUMB
comm_imei:
        MOV      R1,R0
        MOVS     R2,#+16
        LDR.W    R0,??DataTable197
          CFI FunCall __aeabi_memcpy
        B.W      __aeabi_memcpy
          CFI EndBlock cfiBlock483

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable180:
        DATA32
        DC32     fw_write_errors

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock484 Using cfiCommon0
          CFI Function user_comm_boot
        THUMB
user_comm_boot:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        LDR.W    R4,??DataTable197_4
        LDRB     R0,[R4, #+219]
        CBNZ.N   R0,??user_comm_boot_0
        LDRB     R0,[R4, #+220]
        CBNZ.N   R0,??user_comm_boot_1
??user_comm_boot_0:
        ADD      R1,R4,#+8
        B.N      ??user_comm_boot_2
??user_comm_boot_1:
        ADD      R1,R4,#+40
??user_comm_boot_2:
        MOVS     R0,#+0
          CFI FunCall comm_split
        BL       comm_split
        ADD      R1,R4,#+40
        MOVS     R0,#+1
          CFI FunCall comm_split
        BL       comm_split
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        MOV      R2,R0
        LDR.W    R1,??DataTable197_15
        LDR.W    R0,??DataTable197_16
          CFI FunCall user_comm_start
        BL       user_comm_start
        CBNZ.N   R0,??user_comm_boot_3
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        MOV      R1,R0
        MOVS     R0,#+1
        POP      {R4,LR}
          CFI R4 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall comm_modem_restart
        B.W      comm_modem_restart
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
??user_comm_boot_3:
        POP      {R4,PC}
          CFI EndBlock cfiBlock484

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable181:
        DATA32
        DC32     ?_76

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
`user_comm_boot::handlers`:
        DATA32
        DC32 comm_gpio, comm_type, comm_imei

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock485 Using cfiCommon0
          CFI Function store_usb_session
        THUMB
store_usb_session:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        LDR.W    R2,??DataTable197_17
        LDR.W    R4,??DataTable197_18
        LDR      R0,[R4, #+12]
        LDR      R1,[R2, #+0]
        CMP      R0,R1
        BEQ.N    ??store_usb_session_0
        LDR      R0,[R2, #+0]
        STR      R0,[R4, #+12]
        MOVS     R0,#+0
          CFI FunCall fw_async_quiet
        BL       fw_async_quiet
        MOVS     R0,#+0
          CFI FunCall table_runtime_quiet
        BL       table_runtime_quiet
        MOVS R0,#0
          CFI FunCall service_crc_quiet
        BL service_crc_quiet
        MOVS     R1,#+0
        LDR.W    R0,??DataTable197_19
        STRH     R1,[R0, #+0]
        LDR.W    R0,??DataTable197_20
        STRH     R1,[R0, #+0]
        LDRB     R0,[R4, #+30]
        AND      R0,R0,#0xA
        STRB     R0,[R4, #+30]
        LDRB     R0,[R4, #+18]
        CBZ.N    R0,??store_usb_session_0
        LDRB     R0,[R4, #+22]
        CBNZ.N   R0,??store_usb_session_0
        MOVS     R0,#+1
        STRB     R0,[R4, #+24]
??store_usb_session_0:
        POP      {R4,PC}
          CFI EndBlock cfiBlock485

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable182:
        DATA32
        DC32     log_job

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock486 Using cfiCommon0
          CFI Function store_marker
          CFI NoCalls
        THUMB
store_marker:
        PUSH     {R0,R1}
          CFI CFA R13+8
        MOVS     R1,R0
        ITTE     NE
        ORRNE    R0,R0,#0x31400000
        ORRNE    R0,R0,#0x34C00
        MOVEQ    R0,#+0
        STR      R0,[SP, #+0]
        LDR.W    R0,??DataTable197
        ADD      R1,R0,#+189
        LDR      R3,[SP, #+0]
        STR      R3,[R1, #+0]
        ADD      R1,SP,#+4
        ADD      R3,R0,#+193
        LDR      R0,[R1, #+0]
        STR      R0,[R3, #+0]
        ADD      SP,SP,#+8
          CFI CFA R13+0
        BX       LR
          CFI EndBlock cfiBlock486

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock487 Using cfiCommon0
          CFI Function store_snapshot
        THUMB
store_snapshot:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
          CFI FunCall photo_storage_busy
        BL       photo_storage_busy
        CBNZ.N   R0,??store_snapshot_0
          CFI FunCall table_runtime_active
        BL       table_runtime_active
        CBNZ.N   R0,??store_snapshot_0
          CFI FunCall fw_async_active
        BL       fw_async_active
        CBNZ.N   R0,??store_snapshot_0
          CFI FunCall log_async_active
        BL       log_async_active
        CBZ.N    R0,??store_snapshot_1
??store_snapshot_0:
        MOVS     R0,#+0
        POP      {R1,R4,R5,PC}
??store_snapshot_1:
        LDR.W    R5,??DataTable197_4
        BL       ?Subroutine55
??CrossCallReturnLabel_189:
        STRH     R0,[R5, #+510]
        LDR.W    R4,??DataTable197
        BL       ??Subroutine24_0
??CrossCallReturnLabel_258:
        STRH     R0,[R4, #+510]
        MOV      R1,R4
        MOV      R0,R5
        POP      {R2,R4,R5,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall mem_config_begin
        B.W      mem_config_begin
          CFI EndBlock cfiBlock487

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond488 Using cfiCommon0
          CFI Function user_writeConfigAndDevice
          CFI Conditional ??CrossCallReturnLabel_190
          CFI R4 Frame(CFA, -12)
          CFI R5 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+16
          CFI Block cfiCond489 Using cfiCommon0
          CFI (cfiCond489) Function store_snapshot
          CFI (cfiCond489) Conditional ??CrossCallReturnLabel_189
          CFI (cfiCond489) R4 Frame(CFA, -12)
          CFI (cfiCond489) R5 Frame(CFA, -8)
          CFI (cfiCond489) R14 Frame(CFA, -4)
          CFI (cfiCond489) CFA R13+16
          CFI Block cfiPicker490 Using cfiCommon1
          CFI (cfiPicker490) NoFunction
          CFI (cfiPicker490) Picker
        THUMB
?Subroutine55:
        MOV      R1,#+510
        MOV      R0,R5
          CFI (cfiCond488) FunCall user_writeConfigAndDevice crc16_calc
          CFI (cfiCond489) FunCall store_snapshot crc16_calc
        B.W      crc16_calc
          CFI EndBlock cfiCond488
          CFI EndBlock cfiCond489
          CFI EndBlock cfiPicker490

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond491 Using cfiCommon0
          CFI Function user_writeconfig
          CFI Conditional ??CrossCallReturnLabel_262
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI Block cfiCond492 Using cfiCommon0
          CFI (cfiCond492) Function user_writeConfigAndDevice
          CFI (cfiCond492) Conditional ??CrossCallReturnLabel_261
          CFI (cfiCond492) R4 Frame(CFA, -12)
          CFI (cfiCond492) R5 Frame(CFA, -8)
          CFI (cfiCond492) R14 Frame(CFA, -4)
          CFI (cfiCond492) CFA R13+16
          CFI Block cfiPicker493 Using cfiCommon1
          CFI (cfiPicker493) NoFunction
          CFI (cfiPicker493) Picker
        THUMB
?Subroutine24:
        LDR.W    R4,??DataTable197_4
          CFI Block cfiCond494 Using cfiCommon0
          CFI (cfiCond494) Function config_terminate_strings
          CFI (cfiCond494) Conditional ??CrossCallReturnLabel_260
          CFI (cfiCond494) R4 Frame(CFA, -12)
          CFI (cfiCond494) R5 Frame(CFA, -8)
          CFI (cfiCond494) R14 Frame(CFA, -4)
          CFI (cfiCond494) CFA R13+16
          CFI Block cfiCond495 Using cfiCommon0
          CFI (cfiCond495) Function user_readconfig
          CFI (cfiCond495) Conditional ??CrossCallReturnLabel_259
          CFI (cfiCond495) R4 Frame(CFA, -12)
          CFI (cfiCond495) R5 Frame(CFA, -8)
          CFI (cfiCond495) R14 Frame(CFA, -4)
          CFI (cfiCond495) CFA R13+16
          CFI Block cfiCond496 Using cfiCommon0
          CFI (cfiCond496) Function store_snapshot
          CFI (cfiCond496) Conditional ??CrossCallReturnLabel_258
          CFI (cfiCond496) R4 Frame(CFA, -12)
          CFI (cfiCond496) R5 Frame(CFA, -8)
          CFI (cfiCond496) R14 Frame(CFA, -4)
          CFI (cfiCond496) CFA R13+16
??Subroutine24_0:
        MOV      R1,#+510
          CFI EndBlock cfiCond491
          CFI EndBlock cfiCond492
          CFI EndBlock cfiPicker493
          CFI EndBlock cfiCond494
          CFI EndBlock cfiCond495
          CFI EndBlock cfiCond496
        REQUIRE ??Subroutine73_0

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond497 Using cfiCommon0
          CFI Function log_async_step
          CFI Conditional ??CrossCallReturnLabel_257
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+56
          CFI Block cfiCond498 Using cfiCommon0
          CFI (cfiCond498) Function user_writeconfig
          CFI (cfiCond498) Conditional ??CrossCallReturnLabel_262
          CFI (cfiCond498) R4 Frame(CFA, -8)
          CFI (cfiCond498) R14 Frame(CFA, -4)
          CFI (cfiCond498) CFA R13+8
          CFI Block cfiCond499 Using cfiCommon0
          CFI (cfiCond499) Function user_writeConfigAndDevice
          CFI (cfiCond499) Conditional ??CrossCallReturnLabel_261
          CFI (cfiCond499) R4 Frame(CFA, -12)
          CFI (cfiCond499) R5 Frame(CFA, -8)
          CFI (cfiCond499) R14 Frame(CFA, -4)
          CFI (cfiCond499) CFA R13+16
          CFI Block cfiCond500 Using cfiCommon0
          CFI (cfiCond500) Function config_terminate_strings
          CFI (cfiCond500) Conditional ??CrossCallReturnLabel_260
          CFI (cfiCond500) R4 Frame(CFA, -12)
          CFI (cfiCond500) R5 Frame(CFA, -8)
          CFI (cfiCond500) R14 Frame(CFA, -4)
          CFI (cfiCond500) CFA R13+16
          CFI Block cfiCond501 Using cfiCommon0
          CFI (cfiCond501) Function user_readconfig
          CFI (cfiCond501) Conditional ??CrossCallReturnLabel_259
          CFI (cfiCond501) R4 Frame(CFA, -12)
          CFI (cfiCond501) R5 Frame(CFA, -8)
          CFI (cfiCond501) R14 Frame(CFA, -4)
          CFI (cfiCond501) CFA R13+16
          CFI Block cfiCond502 Using cfiCommon0
          CFI (cfiCond502) Function store_snapshot
          CFI (cfiCond502) Conditional ??CrossCallReturnLabel_258
          CFI (cfiCond502) R4 Frame(CFA, -12)
          CFI (cfiCond502) R5 Frame(CFA, -8)
          CFI (cfiCond502) R14 Frame(CFA, -4)
          CFI (cfiCond502) CFA R13+16
          CFI Block cfiPicker503 Using cfiCommon1
          CFI (cfiPicker503) NoFunction
          CFI (cfiPicker503) Picker
        THUMB
??Subroutine73_0:
        MOV      R0,R4
          CFI (cfiCond497) FunCall log_async_step crc16_calc
          CFI (cfiCond498) FunCall user_writeconfig crc16_calc
          CFI (cfiCond499) FunCall user_writeConfigAndDevice crc16_calc
          CFI (cfiCond500) FunCall config_terminate_strings crc16_calc
          CFI (cfiCond501) FunCall user_readconfig crc16_calc
          CFI (cfiCond502) FunCall store_snapshot crc16_calc
        B.W      crc16_calc
          CFI EndBlock cfiCond497
          CFI EndBlock cfiCond498
          CFI EndBlock cfiCond499
          CFI EndBlock cfiCond500
          CFI EndBlock cfiCond501
          CFI EndBlock cfiCond502
          CFI EndBlock cfiPicker503

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable185:
        DATA32
        DC32     ?_81

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock504 Using cfiCommon0
          CFI Function store_restore
        THUMB
store_restore:
        PUSH     {R1-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+24
        ADD      R4,R0,#+189
        ADD      R1,SP,#+4
        LDR      R3,[R4, #+0]
        STR      R3,[R1, #+0]
        ADD      R3,R0,#+193
        LDR      R2,[R3, #+0]
        STR      R2,[SP, #+0]
        LDR      R1,[SP, #+4]
        LDR.W    R0,??DataTable197_21
        CMP      R1,R0
        ITT      NE
        LDRNE.W  R0,??DataTable197_22
        CMPNE    R1,R0
        BNE.N    ??store_restore_0
        MOV      R4,R1
        UXTB     R0,R4
        CMP      R0,#+10
        LDR.N    R0,??DataTable193
        ITTEE    EQ
        LDREQ    R5,[R0, #+56]
        MOVEQ    R0,#+56
        LDRNE    R5,[R0, #+64]
        MOVNE    R0,#+64
        CMP      R2,R5
        BCC.N    ??store_restore_0
        LDR.N    R1,??DataTable193
        ADD      R0,R1,R0
        LDR      R1,[R0, #+4]
        ADDS     R1,R1,R5
        CMP      R1,R2
        BCC.N    ??store_restore_0
        MOV      R1,R2
        UXTB     R0,R4
          CFI FunCall store_marker
        BL       store_marker
        LDR.W    R0,??DataTable197_18
        STRB     R4,[R0, #+21]
        STRB     R4,[R0, #+19]
        LSRS     R1,R5,#+12
        LSLS     R1,R1,#+12
        STR      R1,[R0, #+0]
        LDR      R1,[SP, #+0]
        STR      R1,[R0, #+4]
        MOVS     R1,#+10
        STRB     R1,[R0, #+18]
        MOVS     R1,#+0
        STRB     R1,[R0, #+20]
        MOVS     R1,#+1
        STRB     R1,[R0, #+24]
        STR      R5,[R0, #+0]
??store_restore_0:
        POP      {R0-R2,R4,R5,PC}
          CFI EndBlock cfiBlock504

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock505 Using cfiCommon0
          CFI Function store_upload_request
        THUMB
store_upload_request:
        PUSH     {R4-R8,LR}
          CFI R14 Frame(CFA, -4)
          CFI R8 Frame(CFA, -8)
          CFI R7 Frame(CFA, -12)
          CFI R6 Frame(CFA, -16)
          CFI R5 Frame(CFA, -20)
          CFI R4 Frame(CFA, -24)
          CFI CFA R13+24
        MOV      R8,R0
        MOV      R4,R1
        MOV      R5,R2
        CMP      R0,#+10
        ITE      EQ
        MOVEQ    R6,#+0
        MOVNE    R6,#+2
        CBZ.N    R4,??store_upload_request_0
        ADDS     R6,R6,#+1
??store_upload_request_0:
        LDR.W    R7,??DataTable197_18
        LDRB     R0,[R7, #+18]
        CBNZ.N   R0,??store_upload_request_1
          CFI FunCall mem_config_result
        BL       mem_config_result
        CBZ.N    R0,??store_upload_request_2
??store_upload_request_1:
        MOVS     R0,#+0
        B.N      ??store_upload_request_3
??store_upload_request_2:
        STRB     R8,[R7, #+21]
        STRB     R4,[R7, #+22]
        STRB     R5,[R7, #+23]
        STRB     R0,[R7, #+24]
        LDRB     R0,[R7, #+30]
        LSRS     R0,R0,R6
        LSLS     R0,R0,#+31
        BPL.N    ??store_upload_request_4
        ADDS     R0,R7,R6
        LDRB     R1,[R0, #+26]
        CMP      R1,R5
        IT       EQ
        MOVEQ    R0,#+1
        BEQ.N    ??store_upload_request_5
??store_upload_request_4:
        MOVS     R0,#+0
??store_upload_request_5:
        STRB     R0,[R7, #+25]
        ADDS     R0,R7,R6
        STRB     R5,[R0, #+26]
        LDRB     R1,[R7, #+30]
        MOVS     R0,#+1
        LSL      R6,R0,R6
        ORRS     R6,R6,R1
        STRB     R6,[R7, #+30]
??store_upload_request_3:
        POP      {R4-R8,PC}
          CFI EndBlock cfiBlock505

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock506 Using cfiCommon0
          CFI Function store_ack
        THUMB
store_ack:
        PUSH     {R3-R9,LR}
          CFI R14 Frame(CFA, -4)
          CFI R9 Frame(CFA, -8)
          CFI R8 Frame(CFA, -12)
          CFI R7 Frame(CFA, -16)
          CFI R6 Frame(CFA, -20)
          CFI R5 Frame(CFA, -24)
          CFI R4 Frame(CFA, -28)
          CFI CFA R13+32
        MOV      R4,R0
        MOV      R8,R1
        CMP      R0,#+10
        ITE      EQ
        LDREQ.W  R5,??DataTable197_20
        LDRNE.W  R5,??DataTable197_19
        MOVS     R6,R8
        IT       NE
        MOVNE    R6,#+1
        LDRH     R0,[R5, R6, LSL #+1]
        CBZ.N    R0,??store_ack_0
        LDR.W    R7,??DataTable197_18
        LDRB     R0,[R7, #+25]
        CBZ.N    R0,??store_ack_1
??store_ack_0:
        MOVS     R0,#+1
        B.N      ??store_ack_2
??store_ack_1:
        BL       ??Subroutine23_0
??CrossCallReturnLabel_71:
        MOV      R9,R0
        LDRH     R5,[R5, R6, LSL #+1]
        MOV      R2,R5
        MOV      R1,R8
        BL       ??Subroutine21_0
??CrossCallReturnLabel_49:
        STRH     R5,[R7, #+16]
          CFI FunCall upload_store
        BL       upload_store
          CFI FunCall store_snapshot
        BL       store_snapshot
        MOV      R5,R0
        MOV      R2,R9
        MOV      R1,R8
        BL       ??Subroutine21_0
??CrossCallReturnLabel_48:
          CFI FunCall upload_store
        BL       upload_store
        CBZ.N    R5,??store_ack_3
        MOVS     R0,#+2
        STRB     R0,[R7, #+18]
        B.N      ??store_ack_4
??store_ack_3:
        CMP      R4,#+10
        IT       NE
        ADDNE    R6,R6,#+2
        LDRB     R1,[R7, #+30]
        MOVS     R0,#+1
        LSL      R6,R0,R6
        BIC      R6,R1,R6
        STRB     R6,[R7, #+30]
??store_ack_4:
        MOVS     R0,#+0
??store_ack_2:
        POP      {R1,R4-R9,PC}
          CFI EndBlock cfiBlock506

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond507 Using cfiCommon0
          CFI Function upload_finish
          CFI Conditional ??CrossCallReturnLabel_75
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
          CFI Block cfiCond508 Using cfiCommon0
          CFI (cfiCond508) Function store_finish
          CFI (cfiCond508) Conditional ??CrossCallReturnLabel_74
          CFI (cfiCond508) R4 Frame(CFA, -12)
          CFI (cfiCond508) R5 Frame(CFA, -8)
          CFI (cfiCond508) R14 Frame(CFA, -4)
          CFI (cfiCond508) CFA R13+16
          CFI Block cfiPicker509 Using cfiCommon1
          CFI (cfiPicker509) NoFunction
          CFI (cfiPicker509) Picker
        THUMB
?Subroutine23:
        MOVS     R1,#+1
          CFI Block cfiCond510 Using cfiCommon0
          CFI (cfiCond510) Function upload_ack
          CFI (cfiCond510) Conditional ??CrossCallReturnLabel_73
          CFI (cfiCond510) R4 Frame(CFA, -24)
          CFI (cfiCond510) R5 Frame(CFA, -20)
          CFI (cfiCond510) R6 Frame(CFA, -16)
          CFI (cfiCond510) R7 Frame(CFA, -12)
          CFI (cfiCond510) R8 Frame(CFA, -8)
          CFI (cfiCond510) R14 Frame(CFA, -4)
          CFI (cfiCond510) CFA R13+24
          CFI Block cfiCond511 Using cfiCommon0
          CFI (cfiCond511) Function upload_finish
          CFI (cfiCond511) Conditional ??CrossCallReturnLabel_72
          CFI (cfiCond511) R4 Frame(CFA, -20)
          CFI (cfiCond511) R5 Frame(CFA, -16)
          CFI (cfiCond511) R6 Frame(CFA, -12)
          CFI (cfiCond511) R7 Frame(CFA, -8)
          CFI (cfiCond511) R14 Frame(CFA, -4)
          CFI (cfiCond511) CFA R13+24
          CFI Block cfiCond512 Using cfiCommon0
          CFI (cfiCond512) Function store_ack
          CFI (cfiCond512) Conditional ??CrossCallReturnLabel_71
          CFI (cfiCond512) R4 Frame(CFA, -28)
          CFI (cfiCond512) R5 Frame(CFA, -24)
          CFI (cfiCond512) R6 Frame(CFA, -20)
          CFI (cfiCond512) R7 Frame(CFA, -16)
          CFI (cfiCond512) R8 Frame(CFA, -12)
          CFI (cfiCond512) R9 Frame(CFA, -8)
          CFI (cfiCond512) R14 Frame(CFA, -4)
          CFI (cfiCond512) CFA R13+32
          CFI Block cfiCond513 Using cfiCommon0
          CFI (cfiCond513) Function store_finish
          CFI (cfiCond513) Conditional ??CrossCallReturnLabel_70
          CFI (cfiCond513) R4 Frame(CFA, -12)
          CFI (cfiCond513) R5 Frame(CFA, -8)
          CFI (cfiCond513) R14 Frame(CFA, -4)
          CFI (cfiCond513) CFA R13+16
??Subroutine23_0:
        MOV      R0,R4
          CFI (cfiCond507) FunCall upload_finish upload_index
          CFI (cfiCond508) FunCall store_finish upload_index
          CFI (cfiCond510) FunCall upload_ack upload_index
          CFI (cfiCond507) FunCall upload_finish upload_index
          CFI (cfiCond512) FunCall store_ack upload_index
          CFI (cfiCond508) FunCall store_finish upload_index
        B.W      upload_index
          CFI EndBlock cfiCond507
          CFI EndBlock cfiCond508
          CFI EndBlock cfiPicker509
          CFI EndBlock cfiCond510
          CFI EndBlock cfiCond511
          CFI EndBlock cfiCond512
          CFI EndBlock cfiCond513

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond514 Using cfiCommon0
          CFI Function upload_reset
          CFI Conditional ??CrossCallReturnLabel_54
          CFI R4 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI Block cfiCond515 Using cfiCommon0
          CFI (cfiCond515) Function upload_finish
          CFI (cfiCond515) Conditional ??CrossCallReturnLabel_53
          CFI (cfiCond515) R4 Frame(CFA, -20)
          CFI (cfiCond515) R5 Frame(CFA, -16)
          CFI (cfiCond515) R6 Frame(CFA, -12)
          CFI (cfiCond515) R7 Frame(CFA, -8)
          CFI (cfiCond515) R14 Frame(CFA, -4)
          CFI (cfiCond515) CFA R13+24
          CFI Block cfiPicker516 Using cfiCommon1
          CFI (cfiPicker516) NoFunction
          CFI (cfiPicker516) Picker
        THUMB
?Subroutine21:
        MOVS     R1,#+1
          CFI Block cfiCond517 Using cfiCommon0
          CFI (cfiCond517) Function upload_ack
          CFI (cfiCond517) Conditional ??CrossCallReturnLabel_52
          CFI (cfiCond517) R4 Frame(CFA, -24)
          CFI (cfiCond517) R5 Frame(CFA, -20)
          CFI (cfiCond517) R6 Frame(CFA, -16)
          CFI (cfiCond517) R7 Frame(CFA, -12)
          CFI (cfiCond517) R8 Frame(CFA, -8)
          CFI (cfiCond517) R14 Frame(CFA, -4)
          CFI (cfiCond517) CFA R13+24
          CFI Block cfiCond518 Using cfiCommon0
          CFI (cfiCond518) Function upload_ack
          CFI (cfiCond518) Conditional ??CrossCallReturnLabel_51
          CFI (cfiCond518) R4 Frame(CFA, -24)
          CFI (cfiCond518) R5 Frame(CFA, -20)
          CFI (cfiCond518) R6 Frame(CFA, -16)
          CFI (cfiCond518) R7 Frame(CFA, -12)
          CFI (cfiCond518) R8 Frame(CFA, -8)
          CFI (cfiCond518) R14 Frame(CFA, -4)
          CFI (cfiCond518) CFA R13+24
          CFI Block cfiCond519 Using cfiCommon0
          CFI (cfiCond519) Function upload_finish
          CFI (cfiCond519) Conditional ??CrossCallReturnLabel_50
          CFI (cfiCond519) R4 Frame(CFA, -20)
          CFI (cfiCond519) R5 Frame(CFA, -16)
          CFI (cfiCond519) R6 Frame(CFA, -12)
          CFI (cfiCond519) R7 Frame(CFA, -8)
          CFI (cfiCond519) R14 Frame(CFA, -4)
          CFI (cfiCond519) CFA R13+24
          CFI Block cfiCond520 Using cfiCommon0
          CFI (cfiCond520) Function store_ack
          CFI (cfiCond520) Conditional ??CrossCallReturnLabel_49
          CFI (cfiCond520) R4 Frame(CFA, -28)
          CFI (cfiCond520) R5 Frame(CFA, -24)
          CFI (cfiCond520) R6 Frame(CFA, -20)
          CFI (cfiCond520) R7 Frame(CFA, -16)
          CFI (cfiCond520) R8 Frame(CFA, -12)
          CFI (cfiCond520) R9 Frame(CFA, -8)
          CFI (cfiCond520) R14 Frame(CFA, -4)
          CFI (cfiCond520) CFA R13+32
          CFI Block cfiCond521 Using cfiCommon0
          CFI (cfiCond521) Function store_ack
          CFI (cfiCond521) Conditional ??CrossCallReturnLabel_48
          CFI (cfiCond521) R4 Frame(CFA, -28)
          CFI (cfiCond521) R5 Frame(CFA, -24)
          CFI (cfiCond521) R6 Frame(CFA, -20)
          CFI (cfiCond521) R7 Frame(CFA, -16)
          CFI (cfiCond521) R8 Frame(CFA, -12)
          CFI (cfiCond521) R9 Frame(CFA, -8)
          CFI (cfiCond521) R14 Frame(CFA, -4)
          CFI (cfiCond521) CFA R13+32
??Subroutine21_0:
        MOV      R0,R4
          CFI (cfiCond514) FunCall upload_reset upload_setIndex
          CFI (cfiCond515) FunCall upload_finish upload_setIndex
          CFI (cfiCond517) FunCall upload_ack upload_setIndex
          CFI (cfiCond517) FunCall upload_ack upload_setIndex
          CFI (cfiCond515) FunCall upload_finish upload_setIndex
          CFI (cfiCond520) FunCall store_ack upload_setIndex
          CFI (cfiCond520) FunCall store_ack upload_setIndex
        B.W      upload_setIndex
          CFI EndBlock cfiCond514
          CFI EndBlock cfiCond515
          CFI EndBlock cfiPicker516
          CFI EndBlock cfiCond517
          CFI EndBlock cfiCond518
          CFI EndBlock cfiCond519
          CFI EndBlock cfiCond520
          CFI EndBlock cfiCond521

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock522 Using cfiCommon0
          CFI Function store_finish
        THUMB
store_finish:
        PUSH     {R3-R5,LR}
          CFI R14 Frame(CFA, -4)
          CFI R5 Frame(CFA, -8)
          CFI R4 Frame(CFA, -12)
          CFI CFA R13+16
        MOV      R4,R0
        CMP      R0,#+10
        LDR.N    R0,??DataTable193_1
        LDRH     R1,[R0, #+129]
        BNE.N    ??store_finish_0
        UBFX     R1,R1,#+12,#+1
        CMP      R1,#+0
        ITT      EQ
        LDRHEQ   R0,[R0, #+89]
        CMPEQ    R0,#+0
        BNE.N    ??store_finish_1
        B.N      ??store_finish_2
??store_finish_0:
        UBFX     R1,R1,#+14,#+1
        CMP      R1,#+0
        ITT      EQ
        LDRHEQ   R0,[R0, #+83]
        CMPEQ    R0,#+0
        BNE.N    ??store_finish_3
??store_finish_2:
        BL       ?Subroutine23
??CrossCallReturnLabel_74:
        CBNZ.N   R0,??store_finish_4
        MOVS     R1,#+0
        BL       ??Subroutine23_0
??CrossCallReturnLabel_70:
        CBNZ.N   R0,??store_finish_4
        MOVS     R0,#+1
        POP      {R1,R4,R5,PC}
??store_finish_4:
        CMP      R4,#+10
        BNE.N    ??store_finish_3
??store_finish_1:
        LDR.N    R0,??DataTable193
        LDR      R0,[R0, #+56]
        MOVS     R2,#+56
        B.N      ??store_finish_5
??store_finish_3:
        LDR.N    R0,??DataTable193
        LDR      R0,[R0, #+64]
        MOVS     R2,#+64
??store_finish_5:
        LDR.N    R1,??DataTable193
        ADD      R1,R1,R2
        LDR      R1,[R1, #+4]
        ADDS     R1,R1,R0
        CMP      R4,#+10
        LDR.N    R2,??DataTable193_1
        ITE      EQ
        LDREQ    R2,[R2, #+91]
        LDRNE    R2,[R2, #+85]
        CMP      R0,R2
        BCS.N    ??store_finish_6
        CMP      R2,R1
        IT       LS
        MOVLS    R1,R2
??store_finish_6:
        LDR.W    R5,??DataTable197_18
        STRB     R4,[R5, #+19]
        LSRS     R0,R0,#+12
        LSLS     R0,R0,#+12
        STR      R0,[R5, #+0]
        STR      R1,[R5, #+4]
        MOV      R0,R4
          CFI FunCall store_marker
        BL       store_marker
        MOVS     R0,#+3
        STRB     R0,[R5, #+18]
          CFI FunCall store_snapshot
        BL       store_snapshot
        CBNZ.N   R0,??store_finish_7
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        ADD      R0,R0,#+1000
        STR      R0,[R5, #+8]
??store_finish_7:
        MOVS     R0,#+0
        POP      {R1,R4,R5,PC}
          CFI EndBlock cfiBlock522

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock523 Using cfiCommon0
          CFI Function store_command_start
        THUMB
store_command_start:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        MOV      R4,R0
        MOV      R5,R1
        MOV      R6,R2
        LDR.W    R7,??DataTable197_18
        STRB     R4,[R7, #+21]
        STRB     R5,[R7, #+22]
        STRB     R6,[R7, #+23]
        MOVS     R0,#+0
        STRB     R0,[R7, #+24]
          CFI FunCall store_snapshot
        BL       store_snapshot
        CBZ.N    R0,??store_command_start_0
        MOVS     R0,#+9
        STRB     R0,[R7, #+18]
        MOVS     R1,#+0
        LDR.W    R0,??DataTable197_23
        STRB     R1,[R0, #+0]
        POP      {R0,R4-R7,PC}
??store_command_start_0:
        BL       ?Subroutine71
??CrossCallReturnLabel_252:
        MOV      R0,R4
        B.N      ?Subroutine4
          CFI EndBlock cfiBlock523

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable190:
        DATA32
        DC32     id_endScan

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond524 Using cfiCommon0
          CFI Function user_parse_comand
          CFI Conditional ??CrossCallReturnLabel_249
          CFI R4 Frame(CFA, -36)
          CFI R5 Frame(CFA, -32)
          CFI R6 Frame(CFA, -28)
          CFI R7 Frame(CFA, -24)
          CFI R8 Frame(CFA, -20)
          CFI R9 Frame(CFA, -16)
          CFI R10 Frame(CFA, -12)
          CFI R11 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+64
          CFI Block cfiCond525 Using cfiCommon0
          CFI (cfiCond525) Function user_parse_comand
          CFI (cfiCond525) Conditional ??CrossCallReturnLabel_250
          CFI (cfiCond525) R4 Frame(CFA, -36)
          CFI (cfiCond525) R5 Frame(CFA, -32)
          CFI (cfiCond525) R6 Frame(CFA, -28)
          CFI (cfiCond525) R7 Frame(CFA, -24)
          CFI (cfiCond525) R8 Frame(CFA, -20)
          CFI (cfiCond525) R9 Frame(CFA, -16)
          CFI (cfiCond525) R10 Frame(CFA, -12)
          CFI (cfiCond525) R11 Frame(CFA, -8)
          CFI (cfiCond525) R14 Frame(CFA, -4)
          CFI (cfiCond525) CFA R13+64
          CFI Block cfiCond526 Using cfiCommon0
          CFI (cfiCond526) Function user_parse_comand
          CFI (cfiCond526) Conditional ??CrossCallReturnLabel_251
          CFI (cfiCond526) R4 Frame(CFA, -36)
          CFI (cfiCond526) R5 Frame(CFA, -32)
          CFI (cfiCond526) R6 Frame(CFA, -28)
          CFI (cfiCond526) R7 Frame(CFA, -24)
          CFI (cfiCond526) R8 Frame(CFA, -20)
          CFI (cfiCond526) R9 Frame(CFA, -16)
          CFI (cfiCond526) R10 Frame(CFA, -12)
          CFI (cfiCond526) R11 Frame(CFA, -8)
          CFI (cfiCond526) R14 Frame(CFA, -4)
          CFI (cfiCond526) CFA R13+64
          CFI Block cfiCond527 Using cfiCommon0
          CFI (cfiCond527) Function store_command_start
          CFI (cfiCond527) Conditional ??CrossCallReturnLabel_252
          CFI (cfiCond527) R4 Frame(CFA, -20)
          CFI (cfiCond527) R5 Frame(CFA, -16)
          CFI (cfiCond527) R6 Frame(CFA, -12)
          CFI (cfiCond527) R7 Frame(CFA, -8)
          CFI (cfiCond527) R14 Frame(CFA, -4)
          CFI (cfiCond527) CFA R13+24
          CFI Block cfiPicker528 Using cfiCommon1
          CFI (cfiPicker528) NoFunction
          CFI (cfiPicker528) Picker
        THUMB
?Subroutine71:
        MOV      R3,R6
        MOV      R2,R5
        MOVS     R1,#+2
        BX       LR
          CFI EndBlock cfiCond524
          CFI EndBlock cfiCond525
          CFI EndBlock cfiCond526
          CFI EndBlock cfiCond527
          CFI EndBlock cfiPicker528

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock529 Using cfiCommon0
          CFI Function store_config_complete
        THUMB
store_config_complete:
        PUSH     {R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI FunCall mem_config_snapshot
        BL       mem_config_snapshot
          CFI FunCall rs485_set_config
        BL       rs485_set_config
        LDR.N    R0,??DataTable193_1
        LDR.W    R1,??DataTable197_4
        LDR      R1,[R1, #+0]
        STR      R1,[R0, #+27]
        LDRH     R1,[R0, #+129]
        UBFX     R1,R1,#+0,#+15
        STRH     R1,[R0, #+129]
        MOVS     R0,#+2
          CFI FunCall sync_packet
        BL       sync_packet
          CFI FunCall comm_modem_state
        BL       comm_modem_state
        CBNZ.N   R0,??store_config_complete_0
        POP      {R0,LR}
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall user_comm_boot
        B.N      user_comm_boot
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
??store_config_complete_0:
        POP      {R0,PC}
          CFI EndBlock cfiBlock529

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable191:
        DATA32
        DC32     log_pending

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock530 Using cfiCommon0
          CFI Function store_config_start
        THUMB
store_config_start:
        PUSH     {R4,LR}
          CFI R14 Frame(CFA, -4)
          CFI R4 Frame(CFA, -8)
          CFI CFA R13+8
        LDR.W    R4,??DataTable197_18
        LDRB     R0,[R4, #+18]
        CBNZ.N   R0,??store_config_start_0
          CFI FunCall store_snapshot
        BL       store_snapshot
        CBNZ.N   R0,??store_config_start_1
??store_config_start_0:
        MOVS     R0,#+0
        POP      {R4,PC}
??store_config_start_1:
        MOVS     R0,#+6
        STRB     R0,[R4, #+18]
        MOVS     R0,#+1
        POP      {R4,PC}
          CFI EndBlock cfiBlock530

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable192:
        DATA32
        DC32     store_runtime

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock531 Using cfiCommon0
          CFI Function store_reply
        THUMB
store_reply:
        PUSH     {R3-R7,LR}
          CFI R14 Frame(CFA, -4)
          CFI R7 Frame(CFA, -8)
          CFI R6 Frame(CFA, -12)
          CFI R5 Frame(CFA, -16)
          CFI R4 Frame(CFA, -20)
          CFI CFA R13+24
        LDR.W    R4,??DataTable197_18
        LDRB     R5,[R4, #+21]
        LDRB     R6,[R4, #+23]
        LDRB     R7,[R4, #+22]
        LDRB     R0,[R4, #+24]
        CBNZ.N   R0,??store_reply_0
        CMP      R7,#+1
        BEQ.N    ??store_reply_1
??store_reply_2:
        CMP R5,#12
        BNE sc_store_reply_other
        MOV R0,R7
        MOV R1,R6
          CFI FunCall service_crc_begin_config
        BL service_crc_begin_config
        CBZ R0,sc_store_reply_wait
        MOVS R0,#0
        STRB R0,[R4,#18]
sc_store_reply_wait:
        POP {R0,R4-R7,PC}
sc_store_reply_other:
        MOVS     R0,#+0
        STRB     R0,[R4, #+18]
        CMP      R5,#+10
        BNE.N    ??store_reply_3
        MOV      R1,R6
        MOV      R0,R7
        POP      {R2,R4-R7,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R7 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall user_send_logRecords
        B.W      user_send_logRecords
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
??store_reply_1:
          CFI FunCall comm_tx_busy
        BL       comm_tx_busy
        CMP      R0,#+0
        BEQ.N    ??store_reply_2
        POP      {R0,R4-R7,PC}
??store_reply_0:
        MOVS     R0,#+0
        STRB     R0,[R4, #+18]
        POP      {R0,R4-R7,PC}
??store_reply_3:
        CMP      R5,#+9
        BNE.N    ??store_reply_4
        MOV      R1,R6
        MOV      R0,R7
        POP      {R2,R4-R7,LR}
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R7 SameValue
          CFI R14 SameValue
          CFI CFA R13+0
          CFI FunCall user_send_tempIDs
        B.W      user_send_tempIDs
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+24
??store_reply_4:
        CMP      R5,#+12
        BNE.N    ??store_reply_5
        LDR.N    R0,??DataTable193
        LDR      R1,[R0, #+52]
        LDR      R0,[R0, #+48]
          CFI FunCall user_calcCRC16InFlash
        BL       user_calcCRC16InFlash
        STR      R6,[SP, #+0]
        MOV      R3,R7
        MOV      R2,R0
        LDR.W    R0,??DataTable197_4
        LDR      R1,[R0, #+0]
        MOVS     R0,#+4
          CFI FunCall user_send_timeshtamp
        BL       user_send_timeshtamp
        POP      {R0,R4-R7,PC}
??store_reply_5:
        MOV      R3,R6
        MOV      R2,R7
        MOVS     R1,#+0
        MOV      R0,R5
          CFI EndBlock cfiBlock531
        REQUIRE ?Subroutine4

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock532 Using cfiCommon0
          CFI NoFunction
          CFI CFA R13+24
          CFI R4 Frame(CFA, -20)
          CFI R5 Frame(CFA, -16)
          CFI R6 Frame(CFA, -12)
          CFI R7 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
        THUMB
?Subroutine4:
        ADD      SP,SP,#+4
          CFI CFA R13+20
        POP      {R4-R7,LR}
          CFI CFA R13+0
          CFI R4 SameValue
          CFI R5 SameValue
          CFI R6 SameValue
          CFI R7 SameValue
          CFI R14 SameValue
          CFI FunCall store_command_start user_send_answer_comand
          CFI FunCall store_reply user_send_answer_comand
        B.W      user_send_answer_comand
          CFI EndBlock cfiBlock532

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable193:
        DATA32
        DC32     mem

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable193_1:
        DATA32
        DC32     device

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable193_2:
        DATA32
        DC32     loop_max_ms

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable193_3:
        DATA32
        DC32     loop_stalls

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable193_4:
        DATA32
        DC32     mem_config_errors

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable193_5:
        DATA32
        DC32     fault_record

        SECTION `.text`:CODE:NOROOT(2)
        DATA
ix_owner_mem:
        DC32 mem

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiBlock533 Using cfiCommon0
          CFI Function user_storage_poll
        THUMB
user_storage_poll:
        PUSH     {R4-R6,LR}
          CFI R14 Frame(CFA, -4)
          CFI R6 Frame(CFA, -8)
          CFI R5 Frame(CFA, -12)
          CFI R4 Frame(CFA, -16)
          CFI CFA R13+16
        SUB      SP,SP,#+64
          CFI CFA R13+80
          CFI FunCall store_usb_session
        BL       store_usb_session
          CFI FunCall idindex_runtime_active
        BL idindex_runtime_active
        CBZ R0,ix_owner_poll_idle
        LDR.W R0,ix_owner_mem
        LDR R0,[R0,#16]
          CFI FunCall idindex_poll
        BL idindex_poll
        B.W ??user_storage_poll_1
ix_owner_poll_idle:
          CFI FunCall service_crc_active
        BL service_crc_active
        CBZ R0,sc_storage_idle
          CFI FunCall service_crc_poll
        BL service_crc_poll
        B.W ??user_storage_poll_1
sc_storage_idle:
          CFI FunCall expiry_runtime_active
        BL expiry_runtime_active
        CBZ R0,ex_storage_idle
          CFI FunCall timers_getMillis
        BL timers_getMillis
          CFI FunCall expiry_runtime_poll
        BL expiry_runtime_poll
        B.W ??user_storage_poll_1
ex_storage_idle:
          CFI FunCall timers_getMillis
        BL       timers_getMillis
        MOV      R5,R0
          CFI FunCall mem_config_result
        BL       mem_config_result
        MOV      R6,R0
          CFI FunCall table_runtime_active
        BL       table_runtime_active
        CBZ.N    R0,??user_storage_poll_0
        MOV      R0,R5
          CFI FunCall table_runtime_poll
        BL       table_runtime_poll
        B.N      ??user_storage_poll_1
??user_storage_poll_0:
          CFI FunCall photo_storage_busy
        BL       photo_storage_busy
        CBZ.N    R0,??user_storage_poll_2
        MOV      R0,R5
          CFI FunCall photo_storage_poll
        BL       photo_storage_poll
        B.N      ??user_storage_poll_1
??user_storage_poll_2:
          CFI FunCall fw_async_active
        BL       fw_async_active
        CBZ.N    R0,??user_storage_poll_3
        MOV      R0,R5
          CFI FunCall fw_storage_poll
        BL       fw_storage_poll
        B.N      ??user_storage_poll_1
??user_storage_poll_3:
          CFI FunCall log_async_active
        BL       log_async_active
        CMP      R0,#+0
        BNE.W    ??user_storage_poll_4
        CMP      R6,#+1
        BNE.N    ??user_storage_poll_5
        MOV      R0,R5
          CFI FunCall mem_config_poll
        BL       mem_config_poll
        B.N      ??user_storage_poll_1
??user_storage_poll_5:
        LDR.W    R4,??DataTable197_18
        LDRB     R0,[R4, #+18]
        CMP      R0,#+8
        BNE.N    ??user_storage_poll_6
          CFI FunCall store_reply
        BL       store_reply
??user_storage_poll_7:
        B.N      ??user_storage_poll_1
??user_storage_poll_6:
        CMP      R0,#+10
        BNE.N    ??user_storage_poll_8
        LDRB     R0,[R4, #+19]
        CMP      R0,#+10
        ITE      EQ
        MOVEQ    R1,#+56
        MOVNE    R1,#+64
        LDR.W    R0,??DataTable197_24
        LDR      R6,[R0, R1]
        LDR      R0,[R4, #+8]
        SUBS     R0,R5,R0
        BMI.N    ??user_storage_poll_7
        MOVS     R2,#+64
        MOV      R1,SP
        LDR      R0,[R4, #+0]
          CFI FunCall w25_read_data
        BL       w25_read_data
        CMP      R0,#+0
        BEQ.N    ??user_storage_poll_9
        MOVS     R1,#+0
??user_storage_poll_10:
        UXTH     R2,R1
        LDRB     R0,[SP, R2]
        CMP      R0,#+255
        BNE.N    ??user_storage_poll_11
        ADDS     R1,R1,#+1
        UXTH     R0,R1
        CMP      R0,#+64
        BCC.N    ??user_storage_poll_10
        B.N      ??user_storage_poll_12
??user_storage_poll_11:
        LDRB     R0,[R4, #+19]
          CFI FunCall upload_reset
        BL       upload_reset
        MOVS     R1,#+0
        MOVS     R0,#+0
          CFI FunCall store_marker
        BL       store_marker
        MOVS     R0,#+11
        B.N      ??user_storage_poll_13
??user_storage_poll_12:
        LDR      R0,[R4, #+0]
        ADDS     R0,R0,#+64
        LSLS     R1,R0,#+20
        BEQ.N    ??user_storage_poll_14
        LDR      R1,[R4, #+4]
        CMP      R0,R1
        BCC.N    ??user_storage_poll_15
??user_storage_poll_14:
        LSRS     R6,R6,#+12
        LSLS     R0,R6,#+12
        MOVS     R1,#+4
        STRB     R1,[R4, #+18]
        B.N      ??user_storage_poll_15
??user_storage_poll_8:
        CMP      R0,#+4
        BNE.N    ??user_storage_poll_16
        LDR      R0,[R4, #+8]
        SUBS     R0,R5,R0
        BMI.N    ??user_storage_poll_17
        LDRB     R0,[R4, #+20]
        CBZ.N    R0,??user_storage_poll_18
        BL       ?Subroutine41
??CrossCallReturnLabel_143:
        CMP      R0,#+1
        BEQ.N    ??user_storage_poll_17
        MOVS     R1,#+0
        STRB     R1,[R4, #+20]
        CMP      R0,#+2
        BNE.W    ??user_storage_poll_19
        LDR      R0,[R4, #+0]
        ADD      R0,R0,#+4096
??user_storage_poll_15:
        STR      R0,[R4, #+0]
??user_storage_poll_17:
        B.N      ??user_storage_poll_1
??user_storage_poll_18:
        LDR      R1,[R4, #+0]
        LDR      R0,[R4, #+4]
        CMP      R1,R0
        BCS.N    ??user_storage_poll_20
        BL       ??Subroutine43_0
??CrossCallReturnLabel_153:
        LDR      R0,[R4, #+0]
          CFI FunCall w25_async_start
        BL       w25_async_start
        MOVS     R1,R0
        ITT      EQ
        ADDEQ    R5,R5,#+1000
        STREQ    R5,[R4, #+8]
        STRB     R0,[R4, #+20]
        B.N      ??user_storage_poll_1
??user_storage_poll_20:
        LDRB     R0,[R4, #+19]
          CFI FunCall upload_reset
        BL       upload_reset
        LDRB     R0,[R4, #+19]
        CMP      R0,#+10
        LDR.N    R1,??DataTable197
        BNE.N    ??user_storage_poll_21
        MOVS     R0,#+0
        STRH     R0,[R1, #+89]
        LDRH     R2,[R1, #+129]
        MOVW     R0,#+61439
        BL       ?Subroutine58
??CrossCallReturnLabel_205:
        LDR      R0,[R0, #+56]
        STR      R0,[R1, #+91]
        B.N      ??user_storage_poll_22
??user_storage_poll_21:
        MOVS     R0,#+0
        STRH     R0,[R1, #+83]
        LDRH     R2,[R1, #+129]
        MOVW     R0,#+49151
        BL       ?Subroutine58
??CrossCallReturnLabel_206:
        LDR      R0,[R0, #+64]
        STR      R0,[R1, #+85]
??user_storage_poll_22:
        MOVS     R1,#+0
        MOVS     R0,#+0
          CFI FunCall store_marker
        BL       store_marker
        MOVS     R0,#+5
??user_storage_poll_13:
        STRB     R0,[R4, #+18]
        B.N      ??user_storage_poll_23
??user_storage_poll_16:
        CMP      R6,#+3
        BNE.N    ??user_storage_poll_24
          CFI FunCall mem_config_reset
        BL       mem_config_reset
        LDRB     R0,[R4, #+18]
        CMP      R0,#+3
        ITT      NE
        CMPNE    R0,#+5
        CMPNE    R0,#+11
??user_storage_poll_9:
        BEQ.N    ??user_storage_poll_19
        MOV      R1,R0
        MOVS     R0,#+0
        STRB     R0,[R4, #+18]
        CMP      R1,#+1
        BNE.N    ??user_storage_poll_25
        LDR.N    R0,??DataTable197_23
        STRB     R1,[R0, #+0]
        B.N      ??user_storage_poll_1
??user_storage_poll_25:
        CMP      R1,#+2
        BNE.N    ??user_storage_poll_26
        LDRB     R0,[R4, #+21]
        CMP      R0,#+10
        IT       EQ
        MOVEQ    R1,#+0
        LDRB     R0,[R4, #+22]
        CBZ.N    R0,??user_storage_poll_27
        ADDS     R1,R1,#+1
??user_storage_poll_27:
        LDRB     R2,[R4, #+30]
        MOVS     R0,#+1
        LSL      R1,R0,R1
        BIC      R1,R2,R1
        STRB     R1,[R4, #+30]
??user_storage_poll_28:
        B.N      ??user_storage_poll_1
??user_storage_poll_26:
        CMP      R1,#+6
        ITEE     NE
        CMPNE    R1,#+9
        LDRBEQ   R0,[R4, #+24]
        CMPEQ    R0,#+0
        BNE.N    ??user_storage_poll_28
        LDRB     R3,[R4, #+23]
        LDRB     R2,[R4, #+22]
        MOVS     R1,#+2
        LDRB     R0,[R4, #+21]
          CFI FunCall user_send_answer_comand
        BL       user_send_answer_comand
        B.N      ??user_storage_poll_1
??user_storage_poll_24:
        CMP      R6,#+2
        BNE.N    ??user_storage_poll_29
        MOV      R6,R0
        CMP      R6,#+6
          CFI FunCall store_config_complete
        IT       EQ
        BLEQ     store_config_complete
          CFI FunCall mem_config_reset
        BL       mem_config_reset
        CMP      R6,#+2
        BNE.N    ??user_storage_poll_30
        LDRB     R0,[R4, #+21]
        CMP      R0,#+10
        ITE      EQ
        LDREQ.N  R5,??DataTable197_20
        LDRNE.N  R5,??DataTable197_19
        LDRH     R2,[R4, #+16]
        LDRB     R1,[R4, #+22]
          CFI FunCall upload_setIndex
        BL       upload_setIndex
          CFI FunCall upload_store
        BL       upload_store
        LDRB     R0,[R4, #+22]
        CBZ.N    R0,??user_storage_poll_31
        ADDS     R5,R5,#+2
??user_storage_poll_31:
        MOVS     R0,#+0
        STRH     R0,[R5, #+0]
        B.N      ??user_storage_poll_32
??user_storage_poll_30:
        CMP      R6,#+3
        BNE.N    ??user_storage_poll_33
        LDRB     R0,[R4, #+19]
          CFI FunCall upload_reset
        BL       upload_reset
        MOVS     R0,#+4
        STR      R5,[R4, #+8]
        B.N      ??user_storage_poll_34
??user_storage_poll_33:
        CMP      R6,#+5
        IT       NE
        CMPNE    R6,#+11
        BNE.N    ??user_storage_poll_35
        MOVS     R0,#+0
        STRB     R0,[R4, #+19]
        B.N      ??user_storage_poll_32
??user_storage_poll_35:
        CMP      R6,#+6
        IT       NE
        CMPNE    R6,#+9
        BNE.N    ??user_storage_poll_36
??user_storage_poll_32:
        MOVS     R0,#+8
        B.N      ??user_storage_poll_34
??user_storage_poll_36:
        CMP      R6,#+7
        BNE.N    ??user_storage_poll_37
        LDR.N    R1,??DataTable197_23
        LDR.N    R0,??DataTable197_25
        LDRB     R2,[R0, #+0]
        LDRB     R0,[R1, #+0]
        ORRS     R2,R0,R2
        BEQ.N    ??user_storage_poll_38
        MOVS     R0,#+0
        STRB     R0,[R4, #+18]
        STRB     R0,[R1, #+0]
        B.N      ??user_storage_poll_1
??user_storage_poll_38:
        MOVS     R1,#+0
        LDR.N    R0,??DataTable197_26
        STRB     R1,[R0, #+0]
        STRB     R1,[R4, #+18]
        BL       ?Subroutine34
??CrossCallReturnLabel_112:
          CFI FunCall NVIC_SystemReset
        BL       NVIC_SystemReset
        LDRB     R0,[R4, #+18]
        B.N      ??user_storage_poll_34
??user_storage_poll_37:
        MOVS     R0,#+0
        B.N      ??user_storage_poll_34
??user_storage_poll_29:
        CMP      R0,#+3
        ITT      NE
        CMPNE    R0,#+5
        CMPNE    R0,#+11
        BNE.N    ??user_storage_poll_39
        LDR      R0,[R4, #+8]
        SUBS     R0,R5,R0
        BMI.N    ??user_storage_poll_1
??user_storage_poll_23:
          CFI FunCall store_snapshot
        BL       store_snapshot
        CMP      R0,#+0
        BNE.N    ??user_storage_poll_1
??user_storage_poll_19:
        ADD      R5,R5,#+1000
        STR      R5,[R4, #+8]
        B.N      ??user_storage_poll_1
??user_storage_poll_39:
        CMP      R0,#+0
        BNE.N    ??user_storage_poll_1
          CFI FunCall w25_read_available
        BL       w25_read_available
        CMP      R0,#+0
        BEQ.N    ??user_storage_poll_1
        LDR.N    R6,??DataTable197_25
        LDRB     R0,[R6, #+0]
        CMP      R0,#+0
        ITT      NE
        LDRBNE   R0,[R4, #+19]
        CMPNE    R0,#+10
        BEQ.N    ??user_storage_poll_40
        LDR.N    R0,??DataTable197_26
        LDRB     R0,[R0, #+0]
        CBNZ.N   R0,??user_storage_poll_4
        LDR.N    R0,??DataTable197_23
        LDRB     R0,[R0, #+0]
        CBNZ.N   R0,??user_storage_poll_41
        LDR.N    R0,??DataTable197_27
        LDRB     R0,[R0, #+0]
        CBNZ.N   R0,??user_storage_poll_4
        BL       ?Subroutine12
??CrossCallReturnLabel_19:
        BCC.N    ??user_storage_poll_1
??user_storage_poll_4:
        MOV      R0,R5
          CFI FunCall log_storage_poll
        BL       log_storage_poll
        B.N      ??user_storage_poll_1
??user_storage_poll_40:
        LDR.N    R0,??DataTable197_26
        LDRB     R0,[R0, #+0]
        CBZ.N    R0,??user_storage_poll_41
          CFI FunCall log_flushPending
        BL       log_flushPending
        LDRB     R0,[R6, #+0]
        CBNZ.N   R0,??user_storage_poll_1
          CFI FunCall store_snapshot
        BL       store_snapshot
        CBZ.N    R0,??user_storage_poll_1
        MOVS     R0,#+7
        B.N      ??user_storage_poll_34
??user_storage_poll_41:
        LDR.N    R5,??DataTable197_23
        LDRB     R0,[R5, #+0]
        CBZ.N    R0,??user_storage_poll_1
          CFI FunCall store_snapshot
        BL       store_snapshot
        CBZ.N    R0,??user_storage_poll_1
        MOVS     R0,#+0
        STRB     R0,[R5, #+0]
        MOVS     R0,#+1
??user_storage_poll_34:
        STRB     R0,[R4, #+18]
??user_storage_poll_1:
        ADD      SP,SP,#+64
          CFI CFA R13+16
        POP      {R4-R6,PC}
          CFI EndBlock cfiBlock533

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond534 Using cfiCommon0
          CFI Function user_storage_poll
          CFI Conditional ??CrossCallReturnLabel_205
          CFI R4 Frame(CFA, -16)
          CFI R5 Frame(CFA, -12)
          CFI R6 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+80
          CFI Block cfiCond535 Using cfiCommon0
          CFI (cfiCond535) Function user_storage_poll
          CFI (cfiCond535) Conditional ??CrossCallReturnLabel_206
          CFI (cfiCond535) R4 Frame(CFA, -16)
          CFI (cfiCond535) R5 Frame(CFA, -12)
          CFI (cfiCond535) R6 Frame(CFA, -8)
          CFI (cfiCond535) R14 Frame(CFA, -4)
          CFI (cfiCond535) CFA R13+80
          CFI Block cfiPicker536 Using cfiCommon1
          CFI (cfiPicker536) NoFunction
          CFI (cfiPicker536) Picker
        THUMB
?Subroutine58:
        ANDS     R2,R0,R2
        STRH     R2,[R1, #+129]
        LDR.N    R0,??DataTable197_24
        BX       LR
          CFI EndBlock cfiCond534
          CFI EndBlock cfiCond535
          CFI EndBlock cfiPicker536

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable195:
        DATA32
        DC32     _A_tempStrBuffer

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond537 Using cfiCommon0
          CFI Function user_parse_comand
          CFI Conditional ??CrossCallReturnLabel_150
          CFI R4 Frame(CFA, -36)
          CFI R5 Frame(CFA, -32)
          CFI R6 Frame(CFA, -28)
          CFI R7 Frame(CFA, -24)
          CFI R8 Frame(CFA, -20)
          CFI R9 Frame(CFA, -16)
          CFI R10 Frame(CFA, -12)
          CFI R11 Frame(CFA, -8)
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+64
          CFI Block cfiCond538 Using cfiCommon0
          CFI (cfiCond538) Function user_parse_comand
          CFI (cfiCond538) Conditional ??CrossCallReturnLabel_151
          CFI (cfiCond538) R4 Frame(CFA, -36)
          CFI (cfiCond538) R5 Frame(CFA, -32)
          CFI (cfiCond538) R6 Frame(CFA, -28)
          CFI (cfiCond538) R7 Frame(CFA, -24)
          CFI (cfiCond538) R8 Frame(CFA, -20)
          CFI (cfiCond538) R9 Frame(CFA, -16)
          CFI (cfiCond538) R10 Frame(CFA, -12)
          CFI (cfiCond538) R11 Frame(CFA, -8)
          CFI (cfiCond538) R14 Frame(CFA, -4)
          CFI (cfiCond538) CFA R13+64
          CFI Block cfiPicker539 Using cfiCommon1
          CFI (cfiPicker539) NoFunction
          CFI (cfiPicker539) Picker
        THUMB
?Subroutine43:
        STR      R6,[SP, #+0]
          CFI Block cfiCond540 Using cfiCommon0
          CFI (cfiCond540) Function fw_storage_poll
          CFI (cfiCond540) Conditional ??CrossCallReturnLabel_152
          CFI (cfiCond540) R4 Frame(CFA, -16)
          CFI (cfiCond540) R5 Frame(CFA, -12)
          CFI (cfiCond540) R6 Frame(CFA, -8)
          CFI (cfiCond540) R14 Frame(CFA, -4)
          CFI (cfiCond540) CFA R13+56
          CFI Block cfiCond541 Using cfiCommon0
          CFI (cfiCond541) Function user_storage_poll
          CFI (cfiCond541) Conditional ??CrossCallReturnLabel_153
          CFI (cfiCond541) R4 Frame(CFA, -16)
          CFI (cfiCond541) R5 Frame(CFA, -12)
          CFI (cfiCond541) R6 Frame(CFA, -8)
          CFI (cfiCond541) R14 Frame(CFA, -4)
          CFI (cfiCond541) CFA R13+80
??Subroutine43_0:
        MOV      R3,R5
        MOVS     R2,#+0
        MOVS     R1,#+0
        BX       LR
          CFI EndBlock cfiCond537
          CFI EndBlock cfiCond538
          CFI EndBlock cfiPicker539
          CFI EndBlock cfiCond540
          CFI EndBlock cfiCond541

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond542 Using cfiCommon0
          CFI Function user_savereset
          CFI Conditional ??CrossCallReturnLabel_113
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI Block cfiCond543 Using cfiCommon0
          CFI (cfiCond543) Function user_storage_poll
          CFI (cfiCond543) Conditional ??CrossCallReturnLabel_112
          CFI (cfiCond543) R4 Frame(CFA, -16)
          CFI (cfiCond543) R5 Frame(CFA, -12)
          CFI (cfiCond543) R6 Frame(CFA, -8)
          CFI (cfiCond543) R14 Frame(CFA, -4)
          CFI (cfiCond543) CFA R13+80
          CFI Block cfiPicker544 Using cfiCommon1
          CFI (cfiPicker544) NoFunction
          CFI (cfiPicker544) Picker
        THUMB
?Subroutine34:
        MOV      R1,#+256
        LDR.N    R0,??DataTable197_28
        STR      R1,[R0, #+0]
          CFI (cfiCond542) FunCall user_savereset comm_modem_off
          CFI (cfiCond543) FunCall user_storage_poll comm_modem_off
        B.W      comm_modem_off
          CFI EndBlock cfiCond542
          CFI EndBlock cfiCond543
          CFI EndBlock cfiPicker544

        SECTION `.text`:CODE:NOROOT(1)
          CFI Block cfiCond545 Using cfiCommon0
          CFI Function user_storage_background_ready
          CFI Conditional ??CrossCallReturnLabel_18
          CFI R14 Frame(CFA, -4)
          CFI CFA R13+8
          CFI Block cfiCond546 Using cfiCommon0
          CFI (cfiCond546) Function user_storage_poll
          CFI (cfiCond546) Conditional ??CrossCallReturnLabel_19
          CFI (cfiCond546) R4 Frame(CFA, -16)
          CFI (cfiCond546) R5 Frame(CFA, -12)
          CFI (cfiCond546) R6 Frame(CFA, -8)
          CFI (cfiCond546) R14 Frame(CFA, -4)
          CFI (cfiCond546) CFA R13+80
          CFI Block cfiPicker547 Using cfiCommon1
          CFI (cfiPicker547) NoFunction
          CFI (cfiPicker547) Picker
        THUMB
?Subroutine12:
        LDR.N    R0,??DataTable197
        LDR      R0,[R0, #+51]
        CMP      R0,#+600
        BX       LR
          CFI EndBlock cfiCond545
          CFI EndBlock cfiCond546
          CFI EndBlock cfiPicker547

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197:
        DATA32
        DC32     device

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_1:
        DATA32
        DC32     0x31544c46

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_2:
        DATA32
        DC32     0x5f5e1000

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_3:
        DATA32
        DC32     config+161

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_4:
        DATA32
        DC32     config

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_5:
        DATA32
        DC32     `user_comm_link::first`

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_6:
        DATA32
        DC32     comm_host

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_7:
        DATA32
        DC32     comm_port

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_8:
        DATA32
        DC32     config+40

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_9:
        DATA32
        DC32     comm_host+32

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_10:
        DATA32
        DC32     `comm_text::empty`

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_11:
        DATA32
        DC32     comm_credential

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_12:
        DATA32
        DC32     `comm_number{1}{2}{3}::rat`

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_13:
        DATA32
        DC32     0x40011010

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_14:
        DATA32
        DC32     0x40011014

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_15:
        DATA32
        DC32     `user_comm_boot::handlers`

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_16:
        DATA32
        DC32     comm_settings

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_17:
        DATA32
        DC32     usb_bus_resets

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_18:
        DATA32
        DC32     store

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_19:
        DATA32
        DC32     temp_unacked

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_20:
        DATA32
        DC32     log_unacked

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_21:
        DATA32
        DC32     0x31434c0a

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_22:
        DATA32
        DC32     0x31434c09

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_23:
        DATA32
        DC32     store_save_requested

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_24:
        DATA32
        DC32     mem

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_25:
        DATA32
        DC32     log_pendingCount

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_26:
        DATA32
        DC32     store_reset_requested

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_27:
        DATA32
        DC32     log_timeKnown

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable197_28:
        DATA32
        DC32     0x40010814

        SECTION `.iar_vfe_header`:DATA:NOALLOC:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
        DC32 0

        END
