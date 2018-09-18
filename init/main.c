/* Arch indenpendant initialization 
* Oscar
* Jul, 2018
*
*/
void boot_fb_init();
void graphic_con_init();

#include <types.h>
#include <kernel.h>
#include <string.h>

int start_kernel()
{
	boot_fb_init();
	graphic_con_init();

	printk("Oscar Kernel init start...\n");
	printk("Build:%s %s\n", __DATE__, __TIME__);
	
	return 0;
}
