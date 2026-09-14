/*
Пользовательские функции работы со строками
*/

#include "user_str_fun.h"


// проверка символа на удовлетворение числа
bool chkdigitchar(char ch)
{
  if (ch == 0x2b) return true; // '+'
  if (ch == 0x2d) return true; // '-'
  if ((ch >= 0x30) && (ch <= 0x39)) return true; // 0-9
  
  return false;
}

// проверка набора символов на удовлетворение числу
bool chkdigitstr (char *s, unsigned char c)
{
  for (unsigned char i=0; i<c; i++)
    if (chkdigitchar(s[i]) == false)
      return false;

  return true;
}

// счетчик символов
// в строке производится поиск заданного символа, если символов столько сколько надо возвращается true
bool comparecountchar(char ch, char *p, unsigned char c, char ec)
{
  unsigned char count = 0; 

  while ((*p != ec) && (*p != 0)) // пока нет заданного символа и конца строки
   {
   if (*p == ch) count++;
   p++;
   }
  
  if (count == c) 
    return true;
  else 
    return false;
}

// проверка беззнакового числа на удовлетворение диапазону
bool chkunsigchar (unsigned char d, unsigned char min, unsigned char max)
{
  if ((d >= min) && (d <= max)) 
    return true;
  else 
    return false;
}

// пропуск заданного количества символов
char* skipchar(char ch, char* p, unsigned char c)
{
  unsigned short i,j;
  char* res = p;

  for (i=0; i<c; i++) 
  {
    j = 1024;
    while (j)
    {
      j--;
      if (*p == ch)
      {
        j = 0; 
        res = p+1;
      }
      if (*p == 0) 
        return res;
      p++;
    }
  }
  
  return res;
}

// Преобразует символ в hex значение
unsigned char symboltohex(char sym)
{
  if ((sym > 0x29) && (sym < 0x3a)) return sym-0x30;
  if ((sym > 0x40) && (sym < 0x47)) return sym-0x37;
  if ((sym > 0x60) && (sym < 0x67)) return sym-0x57;
  return 0;
}

// Преобразует строку в hex массив
void strtohex(char *str, unsigned char *dst, unsigned short n)
{
  unsigned short i=0;
  
  for (i=0; i<n; i++)
  {
    if (*str == 0) break;
    dst[i] = (symboltohex(str[i*2])<<4) | symboltohex(str[i*2+1]);
  }
}

// Преобразует hex значение в символ
char hextosymbol(unsigned char sym)
{
  if (sym < 10) return sym+0x30;
  if ((sym >= 10) && (sym < 16)) return sym+0x37;
  return 0;
}

// Преобразует hex массив в строку
void hextostr(char *str, unsigned char *src, unsigned short n)
{
  unsigned short i=0;

  for (i=0; i<n; i++)
   {
   str[i*2] = hextosymbol((src[i]>>4)&0x0f);
   str[i*2+1] = hextosymbol(src[i]&0x0f);
   }
  str[i*2] = 0;
}

// Пропустить символ, если найден
char* skipchariffound(char* ptr, char ch, unsigned char count)
{
  unsigned short i;
  char* res = ptr;

  for (i=0; i<count; i++)
    if (*(ptr + i) == ch)
     {
     res = skipchar(ch, ptr, 1);
     break;
     }

  return res;
}

// копирование строки до указанного символа, либо по лимиту
void user_strcpy(char *dest, char *src, char ch, unsigned char lmt)
{
  while((*src != ch) && (*src != 0) && lmt--) *dest++ = *src++;
  *dest = 0;
}

// строку в число
unsigned short strtodign (char* s, unsigned char n)
{
  unsigned short d = 0;
  unsigned char i;

  for (i=0; i<n; i++)
   {
   d *= 10;
   if ((*s < 0x30) || (*s > 0x39)) break;
   d += *s - '0';
   s++;
   }
  return d;  
}

// преобразование UCS2 в текст
void decodeUCS2toText (char* dst, char* src, unsigned short srclen){
  for (unsigned short i=0; i<srclen; i+=4){
    unsigned short code = (((unsigned short)symboltohex(src[i]))<<12) +
                          (((unsigned short)symboltohex(src[i+1]))<<8) +
                          (((unsigned short)symboltohex(src[i+2]))<<4) +
                          (unsigned short)symboltohex(src[i+3]);
    
    if (code <= 0x7f){ // latin symbol
      *dst = code&0x7f;
    } else if (code == 0x0401){ // Ё
      *dst = 'Ё';
    } else if (code == 0x0451){ // ё
      *dst = 'ё';
    } else if (code < 0x045f){ // cyrilic symbol
      *dst = code - 0x0410 + 192;
    } else *dst = ' ';
    dst++;
  }
  *dst = 0;
}

