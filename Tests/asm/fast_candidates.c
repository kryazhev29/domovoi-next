
#include <stdint.h>
#include <string.h>
#include <intrinsics.h>

uint32_t old_fnv10(const uint8_t *p) {
  uint32_t h = 2166136261u;
  uint8_t n = 10;
  while (n--) { h ^= *p++; h *= 16777619u; }
  return h;
}
uint32_t opt_fnv10(const uint8_t *p) {
  uint32_t h = 2166136261u;
#define F(i) h = (h ^ p[i]) * 16777619u
  F(0); F(1); F(2); F(3); F(4); F(5); F(6); F(7); F(8); F(9);
#undef F
  return h;
}
uint16_t old_gf2(const uint16_t *m, uint16_t x) {
  uint16_t r = 0;
  for (uint8_t j = 0; x; j++, x >>= 1) if (x & 1u) r ^= m[j];
  return r;
}
uint16_t opt_gf2(const uint16_t *m, uint16_t x) {
  uint16_t r = 0;
#define G(i) if (x & (1u << (i))) r ^= m[i]
  G(0); G(1); G(2); G(3); G(4); G(5); G(6); G(7);
  G(8); G(9); G(10); G(11); G(12); G(13); G(14); G(15);
#undef G
  return r;
}
uint16_t opt_gf2_sparse(const uint16_t *m, uint16_t x) {
  uint16_t r = 0;
  while (x) {
    r ^= m[__CLZ(__RBIT(x))];
    x &= x - 1u;
  }
  return r;
}
uint32_t old_all_ff(const uint8_t *p, uint16_t n) {
  while (n--) if (*p++ != 255) return 0;
  return 1;
}
uint32_t opt_all_ff(const uint8_t *p, uint16_t n) {
  if (n && *p != 255) return 0;
  while (n && ((uintptr_t)p & 3u)) {
    if (*p++ != 255) return 0;
    n--;
  }
  while (n >= 4) {
    uint32_t v;
    memcpy(&v, p, 4);
    if (v != 0xffffffffu) return 0;
    p += 4; n -= 4;
  }
  while (n--) if (*p++ != 255) return 0;
  return 1;
}
uint32_t old_id_equal(const uint8_t *a, const uint8_t *b) {
  return memcmp(a, b, 10) == 0;
}
uint32_t opt_id_equal(const uint8_t *a, const uint8_t *b) {
#define E(i) if (a[i] != b[i]) return 0
  E(0); E(1); E(2); E(3); E(4); E(5); E(6); E(7); E(8); E(9);
#undef E
  return 1;
}

uint32_t old_sldn(const uint8_t *p, uint16_t n) {
  uint16_t i = 0;
  while (i < n && memcmp(p+i, "SLDN", n-i < 4 ? n-i : 4)) i++;
  return i;
}
uint32_t opt_sldn(const uint8_t *p, uint16_t n) {
  uint16_t i;
  for (i = 0; i < n; i++) {
    if (p[i] != 'S') continue;
    if (n-i == 1) break;
    if (p[i+1] != 'L') continue;
    if (n-i == 2) break;
    if (p[i+2] != 'D') continue;
    if (n-i == 3 || p[i+3] == 'N') break;
  }
  return i;
}
