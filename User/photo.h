// Фото: внешние команды, камера RS485 и границы области хранения.
// Номер и размер на проводе остаются по 4 байта; ACK хранится одним байтом.
// Незавершённая запись не имеет метки формата и не попадает в очередь.
// Доработка: Кряжев А., 2026
#ifndef PHOTO_H
#define PHOTO_H
#include "includes.h"
#define PHOTO_FORMAT 0x32504850u /* Метка завершённой записи PHP2. */
#define PHOTO_RING_START 0x400000u
#define PHOTO_RING_END   0x800000u
#define PHOTO_RING_FORMAT 0x33504850u
#define PHOTO_RECORD_HEAD (sizeof(photoParam_s) + 4u + 1u)
// Проверить команду 7 и запланировать съёмку выбранной камеры.
cmdCode_e photo_start(uint8_t address);
// Разрешить повтор команды 21 во время стирания пустой очереди.
bool photo_empty_cleanup(void);
// Съёмка или принятый поток имеют приоритет перед серверным ACK журнала.
bool photo_busy(void);
// Обслужить камеру, восстановление очереди и поток пакетов 14.
void photo_poll(void);
// Остановить поток сервера, сохранив подтверждённый курсор.
void photo_connection_lost(void);
// Получить общую скорость шины из настройки.
uint32_t rs485_baud(void);
// Настроить приём обычных устройств RS485.
void rs485_init(void);
// Применить параметры RS485 и остановить несовместимую съёмку.
void rs485_set_config(const uint8_t *data);
// Получить исключающую верхнюю границу области фото.
uint32_t photo_storage_end(void);
#endif
