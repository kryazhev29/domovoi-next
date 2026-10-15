/* Разработчик: Кряжев А., 2026 */
#ifndef DOMOVOI_BOOT_MANUAL_H
#define DOMOVOI_BOOT_MANUAL_H
#include <stdint.h>
// Общий CRC слова для двух путей проверки образа в приложении.
uint32_t app_crc_word(uint32_t crc,uint32_t word);
#define MANUAL_BOOT_SIZE 32768u
#define MANUAL_BOOT_COMMAND 110u
enum { MANUAL_OK=0, MANUAL_BAD=1, MANUAL_IO=2, MANUAL_BUSY=3,
       MANUAL_DENIED=4, MANUAL_CRC=5 };
/* Данные сервисной команды USB: операция, затем её данные. Ответ: 8 байт.
 * 0 BEGIN(size:u32,crc:u32,selectedHW:u8), 1 DATA(offset:u32,1..256 байт),
 * 2 COMMIT, 3 CANCEL, 4 STATUS. Все целые поля - младшим байтом вперёд.
 * COMMIT только взводит запрос в ОЗУ; применение - явной командой, никогда при старте. */
void boot_manual_command(int usb,const uint8_t *p,uint16_t n,uint8_t reply[8]);
int boot_manual_busy(void);
int boot_manual_apply(void);
/* Слой платы. Все смещения — внутри слота кандидата; приложение уже владеет STORE. */
int manual_stage_read(uint32_t off,void *data,uint32_t n);
int manual_stage_write(uint32_t off,const void *data,uint32_t n);
int manual_stage_erase(uint32_t off);
int manual_platform_supported(void);
/* Не ноль, если загрузчик с этим номером железа подходит плате, для которой собрана работающая
 * прошивка. Неверный файл, выбранный по фото платы, отвергается. */
int manual_board_hw(uint8_t hw);
int manual_program(uint32_t expected_crc);
void manual_service(void);
#endif
