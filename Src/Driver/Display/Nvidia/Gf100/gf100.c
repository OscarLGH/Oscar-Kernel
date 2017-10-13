/*
 * Copyright 2012 Red Hat Inc.
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
#include "../Common/GraphicsInitLib.h"
#include "../Common/GraphicsContextLib.h"


/*******************************************************************************
 * Zero Bandwidth Clear
 ******************************************************************************/

static void
gf100_gr_zbc_clear_color(struct gf100_gr *gr, int zbc)
{
	struct nvkm_device *device = gr->device;
	if (gr->zbc_color[zbc].format) {
		nvkm_wr32(device, 0x405804, gr->zbc_color[zbc].ds[0]);
		nvkm_wr32(device, 0x405808, gr->zbc_color[zbc].ds[1]);
		nvkm_wr32(device, 0x40580c, gr->zbc_color[zbc].ds[2]);
		nvkm_wr32(device, 0x405810, gr->zbc_color[zbc].ds[3]);
	}
	nvkm_wr32(device, 0x405814, gr->zbc_color[zbc].format);
	nvkm_wr32(device, 0x405820, zbc);
	nvkm_wr32(device, 0x405824, 0x00000004); /* TRIGGER | WRITE | COLOR */
}
/*
static int
gf100_gr_zbc_color_get(struct gf100_gr *gr, int format,
		       const u32 ds[4], const u32 l2[4])
{
	struct nvkm_ltc *ltc = gr->base.engine.subdev.device->ltc;
	int zbc = -ENOSPC, i;

	for (i = ltc->zbc_min; i <= ltc->zbc_max; i++) {
		if (gr->zbc_color[i].format) {
			if (gr->zbc_color[i].format != format)
				continue;
			if (memcmp(gr->zbc_color[i].ds, ds, sizeof(
				   gr->zbc_color[i].ds)))
				continue;
			if (memcmp(gr->zbc_color[i].l2, l2, sizeof(
				   gr->zbc_color[i].l2))) {
				WARN_ON(1);
				return -EINVAL;
			}
			return i;
		} else {
			zbc = (zbc < 0) ? i : zbc;
		}
	}

	if (zbc < 0)
		return zbc;

	memcpy(gr->zbc_color[zbc].ds, ds, sizeof(gr->zbc_color[zbc].ds));
	memcpy(gr->zbc_color[zbc].l2, l2, sizeof(gr->zbc_color[zbc].l2));
	gr->zbc_color[zbc].format = format;
	nvkm_ltc_zbc_color_get(ltc, zbc, l2);
	gf100_gr_zbc_clear_color(gr, zbc);
	return zbc;
}
*/
static void
gf100_gr_zbc_clear_depth(struct gf100_gr *gr, int zbc)
{
	struct nvkm_device *device = gr->device;
	if (gr->zbc_depth[zbc].format)
		nvkm_wr32(device, 0x405818, gr->zbc_depth[zbc].ds);
	nvkm_wr32(device, 0x40581c, gr->zbc_depth[zbc].format);
	nvkm_wr32(device, 0x405820, zbc);
	nvkm_wr32(device, 0x405824, 0x00000005); /* TRIGGER | WRITE | DEPTH */
}
/*
static int
gf100_gr_zbc_depth_get(struct gf100_gr *gr, int format,
		       const u32 ds, const u32 l2)
{
	struct nvkm_ltc *ltc = gr->base.engine.subdev.device->ltc;
	int zbc = -ENOSPC, i;

	for (i = ltc->zbc_min; i <= ltc->zbc_max; i++) {
		if (gr->zbc_depth[i].format) {
			if (gr->zbc_depth[i].format != format)
				continue;
			if (gr->zbc_depth[i].ds != ds)
				continue;
			if (gr->zbc_depth[i].l2 != l2) {
				WARN_ON(1);
				return -EINVAL;
			}
			return i;
		} else {
			zbc = (zbc < 0) ? i : zbc;
		}
	}

	if (zbc < 0)
		return zbc;

	gr->zbc_depth[zbc].format = format;
	gr->zbc_depth[zbc].ds = ds;
	gr->zbc_depth[zbc].l2 = l2;
	nvkm_ltc_zbc_depth_get(ltc, zbc, l2);
	gf100_gr_zbc_clear_depth(gr, zbc);
	return zbc;
}
*/
/*******************************************************************************
 * Graphics object classes
 ******************************************************************************/
#define gf100_gr_object(p) container_of((p), struct gf100_gr_object, object)


