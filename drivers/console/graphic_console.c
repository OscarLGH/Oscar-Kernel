#include <console.h>
#include "font_8x16.h"
#include <fb.h>
#include <math.h>

struct console graphics_con = {0};
u32 blit_buffer[0x100000];


int fbcon_scroll(struct console *con, int direction)
{
	struct fb_copyarea cp_area;
	struct fb_fillrect fi_area;
	if (direction == 1) {
		cp_area.dx = 0;
		cp_area.dy = 0;
		cp_area.width = con->width;
		cp_area.height = rounddown(con->height - con->font_height, 16);
		cp_area.sx = 0;
		cp_area.sy = con->font_height;

		copyarea_active_fb(&cp_area);

		fi_area.dx = 0;
		fi_area.dy = rounddown(con->height - con->font_height, 16) - 1;
		fi_area.width = con->width;
		fi_area.height = con->font_height;
		fi_area.color = 0;
		fillrect_active_fb(&fi_area);
	}
}

static void render_char(struct console *con, u32 chr)
{
	char *font_data = con->font_data;
	char font;
	int i, j;
	for (i = 0; i < con->font_height; i++) {
		font = font_data[chr * con->font_height + i];
		for (j = 0; j < con->font_width; j++) {
			if (font & (0x80 >> j)) {
				blit_buffer[i * con->font_width + j] = con->font_color;
			} else {
				blit_buffer[i * con->font_width + j] = con->back_color;
			}
		}
	}
}

int graphic_con_write_char(struct console *con, u32 chr)
{
	struct fb_image image;
	switch (chr) {
		case '\n':
			con->cursor_x = 0;

			if (con->cursor_y + con->font_height * 2 > con->height) {
				fbcon_scroll(con, 1);
				con->cursor_x = 0;
			} else {
				con->cursor_y += con->font_height;
			}
			break;

		default:

			image.dx = con->cursor_x;
			image.dy = con->cursor_y;
			image.width = con->font_width;
			image.height = con->font_height;
			image.data = (char *)blit_buffer;

			render_char(con, chr);
			imageblit_active_fb(&image);

			if (con->cursor_x + con->font_width * 2 > con->width) {
				con->cursor_x = 0;

				if (con->cursor_y + con->font_height * 2 >= con->height) {
					fbcon_scroll(con, 1);
					con->cursor_x = 0;
				} else {
					con->cursor_y += con->font_height;
				}
			} else {
				con->cursor_x += con->font_width;
			}
			break;
	}
}

int graphic_con_write_line(struct console *con, u32 *chr)
{
	return 0;
}

void graphic_con_init()
{
	graphics_con.active = 1;
	graphics_con.font_height = 16;
	graphics_con.font_width = 8;
	graphics_con.font_data = (void *)fontdata_8x16;
	graphics_con.width  = get_current_fb()->var.xres;
	graphics_con.height = get_current_fb()->var.yres;
	graphics_con.console_write_char = graphic_con_write_char;
	graphics_con.font_color = 0x00ffffff;
	graphics_con.back_color = 0x00000000;
	
	console_register(&graphics_con);
}

