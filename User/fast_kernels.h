/* Fixed contracts for normal-memory buffers; no peripheral/MMIO pointers. */
#ifndef FAST_KERNELS_H
#define FAST_KERNELS_H
#include <stdint.h>
#include <stdbool.h>

#ifdef ASM_FAST_KERNELS
/* Exactly ten byte reads, any input alignment. */
uint32_t fast_fnv10(const uint8_t *p);
/* Matrix has sixteen aligned uint16_t columns; x is a uint16_t. */
uint16_t fast_gf2_apply(const uint16_t *m, uint16_t x);
/* Read only [p,p+n); n==0 performs no read and returns one. */
bool fast_all_ff(const uint8_t *p, uint16_t n);
/* First complete SLDN or matching suffix S/SL/SLD, otherwise n. */
uint32_t fast_sldn(const uint8_t *p, uint16_t n);
#else
/* Portable C references for host tests and builds without the ASM option. */
static inline uint32_t fast_fnv10(const uint8_t *p) {
  uint32_t h = 2166136261u;
  uint8_t n = 10;
  while (n--) { h ^= *p++; h *= 16777619u; }
  return h;
}
static inline bool fast_all_ff(const uint8_t *p, uint16_t n) {
  while (n--) if (*p++ != 255u) return 0;
  return 1;
}
static inline uint32_t fast_sldn(const uint8_t *p, uint16_t n) {
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
#endif
#endif
