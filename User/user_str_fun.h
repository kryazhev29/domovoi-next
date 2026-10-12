// Доработка: Кряжев А., 2026
// Ограниченный разбор строк SMS и параметров адреса сервера.
// Разделители не записываются в результат копирования; результат завершён нулём.
// Модуль не хранит состояние и не владеет памятью вызывающего.
#ifndef USERSTRFUN_H
#define USERSTRFUN_H
#include <stdbool.h>
#include <stdint.h>
// Найти указанный разделитель; при отсутствии вернуть исходный указатель.
char *skipchar(char ch, char *text, unsigned char count);
// Скопировать до разделителя или лимита, добавив нулевой байт.
void user_strcpy(char *dest, char *src, char separator, unsigned char limit);
// Разобрать ровно 4–5 цифр ПИНа; при отказе результат не менять.
bool user_parse_pin(const char *text, uint32_t length, uint32_t *value);
#endif
