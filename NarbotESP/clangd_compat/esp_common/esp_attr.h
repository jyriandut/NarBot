#pragma once

// clangd compatibility overlay for ESP-IDF attributes that Apple clangd parses
// as Mach-O section attributes. Firmware builds use the real PlatformIO SDK
// header; this directory is referenced only from .clangd.

#define ROMFN_ATTR
#define IRAM_ATTR
#define DRAM_ATTR
#define IRAM_DATA_ATTR
#define COREDUMP_IRAM_DATA_ATTR
#define IRAM_BSS_ATTR
#define WORD_ALIGNED_ATTR __attribute__((aligned(4)))
#define DMA_ATTR WORD_ALIGNED_ATTR
#define FORCE_INLINE_ATTR static inline __attribute__((always_inline))
#define DRAM_STR(str) (str)
#define RTC_IRAM_ATTR
#define EXT_RAM_ATTR
#define RTC_DATA_ATTR
#define RTC_RODATA_ATTR
#define RTC_SLOW_ATTR
#define RTC_FAST_ATTR
#define __NOINIT_ATTR
#define EXT_RAM_NOINIT_ATTR
#define RTC_NOINIT_ATTR
#define COREDUMP_DRAM_ATTR
#define COREDUMP_RTC_DATA_ATTR
#define COREDUMP_RTC_FAST_ATTR
#define NOINLINE_ATTR __attribute__((noinline))

#ifdef __cplusplus
#define FLAG_ATTR_IMPL(TYPE, INT_TYPE) \
FORCE_INLINE_ATTR constexpr TYPE operator~(TYPE a) { return (TYPE)~(INT_TYPE)a; } \
FORCE_INLINE_ATTR constexpr TYPE operator|(TYPE a, TYPE b) { return (TYPE)((INT_TYPE)a | (INT_TYPE)b); } \
FORCE_INLINE_ATTR constexpr TYPE operator&(TYPE a, TYPE b) { return (TYPE)((INT_TYPE)a & (INT_TYPE)b); } \
FORCE_INLINE_ATTR constexpr TYPE operator^(TYPE a, TYPE b) { return (TYPE)((INT_TYPE)a ^ (INT_TYPE)b); } \
FORCE_INLINE_ATTR constexpr TYPE operator>>(TYPE a, int b) { return (TYPE)((INT_TYPE)a >> b); } \
FORCE_INLINE_ATTR constexpr TYPE operator<<(TYPE a, int b) { return (TYPE)((INT_TYPE)a << b); } \
FORCE_INLINE_ATTR TYPE &operator|=(TYPE &a, TYPE b) { a = a | b; return a; } \
FORCE_INLINE_ATTR TYPE &operator&=(TYPE &a, TYPE b) { a = a & b; return a; } \
FORCE_INLINE_ATTR TYPE &operator^=(TYPE &a, TYPE b) { a = a ^ b; return a; } \
FORCE_INLINE_ATTR TYPE &operator>>=(TYPE &a, int b) { a = a >> b; return a; } \
FORCE_INLINE_ATTR TYPE &operator<<=(TYPE &a, int b) { a = a << b; return a; }
#define FLAG_ATTR_U32(TYPE) FLAG_ATTR_IMPL(TYPE, uint32_t)
#define FLAG_ATTR FLAG_ATTR_U32
#else
#define FLAG_ATTR(TYPE)
#endif

#define IDF_DEPRECATED(REASON)
