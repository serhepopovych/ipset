#ifndef _IPSET_COMPILER_TYPES_H
#define _IPSET_COMPILER_TYPES_H

#include_next <linux/compiler_types.h>

/* gcc < 4.9 does not support asm __inline */
#ifdef asm_inline
#undef asm_inline
#define asm_inline asm
#endif

#endif /* _IPSET_COMPILER_TYPES_H */
