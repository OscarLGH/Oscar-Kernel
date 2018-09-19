/* Arch indenpendant initialization 
* Oscar
* Jul, 2018
*
*/

#include <types.h>
#include <kernel.h>
#include <string.h>
#include <fb.h>

void boot_fb_init();
void graphic_con_init();

void fb_test()
{
	u32 image_array[0x100000];
	for (int i = 0; i < 200; i++) {
		for (int j = 0; j < 200; j++) {
			if ((i - 100) * (i - 100) + (j - 100) * (j - 100) <= 10000)
				image_array[i + j * 200] = 0x0000ffff + 0x00010000 * j;
			else
				image_array[i + j * 200] = 0x008f9f7f + 0x00010000 * i;
		}
	}

	struct fb_image image_test = {100, 100, 200, 200, 0, 0, 0, (char *)image_array};
	imageblit_active_fb(&image_test);
}

int start_kernel()
{
	boot_fb_init();
	graphic_con_init();
	fb_test();

	printk("Oscar Kernel init start...\n");
	printk("Build:%s %s\n", __DATE__, __TIME__);
	struct fb_copyarea area;

	for (int i = 0; i < 1000000; i++) {
		printk("%d\n", i);
	}
	
	return 0;
}