// преобразование текст в UCS2
unsigned short decodeTexttoUCS2 (char* dst, char* src){
  unsigned short code;
  unsigned short count = 0;
  
  for (;*src != 0;src++){
    if (*src <= 0x7f){ // latin symbol
      code = *src;
    } else if (*src == 'Ё'){
      code = 0x0401;
    } else if (*src == 'ё'){
      code = 0x0451;
    } else {
      code = 0x0410 + (*src) - 192;
    }
    *dst++ = hextosymbol((code>>16)&0x0f);                        
    *dst++ = hextosymbol((code>>8)&0x0f);
    *dst++ = hextosymbol((code>>4)&0x0f);
    *dst++ = hextosymbol(code&0x0f);
    count += 4;
  }
  *dst = 0;
  count++;
  return count;
}

// пропуск заданного количества символов с регулируемым лимитом строки
char* skipchar2limit(char ch, char* p, unsigned char c, unsigned short lim)
{
  unsigned short i,j;
  char* res = p;

  for (i=0; i<c; i++) 
   {
   j = lim;
   while (j)
    {
    j--;
    if (*p == ch) {j = 0; res = p+1;}
    if (*p == 0) return res;
    p++;
    }
   }
  return res;
}

// вернет указатель на N последних символов в строке, или строку если она менее N
char* lastNsymbols (char* num, unsigned char n)
{
  unsigned short l = strlen(num);

  return (l <= n)?num:(num + (l-n));
}

// перевод в транслит
void convertToTranslit(char *dst, const char *src)
{
  unsigned short len = strlen(src);
  
  for (unsigned short i=0; i<len; i++){
    if (src[i] >= 0xc0){ // А-Я, а-я
      char a = src[i];
      unsigned char Upper = 0;
      
      if (src[i] < 0xe0){ // А-Я
        a += 32;
        Upper = 32;
      }
      
      switch (a){
      case 'а': *dst++ = 'a' - Upper; break;
      case 'б': *dst++ = 'b' - Upper; break;
      case 'в': *dst++ = 'v' - Upper; break;
      case 'г': *dst++ = 'g' - Upper; break;
      case 'д': *dst++ = 'd' - Upper; break;
      case 'е': *dst++ = 'e' - Upper; break;
      case 'ж': *dst++ = 'j' - Upper; break;
      case 'з': *dst++ = 'z' - Upper; break;
      case 'и': *dst++ = 'i' - Upper; break;
      case 'й': *dst++ = 'y' - Upper; break;
      case 'к': *dst++ = 'k' - Upper; break;
      case 'л': *dst++ = 'l' - Upper; break;
      case 'м': *dst++ = 'm' - Upper; break;
      case 'н': *dst++ = 'n' - Upper; break;
      case 'о': *dst++ = 'o' - Upper; break;
      case 'п': *dst++ = 'p' - Upper; break;
      case 'р': *dst++ = 'r' - Upper; break;
      case 'с': *dst++ = 's' - Upper; break;
      case 'т': *dst++ = 't' - Upper; break;
      case 'у': *dst++ = 'u' - Upper; break;
      case 'ф': *dst++ = 'f' - Upper; break;
      case 'х': *dst++ = 'h' - Upper; break;
      case 'ц': *dst++ = 'c' - Upper; break;
      case 'ч': *dst++ = 'c' - Upper; *dst++ = 'h' - Upper; break;
      case 'ш': *dst++ = 's' - Upper; *dst++ = 'h' - Upper; break;
      case 'щ': *dst++ = 's' - Upper; *dst++ = 'c' - Upper; *dst++ = 'h' - Upper; break;
      case 'ъ': *dst++ = '\"'; break;
      case 'ы': *dst++ = 'i' - Upper; *dst++ = '\'';break;
      case 'ь': *dst++ = '\''; break;
      case 'э': *dst++ = 'e' - Upper; *dst++ = 'h' - Upper; break;
      case 'ю': *dst++ = 'y' - Upper; *dst++ = 'u' - Upper; break;
      case 'я': *dst++ = 'y' - Upper; *dst++ = 'a' - Upper; break;
      }        
    } else if (src[i] == 0xa8){ // Ё
        *dst++ = 'J';
        *dst++ = 'O';
    } else if (src[i] == 0xb8){ // ё
        *dst++ = 'j';
        *dst++ = 'o';
    } else
      *dst++ = src[i];
  }
  *dst = 0;
}


