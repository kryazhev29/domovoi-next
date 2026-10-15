// Доработка: Кряжев А., 2026
// Форматирование ID, чисел и статуса SMS без зависимости от printf.
// Размеры и текст ответов сохраняют прежний протокол; память выделяет вызывающий.
#include "user_format.h"
// Дописать постоянный текст в строку ответа.
static char *text(char *dst, const char *src)
{ while(*src) *dst++=*src++; return dst; }
// Дописать десятичные цифры без временной библиотеки преобразования.
static char *decimal(char *dst, uint32_t value)
{
  char digits[10]; uint8_t count=0;
  do { digits[count++]=(char)('0'+value%10u); value/=10u; } while(value);
  do { *dst++=digits[--count]; } while(count);
  return dst;
}
// Сформировать ID из десяти шестнадцатеричных символов.
uint32_t format_id(char *dst, uint32_t value)
{
  static const char hex[]="0123456789ABCDEF";
  uint8_t i; dst[0]=dst[1]='0';
  for(i=0;i<8;i++) dst[i+2]=hex[(value>>(28-4*i))&15];
  dst[10]=0; return 10;
}
// Сформировать десятичное беззнаковое число.
uint32_t format_u32(char *dst, uint32_t value)
{ char *end; end=decimal(dst,value); *end=0; return (uint32_t)(end-dst); }
// Сформировать десятичное число со знаком.
uint32_t format_i32(char *dst, int32_t value)
{
  char *end=dst; uint32_t magnitude=(uint32_t)value;
  if(value<0) { *end++='-'; magnitude=0u-magnitude; }
  end=decimal(end,magnitude); *end=0; return (uint32_t)(end-dst);
}
// Сформировать полный текст статуса SMS с прежними полями.
uint32_t format_status(char *dst, uint32_t signal,
                      const char *network, uint32_t connected, uint32_t server,
                      uint32_t firmware, uint32_t t1, uint32_t t2, uint32_t t3, uint32_t t4)
{
  char *p;
  p=decimal(text(dst,"STATUS:\r\n-SIGNAL: "),signal);
  p=text(text(p,"\r\n-NET: "),network);
  p=decimal(text(p,"\r\n-CLOUD: "),connected); *p++='/';p=decimal(p,server);
  p=decimal(text(p,"\r\n-FW: "),firmware);
  p=decimal(text(p,"\r\n-TRIG: "),t1);*p++='/';p=decimal(p,t2);
  *p++='/';p=decimal(p,t3);*p++='/';p=decimal(p,t4);*p=0;
  return (uint32_t)(p-dst);
}
