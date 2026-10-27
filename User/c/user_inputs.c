// Доработка: Кряжев А., 2026
// Функции входов 1 и 2 по ТЗ «Домовой NEXT» п. 6.3.3.4: SMS о неисправностях, датчик транспорта,
// снимок по тревоге, кнопка открытия, аппаратная разблокировка (номера 1..7, список в user_logic_internal.h).
// SMS уходят серверу пакетом 7, как уведомление о неизвестном ID: телефон админа знает только сервер.
// Датчик транспорта и аппаратную разблокировку читают access и unblock по текущему состоянию входа.
#include "user_logic_internal.h"
#include "../photo.h"
static uint8_t sms_due; // биты 0..2 - функции 1..3 ждут свободного слота сервера
static const char *const sms_text[3] = { "Drive board failure", "Case opened", "Gate not closed" };

// Назначена ли функция хотя бы одному входу.
bool input_assigned(uint8_t function)
{
  return config.inputMode[0] == function || config.inputMode[1] == function;
}

// Активен ли вход с этой функцией.
bool input_active(uint8_t function)
{
  return (config.inputMode[0] == function && device.status.stateIn1) ||
         (config.inputMode[1] == function && device.status.stateIn2);
}

// Снимок по тревоге: камера из таблицы 485 в активном режиме; занятая камера пропускается без события.
static void input_photo(void)
{
  if (config.rs485Mode != 1) return;
  for (uint8_t i = 0; i < 8; ++i)
    if (config.rs485Periphery[i].type == RS485_CAMERA) {
      if (photo_start(config.rs485Periphery[i].address) == CMD_SUCCESS) log_addSystemEvent(LOG_SYS_IN_PHOTO);
      return;
    }
}

// Кнопка открытия: обычный алгоритм выходов без ID; при разблокировке и занятом выходе ничего.
static void input_button(void)
{
  if (user_activateOut(true, true, CT_BUTTON, NULL)) log_addControlEvent(LOG_CON_OUTONBYBUTTON, NULL, 255);
}

// Смена состояния входа index (0/1) на state: выполнить его функцию.
void input_edge(uint8_t index, uint8_t state)
{
  uint8_t f = config.inputMode[index];
  if (f == IN_UNBLOCK) { user_unblock_recheck(false); return; }
  if (!state) return;
  switch (f) {
  case IN_SMS_DRIVE: case IN_SMS_TAMPER: case IN_SMS_GATE:
    sms_due |= (uint8_t)(1u << (f - 1u));
    log_addSystemEvent((logSystem_e)(LOG_SYS_IN_DRIVEFAULT + f - 1u));
    break;
  case IN_PHOTO: input_photo(); break;
  case IN_BUTTON: input_button(); break;
  default: break;
  }
}

// Отправить ожидающие SMS серверу по одному за проход, когда слот свободен.
void input_sms_poll(void)
{
  for (uint8_t i = 0; i < 3u; ++i)
    if (sms_due & (1u << i)) {
      if (user_send_smsToServer((char *)sms_text[i])) sms_due &= (uint8_t)~(1u << i);
      return;
    }
}
