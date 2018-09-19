#include <fb.h>

LIST_HEAD(fb_list);

int fb_register(struct fb_info *fb_ptr)
{
	list_add_tail(&fb_ptr->list, &fb_list);
}

int fb_unregister(struct fb_info *fb_ptr)
{
	list_del(&fb_ptr->list);
}

int early_fb_init()
{
	INIT_LIST_HEAD(&fb_list);
}

int fb_active(struct fb_info *fb_ptr)
{
	fb_ptr->var.active = 1;
	return 0;
}

int fb_deactive(struct fb_info *fb_ptr)
{
	fb_ptr->var.active = 0;
	return 0;
}

int fillrect_active_fb(const struct fb_fillrect *rect)
{
	struct fb_info *fb_ptr;
	list_for_each_entry(fb_ptr, &fb_list, list) {
		if (fb_ptr->var.active == 1) {
			fb_ptr->fbops->fb_fillrect(fb_ptr, rect);
		}
	}
}

int copyarea_active_fb(const struct fb_copyarea *area)
{
	struct fb_info *fb_ptr;
	list_for_each_entry(fb_ptr, &fb_list, list) {
		if (fb_ptr->var.active == 1) {
			fb_ptr->fbops->fb_copyarea(fb_ptr, area);
		}
	}
}

int imageblit_active_fb(const struct fb_image *image)
{
	struct fb_info *fb_ptr;
	list_for_each_entry(fb_ptr, &fb_list, list) {
		if (fb_ptr->var.active == 1) {
			fb_ptr->fbops->fb_imageblit(fb_ptr, image);
		}
	}
}

struct fb_info *get_current_fb()
{
	struct fb_info *fb_ptr;
	list_for_each_entry(fb_ptr, &fb_list, list) {
		if (fb_ptr->var.active == 1) {
			return fb_ptr;
		}
	}
}

