#include "Lib/Memory.h"

void memcpy(void *dst,const void *src,int size)
{
	if(dst == 0 || src == 0)
		return;
	
	//unsigned char *dstb = (unsigned char *)dst;
	//unsigned char *srcb = (unsigned char *)src;
	int i = 0;
	
	size--;
	if(dst>src)
	{
		while(size >= 0)
		{
			((char *)dst)[size] = ((char *)src)[size];
			size--;
		}
	}
	else
	{
		while(i<=size)
		{
			((char *)dst)[i] = ((char *)src)[i];
			i++;
		}
	}
}

void memset(void *dst,char value,int size)
{
	//char *dstb = (char *)dst;		//这么写会导致中断不使用IST时在ThreadCreate里崩溃，不知为何
	if(!dst)
		return;
	int i = 0;
	while(i<size)
		*((char *)dst + i++) = value;
}

int memcmp(const void *s1,const void *s2,int size)
{
	int i;
	for(i=0; i<size; i++)
	{
		if(*(char *)(s1 + i) != *(char *)(s2 + i))
			return *(char *)(s1 + i) - *(char *)(s2 + i);
	}
	return 0;
}

void DumpMem(void * addr, int size)
{
	int i;

	printk("Memory dump @ 0x%016x \n", addr);
	for(i=0; i<size; i++)
	{
		if(i%16 == 0)
			printk("\n0x%016x: ", addr + i);
		
		printk("%02x ",*((unsigned char*)addr + i));
		
	}
	printk("\n\n");
}