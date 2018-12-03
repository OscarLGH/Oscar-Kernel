#ifndef BITMAP_H
#define BITMAP_H

#include <types.h>

static inline char bitmap_get(u8 *bitmap, u64 bit)
{
	return bitmap[bit >> 3] & (1 << (bit & 0x7));
}

static inline void bitmap_set(u8 *bitmap, u64 bit)
{
	bitmap[bit >> 3] |= (1 << (bit & 0x7));
}

static inline void bitmap_clear(u8 *bitmap, u64 bit)
{
	bitmap[bit >> 3] &= ~(1 << (bit & 0x7));
}

static inline u64 bitmap_find_1st_one(u8 *bitmap)
{
	
}

static inline u64 bitmap_find_1st_zero(u8 *bitmap)
{
	
}

#endif
