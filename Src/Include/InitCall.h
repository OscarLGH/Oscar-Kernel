#ifndef _INITCALL_H
#define _INITCALL_H

#define INIT_CALL			__attribute__((__section__(".initcall1.init")))
#define CORE_INIT_CALL		__attribute__((__section__(".initcall2.init")))
#define POSTCORE_INIT_CALL	__attribute__((__section__(".initcall3.init")))
#define ARCH_INIT_CALL		__attribute__((__section__(".initcall4.init")))
#define SUBSYS_INIT_CALL	__attribute__((__section__(".initcall5.init")))
#define FS_INIT_CALL		__attribute__((__section__(".initcall6.init")))
#define DEVICE_INIT_CALL	__attribute__((__section__(".initcall7.init")))
#define LATE_INIT_CALL		__attribute__((__section__(".initcall8.init")))

typedef void(*INIT_CALL_FUNC)(void);

#define CORE_INIT(x) static INIT_CALL_FUNC MODULE_INIT_CALL_##x CORE_INIT_CALL = x;
#define POSTCORE_INIT(x) static INIT_CALL_FUNC MODULE_INIT_CALL_##x POSTCORE_INIT_CALL = x;
#define ARCH_INIT(x) static INIT_CALL_FUNC MODULE_INIT_CALL_##x ARCH_INIT_CALL = x;
#define EARLY_MODULE_INIT(x)  static INIT_CALL_FUNC MODULE_INIT_CALL_##x SUBSYS_INIT_CALL = x;
#define MODULE_INIT(x) static INIT_CALL_FUNC MODULE_INIT_CALL_##x DEVICE_INIT_CALL = x;

#endif
