/* Shared framing only; the caller retains ownership of the buffer. */
#include "packet.h"
#include "includes.h"
#include "../Modules/comm_channel.h"
bool packet_send(uint8_t *buffer, uint16_t length, uint8_t channel)
{
  uint16_t crc;
  buffer[0]='S'; buffer[1]='L'; buffer[2]='D'; buffer[3]='N';
  buffer[4]=(uint8_t)length; buffer[5]=(uint8_t)(length>>8);
  crc=crc16_calc(buffer+4,length+2);
  buffer[length+6]=(uint8_t)crc; buffer[length+7]=(uint8_t)(crc>>8);
  if(channel==0) return usb_send(buffer,length+8)!=0;
  if(channel==1) return comm_tx_commit(length+8);
  return false;
}
