/* ƒоработка:  р€жев ј., 2026.
   «апрашивает баланс SIM через USSD и разбирает ответ оператора.
   «апрос идЄт раз в сутки, отказ повтор€етс€ через заданную паузу.
   ћеню оператора закрываетс€ после получени€ результата.
*/
#ifndef COMM_BALANCE_H
#define COMM_BALANCE_H
#include <stdint.h>
#include <stdbool.h>
/* code() - USSD из настроек ("*100#", пусто - не спрашивать); result(рубли) - только
   при разобранном ответе, с минусом, как в тексте оператора. */
void comm_balance_init(const char *(*code)(void), void (*result)(int32_t rubles));
/* ѕродвинуть запрос баланса, ожидание ответа или закрытие меню. */
void comm_balance_poll(uint32_t now_ms);
/* –убли из текста ответа (cp1251); false - числа нет. */
bool comm_balance_parse(const char *text, int32_t *rubles);
#endif
