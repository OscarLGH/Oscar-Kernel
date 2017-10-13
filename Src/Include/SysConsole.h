#ifndef CONSOLE_H
#define CONSOLE_H

#include "Type.h"
#include "BaseLib.h"
#include "ArchLib.h"
//#include "../Driver/Display/Display.h"

#include "SystemParameters.h"

#include "../Arch/X86_64/Lib/Lib.h"
#include <stdarg.h>
#include "Display.h"

//#define DEBUG
#include "Debug.h"

struct _FB_DEVICE;
struct _SPIN_LOCK;

typedef struct _CONSOLE
{
	UINT64  CorX;				//控制台X坐标
	UINT64  CorY;				//控制台Y坐标
	UINT64	Width;				//控制台长度
	UINT64	Height;				//控制台宽度
	UINT64	FontColor;			//字体颜色
	UINT64	BackColor;			//背景颜色
	UINT64  FontWidth;
	UINT64  FontHeight;
	UINT64	CursorX;			//当前光标X位置
	UINT64	CursorY;			//当前光标Y位置
	UINT8	*CharBuffer;		//字符缓冲区
	UINT32  *ColorBuffer;
	UINT32	*FrameBuffer;		//帧缓冲
	UINT64	BufferIndex;		//当前字符指针
	UINT64	FirstLineIndex;		//第一行指针
	UINT64	LastLineIndex;		//最后行指针

	struct _FB_DEVICE * FbDevice;
	//SPIN_LOCK SpinLock;

	RETURN_STATUS(*SetColor)(struct _CONSOLE *Con, UINT32 FontColor, UINT32 BackColor);
	RETURN_STATUS(*GetPosition)(struct _CONSOLE *Con, UINT32 * X, UINT32 * Y);
	RETURN_STATUS(*SetPosition)(struct _CONSOLE *Con, UINT32 X, UINT32 Y);
	RETURN_STATUS(*ReadConsole)(struct _CONSOLE *Con);
	RETURN_STATUS(*WriteConsole)(struct _CONSOLE *Con, UINT8 Char);
}CONSOLE;

#define SYSCON_MAX_BUFFER 16384

void ConsoleInit(CONSOLE* SysConPtr);
void ClearScreen(CONSOLE* SysConPtr);
void PutChar(CONSOLE* SysConPtr,char Char,UINT32 X,UINT32 Y,UINT32 FontColor,UINT32 BackColor);
void print_hex(long long i);
void print_int(long long i);
void print_str(char * str);
UINT32 GetColor(CONSOLE * ConPtr);
void SetColor(CONSOLE * ConPtr,UINT32 FontColor);
void GetXY(CONSOLE* SysConPtr,UINT32 *X,UINT32 *Y);
void SetXY(CONSOLE* SysConPtr,UINT32 X,UINT32 Y);
void print_int_at(long long i,UINT32 X,UINT32 Y,UINT32 Color);
void print_str_at(char * str,UINT32 X,UINT32 Y,UINT32 Color);
int printk(const char *fmt, ...);
int serial_print(const char *fmt, ...);
UINT64 GetProcParams(UINT64 i);

#endif