#ifdef KDEBUG
#undef KDEBUG
#endif

#ifdef	DEBUG
	//#define KDEBUG(x,args...) serial_print(x,##args)
	#define KDEBUG(x,args...) printk(x,##args)
	#undef DEBUG
#else
	#define KDEBUG(x,args...) {}
#endif

