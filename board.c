// Описание входов/выходов платы

#include "board.h"

// инициализация
void board_init (void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  GPIO_StructInit(&GPIO_InitStruct);

  // включаем управление портами
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | 
                         RCC_APB2Periph_GPIOA | 
                         RCC_APB2Periph_GPIOB | 
                         RCC_APB2Periph_GPIOC, ENABLE);

  // отключаем JTAG? т.к. ножки используются под 1-wire
  //GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);

  // Выходы Push-Pull
  // USB_PULLUP, OUT2
#ifdef BOARD_IP_V3
  // IP_v3: PA0 - выход микросхемы RS232, остаётся входом. Доработка: Кряжев А., 2026
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8;
#else
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_0;
#endif
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStruct); 
  // FLASHCS, RTS_G, OUT1, DATALED, NETLED, PWRLED
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_14 | GPIO_Pin_3 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStruct);  
  // GSM_PWR
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStruct);

  // Open drain
  // GSM_KEY
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_14;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStruct);

  // Входы
  // VCC_VOLTAGE
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStruct);
  // Input 1,2
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStruct);
  // USB_DET
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_15;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStruct);
  // 433RX, D0, D1
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStruct);
  // BOOT PIN
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStruct);
    
  // Начальные установки
  LED1_OFF;
  LED2_OFF;
  LED3_OFF;
  GSM_PWR_OFF;
  OUT1_OFF;
  OUT2_OFF;
  GSM_KEY_OFF;
  CSEL_HIGH;
  RTSG_LOW;

  // подаём тактирование на модуль CRC
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);
  
  // Включение тактирования Backup памяти
  // Данные сохраняются пока есть внешнее питание, не зависимо от сброса
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
  PWR_BackupAccessCmd(ENABLE);
  
  // детектор напряжения питания процессора
  PWR_PVDLevelConfig(PWR_PVDLevel_2V5); // устанавливаем порог в 2,5 В
  // разрешаем прерывание по нарастающему фронту PVDO
  // PVDO растёт при падении питания: запускаем согласованный сброс ниже 2,5 В
  EXTI_InitTypeDef EXTI_InitStructure;
  EXTI_InitStructure.EXTI_Line = EXTI_Line16;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_ClearITPendingBit(EXTI_Line16);
  EXTI_Init(&EXTI_InitStructure);

  NVIC_InitTypeDef NVIC_InitStructure;
  
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0); // наивысший приоритет
  NVIC_InitStructure.NVIC_IRQChannel = PVD_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  PWR_PVDCmd(ENABLE);
}

// деинициализировать периферию
void board_deinit (void)
{
  // отключаем порты  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, DISABLE);
  // сбрасываем
  RCC_APB2PeriphResetCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
  RCC_APB2PeriphResetCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, DISABLE);
}

// перезагрузка
void board_reset (void)
{
  NVIC_SystemReset();
}

// ожидание стабилизации питания
void board_waitPowerStability (void)
{
  if (PWR_GetFlagStatus(PWR_FLAG_PVDO) != RESET)
  {
    uint32_t timeout = SystemCoreClock;
    
    do
    {
      IWDG_ReloadCounter();
      timeout--;
    }
    while ((PWR_GetFlagStatus(PWR_FLAG_PVDO) != RESET) && (timeout != 0)); // ждем пока напряжение ниже порога
  }  
}