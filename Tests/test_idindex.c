/* Доработка: Кряжев А., 2026. Индекс базы ID по корзинам на модели NOR-флеш. */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int stage;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL stage %d line %d: %s\n",stage,__LINE__,#x);exit(1);}}while(0)
#define IDINDEX_HOST_TEST 1
#define BASE 266240u
#define CELLS 50048u
static uint8_t flash[0x342000];
static unsigned long reads, read_bytes, erases;
static int fail_write_at = -1, writes;
/* NOR: запись только сбрасывает биты, стирание - сектором 4 КБ. */
static bool mem_read(uint32_t a, uint8_t *p, uint16_t n){CHECK(a + n <= sizeof(flash));memcpy(p, flash + a, n);reads++;read_bytes += n;return true;}
static bool mem_write(uint32_t a, uint8_t *p, uint16_t n){CHECK(a + n <= sizeof(flash));if (writes++ == fail_write_at) return false;for (uint16_t i = 0; i < n; i++) flash[a + i] &= p[i];return true;}
static bool mem_erase_sector(uint32_t a){a &= ~4095u;CHECK(a + 4096 <= sizeof(flash));memset(flash + a, 0xFF, 4096);erases++;return true;}
/* PRODUCTION */
static uint32_t rnd = 12345;
static uint32_t next(void){rnd = rnd * 1103515245u + 12345u;return rnd >> 8;}
static void make_id(uint8_t *id, uint32_t k){for (int i = 0; i < 10; i++) id[i] = (uint8_t)('0' + (k >> (i * 3)) % 10);id[9] = (uint8_t)k;}
/* Ячейка базы: номер, ID, остальное - признаки. Запись как у команды 13: сектор переписывается целиком. */
static void cell_write(uint16_t index, const uint8_t *id){uint8_t *c = flash + BASE + (uint32_t)(index - 1u) * 32u;memset(c, 0, 32);memcpy(c, &index, 2);memcpy(c + 2, id, 10);}
static void cell_clear(uint16_t index){memset(flash + BASE + (uint32_t)(index - 1u) * 32u, 0xFF, 32);}
/* Полный проход как при старте: сумма хешей и конец базы. */
static uint16_t scan(uint32_t *sum){uint16_t end = 0;*sum = 0;for (uint16_t i = 1; i <= CELLS; i++){const uint8_t *c = flash + BASE + (uint32_t)(i - 1u) * 32u;if (c[0] == 0xFF && c[1] == 0xFF) continue;end = i;*sum ^= idindex_hash(i, c + 2);}return end;}
/* Эталон: перебор всей базы. */
static int linear(const uint8_t *id){for (uint32_t i = 0; i < CELLS; i++){const uint8_t *c = flash + BASE + i * 32u;if (!(c[0] == 0xFF && c[1] == 0xFF) && !memcmp(c + 2, id, 10)) return 1;}return 0;}
static void boot(void){uint32_t s;uint16_t e = scan(&s);memset(&ix, 0, sizeof(ix));idindex_scanned(s, e);}
static void settle(void){for (int i = 0; i < 200000 && ix.state != IX_READY && ix.state != IX_OFF; i++) idindex_poll(BASE);}
/* Пакет команды 13, как в прошивке: номер в индекс до записи, сумма после. */
static void sync(const uint16_t *index, uint8_t (*id)[10], int n){
  uint32_t delta = 0;uint16_t last = 0;
  for (int i = 0; i < n; i++){
    uint8_t old[12];int j = i - 1;
    if (index[i] > last) last = index[i];
    (void)idindex_put(index[i], id[i]);
    while (j >= 0 && index[j] != index[i]) j--;
    if (j >= 0){memcpy(old, &index[j], 2);memcpy(old + 2, id[j], 10);} else memcpy(old, flash + BASE + (uint32_t)(index[i] - 1u) * 32u, 12);
    if (!(old[0] == 0xFF && old[1] == 0xFF)) delta ^= idindex_hash(index[i], old + 2);
    delta ^= idindex_hash(index[i], id[i]);
  }
  for (int i = 0; i < n; i++) cell_write(index[i], id[i]);
  idindex_commit(delta, last);
}
static void check_all(uint32_t keys){
  uint8_t id[10], cell[32];
  for (uint32_t k = 0; k < keys; k++){make_id(id, k);int r = idindex_find(BASE, id, cell);CHECK(r == linear(id));if (r) CHECK(!memcmp(cell + 2, id, 10));}
}
int main(void){
  uint8_t id[10], cell[32];
  memset(flash, 0xFF, sizeof(flash));
  stage=1;
  /* 1. Пустая база, первая загрузка: отметки нет - пересборка, потом готов. */
  boot();CHECK(ix.state == IX_ERASE);settle();CHECK(ix.state == IX_READY);
  stage=2;
  /* 2. 5000 ID командой 13 пакетами по 30, с дырами и повторами номеров. */
  {uint32_t k = 0;for (int p = 0; p < 170; p++){uint16_t idx[30];uint8_t ids[30][10];
    for (int i = 0; i < 30; i++){idx[i] = (uint16_t)(1 + next() % 9000);make_id(ids[i], k++ % 6000);}
    if (p % 7 == 0) idx[5] = idx[4];
    sync(idx, ids, 30);}}
  CHECK(ix.state == IX_READY);check_all(8000);
  /* Поиск читает одну корзину и одну-две ячейки, а не базу. */
  make_id(id, 77);reads = read_bytes = 0;(void)idindex_find(BASE, id, cell);CHECK(read_bytes <= 64 + 3 * 32);
  stage=3;
  /* 3. Перезагрузка без изменений: отметка совпала - сразу готов, без стирания. */
  erases = 0;boot();CHECK(ix.state == IX_READY && !erases);check_all(8000);
  stage=4;
  /* 4. Базу поменяли мимо индекса (старая прошивка): сумма не сходится - пересборка. */
  make_id(id, 999999);cell_write(4321, id);cell_clear(17);
  boot();CHECK(ix.state == IX_ERASE);CHECK(idindex_find(BASE, id, cell) == -1);settle();CHECK(ix.state == IX_READY);
  CHECK(idindex_find(BASE, id, cell) == 1);check_all(8000);
  stage=5;
  /* 5. Запись во время пересборки: ниже и выше шага - после неё всё находится. */
  {uint32_t s;uint16_t e = scan(&s);boot();CHECK(ix.state == IX_READY);idindex_scanned(s, e);} /* пересканирование - пересборка */
  for (int i = 0; i < 64 + 50; i++) idindex_poll(BASE);
  CHECK(ix.state == IX_FILL && ix.step > 100);
  {uint16_t idx[2] = {3, 8999};uint8_t ids[2][10];make_id(ids[0], 1000001);make_id(ids[1], 1000002);sync(idx, ids, 2);}
  settle();CHECK(ix.state == IX_READY);make_id(id, 1000001);CHECK(idindex_find(BASE, id, cell) == 1);make_id(id, 1000002);CHECK(idindex_find(BASE, id, cell) == 1);check_all(8000);
  stage=6;
  /* 6. Обрыв питания между номером в индексе и записью ячейки: база не изменилась,
        отметка верна, лишний номер безвреден - ячейка сверяется целиком. */
  {uint16_t idx = 9100;make_id(id, 2000000);(void)idindex_put(idx, id);}
  CHECK(idindex_find(BASE, id, cell) == 0);
  boot();settle();CHECK(ix.state == IX_READY);CHECK(idindex_find(BASE, id, cell) == 0);check_all(8000);
  stage=7;
  /* 7. Обрыв при пересборке (после отметки «неверен»): при старте не верит старой отметке. */
  {uint32_t s;uint16_t e = scan(&s);(void)e;boot();CHECK(ix.state == IX_READY);
   idindex_scanned(s ^ 1u, 0);  /* сумма поменялась - отметка «неверен» и стирание */
   CHECK(ix.state == IX_ERASE);idindex_poll(BASE);  /* стёрт один сектор и «выключили» */
   boot();CHECK(ix.state == IX_ERASE);settle();CHECK(ix.state == IX_READY);check_all(8000);}
  stage=8;
  /* 8. Сбой записи номера: индекс уходит на пересборку, поиск пока перебором. */
  fail_write_at = writes;make_id(id, 3000000);{uint16_t idx[1] = {9200};uint8_t ids[1][10];memcpy(ids[0], id, 10);sync(idx, ids, 1);}
  fail_write_at = -1;CHECK(ix.state != IX_READY);CHECK(idindex_find(BASE, id, cell) == -1);
  settle();CHECK(ix.state == IX_READY);CHECK(idindex_find(BASE, id, cell) == 1);check_all(8000);
  stage=9;
  /* 9. Сектор отметок переполняется и стирается: отметки продолжают работать. */
  for (int i = 0; i < 700; i++){uint16_t idx[1] = {9300};uint8_t ids[1][10];make_id(ids[0], 4000000u + (uint32_t)i);sync(idx, ids, 1);}
  boot();CHECK(ix.state == IX_READY);make_id(id, 4000699);CHECK(idindex_find(BASE, id, cell) == 1);make_id(id, 4000698);CHECK(idindex_find(BASE, id, cell) == 0);
  stage=10;
  /* 10. Переполненная корзина: индекс выключается, поиск перебором, без ложного «нет». */
  {uint32_t target = ix_bucket(id), k = 5000000, put = 0;uint8_t ids[1][10];
   while (put < 40){make_id(ids[0], k++);if (ix_bucket(ids[0]) != target) continue;uint16_t idx[1] = {(uint16_t)(10000 + put++)};sync(idx, ids, 1);}
   CHECK(ix.state != IX_READY);settle();CHECK(ix.state == IX_OFF);CHECK(idindex_find(BASE, ids[0], cell) == -1);}
  puts("idindex: 5000 IDs found as by full scan reading one bucket; restart without erase; foreign writes, writes during rebuild, power cut, write failure, mark wrap, full bucket passed");
  return 0;
}
