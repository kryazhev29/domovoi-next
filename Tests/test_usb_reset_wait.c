// Доработка: Кряжев А., 2026
// Ограниченное ожидание RESET общей библиотеки USB.
// Модель проверяет таймаут, ранний RESET и RESET на последнем допустимом опросе.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
#define BOOTLOADER
#define ISTR_RESET 0x400
static unsigned polls, reset_after;

// Вернуть RESET на выбранном опросе, запрещая бесконечное ожидание.
static uint16_t _GetISTR(void)
{
  CHECK(++polls <= 32000);
  return reset_after && polls >= reset_after ? ISTR_RESET : 0;
}

/* КОД */

// Проверить отказ, раннюю готовность и край бюджета ожидания.
int main(void)
{
  CHECK(!USB_WaitReset() && polls == 32000);
  polls = 0; reset_after = 5;
  CHECK(USB_WaitReset() && polls == 5);
  polls = 0; reset_after = 32000;
  CHECK(USB_WaitReset() && polls == 32000);
  puts("USB RESET wait: timeout, early RESET and final poll PASS");
  return 0;
}
