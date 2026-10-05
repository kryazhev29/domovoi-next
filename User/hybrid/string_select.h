#ifndef DOMOVOI_HYBRID_STRING_SELECT_H
#define DOMOVOI_HYBRID_STRING_SELECT_H
/* Match the IAR High/Size C image's library choice in the ten-byte ID scan.
 * Without this explicit choice, the smaller HYBRID translation unit selects
 * generic memcmp, adding instructions at every record. No source is copied. */
#ifdef HYBRID_CARD
#include <stddef.h>
int __iar_small_memcmp(const void *left, const void *right, size_t size);
#define memcmp __iar_small_memcmp
#endif
#endif
