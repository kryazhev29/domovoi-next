/* Замер времени по счётчику тактов ядра (DWT CYCCNT), 72 МГц: 72 такта = 1 мкс.
 * Только для сравнения сборок на C и ASM; читается пакетом 113 после ответа на команду 112 по USB.
 * Доработка: Кряжев А., 2026 */
#ifndef PROF_H
#define PROF_H

#include <stdint.h>

enum {
  PROF_GSM,        // gsm_check_data
  PROF_PORT,       // port_check_data
  PROF_USB,        // usb_check_data
  PROF_USER,       // user_loop
  PROF_STORAGE,    // user_storage_poll
  PROF_LOOP,       // весь проход главного цикла
  PROF_WG_CALL,    // WG_readedID: проверка карты до возврата
  PROF_CARD_RELAY, // кадр Wiegand принят -> реле включено
  PROF_N
};

#define PROF_NOW() (*(volatile uint32_t*)0xE0001004u)
#define PROF_STEP(i, call) do { uint32_t prof_s_ = PROF_NOW(); call; prof_add((i), PROF_NOW() - prof_s_); } while (0)

extern uint32_t prof_card_mark; // такт приёма кадра Wiegand, 0 - карты нет

void prof_init(void);
void prof_add(uint8_t i, uint32_t cycles);
void prof_relay_on(void);
void prof_send(uint8_t uniqId);

#endif
