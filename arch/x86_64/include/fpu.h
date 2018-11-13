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

struct fpu_legacy_region {
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

	u128 mm_reg[8];
	u128 xmm_reg[16];
	u8 padding[96];
};

struct xsave_header {
	u64 xstate_bv;
	u64 xcomp_bv;
	u64 reserved[6];
};

struct avx_state {
	u128 ymm[16];
};

struct mpx_state {
	u8 place_holder[128];
};

struct avx512_opmask {
	u64 kreg[8];
};

/*ZMM0H - ZMM15H, upper 256 bits */
struct avx512_zmm_hi256 {
	u256 zmm_hi256[16];
};

/*ZMM16 - ZMM31, 512 bits */
struct avx512_hi16_zmm {
	u512 hi_zmm[16];
};

struct avx512_state {
	struct avx512_opmask opmask;
	struct avx512_zmm_hi256 zmm_hi256;
	struct avx512_hi16_zmm hi16_zmm;
};

struct xsave_area {
	/* 0 */
	struct fpu_legacy_region legacy_region;
	/* 512 */
	struct xsave_header xsave_header;
	/*576 Extended state*/
	struct avx_state avx_state;

	u8 padding[128];
	/* 960 */
	struct mpx_state mpx_state;
	/* 1088 */
	struct avx512_state avx512_state;
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

static inline void xsave(struct xsave_area *addr, u64 mask)
{
	u64 low = mask, high = (mask >> 32);
	asm volatile("xsave (%%rdi)\n\t"
			: :"rdi"(addr), "a"(low), "d"(high)
		);
}

static inline void xrstor(struct xsave_area *addr, u64 mask)
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
