#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
typedef uint32_t Int32U;
typedef uint16_t Int16U;
typedef uint8_t *pInt8U;
#define mem_check_power() ((void)0)
#define wdt_reload() (++services)
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); exit(1); } } while(0)
#define TARGET 0x41000u
static uint8_t flash[0x200000], before[4096], after[4096], image[4096];
static unsigned operations, cut, partial, lengths[128], reads, fail_read, fail_op, services;
static uint32_t addresses[128];
static int cut_bits=-1;
static jmp_buf poweroff;
bool w25_read_data(uint32_t a,uint8_t *p,uint16_t n) {
    CHECK(a <= sizeof(flash) && n <= sizeof(flash)-a);
    if (++reads == fail_read) return false;
    memcpy(p,flash+a,n);return true;
}
static unsigned limit(unsigned n) {
    CHECK(operations < 128);lengths[operations]=n;
    return cut == operations+1 && partial < n ? partial : n;
}
static void checkpoint(void) { if (++operations == cut) longjmp(poweroff,1); }
bool w25_erase_sector(uint32_t a) {
    a &= ~4095u;CHECK(a+4096<=sizeof(flash));
    addresses[operations]=a;
    if (operations+1 == fail_op) { operations++; return false; }
    memset(flash+a,255,limit(4096));checkpoint();return true;
}
bool w25_write_data(uint32_t a,uint8_t *p,uint16_t n) {
    while(n) {
        unsigned k=256-(a&255),i,m;if(k>n)k=n;m=limit(k);
        addresses[operations]=a;
        if (operations+1 == fail_op) { operations++; return false; }
        CHECK(a+k<=sizeof(flash));
        if (cut==operations+1 && cut_bits>=0) {
            for(i=0;i<k;i++) {
                unsigned bits=cut_bits>(int)(i*8)?(unsigned)cut_bits-i*8:0;
                uint8_t mask=bits>=8?255u:(uint8_t)((1u<<bits)-1u);
                flash[a+i]&=p[i]|(uint8_t)~mask;
            }
        } else for(i=0;i<m;i++)flash[a+i]&=p[i];
        a+=k;p+=k;n-=k;checkpoint();
    }return true;
}
uint16_t crc16_calcInit(uint8_t *p,uint16_t n,uint16_t c) {
    unsigned bit;while(n--) { c^=*p++;for(bit=0;bit<8;bit++)c=(c>>1)^((c&1)?0xa001:0); }
    return c;
}
/* PRODUCTION */
static void seed(void) {
    unsigned i;memset(flash,255,sizeof(flash));
    for(i=0;i<4096;i++)before[i]=(uint8_t)(i*17u+23u);
    memcpy(after,before,4096);memset(after+100,0x42,32);
    memcpy(flash+TARGET,before,4096);operations=cut=partial=reads=fail_read=fail_op=services=0;cut_bits=-1;
}
static bool restart(void) {
#ifdef SECTOR_JOURNAL_H
    mem_storage_ready=false;cfg_legacy_checked=0;
    return mem_recover();
#else
    /* В прежней прошивке точки восстановления таблиц нет. */
    return true;
#endif
}
#ifdef SECTOR_JOURNAL_H
/* EXPECTED DESTINATIONS */
static uint8_t saved[3][4096];
static uint32_t model_crc(const uint8_t *p,unsigned n) {
    uint32_t c=0xffffffffu;unsigned i;while(n--){c^=*p++;for(i=0;i<8;i++)c=(c>>1)^((c&1)?0xedb88320u:0);}
    return ~c;
}
static void save_flash(void) {
    memcpy(saved[0],flash+TARGET,4096);memcpy(saved[1],flash+SJ_DATA,4096);memcpy(saved[2],flash+SJ_META,4096);
}
static void load_flash(void) {
    memcpy(flash+TARGET,saved[0],4096);memcpy(flash+SJ_DATA,saved[1],4096);memcpy(flash+SJ_META,saved[2],4096);
    operations=cut=partial=reads=fail_read=fail_op=services=0;cut_bits=-1;mem_storage_ready=false;cfg_legacy_checked=0;
}
static void whole(void) {
    unsigned i;CHECK(!memcmp(flash+TARGET,before,4096)||!memcmp(flash+TARGET,after,4096));
    for(i=0;i<4096;i++)CHECK(flash[TARGET-4096+i]==0xff&&flash[TARGET+4096+i]==0xff);
}
static void setup(unsigned reuse) {
    seed();
    if(reuse) {
        /* Оставить действительный прежний commit после отказа очистки. */
        fail_op=38;CHECK(!mem_replace_sector(TARGET,after));fail_op=0;
        CHECK(!memcmp(flash+TARGET,after,4096));
        memcpy(before,after,4096);after[700]^=0x23;after[4020]^=0xa1;
    }
    operations=reads=services=0;save_flash();
}
static void campaign(unsigned reuse) {
    unsigned total,lens[128],k,p,cases=0;uint32_t a[128];
    setup(reuse);CHECK(mem_rewrite(TARGET,after,4096,image));CHECK(services>0);
    total=operations;memcpy(lens,lengths,sizeof(lens));memcpy(a,addresses,sizeof(a));
    CHECK(!memcmp(flash+TARGET,after,4096));
    for(k=1;k<=total;k++)for(p=0;p<=lens[k-1];p++) {
        load_flash();cut=k;partial=p;
        if(!setjmp(poweroff))CHECK(mem_rewrite(TARGET,after,4096,image));
        CHECK(operations==k);cut=0;CHECK(restart());whole();cases++;
    }
    /* Полный marker может получиться до последнего бита; тогда новый образ уже опубликован. */
    for(k=1;k<=total;k++)if(a[k-1]==SJ_META+28)for(p=0;p<=32;p++) {
        uint32_t marker;load_flash();cut=k;cut_bits=(int)p;
        if(!setjmp(poweroff))CHECK(mem_rewrite(TARGET,after,4096,image));
        memcpy(&marker,flash+SJ_META+28,4);cut=0;CHECK(restart());whole();
        if(marker==SJ_COMMIT)CHECK(!memcmp(flash+TARGET,after,4096));cases++;
    }
    printf("sectorjournal: %s, %u operations, %u byte/commit-bit cuts PASS\n",reuse?"previous commit reuse":"fresh transaction",total,cases);
}
static void published(void) {
    uint32_t h[8];
    seed();cut=21;partial=1700;
    if(!setjmp(poweroff))CHECK(mem_replace_sector(TARGET,after));
    memcpy(h,flash+SJ_META,sizeof(h));
    CHECK(h[4]==model_crc(after,4096)&&h[6]==model_crc((uint8_t*)h,24)&&h[7]==SJ_COMMIT);
    cut=0;mem_storage_ready=false;operations=reads=0;save_flash();
}
static void recovery_campaign(void) {
    unsigned total,lens[128],k,p,cases=0;
    published();CHECK(restart());total=operations;memcpy(lens,lengths,sizeof(lens));
    for(k=1;k<=total;k++)for(p=0;p<=lens[k-1];p++) {
        load_flash();cut=k;partial=p;
        if(!setjmp(poweroff))CHECK(restart());
        CHECK(operations==k);cut=0;CHECK(restart());CHECK(!memcmp(flash+TARGET,after,4096));whole();cases++;
    }
    /* Последовательные выключения в одной и той же recovery. */
    load_flash();
    for(k=0;k<6;k++) {
        operations=0;cut=1;partial=19+k*73;
        if(!setjmp(poweroff))CHECK(restart());cut=0;
    }
    CHECK(restart());CHECK(!memcmp(flash+TARGET,after,4096));
    printf("sectorjournal: recovery, %u operations, %u byte cuts and repeated cuts PASS\n",total,cases);
}
static void faults(void) {
    static uint8_t large[65535];
    unsigned nr,nw,k;setup(0);CHECK(mem_replace_sector(TARGET,after));nr=reads;nw=operations;
    for(k=1;k<=nr;k++) {
        load_flash();fail_read=k;CHECK(!mem_replace_sector(TARGET,after));
        fail_read=0;CHECK(restart());whole();
    }
    for(k=1;k<=nw;k++) {
        load_flash();fail_op=k;CHECK(!mem_replace_sector(TARGET,after));
        fail_op=0;CHECK(restart());whole();
    }
    published();flash[SJ_DATA+255]^=1;operations=0;
    CHECK(!restart()&&!mem_read(TARGET,image,32)&&!mem_write(TARGET,after,32)&&!mem_erase_sector(TARGET));
    CHECK(!mem_replace_sector(TARGET+4096,after)&&operations==0);
    CHECK(!mem_read(0x1C8000u,large,sizeof(large))&&operations==0);
    printf("sectorjournal: %u read failures, %u erase/program failures and corrupt scratch PASS\n",nr,nw);
}
static void bounds(void) {
    static const uint32_t bad[]={0,0x20000,0x21000,0x40000,0x41001,0x1c8000,0x1cb000,0x1cd000,0x1ce000,0x1cf000,0x1f5000,0x1f7000,SJ_DATA,SJ_META,0x200000,0xfffff000};
    unsigned i;seed();
    for(i=0;i<sizeof(bad)/sizeof(bad[0]);i++)CHECK(!mem_replace_sector(bad[i],after));
    CHECK(!sj_span(0xffffffffu,2)&&!sj_span(SJ_META+4095,2)&&!sj_span(SJ_META,0));
    CHECK(!mem_write(SJ_DATA,after,32)&&!mem_write(SJ_META,after,32)&&!mem_erase_sector(SJ_DATA)&&!mem_erase_mem(SJ_META,SJ_META));
    CHECK(!mem_rewrite(TARGET+4090,after,32,image)&&operations==0);
    CHECK(mem_rewrite(TARGET,before,4096,image)&&operations==0);
    CHECK(mem_replace_sector(TARGET,before)&&operations==0);
    for(i=0;i<0x300000u;i+=4096) {
        unsigned j;bool allowed=false;
        for(j=0;j<sizeof(expected)/sizeof(expected[0]);j++)if(i>=expected[j][0]&&i<expected[j][1])allowed=true;
        CHECK(sj_destination(i)==allowed);
    }
    /* BOOT_ADAPTER_TEST */
    puts("sectorjournal: destination whitelist, reserved sectors, unchanged image PASS");
}
static void metadata(void) {
    unsigned word,bit;
    published();memcpy(flash+TARGET,before,4096);save_flash();
    for(word=0;word<8;word++)for(bit=0;bit<32;bit++) {
        load_flash();flash[SJ_META+word*4+bit/8]^=(uint8_t)(1u<<(bit%8));
        CHECK(restart()&&operations==0&&!memcmp(flash+TARGET,before,4096));
    }
    puts("sectorjournal: 256 metadata bit corruptions do not erase destination PASS");
}
#endif
int main(void) {
    seed();cut=1;partial=2072;
    if(!setjmp(poweroff))CHECK(mem_rewrite(TARGET+100,after+100,32,image));
    cut=0;CHECK(restart());
    CHECK(!memcmp(flash+TARGET,before,4096) || !memcmp(flash+TARGET,after,4096));
    puts("sectorjournal: power cut does not destroy ID sector and its neighbours");
#ifdef SECTOR_JOURNAL_H
    seed();fail_read=2;
    CHECK(!mem_replace_sector(TARGET,after));
    CHECK(operations==0 && !memcmp(flash+TARGET,before,4096));
    puts("sectorjournal: failed initial comparison does not erase or publish data");
    campaign(0);campaign(1);recovery_campaign();faults();bounds();metadata();
#endif
    return 0;
}
