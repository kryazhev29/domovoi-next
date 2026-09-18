/* Разработчик: Кряжев А., 2026 */
#ifndef DOMOVOI_APP_UPDATE_H
#define DOMOVOI_APP_UPDATE_H
#include <stdint.h>
/* Возвращает 0 при ошибке; сам не перезагружает и не записывает загрузчик. */
int app_boot_request(void);
void app_boot_metadata(void);
void app_boot_error_reported(void);
/* Не ноль, пока в журнале есть ошибка, ещё не отправленная на сервер. */
int app_boot_error_unreported(void);
/* Причина этой ошибки для журнала событий: до 10 символов и завершающий ноль. */
void app_boot_error_text(char *text);
/* Проба этой прошивки после обновления, читается в app_boot_metadata(). */
int app_trial_active(void);
void app_trial_confirm(void);
void app_trial_rollback(void);
/* Вызывать раз в секунду; не ноль, когда время пробы вышло без подтверждения. */
int app_trial_expired(void);
#endif
