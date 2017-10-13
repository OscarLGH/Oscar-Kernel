#include "GraphicsEngine.h"

/*******************************************************************************
* PGRAPH engine/subdev functions
******************************************************************************/

#include "fuc/hubgk208.fuc5.h"

static struct gf100_gr_ucode
gk208_gr_fecs_ucode = {
	.code.data = gk208_grhub_code,
	.code.size = sizeof(gk208_grhub_code),
	.data.data = gk208_grhub_data,
	.data.size = sizeof(gk208_grhub_data),
};

#include "fuc/gpcgk208.fuc5.h"

static struct gf100_gr_ucode
gk208_gr_gpccs_ucode = {
	.code.data = gk208_grgpc_code,
	.code.size = sizeof(gk208_grgpc_code),
	.data.data = gk208_grgpc_data,
	.data.size = sizeof(gk208_grgpc_data),
};

const struct gf100_gr_pack
gk208_gr_pack_mmio[] = {
	{ gk208_gr_init_main_0 },
	{ gk110_gr_init_fe_0 },
	{ gf100_gr_init_pri_0 },
	{ gf100_gr_init_rstr2d_0 },
	{ gf119_gr_init_pd_0 },
	{ gk208_gr_init_ds_0 },
	{ gf100_gr_init_scc_0 },
	{ gk110_gr_init_sked_0 },
	{ gk110_gr_init_cwd_0 },
	{ gf119_gr_init_prop_0 },
	{ gk208_gr_init_gpc_unk_0 },
	{ gf100_gr_init_setup_0 },
	{ gf100_gr_init_crstr_0 },
	{ gk208_gr_init_setup_1 },
	{ gf100_gr_init_zcull_0 },
	{ gf119_gr_init_gpm_0 },
	{ gk110_gr_init_gpc_unk_1 },
	{ gf100_gr_init_gcc_0 },
	{ gk104_gr_init_tpccs_0 },
	{ gk208_gr_init_tex_0 },
	{ gk104_gr_init_pe_0 },
	{ gk208_gr_init_l1c_0 },
	{ gf100_gr_init_mpc_0 },
	{ gk110_gr_init_sm_0 },
	{ gf117_gr_init_pes_0 },
	{ gf117_gr_init_wwdx_0 },
	{ gf117_gr_init_cbm_0 },
	{ gk104_gr_init_be_0 },
	{ gf100_gr_init_fe_1 },
	{}
};

/*******************************************************************************
* PGRAPH context implementation
******************************************************************************/

const struct gf100_grctx_func
gk208_grctx = {
	.main = gk104_grctx_generate_main,
	.unkn = gk104_grctx_generate_unkn,
	.hub = gk208_grctx_pack_hub,
	.gpc = gk208_grctx_pack_gpc,
	.zcull = gf100_grctx_pack_zcull,
	.tpc = gk208_grctx_pack_tpc,
	.ppc = gk208_grctx_pack_ppc,
	.icmd = gk208_grctx_pack_icmd,
	.mthd = gk110_grctx_pack_mthd,
	.bundle = gk104_grctx_generate_bundle,
	.bundle_size = 0x3000,
	.bundle_min_gpm_fifo_depth = 0xc2,
	.bundle_token_limit = 0x200,
	.pagepool = gk104_grctx_generate_pagepool,
	.pagepool_size = 0x8000,
	.attrib = gf117_grctx_generate_attrib,
	.attrib_nr_max = 0x324,
	.attrib_nr = 0x218,
	.alpha_nr_max = 0x7ff,
	.alpha_nr = 0x648,
};


const struct gf100_gr_func
gk208_gr = {
	.init = gk104_gr_init,
	.init_rop_active_fbps = gk104_gr_init_rop_active_fbps,
	.init_ppc_exceptions = gk104_gr_init_ppc_exceptions,
	.mmio = gk208_gr_pack_mmio,
	.fecs.ucode = &gk208_gr_fecs_ucode,
	.gpccs.ucode = &gk208_gr_gpccs_ucode,
	.rops = gf100_gr_rops,
	.ppc_nr = 1,
	.grctx = &gk208_grctx,
	/*
	.sclass = {
		{ -1, -1, FERMI_TWOD_A },
		{ -1, -1, KEPLER_INLINE_TO_MEMORY_B },
		{ -1, -1, KEPLER_B, &gf100_fermi },
		{ -1, -1, KEPLER_COMPUTE_B },
		{}
	}*/
};

RETURN_STATUS Gk208GraphicsEngineInit(GRAPHIC_ENGINE * Engine)
{
	extern const struct gf100_gr_func gk208_gr;
	extern int gk104_gr_init(struct gf100_gr *gr);
	struct gf100_gr * gk208_gr_eng = KMalloc(sizeof(*gk208_gr_eng));
	gk208_gr_eng->device = Engine->Parent;
	gk208_gr_eng->func = &gk208_gr;
	gk208_gr_eng->firmware = false;
	int gf100_gr_oneinit(struct gf100_gr *gr);
	gf100_gr_oneinit(gk208_gr_eng);
	gk104_gr_init(gk208_gr_eng);
}

