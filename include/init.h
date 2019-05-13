#ifndef _INITCALL_H
#define _INITCALL_H

#define init_call			__attribute__((__section__(".initcall1.init")))
#define core_init_call		__attribute__((__section__(".initcall2.init")))
#define postcore_init_call	__attribute__((__section__(".initcall3.init")))
#define arch_init_call		__attribute__((__section__(".initcall4.init")))
#define subsys_init_call	__attribute__((__section__(".initcall5.init")))
#define fs_init_call		__attribute__((__section__(".initcall6.init")))
#define device_init_call	__attribute__((__section__(".initcall7.init")))
#define late_init_call		__attribute__((__section__(".initcall8.init")))

typedef void(*init_call_t)(void);

#define core_init(x) static init_call_t module_init_call##x core_init_call = x;
#define postcore_init(x) static init_call_t module_init_call##x postcore_init_call = x;
#define arch_init(x) static init_call_t module_init_call##x arch_init_call = x;
#define subsys_init(x)  static init_call_t module_init_call##x subsys_init_call = x;
#define module_init(x) static init_call_t module_init_call##x device_init_call = x;

extern init_call_t __initcall1_start;
extern init_call_t __initcall1_end;		
extern init_call_t __initcall2_start;
extern init_call_t __initcall2_end;	
extern init_call_t __initcall3_start;
extern init_call_t __initcall3_end;	
extern init_call_t __initcall4_start;
extern init_call_t __initcall4_end;	
extern init_call_t __initcall5_start;
extern init_call_t __initcall5_end;	
extern init_call_t __initcall6_start;
extern init_call_t __initcall6_end;	
extern init_call_t __initcall7_start;
extern init_call_t __initcall7_end;	
extern init_call_t __initcall8_start;
extern init_call_t __initcall8_end;	

#endif

