#include "GraphicsEngine.h"


const struct gf100_gr_pack
gk104_gr_pack_mmio[] = {
	{ gk104_gr_init_main_0 },
	{ gf100_gr_init_fe_0 },
	{ gf100_gr_init_pri_0 },
	{ gf100_gr_init_rstr2d_0 },
	{ gf119_gr_init_pd_0 },
	{ gk104_gr_init_ds_0 },
	{ gf100_gr_init_scc_0 },
	{ gk104_gr_init_sked_0 },
	{ gk104_gr_init_cwd_0 },
	{ gf119_gr_init_prop_0 },
	{ gf108_gr_init_gpc_unk_0 },
	{ gf100_gr_init_setup_0 },
	{ gf100_gr_init_crstr_0 },
	{ gf108_gr_init_setup_1 },
	{ gf100_gr_init_zcull_0 },
	{ gf119_gr_init_gpm_0 },
	{ gk104_gr_init_gpc_unk_1 },
	{ gf100_gr_init_gcc_0 },
	{ gk104_gr_init_tpccs_0 },
	{ gf119_gr_init_tex_0 },
	{ gk104_gr_init_pe_0 },
	{ gk104_gr_init_l1c_0 },
	{ gf100_gr_init_mpc_0 },
	{ gk104_gr_init_sm_0 },
	{ gf117_gr_init_pes_0 },
	{ gf117_gr_init_wwdx_0 },
	{ gf117_gr_init_cbm_0 },
	{ gk104_gr_init_be_0 },
	{ gf100_gr_init_fe_1 },
	{}
};
/*******************************************************************************
* PGRAPH engine/subdev functions
******************************************************************************/

void
gk104_gr_init_rop_active_fbps(struct gf100_gr *gr)
{
	struct nvkm_device *device = gr->device;
	const u32 fbp_count = nvkm_rd32(device, 0x120074);
	nvkm_mask(device, 0x408850, 0x0000000f, fbp_count); /* zrop */
	nvkm_mask(device, 0x408958, 0x0000000f, fbp_count); /* crop */
}

void
gk104_gr_init_ppc_exceptions(struct gf100_gr *gr)
{
	struct nvkm_device *device = gr->device;
	int gpc, ppc;

	for (gpc = 0; gpc < gr->gpc_nr; gpc++) {
		for (ppc = 0; ppc < gr->ppc_nr[gpc]; ppc++) {
			if (!(gr->ppc_mask[gpc] & (1 << ppc)))
				continue;
			nvkm_wr32(device, PPC_UNIT(gpc, ppc, 0x038), 0xc0000000);
		}
	}
}

