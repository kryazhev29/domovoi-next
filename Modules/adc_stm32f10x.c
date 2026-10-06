// Доработка: Кряжев А., 2026
// АЦП платы v5: аналоговые IN1/IN2 и внешний источник питания.
// Три injected-канала измеряются по запросу; датчик температуры не используется.
// Калибровка и преобразование ограничены по времени, отказ сохраняет прежние входы.

#include "adc_stm32f10x.h"
#include "board.h"
// Проверить доступность измерений после калибровки.
uint8_t adc_ready(void) { return (ADC1->CR2 & ADC_CR2_ADON) != 0; }
// Дождаться аппаратного флага; при таймауте отключить недостоверные измерения.
static uint8_t adc_wait(uint8_t flag)
{
  uint32_t budget = 32000u;
  while ((ADC1->SR & flag) == 0) {
    if (--budget == 0) { ADC1->CR2 &= ~ADC_CR2_ADON; return 0; }
  }
  return 1;
}

// Настроить три injected-канала и ограниченно ждать калибровку.
void adc_init(void)
{
  RCC->APB2RSTR |= RCC_APB2RSTR_ADC1RST;
  RCC->APB2RSTR &= ~RCC_APB2RSTR_ADC1RST;
  RCC->APB2ENR |= RCC_APB2ENR_ADC1EN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;
  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_ADCPRE) |
              (SystemCoreClock == 72000000u ? RCC_CFGR_ADCPRE_DIV6 : RCC_CFGR_ADCPRE_DIV4);
  GPIOA->CRL &= ~0x000F0000u;
  GPIOB->CRL &= ~0x000000FFu;
  ADC1->CR1 = ADC_CR1_SCAN;
  ADC1->CR2 = ADC_CR2_EXTSEL | ADC_CR2_JEXTSEL | ADC_CR2_ADON;
  ADC1->SMPR2 = (3u << 24) | (3u << 27) | (3u << 12);
  // При длине 3 последовательность начинается с JSQ2, затем JSQ3 и JSQ4.
  ADC1->JSQR = (2u << 20) | (IN1_CHANEL << 5) | (IN2_CHANEL << 10) | (VCC_CHANEL << 15);
  ADC1->CR2 |= ADC_CR2_RSTCAL;
  uint32_t budget = 32000u;
  while (ADC1->CR2 & ADC_CR2_RSTCAL) {
    if (--budget == 0) { ADC1->CR2 &= ~ADC_CR2_ADON; return; }
  }
  ADC1->CR2 |= ADC_CR2_CAL;
  budget = 32000u;
  while (ADC1->CR2 & ADC_CR2_CAL) {
    if (--budget == 0) { ADC1->CR2 &= ~ADC_CR2_ADON; return; }
  }
}

// узнать значение входа в мВ
// Вернуть напряжение injected-канала в милливольтах.
uint16_t adc_getinjectedvalue_mV (uint8_t ind)
{
  uint32_t temp = 0;

  switch (ind){
   case IN1_CHANEL: temp = ADC1->JDR1; break;
   case IN2_CHANEL: temp = ADC1->JDR2; break;
   case VCC_CHANEL: temp = ADC1->JDR3; break;
   }

  if (ind != VCC_CHANEL) temp = temp * 33 * 1016;
  else temp = temp * 33 * 1100;

  temp = temp / 4096;

  return (uint16_t)temp;
}

// Запустить измерение и ограниченно ждать готовность.
uint8_t adc_startInjectedConvertion(void)
{
  if (!adc_ready()) return 0;
  if ((ADC1->CR2 & ADC_CR2_SWSTART) && !adc_wait(ADC_SR_EOC)) return 0;
  ADC1->CR2 |= ADC_CR2_JEXTTRIG | ADC_CR2_JSWSTART;
  return adc_wait(ADC_SR_JEOC);
}