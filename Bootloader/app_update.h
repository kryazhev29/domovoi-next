#ifndef DOMOVOI_APP_UPDATE_H
#define DOMOVOI_APP_UPDATE_H
#include <stdint.h>
/* Returns zero on error; does not reset or write the bootloader automatically. */
int app_boot_request(void);
void app_boot_metadata(void);
void app_boot_error_reported(void);
/* Nonzero while the journal holds an error not yet sent to the server. */
int app_boot_error_unreported(void);
/* Reason of that error for the event journal: up to 10 characters and a NUL. */
void app_boot_error_text(char *text);
/* Trial of this application after an update, read by app_boot_metadata(). */
int app_trial_active(void);
void app_trial_confirm(void);
void app_trial_rollback(void);
/* Call once per second; nonzero when the trial ran out of time unconfirmed. */
int app_trial_expired(void);
#endif
