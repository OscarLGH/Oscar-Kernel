#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <types.h>
#include <kernel.h>
#include <list.h>

struct console {
	u64 type;
	char *name;

	u64 active;

	u64 width;
	u64 height;
	u64 font_color;
	u64 back_color;
	u64 font_width;
	u64 font_height;
	u64 cursor_x;
	u64 cursor_y;
	u8 *char_buf;
	u32 *color_buf;

	void *font_data;
	u32 *blend_buffer;

	void *input_dev;
	void *output_dev;

	struct list_head list;

	int (*console_set_color)(struct console *con, u32 font_color, u32 back_color);
	int (*console_get_color)(struct console *con, u32 *font_color, u32 *back_color);

	int (*console_get_position)(struct console *con, u32 *x, u32 *y);
	int (*console_set_position)(struct console *con, u32 x, u32 y);

	int (*console_read_char)(struct console *con);
	int (*console_write_char)(struct console *con, u32 chr);
};


int console_register(struct console *con_ptr);
int console_unregister(struct console *con_ptr);
int console_active(struct console *con_ptr);
int console_deactive(struct console *con_ptr);
int write_console(u32 chr);
#endif