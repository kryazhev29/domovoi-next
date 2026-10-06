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

// Проверить цифру для неизменённого исходного кода универсального ПИН.
bool chkdigitchar(char ch)
{
  if (ch == 0x2b) return true;
  if (ch == 0x2d) return true;
  if ((ch >= 0x30) && (ch <= 0x39)) return true; // цифры

  return false;
}

// Проверить цифры IMEI для неизменённого исходного кода универсального ПИН.
bool chkdigitstr (char *s, unsigned char c)
{
  for (unsigned char i=0; i<c; i++)
    if (chkdigitchar(s[i]) == false)
      return false;

  return true;
}
