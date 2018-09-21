#include <types.h>
#include <fb.h>
#include <console.h>
#include <kernel.h>
#include <string.h>

struct fb_info boot_fb;
struct fb_ops bootfb_ops;

void boot_fb_fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{
	int i, j;
	u32 *framebuffer_ptr = (u32 *)info->screen_base;
	for (j = 0; j < rect->height; j++) {
		u32 dst_offset_y = (j + rect->dy) * info->var.xres_virtual;
		for (i = 0; i < rect->width; i++) {
			framebuffer_ptr[dst_offset_y + rect->dx + i] = rect->color;
		}
	}
}

void boot_fb_copyarea(struct fb_info *info,  const struct fb_copyarea *region)
{
	int i, j;
	u32 *framebuffer_ptr = (u32 *)info->screen_base;
	u32 dst_offset_y, src_offset_y;
	if (region->dy < region->sy && region->dx < region->sx) {
		for (j = 0; j < region->height; j++) {
			dst_offset_y = (j + region->dy) * info->var.xres_virtual;
			src_offset_y = (j + region->sy) * info->var.xres_virtual;
			for (i = 0; i < region->width; i++) {
				framebuffer_ptr[dst_offset_y + i + region->dx] = framebuffer_ptr[src_offset_y + i + region->sx];
			}
		}
	} else if (region->dy < region->sy && region->dx >= region->sx) {
		for (j = 0; j < region->height; j++) {
			dst_offset_y = (j + region->dy) * info->var.xres_virtual;
			src_offset_y = (j + region->sy) * info->var.xres_virtual;
			for (i = region->width - 1; i >= 0 ; i--) {
				framebuffer_ptr[dst_offset_y + i + region->dx] = framebuffer_ptr[src_offset_y + i + region->sx];
			}
		}
	} else if (region->dy >= region->sy && region->dx < region->sx) {
		for (j = region->height - 1; j >= 0; j--) {
			dst_offset_y = (j + region->dy) * info->var.xres_virtual;
			src_offset_y = (j + region->sy) * info->var.xres_virtual;
			for (i = 0; i < region->width; i++) {
				framebuffer_ptr[dst_offset_y + i + region->dx] = framebuffer_ptr[src_offset_y + i + region->sx];
			}
		}
	} else {
		for (j = region->height - 1; j >= 0; j--) {
			dst_offset_y = (j + region->dy) * info->var.xres_virtual;
			src_offset_y = (j + region->sy) * info->var.xres_virtual;
			for (i = region->width - 1; i >= 0 ; i--) {
				framebuffer_ptr[dst_offset_y + i + region->dx] = framebuffer_ptr[src_offset_y + i + region->sx];
			}
		}
	}
}

void boot_fb_imageblit(struct fb_info *info, const struct fb_image *image)
{
	int i, j;
	u32 *framebuffer_ptr = (u32 *)info->screen_base;
	for (j = 0; j < image->height; j++) {
		u32 dst_offset_y = (j + image->dy) * info->var.xres_virtual;
		u32 src_offset_y = j * image->width;
		for (i = 0; i < image->width; i++) {
			framebuffer_ptr[dst_offset_y + i + image->dx] = ((u32 *)image->data)[src_offset_y + i];
		}
	}
}

int boot_fb_init()
{
	struct bootloader_parm_block *boot_parm = (void *)SYSTEM_PARM_BASE;
	boot_fb.screen_base = (void *)PHYS2VIRT(boot_parm->frame_buffer_base);
	boot_fb.screen_size = 0x2000000;
	boot_fb.var.bits_per_pixel = 32;
	boot_fb.var.red_bits = 8;
	boot_fb.var.green_bits = 8;
	boot_fb.var.blue_bits = 8;
	boot_fb.var.transparent_bits = 8;
	boot_fb.var.xres = boot_parm->screen_width;
	boot_fb.var.xres_virtual = boot_parm->pixels_per_scanline;
	boot_fb.var.yres = boot_parm->screen_height;
	boot_fb.var.yres_virtual = boot_parm->screen_height;
	boot_fb.var.active = 1;

	boot_fb.fbops = &bootfb_ops;
	bootfb_ops.fb_fillrect = boot_fb_fillrect;
	bootfb_ops.fb_copyarea = boot_fb_copyarea;
	bootfb_ops.fb_imageblit = boot_fb_imageblit;

	memset(boot_fb.screen_base, 0x00, 0x400000);

	fb_register(&boot_fb);
}
