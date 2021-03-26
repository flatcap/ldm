#ifndef __LDM_TYPES_H__
#define __LDM_TYPES_H__

#ifdef __CYGWIN__
#define __off64_t _off64_t
#define __be16_to_cpu be16_to_cpu
#define __be32_to_cpu be32_to_cpu
#define __be32_to_cpup(x) ( (u32) be32_to_cpu( (u32 *)x))
#define __be64_to_cpu be64_to_cpu
#define lseek64 lseek
#include <asm/types.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN

#ifdef __GNUC__
#define be16_to_cpu(x) __builtin_bswap16(x)
#define be32_to_cpu(x) __builtin_bswap32(x)
#define be64_to_cpu(s) __builtin_bswap64(s)  // must be a better way to fix this ...
#else
#define be16_to_cpu(x) bswap_16(x)
#define be16_to_cpu(x) bswap_32(x)
#define be64_to_cpu(s) bswap_64(s)
#endif

#else
#define be16_to_cpu
#define be32_to_cpu
#define be64_to_cpu
#endif

#else
#include <linux/types.h>
#endif

namespace ldm {

typedef __u8	u8;
typedef __u16	u16;
typedef __u32	u32;
typedef __u64	u64;

};

#endif
