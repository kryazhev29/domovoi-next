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
