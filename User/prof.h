/* Замер времени по счётчику тактов ядра (DWT CYCCNT), 72 МГц: 72 такта = 1 мкс.
 * Только для сравнения сборок на C и ASM; читается пакетом 113 после ответа на команду 112 по USB.
 * Доработка: Кряжев А., 2026 */
#ifndef PROF_H
#define PROF_H

#include <stdint.h>
#include "domovoi_diag.h"

enum {
  PROF_GSM,
  PROF_PORT,
  PROF_USB,
  PROF_USER,
  PROF_STORAGE,
  PROF_LOOP,       // весь проход главного цикла
  PROF_WG_CALL,    // WG_readedID: проверка карты до возврата
  PROF_CARD_RELAY, // кадр Wiegand принят -> реле включено
  PROF_N
};

#if DOMOVOI_DIAG
#define USB_DIAG_VALUES 67u
#define PROF_NOW() (*(volatile uint32_t*)0xE0001004u)
#define PROF_STEP(i, call) do { uint32_t prof_s_ = PROF_NOW(); call; prof_add((i), PROF_NOW() - prof_s_); } while (0)

#define PROF_CARD_MARK() (prof_card_mark = PROF_NOW() | 1u)
extern uint32_t prof_card_mark; // такт приёма кадра Wiegand, 0 - карты нет

void prof_init(void);
void prof_add(uint8_t i, uint32_t cycles);
void prof_relay_on(void);
void prof_send_comm(const uint8_t *reply, uint16_t size);

#else
#define PROF_NOW() 0u
#define PROF_STEP(i, call) do { call; } while (0)
#define PROF_CARD_MARK() ((void)0)
#define prof_init() ((void)0)
#define prof_relay_on() ((void)0)
#define prof_send_comm(reply, size) ((void)0)
#endif

#endif
