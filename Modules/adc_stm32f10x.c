// Доработка: Кряжев А., 2026
// АЦП платы v5: только аналоговые входы IN1/IN2.
// Два инжектированных канала измеряются по запросу главного цикла.
// Калибровка и преобразование ограничены по времени, отказ сохраняет прежние входы.

#include "adc_stm32f10x.h"
#include "board.h"
// Проверить доступность измерений после калибровки.
uint8_t adc_ready(void) { return (ADC1->CR2 & ADC_CR2_ADON) != 0; }
// Дождаться инжектированной последовательности; при таймауте отключить измерения.
static uint8_t adc_wait(void)
{
  uint32_t budget = 32000u;
  while ((ADC1->SR & ADC_SR_JEOC) == 0) {
    if (--budget == 0) { ADC1->CR2 &= ~ADC_CR2_ADON; return 0; }
  }
  return 1;
}

// Настроить два инжектированных канала и ограниченно ждать калибровку.
void adc_init(void)
{
  RCC->APB2RSTR |= RCC_APB2RSTR_ADC1RST;
  RCC->APB2RSTR &= ~RCC_APB2RSTR_ADC1RST;
  RCC->APB2ENR |= RCC_APB2ENR_ADC1EN | RCC_APB2ENR_IOPBEN;
  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_ADCPRE) |
              RCC_CFGR_ADCPRE_DIV6;
  GPIOB->CRL &= ~0x000000FFu;
  ADC1->CR1 = ADC_CR1_SCAN;
  ADC1->CR2 = ADC_CR2_EXTSEL | ADC_CR2_JEXTSEL | ADC_CR2_ADON;
  ADC1->SMPR2 = (3u << 24) | (3u << 27);
  // Длина 2: IN1 занимает JSQ3, IN2 — JSQ4; результаты идут в JDR1/JDR2.
  ADC1->JSQR = (1u << 20) | (IN1_CHANEL << 10) | (IN2_CHANEL << 15);
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

// Вернуть напряжение инжектированного канала в милливольтах.
uint16_t adc_getinjectedvalue_mV (uint8_t ind)
{
  uint32_t temp = 0;

  switch (ind){
   case IN1_CHANEL: temp = ADC1->JDR1; break;
   case IN2_CHANEL: temp = ADC1->JDR2; break;
   }

  // Масштаб IN1/IN2 учитывает делитель платы; порядок округления сохраняется.
  temp = temp * 33 * 1016;

  temp = temp / 4096;

  return (uint16_t)temp;
}

// Запустить измерение и ограниченно ждать готовность.
uint8_t adc_startInjectedConvertion(void)
{
  if (!adc_ready()) return 0;
  // JEOC снимается только программно (RM0008 §11.12.1), иначе ожидание видит флаг прошлого замера.
  ADC1->SR = ~(uint32_t)ADC_SR_JEOC;
  ADC1->CR2 |= ADC_CR2_JEXTTRIG | ADC_CR2_JSWSTART;
  return adc_wait();
}
