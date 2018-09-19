#ifndef _FB_H_
#define _FB_H_

#include <types.h>
#include <kernel.h>
#include <list.h>

struct fb_var_screeninfo {
	u32 active;

	u32 xres;
	u32 yres;
	u32 xres_virtual;
	u32 yres_virtual;
	u32 xoffset;
	u32 yoffset;

	u32 bits_per_pixel;
	u32 grayscale;

	u32 red_bits;
	u32 green_bits;
	u32 blue_bits;
	u32 transparent_bits;

	u32 height;
	u32 width;
};

/* Internal HW accel */
#define ROP_COPY 0
#define ROP_XOR  1

struct fb_copyarea {
	u32 dx;
	u32 dy;
	u32 width;
	u32 height;
	u32 sx;
	u32 sy;
};

struct fb_fillrect {
	u32 dx;	/* screen-relative */
	u32 dy;
	u32 width;
	u32 height;
	u32 color;
	u32 rop;
};

struct fb_image {
	u32 dx;		/* Where to place image */
	u32 dy;
	u32 width;		/* Size of image */
	u32 height;
	u32 fg_color;		/* Only used when a mono bitmap */
	u32 bg_color;
	u8  depth;		/* Depth of the image */
	const char *data;	/* Pointer to image data */
	//struct fb_cmap cmap;	/* color map info */
};

struct fb_info {
	int count;
	int node;
	int flags;
	/*
	 * -1 by default, set to a FB_ROTATE_* value by the driver, if it knows
	 * a lcd is not mounted upright and fbcon should rotate to compensate.
	 */
	int fbcon_rotate_hint;
	int lock;
	struct fb_var_screeninfo var;	/* Current var */
	struct list_head modelist;      /* mode list */
	struct list_head list;			
	//struct fb_videomode *mode;	/* current mode */

#ifdef CONFIG_FB_BACKLIGHT
	/* assigned backlight device */
	/* set before framebuffer registration, 
	   remove after unregister */
	//struct backlight_device *bl_dev;

	/* Backlight level curve */
	//struct mutex bl_curve_mutex;	
	//u8 bl_curve[FB_BACKLIGHT_LEVELS];
#endif

	struct fb_ops *fbops;
	struct device *device;		/* This is the parent */
	struct device *dev;		/* This is this fb device */
	int class_flag;                    /* private sysfs flags */
#ifdef CONFIG_FB_TILEBLITTING
	struct fb_tile_ops *tileops;    /* Tile Blitting */
#endif
	
	char *screen_base;	/* Virtual address */
	unsigned long screen_size;	/* Amount of ioremapped VRAM or 0 */ 
	void *pseudo_palette;		/* Fake palette of 16 colors */ 
};

struct fb_ops {
	/* open/release and usage marking */
	int (*fb_open)(struct fb_info *info, int user);
	int (*fb_release)(struct fb_info *info, int user);

	/* For framebuffers with strange non linear layouts or that do not
	 * work with normal memory mapped access
	 */
	u32 (*fb_read)(struct fb_info *info, char *buf,
			   u32 count, u32 *ppos);
	u32 (*fb_write)(struct fb_info *info, const char *buf,
			    u32 count, u32 *ppos);

	/* checks var and eventually tweaks it to something supported,
	 * DO NOT MODIFY PAR */
	int (*fb_check_var)(struct fb_var_screeninfo *var, struct fb_info *info);

	/* set the video mode according to info->var */
	int (*fb_set_par)(struct fb_info *info);

	/* set color register */
	int (*fb_setcolreg)(unsigned regno, unsigned red, unsigned green,
			    unsigned blue, unsigned transp, struct fb_info *info);

	/* set color registers in batch */
	//int (*fb_setcmap)(struct fb_cmap *cmap, struct fb_info *info);

	/* blank display */
	int (*fb_blank)(int blank, struct fb_info *info);

	/* pan display */
	int (*fb_pan_display)(struct fb_var_screeninfo *var, struct fb_info *info);

	/* Draws a rectangle */
	void (*fb_fillrect) (struct fb_info *info, const struct fb_fillrect *rect);
	/* Copy data from area to another */
	void (*fb_copyarea) (struct fb_info *info, const struct fb_copyarea *region);
	/* Draws a image to the display */
	void (*fb_imageblit) (struct fb_info *info, const struct fb_image *image);

	/* Draws cursor */
	//int (*fb_cursor) (struct fb_info *info, struct fb_cursor *cursor);

	/* wait for blit idle, optional */
	int (*fb_sync)(struct fb_info *info);

	/* perform fb specific ioctl (optional) */
	int (*fb_ioctl)(struct fb_info *info, unsigned int cmd,
			unsigned long arg);

	/* Handle 32bit compat ioctl (optional) */
	int (*fb_compat_ioctl)(struct fb_info *info, unsigned cmd,
			unsigned long arg);

	/* perform fb specific mmap */
	//int (*fb_mmap)(struct fb_info *info, struct vm_area_struct *vma);

	/* get capability given var */
	//void (*fb_get_caps)(struct fb_info *info, struct fb_blit_caps *caps,
	//		    struct fb_var_screeninfo *var);

	/* teardown any resources to do with this framebuffer */
	void (*fb_destroy)(struct fb_info *info);
};


int fb_register(struct fb_info *fb_ptr);
int fb_unregister(struct fb_info *fb_ptr);
int early_fb_init();
int fb_active(struct fb_info *fb_ptr);
int fb_deactive(struct fb_info *fb_ptr);
int fillrect_active_fb(const struct fb_fillrect *rect);
int copyarea_active_fb(const struct fb_copyarea *area);
int imageblit_active_fb(const struct fb_image *image);
struct fb_info *get_current_fb();

#endif