int
gk104_gr_init(struct gf100_gr *gr)
{
	struct nvkm_device *device = gr->device;
	//struct nvkm_fb *fb = device->fb;
	NV_GPU_OBJ * FbRd;
	NV_GPU_OBJ * FbWr;
	NvGpuObjNew(device, 0x1000, 0x1000, &FbRd);
	NvGpuObjNew(device, 0x1000, 0x1000, &FbWr);
	
	const u32 magicgpc918 = DIV_ROUND_UP(0x00800000, gr->tpc_total);
	u32 data[TPC_MAX / 8] = {};
	u8  tpcnr[GPC_MAX];
	int gpc, tpc, rop;
	int i;

	nvkm_wr32(device, GPC_BCAST(0x0880), 0x00000000);
	nvkm_wr32(device, GPC_BCAST(0x08a4), 0x00000000);
	nvkm_wr32(device, GPC_BCAST(0x0888), 0x00000000);
	nvkm_wr32(device, GPC_BCAST(0x088c), 0x00000000);
	nvkm_wr32(device, GPC_BCAST(0x0890), 0x00000000);
	nvkm_wr32(device, GPC_BCAST(0x0894), 0x00000000);
	nvkm_wr32(device, GPC_BCAST(0x08b4), FbRd->Addr >> 8);
	nvkm_wr32(device, GPC_BCAST(0x08b8), FbWr->Addr >> 8);

	gf100_gr_mmio(gr, gr->func->mmio);

	nvkm_wr32(device, GPC_UNIT(0, 0x3018), 0x00000001);

	memset(data, 0x00, sizeof(data));
	memcpy(tpcnr, gr->tpc_nr, sizeof(gr->tpc_nr));
	for (i = 0, gpc = -1; i < gr->tpc_total; i++) {
		do {
			gpc = (gpc + 1) % gr->gpc_nr;
		} while (!tpcnr[gpc]);
		tpc = gr->tpc_nr[gpc] - tpcnr[gpc]--;

		data[i / 8] |= tpc << ((i % 8) * 4);
	}

	nvkm_wr32(device, GPC_BCAST(0x0980), data[0]);
	nvkm_wr32(device, GPC_BCAST(0x0984), data[1]);
	nvkm_wr32(device, GPC_BCAST(0x0988), data[2]);
	nvkm_wr32(device, GPC_BCAST(0x098c), data[3]);

	for (gpc = 0; gpc < gr->gpc_nr; gpc++) {
		nvkm_wr32(device, GPC_UNIT(gpc, 0x0914),
			gr->screen_tile_row_offset << 8 | gr->tpc_nr[gpc]);
		nvkm_wr32(device, GPC_UNIT(gpc, 0x0910), 0x00040000 |
			gr->tpc_total);
		nvkm_wr32(device, GPC_UNIT(gpc, 0x0918), magicgpc918);
	}

	nvkm_wr32(device, GPC_BCAST(0x3fd4), magicgpc918);
	nvkm_wr32(device, GPC_BCAST(0x08ac), nvkm_rd32(device, 0x100800));

	gr->func->init_rop_active_fbps(gr);

	nvkm_wr32(device, 0x400500, 0x00010001);

	nvkm_wr32(device, 0x400100, 0xffffffff);
	nvkm_wr32(device, 0x40013c, 0xffffffff);

	nvkm_wr32(device, 0x409ffc, 0x00000000);
	nvkm_wr32(device, 0x409c14, 0x00003e3e);
	nvkm_wr32(device, 0x409c24, 0x000f0001);
	nvkm_wr32(device, 0x404000, 0xc0000000);
	nvkm_wr32(device, 0x404600, 0xc0000000);
	nvkm_wr32(device, 0x408030, 0xc0000000);
	nvkm_wr32(device, 0x404490, 0xc0000000);
	nvkm_wr32(device, 0x406018, 0xc0000000);
	nvkm_wr32(device, 0x407020, 0x40000000);
	nvkm_wr32(device, 0x405840, 0xc0000000);
	nvkm_wr32(device, 0x405844, 0x00ffffff);
	nvkm_mask(device, 0x419cc0, 0x00000008, 0x00000008);
	nvkm_mask(device, 0x419eb4, 0x00001000, 0x00001000);

	gr->func->init_ppc_exceptions(gr);

	for (gpc = 0; gpc < gr->gpc_nr; gpc++) {
		nvkm_wr32(device, GPC_UNIT(gpc, 0x0420), 0xc0000000);
		nvkm_wr32(device, GPC_UNIT(gpc, 0x0900), 0xc0000000);
		nvkm_wr32(device, GPC_UNIT(gpc, 0x1028), 0xc0000000);
		nvkm_wr32(device, GPC_UNIT(gpc, 0x0824), 0xc0000000);
		for (tpc = 0; tpc < gr->tpc_nr[gpc]; tpc++) {
			nvkm_wr32(device, TPC_UNIT(gpc, tpc, 0x508), 0xffffffff);
			nvkm_wr32(device, TPC_UNIT(gpc, tpc, 0x50c), 0xffffffff);
			nvkm_wr32(device, TPC_UNIT(gpc, tpc, 0x224), 0xc0000000);
			nvkm_wr32(device, TPC_UNIT(gpc, tpc, 0x48c), 0xc0000000);
			nvkm_wr32(device, TPC_UNIT(gpc, tpc, 0x084), 0xc0000000);
			nvkm_wr32(device, TPC_UNIT(gpc, tpc, 0x644), 0x001ffffe);
			nvkm_wr32(device, TPC_UNIT(gpc, tpc, 0x64c), 0x0000000f);
		}
		nvkm_wr32(device, GPC_UNIT(gpc, 0x2c90), 0xffffffff);
		nvkm_wr32(device, GPC_UNIT(gpc, 0x2c94), 0xffffffff);
	}

	for (rop = 0; rop < gr->rop_nr; rop++) {
		nvkm_wr32(device, ROP_UNIT(rop, 0x144), 0xc0000000);
		nvkm_wr32(device, ROP_UNIT(rop, 0x070), 0xc0000000);
		nvkm_wr32(device, ROP_UNIT(rop, 0x204), 0xffffffff);
		nvkm_wr32(device, ROP_UNIT(rop, 0x208), 0xffffffff);
	}

	nvkm_wr32(device, 0x400108, 0xffffffff);
	nvkm_wr32(device, 0x400138, 0xffffffff);
	nvkm_wr32(device, 0x400118, 0xffffffff);
	nvkm_wr32(device, 0x400130, 0xffffffff);
	nvkm_wr32(device, 0x40011c, 0xffffffff);
	nvkm_wr32(device, 0x400134, 0xffffffff);

	nvkm_wr32(device, 0x400054, 0x34ce3464);

	//gf100_gr_zbc_init(gr);

	return gf100_gr_init_ctxctl(gr);
}

