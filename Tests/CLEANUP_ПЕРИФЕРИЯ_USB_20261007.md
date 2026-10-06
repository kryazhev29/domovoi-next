# Чистка «ПЕРИФЕРИЯ + USB», 07.10.2026

База: `3b30322`. Исходники изменены только в разрешённом блоке; коммитов нет. Плата, сервер, прошивка, сброс и команды доступа не использовались. Чистый ASM и `DomovoiNext_ASM.ewp` не менялись и не собирались.

## Результат и размер

Цель −2 КиБ **не достигнута**: HYBRID Release −1668 Б, ReleaseCustomer −1819 Б. До 2048 Б не хватает соответственно 380/229 Б. Неиспользуемая часть уже удалена; GPIO/USART/NVIC и часть RCC остаются нужны чужому `comm_uart.c`, SPI — своему неизменённому модулю. USB ASM→C сам по себе не уменьшает стендовый модуль.

| Вариант / профиль | BIN до | BIN после | Δ BIN | ОЗУ до | ОЗУ после | Δ ОЗУ |
|---|---:|---:|---:|---:|---:|---:|
| HYBRID / Release | 95001 | 93333 | -1668 | 17577 | 17536 | -41 |
| HYBRID / ReleaseCustomer | 92745 | 90926 | -1819 | 17225 | 17178 | -47 |
| C / Release | 92613 | 90945 | -1668 | 17577 | 17536 | -41 |
| C / ReleaseCustomer | 90269 | 88526 | -1743 | 17225 | 17178 | -47 |

ОЗУ — сумма readwrite code и readwrite data карты IAR, включая зарезервированные linker-секции; не замер минимального свободного стека. Размеры модулей ниже — сумма их живых ROM-секций карты, поэтому это не независимые дельты BIN: общие библиотечные функции и выравнивание учитываются один раз в общей таблице.

## Каждый пункт задания и cleanup_findings.md

1. **UART, findings 85–86.** Один scalar `uart_t`, только USART1. Ветви USART2/3, цикл по портам, `USART2_IRQHandler` удалены. ROM модуля 1012→544 Б (−468), RAM 48→16 Б (−32). Девять перечисленных мёртвых функций удалены, также удалены неиспользуемые `uart_tx_failed`, `uart_rxoverflow`, `uart_getbuffersize`, `uart_checkenable`, `uart_setrxbuffer`, `NVIC_USART_Config`. До удаления проверены C/.inc/.s, заголовки, векторы и weak-точки; сами мёртвые функции давали 0 Б BIN. `uart_addbyte` теперь приватный, без номера порта. Ограничение ёмкости RX, флаг переполнения, межбайтовая тишина, атомарность очистки и таймаут TX сохранены.

**Ограничение ABI:** семь вызываемых извне функций пока принимают неиспользуемый `legacy_port`: `uart_init/send/clear/received/getstate/setRecTimeout/isReceiving`. Смена сигнатуры без изменения чужих C/.inc и связанного HYBRID `User/asm/photo_runtime.s` нарушит сборку/передачу аргументов. Старый номер не участвует ни в выборе периферии, ни в индексировании, ни в проверках. Для окончательного удаления ниже дан согласованный дифф; ASM фото не предлагается править — сначала архитектор переводит его строку HYBRID на существующий C-модуль.

Вектор USART2 в `startup_stm32f10x_md.s:376–379` уже имеет штатный `PUBWEAK` обработчик с циклом по умолчанию; в итоговом BIN Release его вектор — 0x0801EC4D, инструкция B.W на 0x0801EC4C замыкается на себя, как в startup; одноимённого символа UART-драйвера в ELF нет. NVIC USART2 не включается. Дополнительная заглушка не нужна.

2. **ST→CMSIS, findings 126 и UART 85.** Регистры использованы в `board_init/deinit/waitPowerStability`, USART1, ADC, IWDG, WG EXTI/NVIC, SysTick, USB RCC/EXTI/NVIC, переносе VTOR и обработчиках PVD/ADC/USB wake-up. Распиновка и режимы GPIO v5 сохранены. Четырёхбитные поля режима GPIO и расположение JSQ2/3/4 пояснены по-русски. Промежуточный совместный шаг «регистры + удаление ветвей uart_init» уменьшил BIN HYBRID Release 95001→93885 (−1116 Б); его 40 полных снимков совпали побитно, ещё до удаления температуры. Это совместная дельта, не прибавлять её к −468 Б UART.

Отдельные ROM-секции до/после: board 408→416 (+8, включая общие функции EXTI/NVIC), timers 300→236 (−64), wdt 48→64 (+16), stm32_it 292→300 (+8), hw_config 682→642 (−40). Локальный рост прямой записи компенсирован удалением общих ST-функций: суммарный BIN уменьшается. Все вызовы GPIO/USART/NVIC ST в чужом модеме сохранены. Повторяющиеся записи EXTI/NVIC объединены в `board_exti_config` (58 Б) и `board_irq_enable` (24 Б). EXTI сначала маскируется, затем меняются фронты, в конце разрешается линия — как в ST. PVD включается только после готовности EXTI/NVIC. В C/HYBRID удалены строки adc/exti/iwdg/pwr/tim и уже не попадавших в ROM bkp/crc/flash. Оставлены misc/gpio/usart/rcc/spi, нужные живому коду. Исходники библиотеки не удалялись.

3. **ADC, findings 125.** Удалены `adc_gettemp`, regular-запуск, канал 16, выбор времени его измерения и TSVREFE. Они не имеют живых внешних вызовов; удаление двух функций само по себе 0 Б. adc_init после перехода на регистры: 180→158 Б (−22), весь ADC-модуль к базе: 488→372 Б (−116). Injected-каналы 8/9/4, масштабирование напряжений и ограниченные ожидания сохранены. VCC не удалялся: это отдельное изменение требований и оно противоречило бы требуемому снимку трёх injected-каналов.

