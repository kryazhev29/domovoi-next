/* Доработка: Кряжев А., 2026. SMS в режиме PDU: приём по +CMTI, отправка, без кучи. */
#ifndef COMM_SMS_H
#define COMM_SMS_H
#include <stdint.h>
#include <stdbool.h>
/* received(номер, текст) - из главного цикла, строки в cp1251, живут до возврата;
   после неё SMS стирается с SIM. Текст PDU - hex: не может притвориться ответом модема. */
void comm_sms_init(void (*received)(char *number, char *text));
void comm_sms_poll(uint32_t now_ms);
/* Номер "+7..." или цифры; текст cp1251. Только латиница - GSM 7 бит до 114 символов,
   иначе UCS2 до 50; длиннее обрезается. false - предыдущая ещё уходит или неверный номер. */
bool comm_sms_send(const char *number, const char *text);
#endif
