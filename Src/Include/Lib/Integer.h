#include "Type.h"

UINT32 Weight32(UINT32 Num);
UINT64 OrderBase2(UINT64 Num);
UINT64 CountBits(UINT64 Value);
UINT64 FirstBitSet(UINT64 Num);

#define RoundUp(x, y) ((((x) + ((y) - 1)) / (y)) * (y))
#define RoundDown(x, y) ((x) / (y) * (y))
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define Min(a,b) ((a)<(b)?(a):(b))
#define Max(a,b) ((a)>=(b)?(a):(b))