#include "fuc/hubgk104.fuc3.h"

static struct gf100_gr_ucode
gk104_gr_fecs_ucode = {
	.code.data = gk104_grhub_code,
	.code.size = sizeof(gk104_grhub_code),
	.data.data = gk104_grhub_data,
	.data.size = sizeof(gk104_grhub_data),
};

#include "fuc/gpcgk104.fuc3.h"

static struct gf100_gr_ucode
gk104_gr_gpccs_ucode = {
	.code.data = gk104_grgpc_code,
	.code.size = sizeof(gk104_grgpc_code),
	.data.data = gk104_grgpc_data,
	.data.size = sizeof(gk104_grgpc_data),
};

static const struct gf100_gr_func
gk104_gr = {
	.init = gk104_gr_init,
	.init_rop_active_fbps = gk104_gr_init_rop_active_fbps,
	.init_ppc_exceptions = gk104_gr_init_ppc_exceptions,
	.mmio = gk104_gr_pack_mmio,
	.fecs.ucode = &gk104_gr_fecs_ucode,
	.gpccs.ucode = &gk104_gr_gpccs_ucode,
	.rops = gf100_gr_rops,
	.ppc_nr = 1,
	.grctx = &gk104_grctx,
	/*
	.sclass = {
		{ -1, -1, FERMI_TWOD_A },
		{ -1, -1, KEPLER_INLINE_TO_MEMORY_A },
		{ -1, -1, KEPLER_A, &gf100_fermi },
		{ -1, -1, KEPLER_COMPUTE_A },
		{}
	}*/
};

#define min(a,b) ((a)<(b)?(a):(b))
/*******************************************************************************
* PGRAPH context implementation
******************************************************************************/

void
gk104_grctx_generate_bundle(struct gf100_grctx *info)
{
	const struct gf100_grctx_func *grctx = info->gr->func->grctx;
	const u32 state_limit = min(grctx->bundle_min_gpm_fifo_depth,
		grctx->bundle_size / 0x20);
	const u32 token_limit = grctx->bundle_token_limit;
	const u32 access = NV_MEM_ACCESS_RW | NV_MEM_ACCESS_SYS;
	const int s = 8;
	const int b = mmio_vram(info, grctx->bundle_size, (1 << s), access);
	mmio_refn(info, 0x408004, 0x00000000, s, b);
	mmio_wr32(info, 0x408008, 0x80000000 | (grctx->bundle_size >> s));
	mmio_refn(info, 0x418808, 0x00000000, s, b);
	mmio_wr32(info, 0x41880c, 0x80000000 | (grctx->bundle_size >> s));
	mmio_wr32(info, 0x4064c8, (state_limit << 16) | token_limit);
}

