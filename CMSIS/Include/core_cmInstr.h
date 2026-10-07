/**************************************************************************/

#ifndef __CORE_CMINSTR_H
#define __CORE_CMINSTR_H

#if   defined ( __CC_ARM )

#if (__ARMCC_VERSION < 400677)
  #error "Please use ARM Compiler Toolchain V4.0.677 or later!"
#endif

#define __NOP                             __nop

#define __WFI                             __wfi

#define __WFE                             __wfe

#define __SEV                             __sev

#define __ISB()                           __isb(0xF)

#define __DSB()                           __dsb(0xF)

#define __DMB()                           __dmb(0xF)

#define __REV                             __rev

static __INLINE __ASM uint32_t __REV16(uint32_t value)
{
  rev16 r0, r0
  bx lr
}

static __INLINE __ASM int32_t __REVSH(int32_t value)
{
  revsh r0, r0
  bx lr
}

#if       (__CORTEX_M >= 0x03)

#define __RBIT                            __rbit

#define __LDREXB(ptr)                     ((uint8_t ) __ldrex(ptr))

#define __LDREXH(ptr)                     ((uint16_t) __ldrex(ptr))

#define __LDREXW(ptr)                     ((uint32_t ) __ldrex(ptr))

#define __STREXB(value, ptr)              __strex(value, ptr)

#define __STREXH(value, ptr)              __strex(value, ptr)

#define __STREXW(value, ptr)              __strex(value, ptr)

#define __CLREX                           __clrex

#define __SSAT                            __ssat

#define __USAT                            __usat

#define __CLZ                             __clz

#endif

#elif defined ( __ICCARM__ )

#include <cmsis_iar.h>

#elif defined ( __GNUC__ )

__attribute__( ( always_inline ) ) static __INLINE void __NOP(void)
{
  __ASM volatile ("nop");
}

__attribute__( ( always_inline ) ) static __INLINE void __WFI(void)
{
  __ASM volatile ("wfi");
}

__attribute__( ( always_inline ) ) static __INLINE void __WFE(void)
{
  __ASM volatile ("wfe");
}

__attribute__( ( always_inline ) ) static __INLINE void __SEV(void)
{
  __ASM volatile ("sev");
}

__attribute__( ( always_inline ) ) static __INLINE void __ISB(void)
{
  __ASM volatile ("isb");
}

__attribute__( ( always_inline ) ) static __INLINE void __DSB(void)
{
  __ASM volatile ("dsb");
}

__attribute__( ( always_inline ) ) static __INLINE void __DMB(void)
{
  __ASM volatile ("dmb");
}

__attribute__( ( always_inline ) ) static __INLINE uint32_t __REV(uint32_t value)
{
  uint32_t result;

  __ASM volatile ("rev %0, %1" : "=r" (result) : "r" (value) );
  return(result);
}

__attribute__( ( always_inline ) ) static __INLINE uint32_t __REV16(uint32_t value)
{
  uint32_t result;

  __ASM volatile ("rev16 %0, %1" : "=r" (result) : "r" (value) );
  return(result);
}

__attribute__( ( always_inline ) ) static __INLINE int32_t __REVSH(int32_t value)
{
  uint32_t result;

  __ASM volatile ("revsh %0, %1" : "=r" (result) : "r" (value) );
  return(result);
}

#if       (__CORTEX_M >= 0x03)

__attribute__( ( always_inline ) ) static __INLINE uint32_t __RBIT(uint32_t value)
{
  uint32_t result;

   __ASM volatile ("rbit %0, %1" : "=r" (result) : "r" (value) );
   return(result);
}

__attribute__( ( always_inline ) ) static __INLINE uint8_t __LDREXB(volatile uint8_t *addr)
{
    uint8_t result;

   __ASM volatile ("ldrexb %0, [%1]" : "=r" (result) : "r" (addr) );
   return(result);
}

__attribute__( ( always_inline ) ) static __INLINE uint16_t __LDREXH(volatile uint16_t *addr)
{
    uint16_t result;

   __ASM volatile ("ldrexh %0, [%1]" : "=r" (result) : "r" (addr) );
   return(result);
}

__attribute__( ( always_inline ) ) static __INLINE uint32_t __LDREXW(volatile uint32_t *addr)
{
    uint32_t result;

   __ASM volatile ("ldrex %0, [%1]" : "=r" (result) : "r" (addr) );
   return(result);
}

__attribute__( ( always_inline ) ) static __INLINE uint32_t __STREXB(uint8_t value, volatile uint8_t *addr)
{
   uint32_t result;

   __ASM volatile ("strexb %0, %2, [%1]" : "=r" (result) : "r" (addr), "r" (value) );
   return(result);
}

__attribute__( ( always_inline ) ) static __INLINE uint32_t __STREXH(uint16_t value, volatile uint16_t *addr)
{
   uint32_t result;

   __ASM volatile ("strexh %0, %2, [%1]" : "=r" (result) : "r" (addr), "r" (value) );
   return(result);
}

__attribute__( ( always_inline ) ) static __INLINE uint32_t __STREXW(uint32_t value, volatile uint32_t *addr)
{
   uint32_t result;

   __ASM volatile ("strex %0, %2, [%1]" : "=r" (result) : "r" (addr), "r" (value) );
   return(result);
}

__attribute__( ( always_inline ) ) static __INLINE void __CLREX(void)
{
  __ASM volatile ("clrex");
}

#define __SSAT(ARG1,ARG2) \
({                          \
  uint32_t __RES, __ARG1 = (ARG1); \
  __ASM ("ssat %0, %1, %2" : "=r" (__RES) :  "I" (ARG2), "r" (__ARG1) ); \
  __RES; \
 })

#define __USAT(ARG1,ARG2) \
({                          \
  uint32_t __RES, __ARG1 = (ARG1); \
  __ASM ("usat %0, %1, %2" : "=r" (__RES) :  "I" (ARG2), "r" (__ARG1) ); \
  __RES; \
 })

__attribute__( ( always_inline ) ) static __INLINE uint8_t __CLZ(uint32_t value)
{
  uint8_t result;

  __ASM volatile ("clz %0, %1" : "=r" (result) : "r" (value) );
  return(result);
}

#endif

#elif defined ( __TASKING__ )

#endif

/*@}*/

#endif
