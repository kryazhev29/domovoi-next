/******************************************************************************
*    ADC functions
******************************************************************************/

#include "adc_stm32f10x.h"
#include "board.h"

#define TS_V25          (int32_t)1430 // mV
#define TS_AVGSLOPE     (int32_t)43 // mV/0C * 10


// инициализация: 2 входа, сканируются по програмному запуску, используются регистры injected
// датчик температуры по regular входу
void adc_init (void)
{
  GPIO_InitTypeDef        GPIO_InitStructure;
  ADC_InitTypeDef         ADC_InitStruct;

  ADC_DeInit(ADC1);

  // включаем АЦП и входы
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

  /* ADCCLK */
  if (SystemCoreClock == 72000000) RCC_ADCCLKConfig(RCC_PCLK2_Div6); // 72 / 6 = 12 MHz
  else RCC_ADCCLKConfig(RCC_PCLK2_Div4); // 48 / 4 = 12 MHz

  GPIO_StructInit(&GPIO_InitStructure);

  // VCC_VOLTAGE
  GPIO_InitStructure.GPIO_Pin = VCC_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(VCC_PORT, &GPIO_InitStructure);
  // Input 1
  GPIO_InitStructure.GPIO_Pin = IN1_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(IN1_PORT, &GPIO_InitStructure);
  // Input 2
  GPIO_InitStructure.GPIO_Pin = IN2_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(IN2_PORT, &GPIO_InitStructure);

  ADC_InitStruct.ADC_Mode = ADC_Mode_Independent;
  ADC_InitStruct.ADC_ScanConvMode = ENABLE; // сканирование каналов, если DISABLE - один канал
  ADC_InitStruct.ADC_ContinuousConvMode = DISABLE; // режим циклического преобразования
  ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right; // выравнивание правое
  ADC_InitStruct.ADC_NbrOfChannel = 1; // количество опрашиваемых каналов в режиме сканера
  ADC_Init(ADC1, &ADC_InitStruct);

  // Set regular chanels
  ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_239Cycles5);

  // Set injected sequencer length
  ADC_InjectedSequencerLengthConfig(ADC1, 3);
  // ADC1 injected channel configuration 
  ADC_InjectedChannelConfig(ADC1, IN1_CHANEL, 1, ADC_SampleTime_28Cycles5);
  ADC_InjectedChannelConfig(ADC1, IN2_CHANEL, 2, ADC_SampleTime_28Cycles5);
  ADC_InjectedChannelConfig(ADC1, VCC_CHANEL, 3, ADC_SampleTime_28Cycles5);

  // ADC1 injected external trigger configuration
  ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_None);

  // Enable ADC1
  ADC_Cmd(ADC1, ENABLE);  

  // Enable ADC1 reset calibration register   
  ADC_ResetCalibration(ADC1);
  // Check the end of ADC1 reset calibration register
  while(ADC_GetResetCalibrationStatus(ADC1));

  // Start ADC1 calibration
  ADC_StartCalibration(ADC1);
  // Check the end of ADC1 calibration
  while(ADC_GetCalibrationStatus(ADC1));

  // включаем датчик температуры
  ADC_TempSensorVrefintCmd(ENABLE);
}

// узнать температуру контроллера
// в градусах
int8_t adc_gettemp (void)
{
  int32_t temp;
  
  adc_startRegularConvertion();

  temp = ((int32_t)ADC_GetConversionValue(ADC1)*3300)/4096; // в милливольтах
  temp = ((TS_V25 - temp)*10)/TS_AVGSLOPE;
  temp += 25;

  return (int8_t)temp;
}

// узнать значение входа в мВ
uint16_t adc_getinjectedvalue_mV (uint8_t ind)
{
  uint32_t temp = 0;

  switch (ind){
   case IN1_CHANEL: temp = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_1); break;
   case IN2_CHANEL: temp = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_2); break;
   case VCC_CHANEL: temp = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_3); break;
   }

  if (ind != VCC_CHANEL) temp = temp * 33 * 1016;
  else temp = temp * 33 * 1100;

  temp = temp / 4096;

  return (uint16_t)temp;
}

void adc_startRegularConvertion (void)
{
  if (ADC_GetSoftwareStartInjectedConvCmdStatus(ADC1) == SET)
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_JEOC) != SET);
  
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);
  while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) != SET);
}

void adc_startInjectedConvertion (void)
{
  if (ADC_GetSoftwareStartConvStatus(ADC1) == SET)
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) != SET);
  
  ADC_SoftwareStartInjectedConvCmd(ADC1, ENABLE);
  while (ADC_GetFlagStatus(ADC1, ADC_FLAG_JEOC) != SET);
}