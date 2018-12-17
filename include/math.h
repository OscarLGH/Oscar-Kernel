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

#endif

