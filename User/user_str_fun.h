/*
Пользовательские функции работы со строками
*/

#ifndef USERSTRFUN_H
#define USERSTRFUN_H

#include "arm_comm.h"
#include "string.h"

// проверка символа на удовлетворение числа
bool chkdigitchar(char ch);

// проверка набора символов на удовлетворение числу
bool chkdigitstr (char *s, unsigned char c);

// счетчик символов
// в строке производится поиск заданного символа, если символов стоко сколько надо возвращается true
bool comparecountchar(char ch, char *p, unsigned char c, char ec);

// проверка беззнакового числа на удовлетворение диапазону
bool chkunsigchar (unsigned char d, unsigned char min, unsigned char max);

// пропуск заданного количества символов
char* skipchar(char ch, char* p, unsigned char c);

// Преобразует символ в hex значение
unsigned char symboltohex(char sym);

// Преобразует строку в hex массив
void strtohex(char *str, unsigned char *dst, unsigned short n);

// Преобразует hex значение в символ
char hextosymbol(unsigned char sym);

// Преобразует hex массив в строку
void hextostr(char *str, unsigned char *src, unsigned short n);

// Пропустить символ, если найден
char* skipchariffound(char* ptr, char ch, unsigned char count);

// копирование строки до указанного символа, либо по лимиту
void user_strcpy(char *dest, char *src, char ch, unsigned char lmt);

// строку в число
unsigned short strtodign (char* s, unsigned char n);

// преобразование UCS2 в текст
void decodeUCS2toText (char* dst, char* src, unsigned short srclen);

// преобразование текст в UCS2
unsigned short decodeTexttoUCS2 (char* dst, char* src);

// пропуск заданного количества символов с регулируемым лимитом строки
char* skipchar2limit(char ch, char* p, unsigned char c, unsigned short lim);

// вернет указатель на N последних символов в строке, или строку если она менее N
char* lastNsymbols (char* num, unsigned char n);

// перевод в транслит
void convertToTranslit(char *dst, const char *src);

#endif