4. **Wiegand, findings 123.** Удалены старые закомментированные ветви ISR и отложенный флаг `wgDataAvalible`. После атомарного снимка и проверки чётности WG_readedID вызывается в том же проходе. WG_process 138→112 Б (−26); весь модуль 364→288 (−76 с учётом общей настройки EXTI/NVIC). Тишина >25 мс, исходное формирование ID, parity, насыщение шумового счётчика и PRIMASK сохранены. Проверены все 2^24 корректных ID в host-тесте, одиночные ошибки всех 26 бит и EXTI на каждой границе исполнения (81/55 границ для профилей).

5. **outs, findings 124.** Удалены три проверки `out >= OUTSCOUNT`; все живые вызывающие стороны используют проверенные индексы 0/1. ROM модуля 488→472 Б (−16). Импульсы, паузы, повторы, 255 с, нулевые режимы и состояния проверяются существующим outs-тестом. Сигнатуры и единицы времени не менялись.

6. **USB, findings 101–102.** HYBRID переведён на `User/c/usb_stm32f10x_runtime.c`; `.s` сохранён без изменений. До перевода ROM ASM/C по карте IAR: Release 1828/1828 Б, ReleaseCustomer 1232/1156 Б. После: 1828 Б в Release, 1076 Б в ReleaseCustomer: дельта HYBRID-модуля 0/−156 Б, C-модуля 0/−80 Б. Диагностические счётчики, снимки, возраста IN/OUT/SOF, usb_sendstr и USB_Send_Debug ограничены DOMOVOI_DIAG. RAM модуля 2192→2192 / 2068→2062 Б. Большинство неиспользованных диагностических переменных уже выбрасывались линковщиком, поэтому их чистка исходника не даёт дополнительной RAM.

**Защита сохранена:** `usb_hung_ms/unconf_ms`, armed-флаги, recovery_pending/ms и ограничение аппаратного RESET работают в обоих профилях. `usb_bus_resets` — функциональная эпоха сессии, а не только диагностика: её читают store/upload/idclear/photo/fw. Она сохраняется и увеличивается в обоих профилях; иначе старый ответ мог бы уйти в новую USB-сессию. Удалены неактивный FULL_CDC_MODULE, заготовки перенастройки физического UART, пустые USB_Cable_Config/USART_Config_Default вместе с их собственными вызовами. Line coding CDC продолжает приниматься; дескрипторы не менялись. Повторная проверка указателя/длины в приватном usb_tx_submit снята, внешние входы проверяют их один раз.

**CRC findings 101:** первая проверка остаётся в usb_check_data. Второй расчёт находится в чужих user_fun_runtime.c/user_fun_hot.s и здесь не менялся. Точный дифф для архитектора приведён ниже; напрямую вызывающие parser тесты должны проверять CRC на входе USB, а не считать parser новой границей.

7. **main, findings 127.** delay_ms(500) оставлен. `git log -S "delay_ms(500)" -- main.c` ведёт к `05e9248` (первичное сохранение предоставленных исходников), пояснения аппаратного назначения нет. Сигналов, разрешающих безопасно убрать задержку, история не даёт. Она находится после SPI/ADC и до USB/user_init; доказанной связи с питанием модема нет. Дельта задержки 0 Б; ROM main 404→404 Б.

8. **Оформление.** Все затронутые C/.h: cp1251, CRLF, русский заголовок 3–8 строк и `Доработка: Кряжев А., 2026`, назначение функций и причины регистровых масок. Лишний английский boilerplate и закомментированные заготовки убраны; исходные юридические уведомления ST сохранены. Indication изменён только в оформлении: 192→192 Б.

## Проверки и пределы доказательства

- IAR: реальные compiler/assembler/linker/ielftool-команды, полученные через IarBuild -jsondb; C/HYBRID Release/ReleaseCustomer — 0 ошибок. Это не заявление о GUI или успешном IarBuild -build. Окончательные сборки находятся в TEMP/final и включают общую настройку EXTI/NVIC и исправленный порядок PVD; после последней правки кода на этих образах завершены оба полных runtime-прогона и оба парных сравнения.
- `check_comm_sources.py`, собственная проверка всех C/.h блока и `git diff --check` — PASS.
- Парное C/HYBRID: оба профиля, по 205 сценариев, 0 расхождений результатов/ответов/NOR; это сравнение вариантов между собой, не физическое измерение устройства.
- Порядок старта PVD: низкое питание в момент включения оставляет pending EXTI16 у базы и финального кода. Промежуточный вариант, включавший PVD слишком рано, использован как отрицательный контроль: он теряет событие. Финальный тест `pvd-start-order.json` — PASS; это дополнение к конечному снимку регистров.
- Регистры: 40 сценариев чистой замены ST, 160 итоговых C/HYBRID обоих профилей (48/72 МГц, USB да/нет, 1200/9600/19200/57600/115200, 1/2 stop bits). По 1112 Б RCC, GPIOA/B/C, AFIO, EXTI, NVIC enable/pending/active/priorities, SCB/SysTick, USART1, ADC1, IWDG, PWR, TIM1–4. В финале ровно три ожидаемых отличия ADC1: CR2 0x008E7001→0x000E7001; SMPR1 0x001C0000→0; SQR3 16→0. Остальное побитно одинаково.
- Снимок исполняет связанные функции периферийного старта, без задержки и запуска хранилища/модема. Модель нормализует GPIO BSRR/BRR, bit-band, W1C/W1S, сброс ADC/USART и завершение калибровки. Неизменённый RCC_GetClocksFreq моделируется из заданных часов: Unicorn пропускал STR после ITE в библиотечном расчёте HSE PLL. Это ограничение модели, а не проверка кварца, аналогового АЦП или полного физического старта.
- Зависшая калибровка/JEOC/EOC и удержание прежних входов при отказе ADC, атомарность timers, WG IRQ и UART — PASS обоих профилей. UART: все 43 границы очистки с IRQ, overflow/idle gap, PRIMASK, ограниченный TX timeout 192030 инструкций. USB recovery/RESET/resume/setup/descriptor/очередь и фото/485 покрыты полным прогоном.

