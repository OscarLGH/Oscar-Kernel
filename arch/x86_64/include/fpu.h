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

struct x86_fpu_region {
	u16 fcw;
	u16 fsw;
	u8 ftw;
	u8 reserved;
	u16 fop;
	u32 fip_l;
	u16 fcs;
	u16 fip_h;

	u32 fdp;
	u16 fds;
	u16 fdp_h;
	u32 mxcsr;
	u32 mxcsr_mask;

	u8 mm_reg[8][16];
	u8 xmm_reg[16][16];

	u8 padding0[96];
	
	/* 512 */
	u8 xstate_bv[8];
	u8 xcomp_bv[8];
	u8 padding1[48];
	
};

static inline u64 xgetbv(u32 xcr_index)
{
	u64 low, high;
	asm volatile("xgetbv\n\t"
				: "=a"(low), "=d"(high) : "c"(xcr_index)
		);
	return low | (high << 32);
}

static inline void xsetbv(u32 xcr_index, u64 value)
{
	u64 low = value, high = (value >> 32);
	asm volatile("xsetbv\n\t"
				: :"c"(xcr_index), "a"(low), "d"(high)
		);
}

static inline void xsave(struct x86_fpu_region *addr, u64 mask)
{
	u64 low = mask, high = (mask >> 32);
	asm volatile("xsave (%%rdi)\n\t"
			: :"rdi"(addr), "a"(low), "d"(high)
		);
}

static inline void xrstor(struct x86_fpu_region *addr, u64 mask)
{
	u64 low = mask, high = (mask >> 32);
	asm volatile("xrstor (%%rdi)\n\t"
			: :"rdi"(addr), "a"(low), "d"(high)
		);
}


static inline void xcr0_set_bits(u64 bits)
{
	u64 xcr0;
	xcr0 = xgetbv(0);
	xcr0 |= bits;
	xsetbv(0, xcr0);
}


static inline void xcr0_clear_bits(u64 bits)
{
	u64 xcr0;
	xcr0 = xgetbv(0);
	xcr0 &= ~bits;
	xsetbv(0, xcr0);
}
#endif