void
gk104_grctx_generate_pagepool(struct gf100_grctx *info)
{
	const struct gf100_grctx_func *grctx = info->gr->func->grctx;
	const u32 access = NV_MEM_ACCESS_RW | NV_MEM_ACCESS_SYS;
	const int s = 8;
	const int b = mmio_vram(info, grctx->pagepool_size, (1 << s), access);
	mmio_refn(info, 0x40800c, 0x00000000, s, b);
	mmio_wr32(info, 0x408010, 0x80000000);
	mmio_refn(info, 0x419004, 0x00000000, s, b);
	mmio_wr32(info, 0x419008, 0x00000000);
	mmio_wr32(info, 0x4064cc, 0x80000000);
}

void
gk104_grctx_generate_unkn(struct gf100_gr *gr)
{
	struct nvkm_device *device = gr->device;
	nvkm_mask(device, 0x418c6c, 0x00000001, 0x00000001);
	nvkm_mask(device, 0x41980c, 0x00000010, 0x00000010);
	nvkm_mask(device, 0x41be08, 0x00000004, 0x00000004);
	nvkm_mask(device, 0x4064c0, 0x80000000, 0x80000000);
	nvkm_mask(device, 0x405800, 0x08000000, 0x08000000);
	nvkm_mask(device, 0x419c00, 0x00000008, 0x00000008);
}

void
gk104_grctx_generate_r418bb8(struct gf100_gr *gr)
{
	struct nvkm_device *device = gr->device;
	u32 data[6] = {}, data2[2] = {};
	u8  tpcnr[GPC_MAX];
	u8  shift, ntpcv;
	int gpc, tpc, i;

	/* calculate first set of magics */
	memcpy(tpcnr, gr->tpc_nr, sizeof(gr->tpc_nr));

	gpc = -1;
	for (tpc = 0; tpc < gr->tpc_total; tpc++) {
		do {
			gpc = (gpc + 1) % gr->gpc_nr;
		} while (!tpcnr[gpc]);
		tpcnr[gpc]--;

		data[tpc / 6] |= gpc << ((tpc % 6) * 5);
	}

	for (; tpc < 32; tpc++)
		data[tpc / 6] |= 7 << ((tpc % 6) * 5);

	/* and the second... */
	shift = 0;
	ntpcv = gr->tpc_total;
	while (!(ntpcv & (1 << 4))) {
		ntpcv <<= 1;
		shift++;
	}

	data2[0] = (ntpcv << 16);
	data2[0] |= (shift << 21);
	data2[0] |= (((1 << (0 + 5)) % ntpcv) << 24);
	for (i = 1; i < 7; i++)
		data2[1] |= ((1 << (i + 5)) % ntpcv) << ((i - 1) * 5);

	/* GPC_BROADCAST */
	nvkm_wr32(device, 0x418bb8, (gr->tpc_total << 8) |
		gr->screen_tile_row_offset);
	for (i = 0; i < 6; i++)
		nvkm_wr32(device, 0x418b08 + (i * 4), data[i]);

	/* GPC_BROADCAST.TP_BROADCAST */
	nvkm_wr32(device, 0x41bfd0, (gr->tpc_total << 8) |
		gr->screen_tile_row_offset | data2[0]);
	nvkm_wr32(device, 0x41bfe4, data2[1]);
	for (i = 0; i < 6; i++)
		nvkm_wr32(device, 0x41bf00 + (i * 4), data[i]);

	/* UNK78xx */
	nvkm_wr32(device, 0x4078bc, (gr->tpc_total << 8) |
		gr->screen_tile_row_offset);
	for (i = 0; i < 6; i++)
		nvkm_wr32(device, 0x40780c + (i * 4), data[i]);
}

