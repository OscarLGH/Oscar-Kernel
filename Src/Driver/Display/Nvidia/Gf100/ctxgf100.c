/*
 * Copyright 2010 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Ben Skeggs
 */

#include "ctxgf100.h"

/*******************************************************************************
 * PGRAPH context implementation
 ******************************************************************************/

int
gf100_grctx_mmio_data(struct gf100_grctx *info, u32 size, u32 align, u32 access)
{
	if (info->data) {
		info->buffer[info->buffer_nr] = RoundUp(info->addr, align);
		info->addr = info->buffer[info->buffer_nr] + size;
		info->data->size = size;
		info->data->align = align;
		info->data->access = access;
		info->data++;
		return info->buffer_nr++;
	}
	return -1;
}

void
gf100_grctx_mmio_item(struct gf100_grctx *info, u32 addr, u32 data,
		      int shift, int buffer)
{
	struct nvkm_device *device = info->gr->device;
	if (info->data) {
		if (shift >= 0) {
			info->mmio->addr = addr;
			info->mmio->data = data;
			info->mmio->shift = shift;
			info->mmio->buffer = buffer;
			if (buffer >= 0)
				data |= info->buffer[buffer] >> shift;
			info->mmio++;
		} else
			return;
	} else {
		if (buffer >= 0)
			return;
	}

	nvkm_wr32(device, addr, data);
}

void
gf100_grctx_generate_bundle(struct gf100_grctx *info)
{
	const struct gf100_grctx_func *grctx = info->gr->func->grctx;
	const u32 access = NV_MEM_ACCESS_RW | NV_MEM_ACCESS_SYS;
	const int s = 8;
	const int b = mmio_vram(info, grctx->bundle_size, (1 << s), access);
	mmio_refn(info, 0x408004, 0x00000000, s, b);
	mmio_wr32(info, 0x408008, 0x80000000 | (grctx->bundle_size >> s));
	mmio_refn(info, 0x418808, 0x00000000, s, b);
	mmio_wr32(info, 0x41880c, 0x80000000 | (grctx->bundle_size >> s));
}

void
gf100_grctx_generate_pagepool(struct gf100_grctx *info)
{
	const struct gf100_grctx_func *grctx = info->gr->func->grctx;
	const u32 access = NV_MEM_ACCESS_RW | NV_MEM_ACCESS_SYS;
	const int s = 8;
	const int b = mmio_vram(info, grctx->pagepool_size, (1 << s), access);
	mmio_refn(info, 0x40800c, 0x00000000, s, b);
	mmio_wr32(info, 0x408010, 0x80000000);
	mmio_refn(info, 0x419004, 0x00000000, s, b);
	mmio_wr32(info, 0x419008, 0x00000000);
}

void
gf100_grctx_generate_attrib(struct gf100_grctx *info)
{
	struct gf100_gr *gr = info->gr;
	const struct gf100_grctx_func *grctx = gr->func->grctx;
	const u32 attrib = grctx->attrib_nr;
	const u32   size = 0x20 * (grctx->attrib_nr_max + grctx->alpha_nr_max);
	const u32 access = NV_MEM_ACCESS_RW;
	const int s = 12;
	const int b = mmio_vram(info, size * gr->tpc_total, (1 << s), access);
	int gpc, tpc;
	u32 bo = 0;

	mmio_refn(info, 0x418810, 0x80000000, s, b);
	mmio_refn(info, 0x419848, 0x10000000, s, b);
	mmio_wr32(info, 0x405830, (attrib << 16));

	for (gpc = 0; gpc < gr->gpc_nr; gpc++) {
		for (tpc = 0; tpc < gr->tpc_nr[gpc]; tpc++) {
			const u32 o = TPC_UNIT(gpc, tpc, 0x0520);
			mmio_skip(info, o, (attrib << 16) | ++bo);
			mmio_wr32(info, o, (attrib << 16) | --bo);
			bo += grctx->attrib_nr_max;
		}
	}
}

void
gf100_grctx_generate_unkn(struct gf100_gr *gr)
{
}

void
gf100_grctx_generate_tpcid(struct gf100_gr *gr)
{
	struct nvkm_device *device = gr->device;
	int gpc, tpc, id;

	for (tpc = 0, id = 0; tpc < 4; tpc++) {
		for (gpc = 0; gpc < gr->gpc_nr; gpc++) {
			if (tpc < gr->tpc_nr[gpc]) {
				nvkm_wr32(device, TPC_UNIT(gpc, tpc, 0x698), id);
				nvkm_wr32(device, TPC_UNIT(gpc, tpc, 0x4e8), id);
				nvkm_wr32(device, GPC_UNIT(gpc, 0x0c10 + tpc * 4), id);
				nvkm_wr32(device, TPC_UNIT(gpc, tpc, 0x088), id);
				id++;
			}

			nvkm_wr32(device, GPC_UNIT(gpc, 0x0c08), gr->tpc_nr[gpc]);
			nvkm_wr32(device, GPC_UNIT(gpc, 0x0c8c), gr->tpc_nr[gpc]);
		}
	}
}

