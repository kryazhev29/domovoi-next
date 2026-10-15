# Функции библиотек ReleaseCustomer

Сборка: 87 358 Б; SHA-256 47f941879a24aca3687aa16a1026a7908332771c223806e5bc154208168ae054. Все 159 именованных диапазонов из ENTRY LIST MAP, включая локальные копии inline. Совпадающие входы объединены. Размер — диапазон функции; байты таблиц, векторов, литералов и выравнивания отдельно включены в итоги модулей. Встроенные без отдельного символа CMSIS операции входят в вызывающие функции приложения.

Число вызовов — статические прямые переходы BL/B, включая условные tail-call. Ноль не означает мёртвый код: callbacks USB и таблицы инициализации вызываются косвенно. Когда работает — вывод по коду, без динамического покрытия платы. CDC содержит и код проекта.

| Группа | Функция / общий вход | Объект / адрес | Б | Мест вызова | Когда |
|---|---|---|---:|---:|---|
| IAR | `__aeabi_memcpy` | `ABImemcpy.o:0x800adbc` | 32 | 46 | обычная работа |
| IAR | `__aeabi_memcpy4/__aeabi_memcpy8` | `ABImemcpy.o:0x800addc` | 134 | 8 | обычная работа |
| IAR | `__iar_packbits_init_single3` | `packbits_init_single.o:0x800ae62` | 58 | 0 | старт |
| IAR | `__aeabi_memclr4` | `ABImemclr4.o:0x800cce8` | 6 | 18 | обычная работа |
| IAR | `__iar_small_memcmp` | `memcmp_small.o:0x800ccf0` | 30 | 21 | обычная работа |
| IAR | `__iar_small_strncpy` | `strncpy_small.o:0x8012e84` | 30 | 1 | обычная работа |
| IAR | `__aeabi_memclr` | `ABImemclr.o:0x8013680` | 6 | 11 | обычная работа |
| IAR | `__aeabi_memmove/__aeabi_memmove4/__aeabi_memmove8` | `ABImemmove.o:0x8014a18` | 150 | 6 | обычная работа |
| IAR | `strlen` | `strlen.o:0x8015920` | 54 | 11 | обычная работа |
| ST USB CDC derived | `Set_System` | `hw_config.o:0x8015958` | 18 | 1 | старт USB и повторное подключение |
| ST USB CDC derived | `Set_USBClock` | `hw_config.o:0x801596a` | 36 | 1 | старт USB и повторное подключение |
| ST USB CDC derived | `USB_Interrupts_Config` | `hw_config.o:0x801598e` | 28 | 1 | старт USB и повторное подключение |
| ST USB CDC derived | `USB_To_USART_Send_Data` | `hw_config.o:0x80159aa` | 4 | 1 | обычная работа; USB по подключению |
| ST USB CDC derived | `Handle_USBAsynchXfer` | `hw_config.o:0x80159ae` | 96 | 2 | обычная работа; USB по подключению |
| ST USB CDC derived | `USB_Tx_Reset` | `hw_config.o:0x8015a0e` | 36 | 2 | обычная работа; USB по подключению |
| ST USB CDC derived | `USB_Tx_SourceBusy` | `hw_config.o:0x8015a32` | 10 | 3 | обычная работа; USB по подключению |
| ST USB CDC derived | `usb_log_next` | `hw_config.o:0x8015a3c` | 68 | 2 | обычная работа; USB по подключению |
| ST USB CDC derived | `USB_Log_Try` | `hw_config.o:0x8015a80` | 100 | 1 | обычная работа; USB по подключению |
| ST USB CDC derived | `usb_tx_submit` | `hw_config.o:0x8015ae4` | 62 | 2 | обычная работа; USB по подключению |
| ST USB CDC derived | `USART_To_USB_Send_Data` | `hw_config.o:0x8015b22` | 38 | 1 | обычная работа; USB по подключению |
| ST USB CDC derived | `USB_Tx_CommandReady` | `hw_config.o:0x8015b48` | 14 | 1 | обычная работа; USB по подключению |
| ST USB CDC derived | `USB_Tx_Try` | `hw_config.o:0x8015b56` | 70 | 1 | обычная работа; USB по подключению |
| ST USB CDC derived | `USB_Tx_Pending` | `hw_config.o:0x8015b9c` | 18 | 2 | обычная работа; USB по подключению |
| ST USB CDC derived | `Get_SerialNum` | `hw_config.o:0x8015bae` | 44 | 1 | старт USB и повторное подключение |
| ST USB CDC derived | `IntToUnicode` | `hw_config.o:0x8015c3c` | 38 | 2 | старт USB и повторное подключение |
| IAR | `__aeabi_memset4/__aeabi_memset8` | `ABImemset48.o:0x8016614` | 8 | 1 | обычная работа |
| IAR | `__iar_Memset4_word/__iar_Memset8_word` | `ABImemset48.o:0x801661c` | 50 | 1 | обычная работа |
| IAR | `__iar_small_Memset/__iar_small_Memset_word/__iar_small_memset` | `ABImemset_small.o:0x8017a20` | 18 | 6 | обычная работа |
| IAR | `strcpy` | `strcpy.o:0x8019028` | 24 | 4 | обычная работа |
| IAR | `__iar_Memchr/memchr` | `memchr.o:0x801a304` | 88 | 3 | обычная работа |
| ST USB core | `USB_Init` | `usb_init.o:0x801ac54` | 52 | 1 | старт USB и повторное подключение |
| ST USB core | `UserToPMABufferCopy` | `usb_mem.o:0x801ac88` | 42 | 3 | обычная работа; USB по подключению |
| ST USB core | `PMAToUserBufferCopy` | `usb_mem.o:0x801acb2` | 40 | 2 | обычная работа; USB по подключению |
| ST USB core | `_SetBTABLE` | `usb_regs.o:0x801ace0` | 12 | 1 | обычная работа; USB по подключению |
| ST USB core | `_GetBTABLE` | `usb_regs.o:0x801acec` | 8 | 1 | обычная работа; USB по подключению |
| ST USB core | `_SetENDPOINT` | `usb_regs.o:0x801acf4` | 8 | 0 | обычная работа; USB по подключению |
| ST USB core | `_GetENDPOINT` | `usb_regs.o:0x801acfc` | 10 | 0 | обычная работа; USB по подключению |
| ST USB core | `_SetEPType` | `usb_regs.o:0x801ad06` | 18 | 1 | обычная работа; USB по подключению |
| ST USB core | `_SetEPTxStatus` | `usb_regs.o:0x801ad18` | 18 | 2 | обычная работа; USB по подключению |
| ST USB core | `_SetEPRxStatus` | `usb_regs.o:0x801ad30` | 16 | 2 | обычная работа; USB по подключению |
| ST USB core | `_ClearEP_KIND` | `usb_regs.o:0x801ad52` | 18 | 1 | обычная работа; USB по подключению |
| ST USB core | `_ToggleDTOG_RX` | `usb_regs.o:0x801ad64` | 16 | 1 | обычная работа; USB по подключению |
| ST USB core | `_ToggleDTOG_TX` | `usb_regs.o:0x801ad80` | 22 | 1 | обычная работа; USB по подключению |
| ST USB core | `_ClearDTOG_RX` | `usb_regs.o:0x801ad9a` | 20 | 1 | обычная работа; USB по подключению |
| ST USB core | `_ClearDTOG_TX` | `usb_regs.o:0x801adae` | 20 | 1 | обычная работа; USB по подключению |
| ST USB core | `usb_pma_field` | `usb_regs.o:0x801adc2` | 24 | 4 | обычная работа; USB по подключению |
| ST USB core | `_SetEPTxAddr` | `usb_regs.o:0x801ade8` | 8 | 1 | обычная работа; USB по подключению |
| ST USB core | `_GetEPTxAddr` | `usb_regs.o:0x801adf0` | 6 | 1 | обычная работа; USB по подключению |
| ST USB core | `_SetEPRxAddr` | `usb_regs.o:0x801adf6` | 6 | 1 | обычная работа; USB по подключению |
| ST USB core | `_GetEPRxAddr` | `usb_regs.o:0x801ae0a` | 4 | 1 | обычная работа; USB по подключению |
| ST USB core | `_SetEPTxCount` | `usb_regs.o:0x801ae18` | 14 | 1 | обычная работа; USB по подключению |
| ST USB core | `_SetEPRxCount` | `usb_regs.o:0x801ae26` | 42 | 1 | обычная работа; USB по подключению |
| ST USB core | `_GetEPRxCount` | `usb_regs.o:0x801ae50` | 16 | 1 | обычная работа; USB по подключению |
| ST USB core | `SetBTABLE` | `usb_regs.o:0x801ae60` | 2 | 1 | обычная работа; USB по подключению |
| ST USB core | `SetEPType` | `usb_regs.o:0x801ae62` | 2 | 4 | обычная работа; USB по подключению |
| ST USB core | `SetEPTxStatus` | `usb_regs.o:0x801ae64` | 2 | 5 | обычная работа; USB по подключению |
| ST USB core | `SetEPRxStatus` | `usb_regs.o:0x801ae66` | 2 | 3 | обычная работа; USB по подключению |
| ST USB core | `SetEPTxValid` | `usb_regs.o:0x801ae68` | 4 | 2 | обычная работа; USB по подключению |
| ST USB core | `SetEPRxValid` | `usb_regs.o:0x801ae6c` | 6 | 2 | обычная работа; USB по подключению |
| ST USB core | `Clear_Status_Out` | `usb_regs.o:0x801ae72` | 2 | 1 | обычная работа; USB по подключению |
| ST USB core | `ClearDTOG_RX` | `usb_regs.o:0x801ae74` | 2 | 1 | обычная работа; USB по подключению |
| ST USB core | `ClearDTOG_TX` | `usb_regs.o:0x801ae76` | 2 | 1 | обычная работа; USB по подключению |
| ST USB core | `SetEPTxAddr` | `usb_regs.o:0x801ae78` | 2 | 3 | обычная работа; USB по подключению |
| ST USB core | `SetEPRxAddr` | `usb_regs.o:0x801ae7a` | 2 | 2 | обычная работа; USB по подключению |
| ST USB core | `GetEPTxAddr` | `usb_regs.o:0x801ae7c` | 2 | 1 | обычная работа; USB по подключению |
| ST USB core | `GetEPRxAddr` | `usb_regs.o:0x801ae7e` | 2 | 2 | обычная работа; USB по подключению |
| ST USB core | `SetEPTxCount` | `usb_regs.o:0x801ae80` | 2 | 4 | обычная работа; USB по подключению |
| ST USB core | `SetEPRxCount` | `usb_regs.o:0x801ae82` | 2 | 2 | обычная работа; USB по подключению |
| ST USB core | `GetEPRxCount` | `usb_regs.o:0x801ae84` | 2 | 1 | обычная работа; USB по подключению |
| ST USB core | `ByteSwap` | `usb_regs.o:0x801ae86` | 12 | 2 | обычная работа; USB по подключению |
| IAR | `strcmp` | `strcmp.o:0x801b1d8` | 18 | 2 | обычная работа |
| IAR | `strcat` | `strcat.o:0x801b1ea` | 30 | 3 | обычная работа |
| ST USB core | `_SetDADDR` | `usb_core.o:0x801b320` | 8 | 1 | обычная работа; USB по подключению |
| ST USB core | `_GetBTABLE` | `usb_core.o:0x801b328` | 10 | 1 | обычная работа; USB по подключению |
| ST USB core | `_SetENDPOINT` | `usb_core.o:0x801b332` | 10 | 0 | обычная работа; USB по подключению |
| ST USB core | `_GetENDPOINT` | `usb_core.o:0x801b33c` | 12 | 2 | обычная работа; USB по подключению |
| ST USB core | `_SetEPTxStatus` | `usb_core.o:0x801b348` | 18 | 1 | обычная работа; USB по подключению |
| ST USB core | `_SetEPRxStatus` | `usb_core.o:0x801b35a` | 16 | 2 | обычная работа; USB по подключению |
| ST USB core | `_GetEPTxStatus` | `usb_core.o:0x801b382` | 12 | 4 | обычная работа; USB по подключению |
| ST USB core | `_GetEPRxStatus` | `usb_core.o:0x801b38e` | 12 | 4 | обычная работа; USB по подключению |
| ST USB core | `_GetTxStallStatus` | `usb_core.o:0x801b39a` | 18 | 2 | обычная работа; USB по подключению |
| ST USB core | `_GetRxStallStatus` | `usb_core.o:0x801b3ac` | 20 | 2 | обычная работа; USB по подключению |
| ST USB core | `_SetEPAddress` | `usb_core.o:0x801b3c0` | 16 | 1 | обычная работа; USB по подключению |
| ST USB core | `usb_pma_field` | `usb_core.o:0x801b3d0` | 26 | 2 | обычная работа; USB по подключению |
| ST USB core | `_GetEPRxAddr` | `usb_core.o:0x801b3ea` | 14 | 1 | обычная работа; USB по подключению |
| ST USB core | `_SetEPTxCount` | `usb_core.o:0x801b3f8` | 14 | 1 | обычная работа; USB по подключению |
| ST USB core | `Send0LengthData` | `usb_core.o:0x801b406` | 16 | 3 | обычная работа; USB по подключению |
| ST USB core | `Standard_GetConfiguration` | `usb_core.o:0x801b420` | 24 | 0 | обычная работа; USB по подключению |
| ST USB core | `Standard_SetConfiguration` | `usb_core.o:0x801b438` | 60 | 1 | обычная работа; USB по подключению |
| ST USB core | `Standard_GetInterface` | `usb_core.o:0x801b474` | 24 | 0 | обычная работа; USB по подключению |
| ST USB core | `Standard_SetInterface` | `usb_core.o:0x801b49a` | 74 | 1 | обычная работа; USB по подключению |
| ST USB core | `Standard_GetStatus` | `usb_core.o:0x801b4e4` | 162 | 0 | обычная работа; USB по подключению |
| ST USB core | `Standard_ClearFeature` | `usb_core.o:0x801b58e` | 180 | 1 | обычная работа; USB по подключению |
| ST USB core | `Standard_SetEndPointFeature` | `usb_core.o:0x801b642` | 98 | 1 | обычная работа; USB по подключению |
| ST USB core | `Standard_SetDeviceFeature` | `usb_core.o:0x801b6a4` | 30 | 1 | обычная работа; USB по подключению |
| ST USB core | `Standard_GetDescriptorData` | `usb_core.o:0x801b6c2` | 28 | 3 | обычная работа; USB по подключению |
| ST USB core | `DataStageOut` | `usb_core.o:0x801b6de` | 124 | 1 | обычная работа; USB по подключению |
| ST USB core | `DataStageIn` | `usb_core.o:0x801b75a` | 144 | 2 | обычная работа; USB по подключению |
| ST USB core | `NoData_Setup0` | `usb_core.o:0x801b7ea` | 192 | 1 | обычная работа; USB по подключению |
| ST USB core | `Data_Setup0` | `usb_core.o:0x801b8b2` | 420 | 1 | обычная работа; USB по подключению |
| ST USB core | `Setup0_Process` | `usb_core.o:0x801ba90` | 86 | 1 | обычная работа; USB по подключению |
| ST USB core | `In0_Process` | `usb_core.o:0x801bae6` | 72 | 1 | обычная работа; USB по подключению |
| ST USB core | `Out0_Process` | `usb_core.o:0x801bb34` | 44 | 1 | обычная работа; USB по подключению |
| ST USB core | `Post0_Process` | `usb_core.o:0x801bb68` | 32 | 1 | обычная работа; USB по подключению |
| ST USB core | `SetDeviceAddress` | `usb_core.o:0x801bb9e` | 38 | 2 | обычная работа; USB по подключению |
| ST USB core | `NOP_Process` | `usb_core.o:0x801bc04` | 2 | 0 | обычная работа; USB по подключению |
| ST USB CDC derived | `Virtual_Com_Port_init` | `usb_prop.o:0x801be24` | 26 | 0 | старт USB и повторное подключение |
| ST USB CDC derived | `Virtual_Com_Port_Reset` | `usb_prop.o:0x801be3e` | 220 | 0 | обычная работа; USB по подключению |
| ST USB CDC derived | `Virtual_Com_Port_SetConfiguration` | `usb_prop.o:0x801bf20` | 16 | 0 | обычная работа; USB по подключению |
| ST USB CDC derived | `Virtual_Com_Port_SetDeviceAddress` | `usb_prop.o:0x801bf30` | 8 | 0 | обычная работа; USB по подключению |
| ST USB CDC derived | `Virtual_Com_Port_Data_Setup` | `usb_prop.o:0x801bf38` | 58 | 0 | обычная работа; USB по подключению |
| ST USB CDC derived | `Virtual_Com_Port_NoData_Setup` | `usb_prop.o:0x801bf80` | 30 | 0 | обычная работа; USB по подключению |
| ST USB CDC derived | `Virtual_Com_Port_GetDeviceDescriptor` | `usb_prop.o:0x801bf9e` | 6 | 0 | обычная работа; USB по подключению |
| ST USB CDC derived | `Virtual_Com_Port_GetConfigDescriptor` | `usb_prop.o:0x801bfa4` | 6 | 0 | обычная работа; USB по подключению |
| ST USB CDC derived | `Virtual_Com_Port_GetStringDescriptor` | `usb_prop.o:0x801bfaa` | 24 | 0 | обычная работа; USB по подключению |
| ST USB CDC derived | `Virtual_Com_Port_Get_Interface_Setting` | `usb_prop.o:0x801bfc2` | 14 | 0 | обычная работа; USB по подключению |
| ST USB CDC derived | `Virtual_Com_Port_GetLineCoding` | `usb_prop.o:0x801bfd0` | 6 | 0 | обычная работа; USB по подключению |
| ST USB CDC derived | `Virtual_Com_Port_SetLineCoding` | `usb_prop.o:0x801bfd8` | 4 | 0 | обычная работа; USB по подключению |
| ST USB CDC derived | `_SetCNTR` | `usb_pwr.o:0x801c00c` | 6 | 3 | обычная работа; USB по подключению |
| ST USB CDC derived | `_SetISTR` | `usb_pwr.o:0x801c012` | 6 | 1 | обычная работа; USB по подключению |
| ST USB CDC derived | `_GetISTR` | `usb_pwr.o:0x801c018` | 8 | 1 | обычная работа; USB по подключению |
| ST USB CDC derived | `USB_WaitReset` | `usb_pwr.o:0x801c020` | 32 | 1 | обычная работа; USB по подключению |
| ST USB CDC derived | `PowerOn` | `usb_pwr.o:0x801c040` | 38 | 1 | старт USB и повторное подключение |
| ST USB core | `_SetCNTR` | `usb_sil.o:0x801c074` | 6 | 1 | обычная работа; USB по подключению |
| ST USB core | `_SetISTR` | `usb_sil.o:0x801c07a` | 6 | 1 | обычная работа; USB по подключению |
| ST USB core | `USB_SIL_Init` | `usb_sil.o:0x801c080` | 24 | 1 | обычная работа; USB по подключению |
| ST USB core | `USB_SIL_Read` | `usb_sil.o:0x801c0a4` | 38 | 1 | обычная работа; USB по подключению |
| ST USB CDC derived | `_SetCNTR` | `usb_istr.o:0x801c0cc` | 6 | 0 | обычная работа; USB по подключению |
| ST USB CDC derived | `_SetISTR` | `usb_istr.o:0x801c0d2` | 6 | 5 | обычная работа; USB по подключению |
| ST USB CDC derived | `_GetCNTR` | `usb_istr.o:0x801c0d8` | 4 | 2 | обычная работа; USB по подключению |
| ST USB CDC derived | `_GetISTR` | `usb_istr.o:0x801c0dc` | 2 | 1 | обычная работа; USB по подключению |
| ST USB CDC derived | `_GetFNR` | `usb_istr.o:0x801c0e4` | 4 | 1 | обычная работа; USB по подключению |
| ST USB CDC derived | `_SetENDPOINT` | `usb_istr.o:0x801c0e8` | 8 | 1 | обычная работа; USB по подключению |
| ST USB CDC derived | `_GetENDPOINT` | `usb_istr.o:0x801c0f0` | 10 | 1 | обычная работа; USB по подключению |
| ST USB CDC derived | `USB_Istr` | `usb_istr.o:0x801c0fa` | 242 | 1 | обычная работа; USB по подключению |
| ST USB CDC derived | `EP1_IN_Callback` | `usb_endp.o:0x801c218` | 28 | 0 | обычная работа; USB по подключению |
| ST USB CDC derived | `EP3_OUT_Callback` | `usb_endp.o:0x801c234` | 30 | 0 | обычная работа; USB по подключению |
| ST USB CDC derived | `SOF_Callback` | `usb_endp.o:0x801c252` | 30 | 1 | обычная работа; USB по подключению |
| ST USB core | `_GetISTR` | `usb_int.o:0x801c280` | 8 | 1 | обычная работа; USB по подключению |
| ST USB core | `_SetENDPOINT` | `usb_int.o:0x801c288` | 8 | 1 | обычная работа; USB по подключению |
| ST USB core | `_GetENDPOINT` | `usb_int.o:0x801c290` | 10 | 6 | обычная работа; USB по подключению |
| ST USB core | `_SetEPRxTxStatus` | `usb_int.o:0x801c29a` | 46 | 2 | обычная работа; USB по подключению |
| ST USB core | `_ClearEP_CTR_RX` | `usb_int.o:0x801c2c8` | 14 | 3 | обычная работа; USB по подключению |
| ST USB core | `_ClearEP_CTR_TX` | `usb_int.o:0x801c2d6` | 12 | 2 | обычная работа; USB по подключению |
| ST USB core | `CTR_LP` | `usb_int.o:0x801c2ec` | 186 | 1 | обычная работа; USB по подключению |
| CMSIS | `NVIC_SystemReset` | `system_stm32f10x.o:0x801c4f0` | 24 | 2 | перезапуск по команде/ошибке питания |
| CMSIS | `clock_wait` | `system_stm32f10x.o:0x801c508` | 28 | 2 | старт |
| CMSIS | `SetSysClockTo72` | `system_stm32f10x.o:0x801c524` | 122 | 1 | старт |
| CMSIS | `SystemInit` | `system_stm32f10x.o:0x801c59e` | 68 | 0 | старт |
| IAR | `__iar_zero_init3` | `zero_init3.o:0x801c9b8` | 56 | 0 | старт |
| IAR | `__iar_data_init3` | `data_init.o:0x801ca50` | 24 | 1 | старт |
| IAR | `?main/__cmain` | `cmain.o:0x801cba8` | 12 | 1 | старт |
| IAR | `_call_main` | `cmain.o:0x801cbb4` | 18 | 1 | обычная работа |
| IAR | `exit` | `exit.o:0x801cbc6` | 4 | 1 | только при возврате main; main бесконечен |
| IAR | `_exit` | `cexit.o:0x801cbcc` | 10 | 1 | только при возврате main; main бесконечен |
| IAR | `__exit` | `XXexit.o:0x801cbd8` | 12 | 1 | только при возврате main; main бесконечен |
| IAR | `__iar_program_start` | `cstartup_M.o:0x801cca8` | 28 | 0 | старт |
| CMSIS | `Reset_Handler` | `startup_stm32f10x_md.o:0x801d070` | 16 | 0 | старт |
| CMSIS | `WWDG_IRQHandler` | `startup_stm32f10x_md.o:0x801d504` | 4 | 0 | аварийная ловушка необработанного IRQ |

