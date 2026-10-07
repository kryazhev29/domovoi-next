/* Доработка: Кряжев А., 2026.
   Запрашивает баланс SIM через USSD и разбирает ответ оператора.
   Запрос идёт раз в сутки, отказ повторяется через заданную паузу.
   Меню оператора закрывается после получения результата.
*/
#ifndef COMM_BALANCE_H
#define COMM_BALANCE_H
#include <stdint.h>
#include <stdbool.h>
/* code() - USSD из настроек ("*100#", пусто - не спрашивать); result(рубли) - только
   при разобранном ответе, с минусом, как в тексте оператора. */
void comm_balance_init(const char *(*code)(void), void (*result)(int32_t rubles));
/* Продвинуть запрос баланса, ожидание ответа или закрытие меню. */
void comm_balance_poll(uint32_t now_ms);
/* Рубли из текста ответа (cp1251); false - числа нет. */
bool comm_balance_parse(const char *text, int32_t *rubles);
#endif