void
gf100_grctx_generate_r406028(struct gf100_gr *gr)
{
	struct nvkm_device *device = gr->device;
	u32 tmp[GPC_MAX / 8] = {}, i = 0;
	for (i = 0; i < gr->gpc_nr; i++)
		tmp[i / 8] |= gr->tpc_nr[i] << ((i % 8) * 4);
	for (i = 0; i < 4; i++) {
		nvkm_wr32(device, 0x406028 + (i * 4), tmp[i]);
		nvkm_wr32(device, 0x405870 + (i * 4), tmp[i]);
	}
}

void
gf100_grctx_generate_r4060a8(struct gf100_gr *gr)
{
	struct nvkm_device *device = gr->device;
	u8  tpcnr[GPC_MAX], data[TPC_MAX];
	int gpc, tpc, i;

	memcpy(tpcnr, gr->tpc_nr, sizeof(gr->tpc_nr));
	memset(data, 0x1f, sizeof(data));

	gpc = -1;
	for (tpc = 0; tpc < gr->tpc_total; tpc++) {
		do {
			gpc = (gpc + 1) % gr->gpc_nr;
		} while (!tpcnr[gpc]);
		tpcnr[gpc]--;
		data[tpc] = gpc;
	}

	for (i = 0; i < 4; i++)
		nvkm_wr32(device, 0x4060a8 + (i * 4), ((u32 *)data)[i]);
}

void
gf100_grctx_generate_r418bb8(struct gf100_gr *gr)
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

	data2[0]  = (ntpcv << 16);
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
	nvkm_wr32(device, 0x419bd0, (gr->tpc_total << 8) |
				     gr->screen_tile_row_offset | data2[0]);
	nvkm_wr32(device, 0x419be4, data2[1]);
	for (i = 0; i < 6; i++)
		nvkm_wr32(device, 0x419b00 + (i * 4), data[i]);

	/* UNK78xx */
	nvkm_wr32(device, 0x4078bc, (gr->tpc_total << 8) |
				     gr->screen_tile_row_offset);
	for (i = 0; i < 6; i++)
		nvkm_wr32(device, 0x40780c + (i * 4), data[i]);
}

void
gf100_grctx_generate_r406800(struct gf100_gr *gr)
{
	struct nvkm_device *device = gr->device;
	u64 tpc_mask = 0, tpc_set = 0;
	u8  tpcnr[GPC_MAX];
	int gpc, tpc;
	int i, a, b;

	memcpy(tpcnr, gr->tpc_nr, sizeof(gr->tpc_nr));
	for (gpc = 0; gpc < gr->gpc_nr; gpc++)
		tpc_mask |= ((1ULL << gr->tpc_nr[gpc]) - 1) << (gpc * 8);

	for (i = 0, gpc = -1, b = -1; i < 32; i++) {
		a = (i * (gr->tpc_total - 1)) / 32;
		if (a != b) {
			b = a;
			do {
				gpc = (gpc + 1) % gr->gpc_nr;
			} while (!tpcnr[gpc]);
			tpc = gr->tpc_nr[gpc] - tpcnr[gpc]--;

			tpc_set |= 1ULL << ((gpc * 8) + tpc);
		}

		nvkm_wr32(device, 0x406800 + (i * 0x20), lower_32_bits(tpc_set));
		nvkm_wr32(device, 0x406c00 + (i * 0x20), lower_32_bits(tpc_set ^ tpc_mask));
		if (gr->gpc_nr > 4) {
			nvkm_wr32(device, 0x406804 + (i * 0x20), upper_32_bits(tpc_set));
			nvkm_wr32(device, 0x406c04 + (i * 0x20), upper_32_bits(tpc_set ^ tpc_mask));
		}
	}
}

void
gf100_grctx_generate_main(struct gf100_gr *gr, struct gf100_grctx *info)
{
	struct nvkm_device *device = gr->device;
	const struct gf100_grctx_func *grctx = gr->func->grctx;
	u32 idle_timeout;

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
	gf100_grctx_generate_r4060a8(gr);
	gf100_grctx_generate_r418bb8(gr);
	gf100_grctx_generate_r406800(gr);

	gf100_gr_icmd(gr, grctx->icmd);
	nvkm_wr32(device, 0x404154, idle_timeout);
	gf100_gr_mthd(gr, grctx->mthd);
	nvkm_mc_unk260(device, 1);
}

