// Доработка: Кряжев А., 2026
// Общая сборка SLDN: заголовок, длина, CRC и отправка по выбранному каналу.
// Готовое тело начинается с байта 6; буфер всегда остаётся у вызывающего.
// Фоновые ответы копируются в серверный TX или передаются USB с учётом владельца.
#include "packet.h"
#include "c/runtime_internal.h"

// Дополняет готовое тело единственными общими заголовком и CRC SLDN.
void packet_frame(uint8_t *buffer, uint16_t length)
{
  rt_put32(buffer,0x4e444c53u); rt_put16(buffer+4,length);
  rt_put16(buffer+length+6,crc16_calc(buffer+4,length+2));
}
// Отправляет тело из буфера, уже подготовленного вызывающим для канала.
bool packet_send(uint8_t *buffer, uint16_t length, uint8_t channel)
{
  packet_frame(buffer,length);
  if(channel==0) return usb_send(buffer,length+8)!=0;
  if(channel==1) return comm_tx_commit(length+8);
  return false;
}
// Отправляет уже собранный ответ автомата, сохраняя его при занятом канале.
uint8_t reply_send(const uint8_t *packet, uint16_t size, uint8_t channel)
{
  uint8_t *out;
  if(!channel) return usb_try_send(packet,size);
  out=user_comm_acquire(size); if(!out) return 0;
  memcpy(out,packet,size);
  if(comm_tx_commit(size)) return 1;
  comm_tx_release(); return 0;
}
// Заполняет единый ответ 11; время и свободный буфер фиксируются при создании.
void answer11(uint8_t *packet,uint8_t uniq,uint8_t command,uint8_t code)
{
  packet[6]=(command==7 || command==21)?5:255; packet[7]=uniq; packet[8]=11; packet[9]=code;
  rt_put32(packet+10,device.datetime);
  rt_put16(packet+14,user_comm_rx_capacity());
  packet_frame(packet,10);
}
