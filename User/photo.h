// Фото: внешние команды, камера RS485 и границы области хранения.
// Номер и размер на проводе остаются по 4 байта; ACK хранится одним байтом.
// Незавершённая запись не имеет метки формата и не попадает в очередь.
// Доработка: Кряжев А., 2026
#ifndef PHOTO_H
#define PHOTO_H
#include "includes.h"
#define PHOTO_FORMAT 0x32504850u /* Метка завершённой записи PHP2. */
#define PHOTO_RECORD_HEAD (sizeof(photoParam_s) + 4u + 1u)
#define RS485_CAMERA 9u /* Тип 485 «фотокамера JPEG (OV)» по нумерации группами по 8 (Протокол2, 28.10.2026). */
#define RS485_ANPR 8u /* Тип 485 «фотокамера домовой Interceptor»: номер и снимок по запросу (camera.docx). */
#define RS485_IS_CAMERA(type) ((type) == RS485_CAMERA || (type) == RS485_ANPR)
// Проверить команду 7 и запланировать съёмку выбранной камеры.
cmdCode_e photo_start(uint8_t address);
// Активный режим 485: опросить камеры номеров и разобрать их ответ.
void anpr_poll(void);
// Номер с камеры по адресу address: захват команды 6 или проверка доступа (user_access.c).
void camera_readedID(char *id, uint8_t address);
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
