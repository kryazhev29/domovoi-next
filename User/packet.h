// Доработка: Кряжев А., 2026
// Общий интерфейс SLDN для немедленных и фоновых ответов.
// packet_send принимает тело с байта 6 в уже выделенном буфере канала.
// reply_send принимает собственный буфер автомата и сохраняет его при занятости.
#ifndef DOMOVOI_PACKET_H
#define DOMOVOI_PACKET_H
#include <stdint.h>
#include <stdbool.h>
// Дополняет тело кадра до передачи или ожидания свободного канала.
void packet_frame(uint8_t *buffer, uint16_t length);
// Собирает кадр и отправляет его из буфера выбранного канала.
bool packet_send(uint8_t *buffer, uint16_t length, uint8_t channel);
// Ставит готовый ответ автомата в TX, не отдавая его память до принятия.
uint8_t reply_send(const uint8_t *packet, uint16_t size, uint8_t channel);
// Фиксирует поля ответа 11 и его кадр на момент возникновения результата.
void answer11(uint8_t *packet, uint8_t uniq, uint8_t command, uint8_t code);
#endif