## Итоги модулей

| Группа | RO code, Б | RO data, Б | RW data, Б |
|---|---:|---:|---:|
| CMSIS | 540 | 4 | 4 |
| StdPeriph | 0 | 0 | 0 |
| ST USB core | 3262 | 0 | 49 |
| ST USB CDC derived | 1806 | 193 | 361 |
| IAR | 918 | 0 | 0 |

Обычный путь не возвращается из main: exit/_exit/__exit = 26 Б. Аварийная ловушка IRQ (4 Б) сохранена как защита. CTR_HP, Resume/Resume_Init/внутренняя процедура remote-wakeup в итоговом MAP отсутствуют — 0 Б. Standard_SetDeviceFeature (30 Б) вызывается по управляющему запросу USB: это обработка разрешения remote wakeup, а не неисполняемый генератор пробуждения.

## Удалённые файлы

- `CMSIS/Include/arm_common_tables.h`
- `CMSIS/Include/arm_math.h`
- `CMSIS/Include/core_cm0.h`
- `CMSIS/Include/core_cm4.h`
- `CMSIS/Include/core_cm4_simd.h`
- `CMSIS/STM32F10x/Source/startup_stm32f10x_cl.s`
- `CMSIS/STM32F10x/Source/startup_stm32f10x_hd.s`
- `CMSIS/STM32F10x/Source/startup_stm32f10x_hd_vl.s`
- `CMSIS/STM32F10x/Source/startup_stm32f10x_ld.s`
- `CMSIS/STM32F10x/Source/startup_stm32f10x_ld_vl.s`
- `CMSIS/STM32F10x/Source/startup_stm32f10x_md_vl.s`
- `CMSIS/STM32F10x/Source/startup_stm32f10x_xl.s`
- `STM32F10x_StdPeriph_Driver/src/stm32f10x_can.c`
- `STM32F10x_StdPeriph_Driver/src/stm32f10x_tim.c`
- `STM32F10x_StdPeriph_Driver/src/stm32f10x_usart.c`
- `STM32F10x_StdPeriph_Driver/src/stm32f10x_spi.c` (после прямой настройки SPI).

Из сохранённых для загрузчика gpio/rcc удалены лишь GPIO_WriteBit и RCC_APB2PeriphResetCmd, больше нигде не вызываемые. BIN Customer/Release/загрузчика до и после удаления файлов 8а и этих двух функций совпадают. Файлы system_stm32f10x и ядра USB ST не менялись.