**Полный run_all и границы интеграции:**

| Профиль HYBRID | Исходные тесты, первый полный прогон | Обновлённые тесты в TEMP | Устаревшие проверки репозитория |
|---|---:|---:|---|
| Release | 83/87 | **87/87** | `uarttimer`, `wiegandirq`, `WG parity off`, `stage5 Wiegand` |
| ReleaseCustomer | 82/87 | **87/87** | `uarttimer`, `wiegandirq`, `WG parity off`, `stage5 Wiegand`, `diagnostics config` |

Исходный полный run_all **не зелёный**. Это обязательное ограничение текущего worktree: uarttimer ожидает три порта; два host Wiegand-теста и linked stage5 Wiegand ожидают отложенный флаг; customer diagnostics config ожидает удалённый диагностический возраст OUT. Соответствующие тесты исправлены только в `test-integration` вне дерева, без отключения групп/мутаций/обрыва питания; функциональные изменения отражены в точном диффе ниже. Первый исходный прогон дал 83/82 зелёные группы; после окончательной правки порядка PVD и общей настройки EXTI/NVIC повторные TEMP-прогоны дали 87/87 в обоих профилях, включая все группы обрыва питания и paired mutations. Правки тестов в рабочем репозитории остаются задачей архитектора.


Артефакты вне дерева: `C:/Users/kryaz/AppData/Local/Temp/domovoi-periph-build`: before/registers/final/verified, resource-measurements.json, registers-register-snapshots.json, final-register-snapshots.json, targeted-uart.json, paired-*.json, логи/результаты run_all и test-integration. В дереве нет временных скриптов и сборок.

## Точные диффы для чужих файлов

Эти диффы **не применены в репозитории**. Удаление ABI-аргумента требует всех изменений одновременно; смена строки photo HYBRID принадлежит архитектору, её размеры и профиль должны быть отдельно согласованы. После смены сигнатур в обновлённом host UART-тесте также убрать `0` из uart_getstate/uart_isReceiving. Это следующий интеграционный шаг, а не часть текущего проверенного BIN.

### UART: чужие вызовы и переход фото на C