int
gf100_grctx_generate(struct gf100_gr *gr)
{
	const struct gf100_grctx_func *grctx = gr->func->grctx;
	//struct nvkm_subdev *subdev = &gr->base.engine.subdev;
	struct nvkm_device *device = gr->device;
	
	//struct nvkm_memory *chan;
	NV_GPU_OBJ * chan;

	struct gf100_grctx info;
	int ret, i;
	u64 addr;

	/* allocate memory to for a "channel", which we'll use to generate
	 * the default context values
	 */
	//ret = nvkm_memory_new(device, NVKM_MEM_TARGET_INST, 0x80000 + gr->size,
	//		      0x1000, true, &chan);
	//if (ret) {
	//	nvkm_error(subdev, "failed to allocate chan memory, %d\n", ret);
	//	return ret;
	//}
	//printk("gr->size = %x\n", gr->size);
	NvGpuObjNew(device, 0x80000 + gr->size, 0x1000, &chan);
	//printk("chan->Size = %x\n", chan->Size);
	addr = chan->Addr;

	/* PGD pointer */
	//nvkm_kmap(chan);
	GpuObjWr32(chan, 0x0200, lower_32_bits(addr + 0x1000));
	GpuObjWr32(chan, 0x0204, upper_32_bits(addr + 0x1000));
	GpuObjWr32(chan, 0x0208, 0xffffffff);
	GpuObjWr32(chan, 0x020c, 0x000000ff);

	/* PGT[0] pointer */
	GpuObjWr32(chan, 0x1000, 0x00000000);
	GpuObjWr32(chan, 0x1004, 0x00000001 | (addr + 0x2000) >> 8);

	/* identity-map the whole "channel" into its own vm */
	for (i = 0; i < chan->Size / 4096; i++) {
		u64 addr = ((chan->Addr + (i * 4096)) >> 8) | 1;
		GpuObjWr32(chan, 0x2000 + (i * 8), lower_32_bits(addr));
		GpuObjWr32(chan, 0x2004 + (i * 8), upper_32_bits(addr));
	}

	/* context pointer (virt) */
	GpuObjWr32(chan, 0x0210, 0x00080004);
	GpuObjWr32(chan, 0x0214, 0x00000000);
	//nvkm_done(chan);

	nvkm_wr32(device, 0x100cb8, (addr + 0x1000) >> 8);
	nvkm_wr32(device, 0x100cbc, 0x80000001);
	nvkm_msec(device, 2000,
		if (nvkm_rd32(device, 0x100c80) & 0x00008000)
			break;
	);

	/* setup default state for mmio list construction */
	info.gr = gr;
	info.data = gr->mmio_data;
	info.mmio = gr->mmio_list;
	info.addr = 0x2000 + (i * 8);
	info.buffer_nr = 0;

	/* make channel current */
	if (gr->firmware) {
		nvkm_wr32(device, 0x409840, 0x00000030);
		nvkm_wr32(device, 0x409500, 0x80000000 | addr >> 12);
		nvkm_wr32(device, 0x409504, 0x00000003);
		nvkm_msec(device, 2000,
			if (nvkm_rd32(device, 0x409800) & 0x00000010)
				break;
		);

		//nvkm_kmap(chan);
		GpuObjWr32(chan, 0x8001c, 1);
		GpuObjWr32(chan, 0x80020, 0);
		GpuObjWr32(chan, 0x80028, 0);
		GpuObjWr32(chan, 0x8002c, 0);
		//nvkm_done(chan);
	} else {
		nvkm_wr32(device, 0x409840, 0x80000000);
		nvkm_wr32(device, 0x409500, 0x80000000 | addr >> 12);
		nvkm_wr32(device, 0x409504, 0x00000001);
		nvkm_msec(device, 2000,
			if (nvkm_rd32(device, 0x409800) & 0x80000000)
				break;
		);
	}
	
	grctx->main(gr, &info);
	
	/* trigger a context unload by unsetting the "next channel valid" bit
	 * and faking a context switch interrupt
	 */
	nvkm_mask(device, 0x409b04, 0x80000000, 0x00000000);
	nvkm_wr32(device, 0x409000, 0x00000100);

	if (nvkm_msec(device, 2000,
		if (!(nvkm_rd32(device, 0x409b00) & 0x80000000))
			break;
	) < 0) {
		ret = -EBUSY;
		goto done;
	}

	gr->data = KMalloc(gr->size);
	if (gr->data) {
		//nvkm_kmap(chan);
		for (i = 0; i < gr->size; i += 4)
			gr->data[i / 4] = GpuObjRd32(chan, 0x80000 + i);
		//nvkm_done(chan);
		ret = 0;
	} else {
		ret = -ENOMEM;
	}

done:
	//nvkm_memory_del(&chan);
	return ret;
}

const struct gf100_grctx_func
gf100_grctx = {
	.main  = gf100_grctx_generate_main,
	.unkn  = gf100_grctx_generate_unkn,
	.hub   = gf100_grctx_pack_hub,
	.gpc   = gf100_grctx_pack_gpc,
	.zcull = gf100_grctx_pack_zcull,
	.tpc   = gf100_grctx_pack_tpc,
	.icmd  = gf100_grctx_pack_icmd,
	.mthd  = gf100_grctx_pack_mthd,
	.bundle = gf100_grctx_generate_bundle,
	.bundle_size = 0x1800,
	.pagepool = gf100_grctx_generate_pagepool,
	.pagepool_size = 0x8000,
	.attrib = gf100_grctx_generate_attrib,
	.attrib_nr_max = 0x324,
	.attrib_nr = 0x218,
};
