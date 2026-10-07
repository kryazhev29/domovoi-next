// Доработка: Кряжев А., 2026
// Строковые границы SMS: поиск разделителя, ограниченное копирование и десятичное число.
// Входы завершены нулём обработчиком модема; копирование ограничено размером поля.
// Внутренние вызовы получают проверенные указатели, модуль не обращается к флешу.
#include "user_str_fun.h"
// Найти окончание указанного числа разделителей.
char *skipchar(char separator, char *text, unsigned char count)
{
  char *result = text;
  while (*text && count) {
    if (*text++ == separator) { result = text; --count; }
  }
  return result;
}

// Скопировать поле SMS до разделителя или установленного лимита.
void user_strcpy(char *dest, char *src, char ch, unsigned char lmt)
{
  while((*src != ch) && (*src != 0) && lmt--) *dest++ = *src++;
  *dest = 0;
}

// Разобрать внешний десятичный аргумент без библиотеки преобразования IAR.
uint32_t user_decimal(const char *text)
{
  uint32_t value = 0;
  bool negative;
  while (*text == ' ' || (*text >= '\t' && *text <= '\r')) ++text;
  negative = *text == '-';
  if (*text == '+' || *text == '-') ++text;
  while (*text >= '0' && *text <= '9') {
    uint32_t digit = (uint32_t)(*text++ - '0');
    if (value > (UINT32_MAX - digit) / 10u) return negative ? 1u : UINT32_MAX;
    value = value * 10u + digit;
  }
  return negative ? 0u - value : value;
}

// Разобрать ПИН числом, сохраняя допустимость ведущих нулей.
bool user_parse_pin(const char *text, uint32_t length, uint32_t *value)
{
  uint32_t pin = 0;
  if (length != 4u && length != 5u) return false;
  for (uint32_t i = 0; i < length; ++i) {
    if (text[i] < '0' || text[i] > '9') return false;
    pin = pin * 10u + (uint32_t)(text[i] - '0');
  }
  *value = pin;
  return true;
}
