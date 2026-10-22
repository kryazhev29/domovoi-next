/* Доработка: Кряжев А., 2026.
   Принимает и отправляет SMS в формате PDU.
   Разбор проверяет внешний поток и преобразует GSM/UCS2 в cp1251.
   Сообщение удаляется с SIM после чтения, приложение принимает его вне AT.
*/
#ifndef COMM_SMS_H
#define COMM_SMS_H
#include <stdint.h>
#include <stdbool.h>
#include "../User/domovoi_diag.h"
/* received(номер, текст, first) — из главного цикла; приложение сбрасывает *first
   после записи события, включая битую PDU (пустой текст).
   true разрешает удаление, false — повтор.
   Строки cp1251 живут до возврата. PDU hex не становится ответом модема. */
void comm_sms_init(bool (*received)(char *number, char *text, bool *first));
#if DOMOVOI_DIAG
/* Семь слов хвоста USB 111 (v[67..73]): настройка, SIM, +CMTI, CMGL, CMGR, CMGD. */
void comm_sms_diag(uint32_t *v);
#endif
/* Продвинуть очередь SIM; access_ready разрешает обработчик приложения, но не сбор результата AT. */
void comm_sms_poll(uint32_t now_ms, bool access_ready);
/* Номер "+7..." или цифры; текст cp1251. Первые 114 символов ASCII - GSM 7 бит,
   иначе UCS2 до 50; длиннее обрезается. false - предыдущая ещё уходит или неверный номер. */
bool comm_sms_send(const char *number, const char *text);
#endif