void
gk104_grctx_generate_main(struct gf100_gr *gr, struct gf100_grctx *info)
{
	struct nvkm_device *device = gr->device;
	const struct gf100_grctx_func *grctx = gr->func->grctx;
	u32 idle_timeout;
	int i;

	nvkm_mc_unk260(device, 0);

	gf100_gr_mmio(gr, grctx->hub);
	gf100_gr_mmio(gr, grctx->gpc);
	gf100_gr_mmio(gr, grctx->zcull);
	gf100_gr_mmio(gr, grctx->tpc);
	gf100_gr_mmio(gr, grctx->ppc);

	idle_timeout = nvkm_mask(device, 0x404154, 0xffffffff, 0x00000000);

	grctx->bundle(info);
	grctx->pagepool(info);
	grctx->attrib(info);
	grctx->unkn(gr);

	gf100_grctx_generate_tpcid(gr);
	gf100_grctx_generate_r406028(gr);
	gk104_grctx_generate_r418bb8(gr);
	gf100_grctx_generate_r406800(gr);

	for (i = 0; i < 8; i++)
		nvkm_wr32(device, 0x4064d0 + (i * 0x04), 0x00000000);

	nvkm_wr32(device, 0x405b00, (gr->tpc_total << 8) | gr->gpc_nr);
	nvkm_mask(device, 0x419f78, 0x00000001, 0x00000000);

	

	gf100_gr_icmd(gr, grctx->icmd);
	nvkm_wr32(device, 0x404154, idle_timeout);
	gf100_gr_mthd(gr, grctx->mthd);

	nvkm_mc_unk260(device, 1);

	nvkm_mask(device, 0x418800, 0x00200000, 0x00200000);
	nvkm_mask(device, 0x41be10, 0x00800000, 0x00800000);
}


void
gf117_grctx_generate_attrib(struct gf100_grctx *info)
{
	struct gf100_gr *gr = info->gr;
	const struct gf100_grctx_func *grctx = gr->func->grctx;
	const u32  alpha = grctx->alpha_nr;
	const u32   beta = grctx->attrib_nr;
	const u32   size = 0x20 * (grctx->attrib_nr_max + grctx->alpha_nr_max);
	const u32 access = NV_MEM_ACCESS_RW;
	const int s = 12;
	const int b = mmio_vram(info, size * gr->tpc_total, (1 << s), access);
	const int timeslice_mode = 1;
	const int max_batches = 0xffff;
	u32 bo = 0;
	u32 ao = bo + grctx->attrib_nr_max * gr->tpc_total;
	int gpc, ppc;

	mmio_refn(info, 0x418810, 0x80000000, s, b);
	mmio_refn(info, 0x419848, 0x10000000, s, b);
	mmio_wr32(info, 0x405830, (beta << 16) | alpha);
	mmio_wr32(info, 0x4064c4, ((alpha / 4) << 16) | max_batches);

	for (gpc = 0; gpc < gr->gpc_nr; gpc++) {
		for (ppc = 0; ppc < gr->ppc_nr[gpc]; ppc++) {
			const u32 a = alpha * gr->ppc_tpc_nr[gpc][ppc];
			const u32 b = beta * gr->ppc_tpc_nr[gpc][ppc];
			const u32 t = timeslice_mode;
			const u32 o = PPC_UNIT(gpc, ppc, 0);
			if (!(gr->ppc_mask[gpc] & (1 << ppc)))
				continue;
			mmio_skip(info, o + 0xc0, (t << 28) | (b << 16) | ++bo);
			mmio_wr32(info, o + 0xc0, (t << 28) | (b << 16) | --bo);
			bo += grctx->attrib_nr_max * gr->ppc_tpc_nr[gpc][ppc];
			mmio_wr32(info, o + 0xe4, (a << 16) | ao);
			ao += grctx->alpha_nr_max * gr->ppc_tpc_nr[gpc][ppc];
		}
	}
}
const struct gf100_grctx_func
gk104_grctx = {
	.main = gk104_grctx_generate_main,
	.unkn = gk104_grctx_generate_unkn,
	.hub = gk104_grctx_pack_hub,
	.gpc = gk104_grctx_pack_gpc,
	.zcull = gf100_grctx_pack_zcull,
	.tpc = gk104_grctx_pack_tpc,
	.ppc = gk104_grctx_pack_ppc,
	.icmd = gk104_grctx_pack_icmd,
	.mthd = gk104_grctx_pack_mthd,
	.bundle = gk104_grctx_generate_bundle,
	.bundle_size = 0x3000,
	.bundle_min_gpm_fifo_depth = 0x180,
	.bundle_token_limit = 0x600,
	.pagepool = gk104_grctx_generate_pagepool,
	.pagepool_size = 0x8000,
	.attrib = gf117_grctx_generate_attrib,
	.attrib_nr_max = 0x324,
	.attrib_nr = 0x218,
	.alpha_nr_max = 0x7ff,
	.alpha_nr = 0x648,
};