```diff
--- a/User/c/user_fun_runtime.c
+++ b/User/c/user_fun_runtime.c
@@ -1083,7 +1083,7 @@
   
   // внешний порт
   rs485_init(); // порт 485
-  uart_setRecTimeout(PORT1_PORT, 10); // время заполнения/ожидания приемного буфера 10 мс  
+  uart_setRecTimeout(10); // время заполнения/ожидания приемного буфера 10 мс  
   store_runtime = 1;
 }
 #endif
@@ -1658,8 +1658,8 @@
 {
   if (!user_storage_flash_ready()) return;
   photo_poll();
-  if (!device.getPhoto && uart_getstate(PORT1_PORT) && !uart_isReceiving(PORT1_PORT))
-    uart_clear(PORT1_PORT);
+  if (!device.getPhoto && uart_getstate() && !uart_isReceiving())
+    uart_clear();
 }
 #endif
 
--- a/User/c/photo_runtime.c
+++ b/User/c/photo_runtime.c
@@ -43,7 +43,7 @@
 }
 static void photo_stop(void)
 {
-  uart_init(PORT1_PORT, rs485_baud(), 0, port_buffer, PORTBUFSIZE);
+  uart_init(rs485_baud(), 0, port_buffer, PORTBUFSIZE);
   photo->stage = P_IDLE;
   photo->used = 0;
   device.getPhoto = 0;
@@ -72,8 +72,8 @@
     else if (config.rs485Periphery[i].type) other = 1;
   }
   if (camera && other) user_debug("RS485: C328 exclusive, other devices ignored\r\n");
-  uart_init(PORT1_PORT, rs485_baud(), 0, port_buffer, PORTBUFSIZE);
-  uart_setRecTimeout(PORT1_PORT, 10);
+  uart_init(rs485_baud(), 0, port_buffer, PORTBUFSIZE);
+  uart_setRecTimeout(10);
 }
 void rs485_set_config(const uint8_t *data)
 {
@@ -115,7 +115,7 @@
 static void photo_command(uint8_t cmd, uint8_t a, uint8_t b, uint8_t c, uint8_t d)
 {
   uint8_t p[6] = {0xAA, cmd, a, b, c, d};
-  uart_send(PHOTO_PORT, p, sizeof(p));
+  uart_send(p, sizeof(p));
   photo->last = timers_getMillis();
   photo->deadline = photo->last + 2000u;
 }
--- a/User/c/photo_runtime.inc
+++ b/User/c/photo_runtime.inc
@@ -239,7 +239,7 @@
     photo->size=photo->written=photo->packet=photo->used=0; photo_erased_to=0;
     photo->retry=photo->syncAck=photo->syncSeen=0; photo->rxBytes=0; photo->stage=P_SYNC;
     photo->started=now; photo->last=now-50; photo->deadline=now;
-    uart_init(PHOTO_PORT,PHOTO_BAUD,0,photo->rx,sizeof(photo->rx)); device.getPhoto=pr.address; pr.phase=0; flash_release(FO_PHOTO); break;
+    uart_init(PHOTO_BAUD,0,photo->rx,sizeof(photo->rx)); device.getPhoto=pr.address; pr.phase=0; flash_release(FO_PHOTO); break;
   case PR_TAIL:
     n=(uint16_t)(pr.limit-pr.scan); if (n>64) n=64;
     if (!w25_read_data(pr.scan,sjr.data,n)) { pr_fail(); break; }
@@ -410,8 +410,8 @@
     if (photo->pending && photo->channel==CH_USB && pr.usb_epoch!=usb_bus_resets) photo->pending=0;
     if (device.status.readyToUpdate) photo_storage_cancel();
     else {
-      if (photo->stage && uart_getstate(PHOTO_PORT) && !uart_isReceiving(PHOTO_PORT)) {
-        uint16_t n=uart_received(PHOTO_PORT); if (n>64) { photo_stop(); n=0; }
+      if (photo->stage && uart_getstate() && !uart_isReceiving()) {
+        uint16_t n=uart_received(); if (n>64) { photo_stop(); n=0; }
         for (uint16_t i=0;i<n && photo->stage && !pr.phase;i++) {
           if (photo->used>=64) { photo_stop(); break; }
           if (photo->rxBytes<sizeof(photo->rxHead)) photo->rxHead[photo->rxBytes]=photo->rx[i];
@@ -420,7 +420,7 @@
           if (photo->stage==P_DATA && photo->used>=4) { want=6u+photo->frame[2]+((uint16_t)photo->frame[3]<<8); if (want>64) { photo_stop(); break; } }
           if (photo->used==want) { photo_frame(photo->frame,want); photo->used=0; }
         }
-        uart_clear(PHOTO_PORT);
+        uart_clear();
       }
       if (pr.phase) return;
       if (photo->stage && (uint32_t)(now-photo->started)>=60000u) photo_fail("60 s limit");
--- a/User/hybrid/user_fun_hot.s
+++ b/User/hybrid/user_fun_hot.s
@@ -4126,8 +4126,7 @@
 ??user_init_3:
           CFI FunCall rs485_init
         BL       rs485_init
-        MOVS     R1,#+10
-        MOVS     R0,#+0
+        MOVS     R0,#+10
           CFI FunCall uart_setRecTimeout
         BL       uart_setRecTimeout
         MOVS     R1,#+1
--- a/DomovoiNext_HYBRID.ewp
+++ b/DomovoiNext_HYBRID.ewp
@@ -3444,7 +3444,7 @@
             <name>$PROJ_DIR$\User\c\idclear_runtime.c</name>
         </file>
         <file>
-            <name>$PROJ_DIR$\User\asm\photo_runtime.s</name>
+            <name>$PROJ_DIR$\User\c\photo_runtime.c</name>
         </file>
         <file>
             <name>$PROJ_DIR$\User\c\idindex_runtime.c</name>
```

### UART: окончательные сигнатуры и ABI mock

