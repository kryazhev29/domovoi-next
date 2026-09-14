// Модуль световой индикации
// Автор: Корякин Сергей

#ifndef INDICATION_H
#define INDICATION_H

#include "includes.h"

#define LED_FL1		0x0001 /* одна вспышка*/
#define LED_FL2		0x0005  
#define LED_FL3		0x0015 
#define LED_FL4		0x0055
#define LED_FL8		0x5555
#define LED_ON		0xffff /* постоянно включен*/
#define LED_OFF		0x0000 /* выключен*/
#define LED_SQR		0x00ff /* меандр*/

// Вызывается по прерываниям таймера раз в 100 мс
void led_fun100ms (void);

// Загрузка начального значения в регистр светодиода
void led_load (Int8U led, Int16U val);

// Загрузить состояние по всем светодиодам
// используется при управлении светодиодами по трем проводам
void led_set_port (unsigned char data);

// Разрешить/запретить загрузку значения в регистр индикации
void led_disableloadregister (uint8_t d);

#endif