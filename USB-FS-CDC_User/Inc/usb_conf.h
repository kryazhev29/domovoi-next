// Доработка: Кряжев А., 2026
// Четыре точки CDC, их буферы и маска событий.
// Дескрипторы и порядок событий сохраняют протокол CDC.
// Общие файлы также используются загрузчиком; память PMA требует особого доступа.

#ifndef __USB_CONF_H
#define __USB_CONF_H

#define EP_NUM                          (4)

#define BTABLE_ADDRESS      (0x00)

#define ENDP0_RXADDR        (0x40)
#define ENDP0_TXADDR        (0x80)

#define ENDP1_TXADDR        (0xC0)
#define ENDP2_TXADDR        (0x100)
#define ENDP3_RXADDR        (0x110)

#define IMR_MSK (CNTR_CTRM | CNTR_ERRM  | CNTR_SOFM | CNTR_ESOFM | CNTR_RESETM)

/* Обработчики передачи конечных точек. */

#define  EP2_IN_Callback   NOP_Process
#define  EP3_IN_Callback   NOP_Process
#define  EP4_IN_Callback   NOP_Process
#define  EP5_IN_Callback   NOP_Process
#define  EP6_IN_Callback   NOP_Process
#define  EP7_IN_Callback   NOP_Process

#define  EP1_OUT_Callback   NOP_Process
#define  EP2_OUT_Callback   NOP_Process

#define  EP4_OUT_Callback   NOP_Process
#define  EP5_OUT_Callback   NOP_Process
#define  EP6_OUT_Callback   NOP_Process
#define  EP7_OUT_Callback   NOP_Process

#endif
