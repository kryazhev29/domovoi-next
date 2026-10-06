#ifndef DOMOVOI_PACKET_H
#define DOMOVOI_PACKET_H
#include <stdint.h>
#include <stdbool.h>
bool packet_send(uint8_t *buffer, uint16_t length, uint8_t channel);
#endif
