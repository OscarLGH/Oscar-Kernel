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
				framebuffer_ptr[dst_offset_y + i + region->dx] = framebuffer_ptr[src_offset_y + i + region->sy];
			}
		}
	} else if (region->dy < region->sy && region->dx >= region->sx) {
		for (j = 0; j < region->height; j++) {
			dst_offset_y = (j + region->dy) * info->var.xres_virtual;
			src_offset_y = (j + region->sy) * info->var.xres_virtual;
			for (i = region->width - 1; i >= 0 ; i--) {
				framebuffer_ptr[dst_offset_y + i + region->dx] = framebuffer_ptr[src_offset_y + i + region->sy];
			}
		}
	} else if (region->dy >= region->sy && region->dx < region->sx) {
		for (j = region->height - 1; j >= 0; j--) {
			dst_offset_y = (j + region->dy) * info->var.xres_virtual;
			src_offset_y = (j + region->sy) * info->var.xres_virtual;
			for (i = region->width - 1; i >= 0 ; i--) {
				framebuffer_ptr[dst_offset_y + i + region->dx] = framebuffer_ptr[src_offset_y + i + region->sy];
			}
		}
	} else {
		for (j = region->height - 1; j >= 0; j--) {
			dst_offset_y = (j + region->dy) * info->var.xres_virtual;
			src_offset_y = (j + region->sy) * info->var.xres_virtual;
			for (i = region->width - 1; i >= 0 ; i--) {
				framebuffer_ptr[dst_offset_y + i + region->dx] = framebuffer_ptr[src_offset_y + i + region->sy];
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

u32 image_array[0x100000];

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

	fb_register(&boot_fb);

	for (int i = 0; i < 200; i++) {
		for (int j = 0; j < 200; j++) {
			if ((i - 100) * (i - 100) + (j - 100) * (j - 100) <= 10000)
				image_array[i + j * 200] = 0x0000ffff + 0x00010000 * j;
			else
				image_array[i + j * 200] = 0x008f9f7f + 0x00010000 * i;
		}
	}

	
	struct fb_image image_test = {100, 100, 200, 200, 0, 0, 0, (char *)image_array};
	blit_active_fb(&image_test);
	//boot_fb.fbops->fb_imageblit(&boot_fb, &image_test);
	//struct fb_fillrect fill_test = {100, 100, 100, 100, 0x0000ff00, 0};
	//boot_fb.fbops->fb_fillrect(&boot_fb, &fill_test);
	//struct fb_copyarea copy_test1 = {0, 0, 200, 200, 100, 100};
	//boot_fb.fbops->fb_copyarea(&boot_fb, &copy_test1);

	//struct fb_copyarea copy_test2 = {200, 0, 200, 200, 100, 100};
	//boot_fb.fbops->fb_copyarea(&boot_fb, &copy_test2);

	//struct fb_copyarea copy_test3 = {0, 200, 200, 200, 100, 100};
	//boot_fb.fbops->fb_copyarea(&boot_fb, &copy_test3);

	//struct fb_copyarea copy_test4 = {200, 200, 200, 200, 100, 100};
	//boot_fb.fbops->fb_copyarea(&boot_fb, &copy_test4);
	
}