static void
gf100_gr_mthd_set_shader_exceptions(NVIDIA_GPU *device, u32 data)
{
	nvkm_wr32(device, 0x419e44, data ? 0xffffffff : 0x00000000);
	nvkm_wr32(device, 0x419e4c, data ? 0xffffffff : 0x00000000);
}

static bool
gf100_gr_mthd_sw(NVIDIA_GPU *device, u16 class, u32 mthd, u32 data)
{
	switch (class & 0x00ff) {
	case 0x97:
	case 0xc0:
		switch (mthd) {
		case 0x1528:
			gf100_gr_mthd_set_shader_exceptions(device, data);
			return TRUE;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return FALSE;
}

/*******************************************************************************
 * PGRAPH engine/subdev functions
 ******************************************************************************/

int
gf100_gr_rops(struct gf100_gr *gr)
{
	struct nvkm_device * device = gr->device;
	return (nvkm_rd32(device, 0x409604) & 0x001f0000) >> 16;
}

/*
void
gf100_gr_zbc_init(struct gf100_gr *gr)
{
	const u32  zero[] = { 0x00000000, 0x00000000, 0x00000000, 0x00000000,
			      0x00000000, 0x00000000, 0x00000000, 0x00000000 };
	const u32   one[] = { 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000,
			      0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff };
	const u32 f32_0[] = { 0x00000000, 0x00000000, 0x00000000, 0x00000000,
			      0x00000000, 0x00000000, 0x00000000, 0x00000000 };
	const u32 f32_1[] = { 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000,
			      0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000 };
	struct nvkm_ltc *ltc = gr->base.engine.subdev.device->ltc;
	int index;

	if (!gr->zbc_color[0].format) {
		gf100_gr_zbc_color_get(gr, 1,  & zero[0],   &zero[4]);
		gf100_gr_zbc_color_get(gr, 2,  &  one[0],    &one[4]);
		gf100_gr_zbc_color_get(gr, 4,  &f32_0[0],  &f32_0[4]);
		gf100_gr_zbc_color_get(gr, 4,  &f32_1[0],  &f32_1[4]);
		gf100_gr_zbc_depth_get(gr, 1, 0x00000000, 0x00000000);
		gf100_gr_zbc_depth_get(gr, 1, 0x3f800000, 0x3f800000);
	}

	for (index = ltc->zbc_min; index <= ltc->zbc_max; index++)
		gf100_gr_zbc_clear_color(gr, index);
	for (index = ltc->zbc_min; index <= ltc->zbc_max; index++)
		gf100_gr_zbc_clear_depth(gr, index);
}
*/


/**
 * Wait until GR goes idle. GR is considered idle if it is disabled by the
 * MC (0x200) register, or GR is not busy and a context switch is not in
 * progress.
 */
int
gf100_gr_wait_idle(struct gf100_gr *gr)
{
	//struct nvkm_subdev *subdev = &gr->base.engine.subdev;
	struct nvkm_device * device = gr->device;
	//unsigned long end_jiffies = jiffies + msecs_to_jiffies(2000);
	bool gr_enabled, ctxsw_active, gr_busy;

	do {
		/*
		 * required to make sure FIFO_ENGINE_STATUS (0x2640) is
		 * up-to-date
		 */
		nvkm_rd32(device, 0x400700);

		gr_enabled = nvkm_rd32(device, 0x200) & 0x1000;
		ctxsw_active = nvkm_rd32(device, 0x2640) & 0x8000;
		gr_busy = nvkm_rd32(device, 0x40060c) & 0x1;

		if (!gr_enabled || (!gr_busy && !ctxsw_active))
			return 0;
	} while (1);

	//nvkm_error(subdev,
	//	   "wait for idle timeout (en: %d, ctxsw: %d, busy: %d)\n",
	//	   gr_enabled, ctxsw_active, gr_busy);
	return -EAGAIN;
}

void
gf100_gr_mmio(struct gf100_gr *gr, const struct gf100_gr_pack *p)
{
	struct nvkm_device * device = gr->device;
	const struct gf100_gr_pack *pack;
	const struct gf100_gr_init *init;

	pack_for_each_init(init, pack, p) {
		u32 next = init->addr + init->count * init->pitch;
		u32 addr = init->addr;
		while (addr < next) {
			nvkm_wr32(device, addr, init->data);
			addr += init->pitch;
		}
	}
}

void
gf100_gr_icmd(struct gf100_gr *gr, const struct gf100_gr_pack *p)
{
	struct nvkm_device * device = gr->device;
	const struct gf100_gr_pack *pack;
	const struct gf100_gr_init *init;
	u32 data = 0;

	nvkm_wr32(device, 0x400208, 0x80000000);

	pack_for_each_init(init, pack, p) {
		u32 next = init->addr + init->count * init->pitch;
		u32 addr = init->addr;

		if ((pack == p && init == p->init) || data != init->data) {
			nvkm_wr32(device, 0x400204, init->data);
			data = init->data;
		}

		while (addr < next) {
			nvkm_wr32(device, 0x400200, addr);
			/**
			 * Wait for GR to go idle after submitting a
			 * GO_IDLE bundle
			 */
			if ((addr & 0xffff) == 0xe100)
				gf100_gr_wait_idle(gr);
			nvkm_msec(device, 2000,
				if (!(nvkm_rd32(device, 0x400700) & 0x00000004))
					break;
			);
			addr += init->pitch;
		}
	}

	nvkm_wr32(device, 0x400208, 0x00000000);
}


void
gf100_gr_mthd(struct gf100_gr *gr, const struct gf100_gr_pack *p)
{
	struct nvkm_device *device = gr->device;
	const struct gf100_gr_pack *pack;
	const struct gf100_gr_init *init;
	u32 data = 0;

	pack_for_each_init(init, pack, p) {
		u32 ctrl = 0x80000000 | pack->type;
		u32 next = init->addr + init->count * init->pitch;
		u32 addr = init->addr;

		if ((pack == p && init == p->init) || data != init->data) {
			nvkm_wr32(device, 0x40448c, init->data);
			data = init->data;
		}

		while (addr < next) {
			nvkm_wr32(device, 0x404488, ctrl | (addr << 14));
			addr += init->pitch;
		}
	}
}

static void
gf100_gr_ctxctl_debug_unit(struct gf100_gr *gr, u32 base)
{
	struct nvkm_device *device = gr->device;
	nvkm_error(subdev, "%06x - done %08x\n", base,
		nvkm_rd32(device, base + 0x400));
	nvkm_error(subdev, "%06x - stat %08x %08x %08x %08x\n", base,
		nvkm_rd32(device, base + 0x800),
		nvkm_rd32(device, base + 0x804),
		nvkm_rd32(device, base + 0x808),
		nvkm_rd32(device, base + 0x80c));
	nvkm_error(subdev, "%06x - stat %08x %08x %08x %08x\n", base,
		nvkm_rd32(device, base + 0x810),
		nvkm_rd32(device, base + 0x814),
		nvkm_rd32(device, base + 0x818),
		nvkm_rd32(device, base + 0x81c));
}

void
gf100_gr_ctxctl_debug(struct gf100_gr *gr)
{
	struct nvkm_device *device = gr->device;
	u32 gpcnr = nvkm_rd32(device, 0x409604) & 0xffff;
	u32 gpc;

	gf100_gr_ctxctl_debug_unit(gr, 0x409000);
	for (gpc = 0; gpc < gpcnr; gpc++)
		gf100_gr_ctxctl_debug_unit(gr, 0x502000 + (gpc * 0x8000));
}

static void
gf100_gr_ctxctl_isr(struct gf100_gr *gr)
{
	//struct nvkm_subdev *subdev = &gr->base.engine.subdev;
	struct nvkm_device * device = gr->device;
	u32 stat = nvkm_rd32(device, 0x409c18);

	if (stat & 0x00000001) {
		u32 code = nvkm_rd32(device, 0x409814);
		if (code == E_BAD_FWMTHD) {
			u32 class = nvkm_rd32(device, 0x409808);
			u32  addr = nvkm_rd32(device, 0x40980c);
			u32  subc = (addr & 0x00070000) >> 16;
			u32  mthd = (addr & 0x00003ffc);
			u32  data = nvkm_rd32(device, 0x409810);

			nvkm_error(subdev, "FECS MTHD subc %d class %04x "
					   "mthd %04x data %08x\n",
				   subc, class, mthd, data);

			nvkm_wr32(device, 0x409c20, 0x00000001);
			stat &= ~0x00000001;
		} else {
			nvkm_error(subdev, "FECS ucode error %d\n", code);
		}
	}

	if (stat & 0x00080000) {
		nvkm_error(subdev, "FECS watchdog timeout\n");
		gf100_gr_ctxctl_debug(gr);
		nvkm_wr32(device, 0x409c20, 0x00080000);
		stat &= ~0x00080000;
	}

	if (stat) {
		nvkm_error(subdev, "FECS %08x\n", stat);
		gf100_gr_ctxctl_debug(gr);
		nvkm_wr32(device, 0x409c20, stat);
	}
}

void
gf100_gr_intr(struct gf100_gr *gr)
{
	//struct gf100_gr *gr = gf100_gr(base);
	//struct nvkm_subdev *subdev = &gr->base.engine.subdev;
	struct nvkm_device * device = gr->device;
	//struct nvkm_fifo_chan *chan;
	unsigned long flags;
	u64 inst = nvkm_rd32(device, 0x409b00) & 0x0fffffff;
	u32 stat = nvkm_rd32(device, 0x400100);
	u32 addr = nvkm_rd32(device, 0x400704);
	u32 mthd = (addr & 0x00003ffc);
	u32 subc = (addr & 0x00070000) >> 16;
	u32 data = nvkm_rd32(device, 0x400708);
	u32 code = nvkm_rd32(device, 0x400110);
	u32 class;
	const char *name = "unknown";
	int chid = -1;

	//chan = nvkm_fifo_chan_inst(device->fifo, (u64)inst << 12, &flags);
	//if (chan) {
	//	name = chan->object.client->name;
	//	chid = chan->chid;
	//}

	//if (device->card_type < NV_E0 || subc < 4)
	if(0)
		class = nvkm_rd32(device, 0x404200 + (subc * 4));
	else
		class = 0x0000;

	if (stat & 0x00000001) {
		/*
		* notifier interrupt, only needed for cyclestats
		* can be safely ignored
		*/
		nvkm_wr32(device, 0x400100, 0x00000001);
		stat &= ~0x00000001;
	}

	if (stat & 0x00000010) {
		if (!gf100_gr_mthd_sw(device, class, mthd, data)) {
			nvkm_error(subdev, "ILLEGAL_MTHD ch %d [%010llx %s] "
				"subc %d class %04x mthd %04x data %08x\n",
				chid, inst << 12, name, subc,
				class, mthd, data);
		}
		nvkm_wr32(device, 0x400100, 0x00000010);
		stat &= ~0x00000010;
	}

	if (stat & 0x00000020) {
		nvkm_error(subdev, "ILLEGAL_CLASS ch %d [%010llx %s] "
			"subc %d class %04x mthd %04x data %08x\n",
			chid, inst << 12, name, subc, class, mthd, data);
		nvkm_wr32(device, 0x400100, 0x00000020);
		stat &= ~0x00000020;
	}

	if (stat & 0x00100000) {
		//const struct nvkm_enum *en =
			//nvkm_enum_find(nv50_data_error_names, code);
		nvkm_error(subdev, "DATA_ERROR %08x [%s] ch %d [%010llx %s] "
			"subc %d class %04x mthd %04x data %08x\n",
			code, "", chid, inst << 12,
			name, subc, class, mthd, data);
		nvkm_wr32(device, 0x400100, 0x00100000);
		stat &= ~0x00100000;
	}

	if (stat & 0x00200000) {
		nvkm_error(subdev, "TRAP ch %d [%010llx %s]\n",
			chid, inst << 12, name);
		//gf100_gr_trap_intr(gr);
		nvkm_wr32(device, 0x400100, 0x00200000);
		stat &= ~0x00200000;
	}

	if (stat & 0x00080000) {
		gf100_gr_ctxctl_isr(gr);
		nvkm_wr32(device, 0x400100, 0x00080000);
		stat &= ~0x00080000;
	}

	if (stat) {
		nvkm_error(subdev, "intr %08x\n", stat);
		nvkm_wr32(device, 0x400100, stat);
	}

	nvkm_wr32(device, 0x400500, 0x00010001);
	//nvkm_fifo_chan_put(device->fifo, flags, &chan);
}

void
gf100_gr_init_fw(struct gf100_gr *gr, u32 fuc_base,
		 struct gf100_gr_fuc *code, struct gf100_gr_fuc *data)
{
	struct nvkm_device * device = gr->device;
	int i;

	nvkm_wr32(device, fuc_base + 0x01c0, 0x01000000);
	for (i = 0; i < data->size / 4; i++)
		nvkm_wr32(device, fuc_base + 0x01c4, data->data[i]);

	nvkm_wr32(device, fuc_base + 0x0180, 0x01000000);
	for (i = 0; i < code->size / 4; i++) {
		if ((i & 0x3f) == 0)
			nvkm_wr32(device, fuc_base + 0x0188, i >> 6);
		nvkm_wr32(device, fuc_base + 0x0184, code->data[i]);
	}

	/* code must be padded to 0x40 words */
	for (; i & 0x3f; i++)
		nvkm_wr32(device, fuc_base + 0x0184, 0);
}

static void
gf100_gr_init_csdata(struct gf100_gr *gr,
		     const struct gf100_gr_pack *pack,
		     u32 falcon, u32 starstar, u32 base)
{
	struct nvkm_device * device = gr->device;
	const struct gf100_gr_pack *iter;
	const struct gf100_gr_init *init;
	u32 addr = ~0, prev = ~0, xfer = 0;
	u32 star, temp;

	nvkm_wr32(device, falcon + 0x01c0, 0x02000000 + starstar);
	star = nvkm_rd32(device, falcon + 0x01c4);
	temp = nvkm_rd32(device, falcon + 0x01c4);
	if (temp > star)
		star = temp;
	nvkm_wr32(device, falcon + 0x01c0, 0x01000000 + star);

	pack_for_each_init(init, iter, pack) {
		u32 head = init->addr - base;
		u32 tail = head + init->count * init->pitch;
		while (head < tail) {
			if (head != prev + 4 || xfer >= 32) {
				if (xfer) {
					u32 data = ((--xfer << 26) | addr);
					nvkm_wr32(device, falcon + 0x01c4, data);
					star += 4;
				}
				addr = head;
				xfer = 0;
			}
			prev = head;
			xfer = xfer + 1;
			head = head + init->pitch;
		}
	}

	nvkm_wr32(device, falcon + 0x01c4, (--xfer << 26) | addr);
	nvkm_wr32(device, falcon + 0x01c0, 0x01000004 + starstar);
	nvkm_wr32(device, falcon + 0x01c4, star + 4);
}

int
gf100_gr_init_ctxctl(struct gf100_gr *gr)
{
	const struct gf100_grctx_func *grctx = gr->func->grctx;
	//struct nvkm_subdev *subdev = &gr->base.engine.subdev;
	struct nvkm_device * device = gr->device;
	//struct nvkm_secboot *sb = device->secboot;
	int i;
	int ret = 0;

	if (gr->firmware) {
		/* load fuc microcode */
		nvkm_mc_unk260(device, 0);

		/* securely-managed falcons must be reset using secure boot */
		//if (nvkm_secboot_is_managed(sb, NVKM_SECBOOT_FALCON_FECS))
		//	ret = nvkm_secboot_reset(sb, NVKM_SECBOOT_FALCON_FECS);
		//else
			gf100_gr_init_fw(gr, 0x409000, &gr->fuc409c,
					 &gr->fuc409d);
		if (ret)
			return ret;

		//if (nvkm_secboot_is_managed(sb, NVKM_SECBOOT_FALCON_GPCCS))
		//	ret = nvkm_secboot_reset(sb, NVKM_SECBOOT_FALCON_GPCCS);
		//else
			gf100_gr_init_fw(gr, 0x41a000, &gr->fuc41ac,
					 &gr->fuc41ad);
		if (ret)
			return ret;

		nvkm_mc_unk260(device, 1);

		/* start both of them running */
		nvkm_wr32(device, 0x409840, 0xffffffff);
		nvkm_wr32(device, 0x41a10c, 0x00000000);
		nvkm_wr32(device, 0x40910c, 0x00000000);

		//if (nvkm_secboot_is_managed(sb, NVKM_SECBOOT_FALCON_GPCCS))
		//	nvkm_secboot_start(sb, NVKM_SECBOOT_FALCON_GPCCS);
		//else
			nvkm_wr32(device, 0x41a100, 0x00000002);
		//if (nvkm_secboot_is_managed(sb, NVKM_SECBOOT_FALCON_FECS))
		//	nvkm_secboot_start(sb, NVKM_SECBOOT_FALCON_FECS);
		//else
			nvkm_wr32(device, 0x409100, 0x00000002);
		if (nvkm_msec(device, 2000,
			if (nvkm_rd32(device, 0x409800) & 0x00000001)
				break;
		) < 0)
			return -EBUSY;

		nvkm_wr32(device, 0x409840, 0xffffffff);
		nvkm_wr32(device, 0x409500, 0x7fffffff);
		nvkm_wr32(device, 0x409504, 0x00000021);

		nvkm_wr32(device, 0x409840, 0xffffffff);
		nvkm_wr32(device, 0x409500, 0x00000000);
		nvkm_wr32(device, 0x409504, 0x00000010);
		if (nvkm_msec(device, 2000,
			if ((gr->size = nvkm_rd32(device, 0x409800)))
				break;
		) < 0)
			return -EBUSY;

		nvkm_wr32(device, 0x409840, 0xffffffff);
		nvkm_wr32(device, 0x409500, 0x00000000);
		nvkm_wr32(device, 0x409504, 0x00000016);
		if (nvkm_msec(device, 2000,
			if (nvkm_rd32(device, 0x409800))
				break;
		) < 0)
			return -EBUSY;

		nvkm_wr32(device, 0x409840, 0xffffffff);
		nvkm_wr32(device, 0x409500, 0x00000000);
		nvkm_wr32(device, 0x409504, 0x00000025);
		if (nvkm_msec(device, 2000,
			if (nvkm_rd32(device, 0x409800))
				break;
		) < 0)
			return -EBUSY;

		//if (device->chipset >= 0xe0) {
		if(1) {
			nvkm_wr32(device, 0x409800, 0x00000000);
			nvkm_wr32(device, 0x409500, 0x00000001);
			nvkm_wr32(device, 0x409504, 0x00000030);
			if (nvkm_msec(device, 2000,
				if (nvkm_rd32(device, 0x409800))
					break;
			) < 0)
				return -EBUSY;

			nvkm_wr32(device, 0x409810, 0xb00095c8);
			nvkm_wr32(device, 0x409800, 0x00000000);
			nvkm_wr32(device, 0x409500, 0x00000001);
			nvkm_wr32(device, 0x409504, 0x00000031);
			if (nvkm_msec(device, 2000,
				if (nvkm_rd32(device, 0x409800))
					break;
			) < 0)
				return -EBUSY;

			nvkm_wr32(device, 0x409810, 0x00080420);
			nvkm_wr32(device, 0x409800, 0x00000000);
			nvkm_wr32(device, 0x409500, 0x00000001);
			nvkm_wr32(device, 0x409504, 0x00000032);
			if (nvkm_msec(device, 2000,
				if (nvkm_rd32(device, 0x409800))
					break;
			) < 0)
				return -EBUSY;

			nvkm_wr32(device, 0x409614, 0x00000070);
			nvkm_wr32(device, 0x409614, 0x00000770);
			nvkm_wr32(device, 0x40802c, 0x00000001);
		}

		if (gr->data == NULL) {
			int ret = gf100_grctx_generate(gr);
			if (ret) {
				nvkm_error(subdev, "failed to construct context\n");
				return ret;
			}
		}

		return 0;
	} else
	if (!gr->func->fecs.ucode) {
		return -ENOSYS;
	}

	/* load HUB microcode */
	nvkm_mc_unk260(device, 0);
	nvkm_wr32(device, 0x4091c0, 0x01000000);
	for (i = 0; i < gr->func->fecs.ucode->data.size / 4; i++)
		nvkm_wr32(device, 0x4091c4, gr->func->fecs.ucode->data.data[i]);

	nvkm_wr32(device, 0x409180, 0x01000000);
	for (i = 0; i < gr->func->fecs.ucode->code.size / 4; i++) {
		if ((i & 0x3f) == 0)
			nvkm_wr32(device, 0x409188, i >> 6);
		nvkm_wr32(device, 0x409184, gr->func->fecs.ucode->code.data[i]);
	}

	/* load GPC microcode */
	nvkm_wr32(device, 0x41a1c0, 0x01000000);
	for (i = 0; i < gr->func->gpccs.ucode->data.size / 4; i++)
		nvkm_wr32(device, 0x41a1c4, gr->func->gpccs.ucode->data.data[i]);

	nvkm_wr32(device, 0x41a180, 0x01000000);
	for (i = 0; i < gr->func->gpccs.ucode->code.size / 4; i++) {
		if ((i & 0x3f) == 0)
			nvkm_wr32(device, 0x41a188, i >> 6);
		nvkm_wr32(device, 0x41a184, gr->func->gpccs.ucode->code.data[i]);
	}
	nvkm_mc_unk260(device, 1);

	/* load register lists */
	gf100_gr_init_csdata(gr, grctx->hub, 0x409000, 0x000, 0x000000);
	gf100_gr_init_csdata(gr, grctx->gpc, 0x41a000, 0x000, 0x418000);
	gf100_gr_init_csdata(gr, grctx->tpc, 0x41a000, 0x004, 0x419800);
	gf100_gr_init_csdata(gr, grctx->ppc, 0x41a000, 0x008, 0x41be00);

	/* start HUB ucode running, it'll init the GPCs */
	nvkm_wr32(device, 0x40910c, 0x00000000);
	nvkm_wr32(device, 0x409100, 0x00000002);
	if (nvkm_msec(device, 2000,
		if (nvkm_rd32(device, 0x409800) & 0x80000000)
			break;
	) < 0) {
		gf100_gr_ctxctl_debug(gr);
		return -EBUSY;
	}
	
	gr->size = nvkm_rd32(device, 0x409804);
	if (gr->data == NULL) {
		int ret = gf100_grctx_generate(gr);
		if (ret) {
			nvkm_error(subdev, "failed to construct context\n");
			return ret;
		}
	}

	return 0;
}

void
gk110_pmu_pgob(struct nvkm_device *device)
{
	static const struct {
		u32 addr;
		u32 data;
	} magic[] = {
		{ 0x020520, 0xfffffffc },
		{ 0x020524, 0xfffffffe },
		{ 0x020524, 0xfffffffc },
		{ 0x020524, 0xfffffff8 },
		{ 0x020524, 0xffffffe0 },
		{ 0x020530, 0xfffffffe },
		{ 0x02052c, 0xfffffffa },
		{ 0x02052c, 0xfffffff0 },
		{ 0x02052c, 0xffffffc0 },
		{ 0x02052c, 0xffffff00 },
		{ 0x02052c, 0xfffffc00 },
		{ 0x02052c, 0xfffcfc00 },
		{ 0x02052c, 0xfff0fc00 },
		{ 0x02052c, 0xff80fc00 },
		{ 0x020528, 0xfffffffe },
		{ 0x020528, 0xfffffffc },
	};
	int i;

	nvkm_mask(device, 0x000200, 0x00001000, 0x00000000);
	nvkm_rd32(device, 0x000200);
	nvkm_mask(device, 0x000200, 0x08000000, 0x08000000);
	MilliSecondDelay(50);

	nvkm_mask(device, 0x10a78c, 0x00000002, 0x00000002);
	nvkm_mask(device, 0x10a78c, 0x00000001, 0x00000001);
	nvkm_mask(device, 0x10a78c, 0x00000001, 0x00000000);

	nvkm_mask(device, 0x0206b4, 0x00000000, 0x00000000);
	for (i = 0; i < sizeof(magic) / 8; i++) {
		nvkm_wr32(device, magic[i].addr, magic[i].data);
		nvkm_msec(device, 2000,
			if (!(nvkm_rd32(device, magic[i].addr) & 0x80000000))
				break;
		);
	}

	nvkm_mask(device, 0x10a78c, 0x00000002, 0x00000000);
	nvkm_mask(device, 0x10a78c, 0x00000001, 0x00000001);
	nvkm_mask(device, 0x10a78c, 0x00000001, 0x00000000);

	nvkm_mask(device, 0x000200, 0x08000000, 0x00000000);
	nvkm_mask(device, 0x000200, 0x00001000, 0x00001000);
	nvkm_rd32(device, 0x000200);
}

#include "gk208_pmu.h"
static int
nvkm_pmu_init(struct nvkm_device * device)
{
	int i;

	/* prevent previous ucode from running, wait for idle, reset */
	nvkm_wr32(device, 0x10a014, 0x0000ffff); /* INTR_EN_CLR = ALL */
	nvkm_msec(device, 2000,
		if (!nvkm_rd32(device, 0x10a04c))
			break;
	);
	nvkm_mask(device, 0x000200, 0x00002000, 0x00000000);
	nvkm_mask(device, 0x000200, 0x00002000, 0x00002000);
	nvkm_rd32(device, 0x000200);
	nvkm_msec(device, 2000,
		if (!(nvkm_rd32(device, 0x10a10c) & 0x00000006))
			break;
	);

	/* upload data segment */
	nvkm_wr32(device, 0x10a1c0, 0x01000000);
	for (i = 0; i < sizeof(gk208_pmu_data) / 4; i++)
		nvkm_wr32(device, 0x10a1c4, gk208_pmu_data[i]);

	/* upload code segment */
	nvkm_wr32(device, 0x10a180, 0x01000000);
	for (i = 0; i < sizeof(gk208_pmu_code) / 4; i++) {
		if ((i & 0x3f) == 0)
			nvkm_wr32(device, 0x10a188, i >> 6);
		nvkm_wr32(device, 0x10a184, gk208_pmu_code[i]);
	}

	/* start it running */
	nvkm_wr32(device, 0x10a10c, 0x00000000);
	nvkm_wr32(device, 0x10a104, 0x00000000);
	nvkm_wr32(device, 0x10a100, 0x00000002);

	/* wait for valid host->pmu ring configuration */
	if (nvkm_msec(device, 2000,
		if (nvkm_rd32(device, 0x10a4d0))
			break;
	) < 0)
		return -EBUSY;
	u32 send_base = nvkm_rd32(device, 0x10a4d0) & 0x0000ffff;
	u32 send_size = nvkm_rd32(device, 0x10a4d0) >> 16;

	/* wait for valid pmu->host ring configuration */
	if (nvkm_msec(device, 2000,
		if (nvkm_rd32(device, 0x10a4dc))
			break;
	) < 0)
		return -EBUSY;
	u32 recv_base = nvkm_rd32(device, 0x10a4dc) & 0x0000ffff;
	u32 recv_size = nvkm_rd32(device, 0x10a4dc) >> 16;

	nvkm_wr32(device, 0x10a010, 0x000000e0);
	return 0;
}

int
gf100_gr_oneinit(struct gf100_gr *gr)
{
	//struct gf100_gr *gr = gf100_gr(base);
	struct nvkm_device * device = gr->device;
	int i, j;
	nvkm_pmu_init(device);
	//nvkm_pmu_pgob(device->pmu, false);
	gk110_pmu_pgob(device);

	gr->rop_nr = gr->func->rops(gr);
	gr->gpc_nr = nvkm_rd32(device, 0x409604) & 0x0000001f;
	KDEBUG("NVIDIA GPU:ROPs:%d,GPCs:%d\n", gr->rop_nr, gr->gpc_nr);
	for (i = 0; i < gr->gpc_nr; i++) {
		gr->tpc_nr[i]  = nvkm_rd32(device, GPC_UNIT(i, 0x2608));
		gr->tpc_total += gr->tpc_nr[i];
		gr->ppc_nr[i]  = gr->func->ppc_nr;
		for (j = 0; j < gr->ppc_nr[i]; j++) {
			u8 mask = nvkm_rd32(device, GPC_UNIT(i, 0x0c30 + (j * 4)));
			if (mask)
				gr->ppc_mask[i] |= (1 << j);
			gr->ppc_tpc_nr[i][j] = Weight32(mask);
		}
	}

	/*XXX: these need figuring out... though it might not even matter */
	switch (device->Chipset) {
	case 0xc0:
		if (gr->tpc_total == 11) { /* 465, 3/4/4/0, 4 */
			gr->screen_tile_row_offset = 0x07;
		} else
		if (gr->tpc_total == 14) { /* 470, 3/3/4/4, 5 */
			gr->screen_tile_row_offset = 0x05;
		} else
		if (gr->tpc_total == 15) { /* 480, 3/4/4/4, 6 */
			gr->screen_tile_row_offset = 0x06;
		}
		break;
	case 0xc3: /* 450, 4/0/0/0, 2 */
		gr->screen_tile_row_offset = 0x03;
		break;
	case 0xc4: /* 460, 3/4/0/0, 4 */
		gr->screen_tile_row_offset = 0x01;
		break;
	case 0xc1: /* 2/0/0/0, 1 */
		gr->screen_tile_row_offset = 0x01;
		break;
	case 0xc8: /* 4/4/3/4, 5 */
		gr->screen_tile_row_offset = 0x06;
		break;
	case 0xce: /* 4/4/0/0, 4 */
		gr->screen_tile_row_offset = 0x03;
		break;
	case 0xcf: /* 4/0/0/0, 3 */
		gr->screen_tile_row_offset = 0x03;
		break;
	case 0xd7:
	case 0xd9: /* 1/0/0/0, 1 */
	case 0xea: /* gk20a */
	case 0x12b: /* gm20b */
		gr->screen_tile_row_offset = 0x01;
		break;
	}

	return 0;
}

int
gf100_gr_init_(struct gf100_gr *gr)
{
	//struct gf100_gr *gr = gf100_gr(base);
	//nvkm_pmu_pgob(gr->base.engine.subdev.device->pmu, false);
	gk110_pmu_pgob(gr->device);
	return gr->func->init(gr);
}