```diff
--- a/Modules/uart_stm32f10x.c
+++ b/Modules/uart_stm32f10x.c
@@ -2,7 +2,6 @@
 // Единственный UART приложения — USART1 для RS-485.
 // ISR дописывает буфер, главный цикл разбирает его после межбайтовой тишины.
 // Граница приёма проверяет ёмкость; ожидание освобождения TX ограничено.
-// Неиспользуемый первый аргумент временно сохраняет ABI чужих C/ASM-модулей.
 
 #include "uart_stm32f10x.h"
 #include <string.h>
@@ -51,9 +50,8 @@
 }
 
 // Настроить USART1 и передать ему приёмный буфер владельца RS-485.
-bool uart_init(uint8_t legacy_port, uint32_t Baudrate, uint8_t Stopbits, uint8_t *rxbuffer, uint16_t bfsize)
+bool uart_init(uint32_t Baudrate, uint8_t Stopbits, uint8_t *rxbuffer, uint16_t bfsize)
 {
-  (void)legacy_port;
   uint32_t mask = __get_PRIMASK();
   __disable_irq();
   uart.rxbuffer = rxbuffer;
@@ -96,18 +94,16 @@
 }
 
 // Передать двоичный буфер; при отказе порта прекратить текущую посылку.
-void uart_send(uint8_t legacy_port, uint8_t *buf, uint16_t size)
+void uart_send(uint8_t *buf, uint16_t size)
 {
-  (void)legacy_port;
   uart.txfailed = 0;
   if (!(USART1->CR1 & USART_CR1_UE) || (size && buf == NULL)) { uart.txfailed = 1; return; }
   for (uint16_t i = 0; i < size; ++i) if (!uart_sendbyte(buf[i])) return;
 }
 
 // Атомарно освободить принятый кадр относительно ISR.
-void uart_clear(uint8_t legacy_port)
+void uart_clear(void)
 {
-  (void)legacy_port;
   if (uart.rxbuffer == NULL) return;
   uint32_t mask = __get_PRIMASK();
   __disable_irq();
@@ -118,16 +114,14 @@
 }
 
 // Вернуть число байтов для разбора фотоответа.
-uint16_t uart_received(uint8_t legacy_port)
+uint16_t uart_received(void)
 {
-  (void)legacy_port;
   return uart.received;
 }
 
 // Разрешить разбор кадра после заданной межбайтовой тишины.
-uint8_t uart_getstate(uint8_t legacy_port)
+uint8_t uart_getstate(void)
 {
-  (void)legacy_port;
   return uart.twait ? 0 : uart.state;
 }
 
@@ -141,15 +135,13 @@
 }
 
 // Задать межбайтовую задержку, минимум 1 мс.
-void uart_setRecTimeout(uint8_t legacy_port, unsigned int t)
+void uart_setRecTimeout(unsigned int t)
 {
-  (void)legacy_port;
   uart.timetowait = t == 0 ? 1 : t;
 }
 
 // Проверить, продолжается ли приём текущего кадра.
-bool uart_isReceiving(uint8_t legacy_port)
+bool uart_isReceiving(void)
 {
-  (void)legacy_port;
   return uart.receiving != 0;
 }
--- a/Modules/uart_stm32f10x.h
+++ b/Modules/uart_stm32f10x.h
@@ -1,7 +1,6 @@
 // Доработка: Кряжев А., 2026
 // Интерфейс единственного RS-485 USART1, без ветвей других портов.
 // Буфер принадлежит вызывающему модулю, драйвер принимает и передаёт байты.
-// legacy_port не используется; его удаление требует правки чужого ABI.
 
 #ifndef __UART_H
 #define __UART_H
@@ -9,12 +8,12 @@
 #include "stm32f10x.h"
 
 void USART1_IRQHandler(void);
-bool uart_init(uint8_t legacy_port, uint32_t Baudrate, uint8_t Stopbits, uint8_t *rxbuffer, uint16_t bfsize);
-void uart_send(uint8_t legacy_port, uint8_t *buf, uint16_t size);
-void uart_clear(uint8_t legacy_port);
-uint16_t uart_received(uint8_t legacy_port);
-uint8_t uart_getstate(uint8_t legacy_port);
+bool uart_init(uint32_t Baudrate, uint8_t Stopbits, uint8_t *rxbuffer, uint16_t bfsize);
+void uart_send(uint8_t *buf, uint16_t size);
+void uart_clear(void);
+uint16_t uart_received(void);
+uint8_t uart_getstate(void);
 void uart_timer1ms(void);
-void uart_setRecTimeout(uint8_t legacy_port, unsigned int t);
-bool uart_isReceiving(uint8_t legacy_port);
+void uart_setRecTimeout(unsigned int t);
+bool uart_isReceiving(void);
 #endif
--- a/Tests/asm/test_photo_runtime.py
+++ b/Tests/asm/test_photo_runtime.py
@@ -25,7 +25,7 @@
         if 'usb_try_send' in self.symbols:
             self.stubs[self.addr('usb_try_send')]=self.usb_send
     def uart_send(self):
-        _,p,n,_=self.args();assert n<=6;self.commands.append(bytes(self.u.mem_read(p,n)));self.ret()
+        p,n,_,_=self.args();assert n<=6;self.commands.append(bytes(self.u.mem_read(p,n)));self.ret()
     def uart_clear(self):self.rx_count=0;self.ret()
     def event(self):self.events.append(self.args()[0]);self.ret()
     def usb_send(self):
```

### USB: декларации диагностики

```diff
--- a/Modules/usb_stm32f10x.h
+++ b/Modules/usb_stm32f10x.h
@@ -5,26 +5,33 @@
 
 #include "main.h"
 
+// Эпоха USB-сессии обязательна для отмены ответов владельцев флеш.
+extern volatile uint32_t usb_bus_resets;
+#if DOMOVOI_DIAG
 extern volatile uint32_t usb_rx_errors;
 extern volatile uint32_t usb_tx_errors;
 extern volatile uint32_t usb_debug_dropped;
 extern volatile uint32_t usb_recovery_count;
 extern volatile uint32_t usb_esof_forced;
 extern volatile uint32_t usb_esof_timeouts;
-extern volatile uint32_t usb_bus_resets;
 extern volatile uint32_t usb_recovery_reason;
+#endif
 void usb_request_recovery(void);
 void usb_in_done(void);
 void usb_sof_seen(void);
 void usb_bus_error_seen(void);
+#if DOMOVOI_DIAG
 void usb_diag_fill(uint32_t *v);
 void usb_diag_fill_regs(uint32_t *v);
 void usb_diag_fill_errors(uint32_t *v);
 void usb_diag_fill_addr(uint32_t *v);
+#endif
 void usb_init(void);
 void usb_datareceived (uint8_t* ptr, uint16_t len);
 uint8_t usb_send (uint8_t* ptr, uint16_t len);
+#if DOMOVOI_DIAG
 void usb_sendstr (char* str);
+#endif
 void usb_timer1ms (void);
 void usb_check_data (void);
 uint8_t usb_get_state (void);
```

### USB: убрать вторую CRC в C и HYBRID hot

```diff
--- a/User/c/user_fun_runtime.c
+++ b/User/c/user_fun_runtime.c
@@ -3135,7 +3135,7 @@
   if (len < 3) return; // В неполном заголовке нет команды и uniqId.
   crc = ptr[len + 3];  // crc
   crc = (crc<<8) | ptr[len + 2];
-  i = (cch == CH_SERVER && server_rx_verified(ptr)) ? crc : crc16_calc(ptr, len + 2);
+  i = (cch == CH_USB || (cch == CH_SERVER && server_rx_verified(ptr))) ? crc : crc16_calc(ptr, len + 2);
 
   //cmd_timeout = ptr[2];
   uniqId = ptr[3];
--- a/User/hybrid/user_fun_hot.s
+++ b/User/hybrid/user_fun_hot.s
@@ -12137,12 +12137,15 @@
         ADDS     R1,R7,R4
         LDRB     R1,[R1, #+2]
         ORR      R9,R1,R0, LSL #+8
+        CMP      R5,#+0
+        BEQ.N    ??user_parse_comand_crc_verified
         CMP      R5,#+1
         BNE.N    ??user_parse_comand_2
         MOV      R0,R7
           CFI FunCall server_rx_verified
         BL       server_rx_verified
         CBZ.N    R0,??user_parse_comand_2
+??user_parse_comand_crc_verified:
         MOV      R8,R9
         B.N      ??user_parse_comand_3
 ??user_parse_comand_2:
```

