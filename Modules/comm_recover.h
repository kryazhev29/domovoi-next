/* Доработка: Кряжев А., 2026. Единый автомат восстановления связи.
   TCP -> мобильный интернет -> питание; паузы AT 10/30/60 с, питания 10/30/60 мин.
   Ранний перезапуск по окну AT; ожидаемый обрыв после звонка без первого отката.
   Только сохранённое «авто»: два LTE-сеанса без данных -> сутки GSM -> одна проба авто.
   Ночной перезапуск сохраняет этот срок; перезапуск платы начинает авто заново.
   Связь никогда не назначает перезагрузку процессора. */
#ifndef COMM_RECOVER_H
#define COMM_RECOVER_H
#include <stdint.h>
#include "domovoi_diag.h"
#include <stdbool.h>

typedef struct {
  void (*reset_session)(void); /* после перезапуска модема: сервер и канал с начала */
  uint32_t (*local_time)(void); /* местное время, с; UINT32_MAX - нет достоверного */
  bool (*busy)(void);           /* звонок, обмен фото/журналом/прошивкой */
} comm_recover_hooks_t;

/* hooks - постоянная (static const) структура, хранится указатель. */
void comm_recover_init(const comm_recover_hooks_t *hooks, uint32_t now_ms);
/* Результат одного сеанса; сутки GSM после двух отказов LTE только при config.netType == 0. */
enum { COMM_DATA_BEGIN, COMM_DATA_FAILED, COMM_DATA_OK };
void comm_recover_data(uint8_t result);
uint8_t comm_recover_network(uint8_t net_type);
/* Исполнить назначенный перезапуск или проверить условия надзора. */
void comm_recover_poll(uint32_t now_ms);
/* Новый отказ транспорта сразу обновляет окна раннего восстановления. */
void comm_recover_failure(uint8_t step, uint8_t reason, uint32_t now_ms);
/* Собрать ступени и остаток паузы для команды 111. */
#if DOMOVOI_DIAG
// Выдать ступень восстановления и оставшийся срок ожидания.
uint32_t comm_recover_diag(uint32_t now_ms, uint32_t *wait_left);
#endif
/* Сервер молчит, ping и DNS не прошли: TCP заново, потом модем с AT, потом питание. */
void comm_recover_no_internet(void);
/* Сервер молчит на первую регистрацию после включения платы: питание модема сразу.
   Один раз за включение; true - перезапуск назначен. */
bool comm_recover_first_silence(void);
/* URC регистрации отменяет только ожидание сети: следующая попытка без снятия питания. */
void comm_recover_registered(void);
/* Код экрана связи и остаток ожидания перезапуска, мс. */
uint8_t comm_recover_link(uint32_t now_ms, uint32_t *wait_left);

#endif
