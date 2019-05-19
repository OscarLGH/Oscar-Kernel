#ifndef _MATH_H
#define _MATH_H
#define roundup(x, y) ((((x) + ((y) - 1)) / (y)) * (y))
#define rounddown(x, y) (((x) / (y)) * (y))

static inline int order2_to_int(int order)
{
	int r = 1;
	while (order--) {
		r << 1;
	}
	return r;
}

static inline int weight32(int num)
{
	int res = (num & 0x55555555) + ((num >> 1) & 0x55555555);
	res = (res & 0x33333333) + ((res >> 2) & 0x33333333);
	res = (res & 0x0f0f0f0f) + ((res >> 4) & 0x0f0f0f0f);
	res = (res & 0x00ff00ff) + ((res >> 8) & 0x00ff00ff);
	return (res & 0x0000ffff) + ((res >> 16) & 0x0000ffff);
}

static inline u64 order_base_2(u64 num)
{
	if (num / 2)
		return order_base_2(num / 2) + 1;
	else
		return 0;
}

static inline u64 count_bits(u64 value)
{
	int i;
	int bits = 0;
	for (i = 0; i < sizeof(value) * 8; i++)
	{
		if ((value >> i) & 0x1 == 1)
			bits++;
	}
	return bits;
}

static inline u64 ffs(u64 num)
{
	long bit_index;

	if (num == 0) {
		return -1;
	}
	for (bit_index = 31; (int)num > 0; bit_index--, num <<= 1);
	return bit_index;
}

#define round_up(x, y) ((((x) + ((y) - 1)) / (y)) * (y))
#define roudn_down(x, y) ((x) / (y) * (y))
#define div_round_up(n, d) (((n) + (d) - 1) / (d))
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>=(b)?(a):(b))


#endif