### Тесты нового UART/Wiegand/диагностики

```diff
--- a/Tests/run_host.py
+++ b/Tests/run_host.py
@@ -347,7 +347,7 @@
         unit=(ROOT/'Tests/test_sms.c').read_text().replace('/* TYPES */',types).replace('/* PRODUCTION */',code)
     elif kind=='uarttimer':
         uart=source('Modules/uart_stm32f10x.c')
-        types=typedefs(uart,'uart_t')+'\n'+re.search(r'^#define UARTS[^\n]*',uart,re.M).group(0)+'\n'
+        types=typedefs(uart,'uart_t')+'\n'
         code='\n'.join(function(uart.replace('inline bool uart_addbyte','bool uart_addbyte'),n) for n in ('uart_addbyte','uart_getstate','uart_isReceiving','uart_timer1ms'))
         unit=(ROOT/'Tests/test_uart_timer.c').read_text().replace('/* TYPES */',types).replace('/* PRODUCTION */',code)
     elif kind=='idtext':
--- a/Tests/test_uart_timer.c
+++ b/Tests/test_uart_timer.c
@@ -1,55 +1,25 @@
+// Доработка: Кряжев А., 2026. Один USART1: тишина, новый байт и переполнение.
 #include <stdint.h>
 #include <stdbool.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
-#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); exit(1); } } while (0)
-typedef uint8_t Int8U;
-typedef uint16_t Int16U;
+#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); exit(1); } } while(0)
 /* TYPES */
-static uart_t uart[UARTS];
-/* Let the old source reach the runtime assertion that reproduces B18. */
-#pragma clang diagnostic push
-#pragma clang diagnostic ignored "-Wmisleading-indentation"
+static uart_t uart;
 /* PRODUCTION */
-#pragma clang diagnostic pop
-static void check_delay(unsigned port, unsigned delay)
+static void check_delay(unsigned delay)
 {
-    uint8_t buffer[16] = {0};
-    memset(uart, 0, sizeof(uart));
-    uart[port].rxbuffer = buffer; uart[port].bfsize = sizeof(buffer);
-    uart[port].timetowait = delay;
-    CHECK(uart_addbyte(port, 'A')); CHECK(uart_isReceiving(port));
-    for (unsigned tick = 1; tick < delay; tick++) {
-        uart_timer1ms(); CHECK(uart[port].twait == delay - tick);
-        CHECK(uart_isReceiving(port)); CHECK(!uart_getstate(port));
-        CHECK(uart[port].received == 1 && buffer[0] == 'A');
-    }
-    uart_timer1ms(); CHECK(!uart[port].twait);
-    CHECK(!uart_isReceiving(port) && uart_getstate(port) == 1);
-    uart_timer1ms(); CHECK(!uart_isReceiving(port) && uart_getstate(port) == 1);
+ uint8_t buffer[2]={0};memset(&uart,0,sizeof uart);
+ uart.rxbuffer=buffer;uart.bfsize=sizeof buffer;uart.timetowait=delay;
+ CHECK(uart_addbyte('A'));CHECK(uart_isReceiving(0));
+ for(unsigned tick=1;tick<delay;tick++) {
+  uart_timer1ms();CHECK(uart.twait==delay-tick);CHECK(uart_isReceiving(0));CHECK(!uart_getstate(0));
+ }
+ uart_timer1ms();CHECK(!uart_isReceiving(0)&&uart_getstate(0)==1);
+ CHECK(uart_addbyte('B'));CHECK(uart_isReceiving(0)&&!uart_getstate(0));
+ CHECK(!uart_addbyte('C')&&uart.rxoverflow&&uart.received==2);
+ for(unsigned tick=0;tick<delay;tick++)uart_timer1ms();
+ CHECK(!uart_isReceiving(0)&&uart_getstate(0)==1&&buffer[0]=='A'&&buffer[1]=='B');
 }
-int main(void)
-{
-    for (unsigned port = 0; port < UARTS; port++) {
-        check_delay(port, 1); check_delay(port, 2); check_delay(port, 10);
-    }
-    uint8_t buffers[UARTS][16] = {{0}};
-    memset(uart, 0, sizeof(uart));
-    for (unsigned port = 0; port < UARTS; port++) {
-        uart[port].rxbuffer = buffers[port]; uart[port].bfsize = sizeof(buffers[port]);
-        uart[port].timetowait = port + 2; CHECK(uart_addbyte(port, 'A' + port));
-    }
-    uart_timer1ms(); CHECK(uart_isReceiving(0) && uart_isReceiving(1) && uart_isReceiving(2));
-    CHECK(uart_addbyte(0, 'Z'));
-    uart_timer1ms(); CHECK(uart_isReceiving(0) && uart_isReceiving(1) && uart_isReceiving(2));
-    uart_timer1ms(); CHECK(!uart_isReceiving(0) && !uart_isReceiving(1) && uart_isReceiving(2));
-    CHECK(uart_getstate(0) == 1 && uart_getstate(1) == 1 && !uart_getstate(2));
-    CHECK(uart[0].received == 2 && buffers[0][1] == 'Z');
-    uart_timer1ms(); CHECK(!uart_isReceiving(2) && uart_getstate(2) == 1);
-    uart[0].twait = 0; uart[0].receiving = 1; uart[0].state = 0;
-    uart_timer1ms(); CHECK(uart_isReceiving(0) && !uart_getstate(0));
-    CHECK(!uart_isReceiving(UARTS) && !uart_getstate(UARTS));
-    puts("uarttimer: receiving stays set until the configured idle gap, renewed by each byte passed");
-    return 0;
-}
+int main(void){check_delay(1);check_delay(2);check_delay(10);puts("uarttimer: single-port idle gap, renewal and overflow PASS");return 0;}
--- a/Tests/test_wiegand_irq.c
+++ b/Tests/test_wiegand_irq.c
@@ -16,31 +16,31 @@
 uint32_t timers_getMillis(void){return clock_ms;}
 void WG_readedID(uint32_t id){seen++;lastid=id;}
 /* PRODUCTION */
-static void reset(void){wgData=wgTime=wgID=0;wgBitCount=0;wgDataAvalible=false;clock_ms=100;irqmask=0;seen=0;}
+static void reset(void){wgData=wgTime=wgID=0;wgBitCount=0;clock_ms=100;irqmask=0;seen=0;}
 static void bits(unsigned n){for(unsigned i=0;i<n;i++){if(i&1)EXTI3_IRQHandler();else EXTI2_IRQHandler();}}
 int main(void){
   reset();bits(282);clock_ms=400;WG_process();WG_process();CHECK(seen==0);
   reset();bits(538);clock_ms=400;WG_process();WG_process();CHECK(seen==0);
-  reset();bits(26);uint32_t expected=(wgData>>1)&0x03FFFFFF;clock_ms=400;WG_process();CHECK(seen==0 && wgDataAvalible && wgBitCount==0);WG_process();CHECK(seen==1 && lastid==expected && !wgDataAvalible);
-  bits(26);clock_ms=700;irqmask=1;WG_process();CHECK(irqmask==1 && wgDataAvalible);irqmask=0;WG_process();CHECK(seen==2);
+  reset();bits(26);uint32_t expected=(wgData>>1)&0x03FFFFFF;clock_ms=400;WG_process();CHECK(seen==1 && wgBitCount==0 && lastid==expected);WG_process();CHECK(seen==1);
+  bits(26);clock_ms=700;irqmask=1;WG_process();CHECK(irqmask==1 && seen==2);irqmask=0;WG_process();CHECK(seen==2);
   reset();clock_ms=0xffffff00;bits(26);clock_ms=0x10;WG_process();WG_process();CHECK(seen==1);
   /* SIA: интервал между битами до 20 мс не рвёт кадр, тишина больше 25 мс завершает его;
-     опрос на каждом проходе цикла, кадр отдаётся следующим вызовом. */
-  reset();for(unsigned i=0;i<26;i++){clock_ms+=20;if(i&1)EXTI3_IRQHandler();else EXTI2_IRQHandler();WG_process();CHECK(!wgDataAvalible&&!seen);}
+     опрос на каждом проходе цикла, карта отдаётся текущим вызовом. */
+  reset();for(unsigned i=0;i<26;i++){clock_ms+=20;if(i&1)EXTI3_IRQHandler();else EXTI2_IRQHandler();WG_process();CHECK(!seen);}
   clock_ms+=25;WG_process();WG_process();CHECK(!seen);
-  clock_ms+=1;WG_process();CHECK(wgDataAvalible);WG_process();CHECK(seen==1&&!wgDataAvalible);
+  clock_ms+=1;WG_process();CHECK(seen==1);WG_process();CHECK(seen==1);
   reset();bits(26);clock_ms=100+26;WG_process();WG_process();CHECK(seen==1); /* 26 мс после последнего бита - уже карта */
   for(unsigned bit=0;bit<26;bit++){
     reset();wgData=1u^(1u<<bit);wgBitCount=26;clock_ms=400;
-    WG_process();CHECK(wgDataAvalible==!WG_PARITY);WG_process();CHECK(seen==!WG_PARITY);
+    WG_process();CHECK(seen==!WG_PARITY);WG_process();CHECK(seen==!WG_PARITY);
   }
   for(uint32_t id=0;id<0x1000000u;id++){
     unsigned upper=0,lower=0;
     for(unsigned k=0;k<12;k++){upper^=(id>>(k+12))&1u;lower^=(id>>k)&1u;}
     uint32_t frame=(upper<<25)|(id<<1)|(lower^1u);
     CHECK(wg_parity_valid(frame));
-    wgData=frame;wgBitCount=26;wgTime=100;clock_ms=400;wgDataAvalible=false;
-    WG_process();CHECK(wgDataAvalible && wgID==((frame>>1)&0x03FFFFFFu));
+    wgData=frame;wgBitCount=26;wgTime=100;clock_ms=400;
+    unsigned before=seen;WG_process();CHECK(seen==before+1 && wgID==((frame>>1)&0x03FFFFFFu));
     for(unsigned bit=0;bit<26;bit++)CHECK(!wg_parity_valid(frame^(1u<<bit)));
   }
   puts("wiegandirq: 2^24 valid frames preserve original ID, all 26 single-bit errors rejected; noise, IRQ mask and timing PASS");return 0;
--- a/Tests/asm/test_stage5_wiegand.py
+++ b/Tests/asm/test_stage5_wiegand.py
@@ -20,13 +20,12 @@
                 m.write(m.addr('wgBitCount'), 26, 1)
                 m.write(m.addr('wgTime'), 100)
                 m.write(m.addr('milliSeconds'), 400)
-                m.write(m.addr('wgDataAvalible'), 0, 1)
                 m.u.reg_write(UC_ARM_REG_PRIMASK, mask)
+                before = len(ids)
                 m.call('WG_process'); maximum = max(maximum, m.steps)
                 assert m.u.reg_read(UC_ARM_REG_PRIMASK) == mask
                 accepted = int(bit < 0 or not parity)
-                assert m.read(m.addr('wgDataAvalible'), 1) == accepted
-                before = len(ids)
+                assert len(ids) == before + accepted
                 m.call('WG_process')
                 assert len(ids) == before + accepted
                 if accepted:
@@ -35,6 +34,7 @@
             m.write(m.addr('wgData'), frame(0x123456))
             m.write(m.addr('wgBitCount'), count, 1)
             m.write(m.addr('wgTime'), 100)
+            before = len(ids)
             m.call('WG_process')
-            assert not m.read(m.addr('wgDataAvalible'), 1)
+            assert len(ids) == before
     return {'valid_cases': len(values) * 2, 'single_errors': len(values) * 52, 'maximum_snapshot_instructions': maximum, 'static_RAM_added': 0}
--- a/Tests/asm/test_diagnostics_config.py
+++ b/Tests/asm/test_diagnostics_config.py
@@ -71,7 +71,10 @@
             assert m.u.mem_read(m.addr(name), 1) == b'\0', name
         assert m.u.mem_read(m.addr('usb_recovery_pending'), 1) == b'\1'
         assert m.u.mem_read(m.addr('usb_recovery_ms'), 1) == b'\x14'
-        assert m.u.mem_read(m.addr('usb_out_ago'), 2) == b'\xff\xff'
+        if enabled:
+            assert m.u.mem_read(m.addr('usb_out_ago'), 2) == b'\xff\xff'
+        else:
+            assert 'usb_out_ago' not in m.symbols
     assert bytes(m.flash) == before
     return {'diagnostics': enabled, 'customer_cases': 12 if not enabled else 0,
             'bench_packets': [111, 112, 113] if enabled else [],
--- a/Tests/test_iar_wiegand_irq.py
+++ b/Tests/test_iar_wiegand_irq.py
@@ -1,12 +1,13 @@
 """B38: EXTI D1 между всеми инструкциями передачи Wiegand, свежий ELF IAR."""
 from iar_irq_support import Machine,UC_ARM_REG_PRIMASK
-m=Machine();seed=0x0123457
+m=Machine();seed=0x2123457;delivered=[]
+m.stubs[m.addr('WG_readedID')]=lambda:(delivered.append(m.read(m.addr('wgID'))),m.ret())
 def setup():
-    m.init();m.write(m.addr('wgData'),seed);m.write(m.addr('wgTime'),100);m.write(m.addr('wgBitCount'),26,1);m.write(m.addr('milliSeconds'),400)
+    delivered.clear();m.init();m.write(m.addr('wgData'),seed);m.write(m.addr('wgTime'),100);m.write(m.addr('wgBitCount'),26,1);m.write(m.addr('milliSeconds'),400)
 setup();m.call('WG_process');steps=m.steps;max_stack=0
 for at in range(steps):
     setup();m.call('WG_process',inject_at=at,irq=('EXTI3_IRQHandler',()))
-    count=m.read(m.addr('wgBitCount'),1);available=m.read(m.addr('wgDataAvalible'),1)
+    count=m.read(m.addr('wgBitCount'),1);available=bool(delivered)
     assert count in (1,27),('lost EXTI edge during snapshot/reset',at,count)
     if available:
         assert count==1 and m.read(m.addr('wgData'))==1,('mixed snapshot/new edge',at,count)
--- a/Tests/asm/c_vs_asm_paths.py
+++ b/Tests/asm/c_vs_asm_paths.py
@@ -17,7 +17,10 @@
     m.put_id(50048,b'0000123456')
     m.write(m.addr('config')+fields('config_s')['rele']+fields('releMode_s')['mode'],1,1)
     for name in ('out_load','out_getmodestate'):m.stubs.pop(m.addr(name),None)
-    m.write(m.addr('wgID'),0x123456);m.write(m.addr('wgDataAvalible'),1,1)
+    value=0x123456
+    raw=(value<<1)|(((value&4095).bit_count()^1)&1)|(((value>>12).bit_count()&1)<<25)
+    m.write(m.addr('wgData'),raw);m.write(m.addr('wgBitCount'),26,1)
+    m.write(m.addr('wgTime'),100);m.write(m.addr('milliSeconds'),400)
     m.write(m.addr('prof_card_mark'),1)
     p=Profile(m,totals);relay=[]
     def gpio(u,access,a,n,value,_):
```

## Короткий итог

Чистка разрешённого блока выполнена; битовая конфигурация действующей периферии сохранена, температурный путь удалён, Wiegand обслуживается за один проход, USB-сторож работает в обоих профилях. Экономия HYBRID 1668/1819 Б BIN и 41/47 Б ОЗУ. Полное удаление legacy_port, повторной CRC parser и обновление тестов требует применения чужих диффов архитектором. Цель 2 КиБ остаётся открытой.

## Контрольные суммы окончательных BIN

| Вариант / профиль | SHA256 |
|---|---|
| C-Release | `54daaa33b3042e268fb9b104d6c1fba1ea134916f9dcd6fe9e5808169d214568` |
| C-ReleaseCustomer | `d06ebdc885de2fa0ce95f8a983a6fe672d71062ce14dab3d491fa78f7cdb062f` |
| HYBRID-Release | `b07c5916ac825dcaedf529cd9585a3578f78c29954f6c86ad094638e45f3974d` |
| HYBRID-ReleaseCustomer | `a2bd4492ae7883da2acfff09bd60c17e0463c21724c2135e87a955fac06ce184` |
