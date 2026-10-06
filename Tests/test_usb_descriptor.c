// Доработка: Кряжев А., 2026
// Проверка контрактов периферии без физической платы.
// Зависимости моделируются, код проверяемого модуля вставляет стенд.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}} while(0)
typedef struct {uint8_t *data;uint16_t length;} ONE_DESCRIPTOR;
static struct {uint8_t USBwValue0;} information;
static struct {uint8_t USBwValue0;} *unused;
#define pInformation (&information)
static ONE_DESCRIPTOR String_Descriptor[4];
static unsigned calls;
static uint8_t payload;
// Вернуть выбранный дескриптор для проверки границы индекса строки.
static uint8_t *Standard_GetDescriptorData(uint16_t n,ONE_DESCRIPTOR *p){CHECK(n==7);CHECK(p>=String_Descriptor&&p<String_Descriptor+4);calls++;return &payload;}
/* КОД */
// Проверить все сценарии модуля и вернуть результат прогона.
int main(void){unsigned i;(void)unused;for(i=0;i<256;i++){information.USBwValue0=(uint8_t)i;uint8_t *p=Virtual_Com_Port_GetStringDescriptor(7);CHECK(i<4?p==&payload:p==NULL);}CHECK(calls==4);puts("usbdesc: only string descriptor indices 0..3 accepted passed");return 0;}
