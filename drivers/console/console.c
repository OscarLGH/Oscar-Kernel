#include <console.h>

LIST_HEAD(con_list);

int console_init()
{
	INIT_LIST_HEAD(&con_list);
	return 0;
}
int console_register(struct console *con_ptr)
{
	list_add_tail(&con_ptr->list, &con_list);
}

int console_unregister(struct console *con_ptr)
{
	list_del(&con_ptr->list);
}

int console_active(struct console *con_ptr)
{
	con_ptr->active = 1;
	return 0;
}

int console_deactive(struct console *con_ptr)
{
	con_ptr->active = 0;
	return 0;
}

int write_console(u32 chr)
{
	struct console *con_ptr;
	list_for_each_entry(con_ptr, &con_list, list) {
		if (con_ptr->active == 1) {
			con_ptr->console_write_char(con_ptr, chr);
		}
	}
}
