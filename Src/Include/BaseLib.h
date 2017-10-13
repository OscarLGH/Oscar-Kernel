#ifndef _BASELIB_H
#define _BASELIB_H

#include "MM.h"
#include "Lib/Memory.h"
#include "Lib/String.h"
#include "Lib/Integer.h"
#include "Lib/List.h"
#include "Irq.h"
#include "SysConsole.h"
#include "SystemParameters.h"
#include "Delay.h"

#define ASSERT(x) {if(!x)printk("Assert:%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);}
#endif