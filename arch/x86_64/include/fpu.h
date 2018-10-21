#ifndef _FPU_H
#define _FPU_H


#define XCR0_X87 BIT0
#define XCR0_SSE BIT1
#define XCR0_AVX BIT2
#define XCR0_BNDREG BIT3
#define XCR0_BNDCSR BIT4
#define XCR0_OPMASK BIT5
#define XCR0_ZMM_H256 BIT6
#define XCR0_H16_ZMM BIT7
#define XCR0_PKRU BIT9

static inline u64 xgetbv(u32 xcr_index)
{
	u64 low, high;
	asm volatile("xgetbv\n"
				: "=a"(low), "=d"(high) : "c"(xcr_index)
		);
	return low | (high << 32);
}

static inline void xsetbv(u32 xcr_index, u64 value)
{
	u64 low = value, high = (value >> 32);
	asm volatile("xsetbv\n"
				: :"c"(xcr_index), "a"(low), "d"(high)
		);
}

#endif