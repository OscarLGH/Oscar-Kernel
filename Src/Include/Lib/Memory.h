#ifndef _MEMORY_H
#define _MEMORY_H

#include "Type.h"
#include "MM.h"
#include "Irq.h"

//#define DEBUG
#include "Debug.h"

void memcpy(void *dst,const void *src,int size);
void memset(void *dst,char value,int size);
int memcmp(const void *s1,const void *s2,int size);
void DumpMem(void * addr, int size);

void * KMalloc(UINT64 Len);
void   KFree(void *Obj);

#endif