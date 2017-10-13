#include "Lib/Integer.h"

UINT32 Weight32(UINT32 Num)
{
	UINT32 Res = (Num & 0x55555555) + ((Num >> 1) & 0x55555555);
	Res = (Res & 0x33333333) + ((Res >> 2) & 0x33333333);
	Res = (Res & 0x0F0F0F0F) + ((Res >> 4) & 0x0F0F0F0F);
	Res = (Res & 0x00FF00FF) + ((Res >> 8) & 0x00FF00FF);
	return (Res & 0x0000FFFF) + ((Res >> 16) & 0x0000FFFF);
}

UINT64 OrderBase2(UINT64 Num)
{
	if (Num / 2)
		return OrderBase2(Num / 2) + 1;
	else
		return 0;
}

UINT64 CountBits(UINT64 Value)
{
	int i;
	int Bits = 0;
	for (i = 0; i < sizeof(Value) * 8; i++)
	{
		if ((Value >> i) & 0x1 == 1)
			Bits++;
	}
	return Bits;
}

UINT64 FirstBitSet(UINT64 Num)
{
	long    BitIndex;

	if (Num == 0) {
		return -1;
	}
	for (BitIndex = 31; (INT32)Num > 0; BitIndex--, Num <<= 1);
	return BitIndex;
}