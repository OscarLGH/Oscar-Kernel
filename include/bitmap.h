#ifndef BITMAP_H
#define BITMAP_H

#include <types.h>

struct bitmap {
	u8 *bitmap_data;
	u64 size;
};

static inline void bitmap_init(struct bitmap *bitmap, u64 size)
{
	int i;
	bitmap->size = size;
	for (i = 0; i < size / 8; i++)
		bitmap->bitmap_data[i] = 0;
}

static inline char bitmap_get(struct bitmap *bitmap, u64 bit)
{
	return bitmap->bitmap_data[bit >> 3] & (1 << (bit & 0x7));
}

static inline void bitmap_set(struct bitmap *bitmap, u64 bit)
{
	bitmap->bitmap_data[bit >> 3] |= (1 << (bit & 0x7));
}

static inline void bitmap_clear(struct bitmap *bitmap, u64 bit)
{
	bitmap->bitmap_data[bit >> 3] &= ~(1 << (bit & 0x7));
}

static inline int bitmap_find_free(struct bitmap *bitmap)
{
	int i, j;
	for (i = 0; i < bitmap->size / 8; i++) {
		if (bitmap->bitmap_data[i] == 0)
			continue;
		for (j = 0; j < 8; j++) {
			if (bitmap->bitmap_data[i] & (1 << j) == 0)
				return i * 8 + j;
		}
	}
	return -1;
}

static inline int bitmap_find_free_region(struct bitmap *bitmap, u64 bits)
{
	int i, j;
	for (i = 0; i < bitmap->size; i++) {
		if (bitmap_get(bitmap, i) == 1)
			continue;
		for (j = i; j < bits; j++) {
			if (bitmap_get(bitmap, i) == 1)
				break;

			if (j - i >= bits)
				return i;
		}
	}
	return -1;
}

static inline int bitmap_allocate_bits(struct bitmap *bitmap, u64 bits)
{
	int i;
	int bit_f = bitmap_find_free_region(bitmap, bits);
	if (bit_f != -1) {
		for (i = bit_f; i < bit_f + bits; i++)
			bitmap_set(bitmap, i);
		return 0;
	}
	return -1;
}

#endif
