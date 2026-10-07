// Доработка: Кряжев А., 2026
// Управляющие запросы и состояние виртуального порта CDC.
// Дескрипторы и порядок событий сохраняют протокол CDC.
// Общие файлы также используются загрузчиком; память PMA требует особого доступа.

#ifndef __usb_prop_H
#define __usb_prop_H

typedef struct
{
  uint32_t bitrate;
  uint8_t format;
  uint8_t paritytype;
  uint8_t datatype;
}LINE_CODING;

#define SET_COMM_FEATURE            0x02
#define SET_LINE_CODING             0x20
#define GET_LINE_CODING             0x21
#define SET_CONTROL_LINE_STATE      0x22

// Подготовить CDC и серийный номер перед подключением к хосту.
void Virtual_Com_Port_init(void);
// Сбросить очередь и настроить точки EP0–EP3 для CDC.
void Virtual_Com_Port_Reset(void);
// Отметить выбранную хостом конфигурацию CDC.
void Virtual_Com_Port_SetConfiguration(void);
// Отметить назначение адреса устройства.
void Virtual_Com_Port_SetDeviceAddress (void);
// Назначить данные GET/SET_LINE_CODING для управляющего запроса.
RESULT Virtual_Com_Port_Data_Setup(uint8_t);
// Принять поддерживаемый CDC-запрос без данных.
RESULT Virtual_Com_Port_NoData_Setup(uint8_t);
// Разрешить только два интерфейса CDC с нулевой альтернативой.
RESULT Virtual_Com_Port_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting);
// Выдать фрагмент дескриптора устройства.
uint8_t *Virtual_Com_Port_GetDeviceDescriptor(uint16_t);
// Выдать фрагмент конфигурации CDC.
uint8_t *Virtual_Com_Port_GetConfigDescriptor(uint16_t);
// Выдать существующую строку USB либо отклонить её индекс.
uint8_t *Virtual_Com_Port_GetStringDescriptor(uint16_t);

// Выдать хосту текущие параметры виртуального порта.
uint8_t *Virtual_Com_Port_GetLineCoding(uint16_t Length);
// Принять параметры виртуального порта в семибайтовое поле CDC.
uint8_t *Virtual_Com_Port_SetLineCoding(uint16_t Length);

#endif
