/* Доработка: Кряжев А., 2026.
   Интерфейс публикации кандидата и подтверждения пробной прошивки.
   Постоянные метки принадлежат журналу загрузчика, записи проверяет W25.
   Перезагрузку приложения назначает пользовательская логика.
*/
#ifndef DOMOVOI_APP_UPDATE_H
#define DOMOVOI_APP_UPDATE_H
#include <stdint.h>
#include <stdbool.h>
/* Возвращает 0 при ошибке; сам не перезагружает и не записывает загрузчик. */
int app_boot_request(void);
bool app_boot_request_ready(void);
void app_boot_request_poll(uint32_t now);
// Прочитать версию загрузчика и состояние пробного запуска.
void app_boot_metadata(void);
// Отметить ошибку установки как переданную серверу.
int app_boot_error_reported(void);
/* Не ноль, пока в журнале есть ошибка, ещё не отправленная на сервер. */
int app_boot_error_unreported(void);
/* Числовая причина этой ошибки для аргумента 2 журнала событий. */
uint32_t app_boot_error_code(void);
/* Проба этой прошивки после обновления, читается в app_boot_metadata(). */
int app_trial_active(void);
// Подтвердить пробную прошивку записью в журнал загрузчика.
void app_trial_confirm(void);
// Запросить откат пробной прошивки при следующем запуске.
int app_trial_rollback(void);
/* Вызывать раз в секунду; не ноль, когда время пробы вышло без подтверждения. */
int app_trial_expired(void);
#endif
