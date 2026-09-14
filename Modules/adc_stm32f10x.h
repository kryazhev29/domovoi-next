/******************************************************************************
*    ADC functions
******************************************************************************/

#ifndef __ADC_H
#define __ADC_H

#include "stm32f10x_gpio.h"
#include "stm32f10x_adc.h"

// инициализация АЦП
void adc_init (void);

// узнать температуру контроллера
// в градусах
int8_t adc_gettemp (void);

// узнать значение входа в мВ
uint16_t adc_getinjectedvalue_mV(uint8_t ind);

void adc_startRegularConvertion (void);
void adc_startInjectedConvertion (void);

#endif