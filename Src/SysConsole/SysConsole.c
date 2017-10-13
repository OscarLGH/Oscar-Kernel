#include "SysConsole.h"
#include "font_8x16.h"

extern SYSTEM_PARAMETERS* SysParam;
CONSOLE SysCon;

SPIN_LOCK ConsoleSpinLock = { 0 };
//DEVICE_NODE * DisplayDevice = NULL;
//extern DISPLAYDRIVER BasicFrameBufferOperations;

void ConsoleInit(CONSOLE * SysConPtr)
{
	SYSTEM_PARAMETERS *SysParam = (SYSTEM_PARAMETERS *)SYSTEMPARABASE;
	SysConPtr->CorX = 0;
	SysConPtr->CorY = 0;
	SysConPtr->Width = SysParam->ScreenWidth / 8 * 8;
	SysConPtr->Height = SysParam->ScreenHeight / 16 * 16;
	SysConPtr->FontWidth = 8;
	SysConPtr->FontHeight = 16;
	SysConPtr->FontColor = 0xFFFFFFFF;
	SysConPtr->BackColor = 0xFF301024;
	SysConPtr->CursorX = 0;
	SysConPtr->CursorY = 0;
	SysConPtr->BufferIndex = 0;
	SysConPtr->FirstLineIndex = 0;
	SysConPtr->LastLineIndex = 0;
	
	SysConPtr->CharBuffer = (UINT8 *)KMalloc(SYSCON_MAX_BUFFER);
	SysConPtr->ColorBuffer = (UINT32 *)KMalloc(SYSCON_MAX_BUFFER * 4);
	//SysConPtr->FrameBuffer = (UINT32 *)KMalloc(SysParam->PixelsPerScanLine * SysParam->ScreenHeight * 4);
	
	memset(SysConPtr->CharBuffer, 0, SYSCON_MAX_BUFFER);
	memset(SysConPtr->ColorBuffer, 0, SYSCON_MAX_BUFFER * 4);
	//memset(SysConPtr->FrameBuffer, 0, SysParam->PixelsPerScanLine * SysParam->ScreenHeight * 4);

	DisplayEarlyInit();
	SysConPtr->FbDevice = DisplayOpen(0)->FbDevice;
	ClearScreen(SysConPtr);
	//while(1);
}
void PutChar(CONSOLE* SysConPtr, char Char, UINT32 X, UINT32 Y, UINT32 FontColor, UINT32 BackColor)
{
	UINT64 i, j;
	char font;
	for(i=0; i<SysConPtr->FontHeight; i++)
	{
		font = fontdata_8x16[Char * SysConPtr->FontHeight + i];
		for(j=0; j<SysConPtr->FontWidth; j++)
		{
			if(font&(0x80>>j))
			{
				SysConPtr->FbDevice->DrawPoint(SysConPtr->FbDevice, X+j, Y+i, FontColor);
				//*(SysConPtr->FrameBufferAddr + (Y+i)*SysParam->BytesPerScanLine + X+j) = FontColor;
			}
			else
			{
				SysConPtr->FbDevice->DrawPoint(SysConPtr->FbDevice, X+j, Y+i, BackColor);
				//*(SysConPtr->FrameBufferAddr + (Y+i)*SysParam->BytesPerScanLine + X+j) = BackColor;
			}
		}
	}
}

extern UINT32 DispDriverReady;

void ScrollScreen(CONSOLE * SysConPtr, UINT64 LineCnt)
{
	int i,j,x,y,line;
	int CharLimit = 16384;//SysConPtr->Width/8*SysConPtr->Height/16;
	int LineRewriteLimit = SysConPtr->Height / SysConPtr->FontHeight - 1;
	int StartChar;
	int BlankStartY = SysConPtr->Height / SysConPtr->FontHeight - 1;

	for(StartChar = SysConPtr->FirstLineIndex, i = 0; i < SysConPtr->Width / SysConPtr->FontWidth; StartChar++, i++)
	{
		if('\n' == SysConPtr->CharBuffer[StartChar%CharLimit])
		{
			StartChar++;
			break;
		}
	}
	
	SysConPtr->FirstLineIndex = StartChar%CharLimit;

	GetXY(SysConPtr, &x, &y);
	SetXY(SysConPtr, 0, 0);
	
	UINT64 WriteAcc = SysConPtr->FbDevice->Device == NULL ? 0 : 1;
	RECTANGLE Src;
	RECTANGLE Dst;
	UINT64 tmp;

	if (WriteAcc)
	{
		Dst.PosX = 0;
		Dst.PosY = 0;
		Dst.Width = SysConPtr->Width;
		Dst.Height = SysConPtr->Height - SysConPtr->FontHeight;

		Src.PosX = 0;
		Src.PosY = SysConPtr->FontHeight;
		Src.Width = SysConPtr->Width;
		Src.Height = SysConPtr->Height - SysConPtr->FontHeight;

		SysConPtr->FbDevice->CopyArea(SysConPtr->FbDevice, &Src, &Dst);

		SysConPtr->CursorY = SysConPtr->Height - SysConPtr->FontHeight;
	}
	else
	{
		for (i = StartChar, line = 0; line < LineRewriteLimit; i++)
		{
			switch (SysConPtr->CharBuffer[i%CharLimit])
			{
			case '\n':
				for (tmp = SysConPtr->CursorX; tmp<SysConPtr->Width; tmp += SysConPtr->FontWidth)
				{
					PutChar(SysConPtr, ' ', \
						tmp, SysConPtr->CursorY, \
						SysConPtr->FontColor, SysConPtr->BackColor);
				}
				SysConPtr->CursorX = 0;
				SysConPtr->CursorY += SysConPtr->FontHeight;
				line++;
				break;
			default:

				PutChar(SysConPtr,
					SysConPtr->CharBuffer[i%CharLimit], \
					SysConPtr->CursorX, SysConPtr->CursorY, \
					SysConPtr->ColorBuffer[i%CharLimit],
					SysConPtr->BackColor);

				if (SysConPtr->CursorX + SysConPtr->FontWidth * 2 > SysConPtr->Width)
				{
					SysConPtr->CursorX = 0;
					SysConPtr->CursorY += SysConPtr->FontHeight;
					line++;
				}
				else
					SysConPtr->CursorX += SysConPtr->FontWidth;
				break;
			}

		}
	}
	
	for(tmp = 0; tmp < SysConPtr->Width; tmp += SysConPtr->FontWidth)
	{
		PutChar(SysConPtr, ' ',\
			tmp, SysConPtr->CursorY,\
			SysConPtr->FontColor, SysConPtr->BackColor);
	}
	SetXY(SysConPtr, 0, y);

}

void WriteConsole(CONSOLE* SysConPtr, char Char)
{
	int CharLimit = 16384;//SysConPtr->Width/8*SysConPtr->Height/16 - 1;
	SysConPtr->ColorBuffer[(SysConPtr->BufferIndex)%CharLimit] = SysConPtr->FontColor;
	switch(Char)
	{
		case '\n':
			SysConPtr->CursorX = 0;
			SysConPtr->CharBuffer[(SysConPtr->BufferIndex++)%CharLimit] = '\n';
			
			if(SysConPtr->CursorY + SysConPtr->FontHeight * 2 > SysConPtr->Height)
			{
				ScrollScreen(SysConPtr, 1);
			}
			else
			{
				SysConPtr->CursorY += SysConPtr->FontHeight;
			}
			
			break;
			
		default:

			SysConPtr->CharBuffer[(SysConPtr->BufferIndex++)%CharLimit] = Char;
			PutChar(SysConPtr, Char, SysConPtr->CursorX, SysConPtr->CursorY, SysConPtr->FontColor, SysConPtr->BackColor);
			
			if(SysConPtr->CursorX + SysConPtr->FontWidth * 2 > SysConPtr->Width)
			{
				SysConPtr->CursorX = 0;
				//SysConPtr->CharBuffer[(SysConPtr->BufferIndex++)%CharLimit] = '\n';
				if(SysConPtr->CursorY + SysConPtr->FontHeight * 2 >= SysConPtr->Height)
				{
					ScrollScreen(SysConPtr, 1);
				}
				else
				{
					SysConPtr->CursorY += SysConPtr->FontHeight;
				}
			}
			else
				SysConPtr->CursorX += SysConPtr->FontWidth;

			break;
	}
}

char* i2a(UINT64 val, UINT64 base, char ** ps)
{
	UINT64 m = val % base;
	UINT64 q = val / base;
	
	if (q) 
		i2a(q, base, ps);
	
	*(*ps)++ = (m < 10) ? (m + '0') : (m - 10 + 'A');

	return *ps;
}

char* i2a_full(UINT64 val, UINT64 base, UINT64 Digit, char ** ps)
{
	UINT64 m;
	UINT64 q = val;
	UINT64 i = 0;
	(*ps) += (Digit-1);
	
	while(q)
	{
		m = q % base;
		q /= base;
		*(*ps)-- = (m < 10) ? (m + '0') : (m - 10 + 'A');
		i++;
	}
	
	for(;i<Digit;i++)
	{
		*(*ps)-- = '0';
	}
	
	return *ps;
}

int vsprintf(char *buf, const char *fmt, va_list args)
{
	char*	p;
	long long m;

	char	inner_buf[256] = {0};
	char	cs;
	int	align_nr;

	for (p=buf;*fmt;fmt++)
	{
		if (*fmt != '%')
		{
			*p++ = *fmt;
			continue;
		}
		else {		/* a format string begins */
			align_nr = 0;
		}

		fmt++;

		if (*fmt == '%')
		{
			*p++ = *fmt;
			continue;
		}
		else if (*fmt == '0')
		{
			cs = '0';
			fmt++;
		}
		else
		{
			cs = ' ';
		}
		
		while (((unsigned char)(*fmt) >= '0') && ((unsigned char)(*fmt) <= '9'))
		{
			align_nr *= 10;
			align_nr += *fmt - '0';
			fmt++;
		}
		
		char * q = inner_buf;
		memset(q, 0, sizeof(inner_buf));

		switch (*fmt)
		{
		case 'c':
			*q++ = va_arg(args, long long);
			//p_next_arg += 8;
			break;
		case 'x':
			m = va_arg(args, long long);
			i2a(m, 16, &q);
			//p_next_arg += 8;
			break;
		case 'd':
			m = va_arg(args, long long);
			if (m < 0) {
				m = m * (-1);
				*q++ = '-';
			}
			i2a(m, 10, &q);
			break;
		case 's':
			strcpy(q, va_arg(args, char *));
			q += strlen(q);
			break;
		default:
			break;
		}
		int k;
		for (k = 0; k < ((align_nr > strlen(inner_buf)) ? (align_nr - strlen(inner_buf)) : 0); k++)
		{
			*p++ = cs;
		}
		q = inner_buf;
		while (*q)
		{
			*p++ = *q++;
		}
	}

	*p = 0;

	return (p - buf);
}

int printk(const char *fmt, ...)
{
	va_list ap;
	va_start(ap,fmt);
	long long i,j;
	char buf[256] = {0};
	
	i = vsprintf(buf, fmt, ap);
	va_end(ap);
	buf[i] = 0;

	SpinLockIrqSave(&ConsoleSpinLock);
	for (j = 0; j<strlen(buf); j++)
	{
		WriteConsole(&SysCon, buf[j]);

		if (buf[j] == '\n')
		{
			SerialPortWrite('\r');
			SerialPortWrite('\n');
		}
		else
		{
			SerialPortWrite(buf[j]);
		}
	}
	
	SpinUnlockIrqRestore(&ConsoleSpinLock);
	
	return i;
}


int serial_print(const char *fmt, ...)
{
	va_list ap;
	va_start(ap,fmt);
	long long i,j;
	char buf[256] = {0};
	
	SpinLockIrqSave(&ConsoleSpinLock);
	
	i = vsprintf(buf, fmt, ap);
	va_end(ap);
	buf[i] = 0;
	
	for(j=0; j<strlen(buf) ;j++)
	{
		if(buf[j] == '\n')
		{
			SerialPortWrite('\r');
			SerialPortWrite('\n');
		}
		else
			SerialPortWrite(buf[j]);
	}
	
	SpinUnlockIrqRestore(&ConsoleSpinLock);
	
	return i;
}


void ClearScreen(CONSOLE* SysConPtr)
{
	UINT64 i, j;

	SysConPtr->CursorX = 0;
	SysConPtr->CursorY = 0;
	SysConPtr->FirstLineIndex = 0;
	SysConPtr->BufferIndex = 0;

	for(i = 0; i < SysConPtr->Height; i+=SysConPtr->FontHeight)
		for(j = 0; j < SysConPtr->Width; j+=SysConPtr->FontWidth)
			PutChar(SysConPtr, ' ', j, i, SysConPtr->BackColor, SysConPtr->BackColor);
}

UINT32 GetColor(CONSOLE* SysConPtr)
{
	return SysConPtr->FontColor;
}

void SetColor(CONSOLE* SysConPtr, UINT32 FontColor)
{
	SysConPtr->FontColor = FontColor;
}

void GetXY(CONSOLE* SysConPtr, UINT32 *X, UINT32 *Y)
{
	*X = SysConPtr->CursorX;
	*Y = SysConPtr->CursorY;
}

void SetXY(CONSOLE* SysConPtr, UINT32 X, UINT32 Y)
{
	UINT32 i;
	
	if(X>SysConPtr->CursorX && Y==SysConPtr->CursorY)
		for(i=SysConPtr->CursorX; i<X; i+=8)
			WriteConsole(SysConPtr,' ');
	
	SysConPtr->CursorX = X;
	SysConPtr->CursorY = Y;
}

void print_hex(long long i)
{
	char Buffer[20]={0};
	char *p =Buffer;
	int delay;
	int k;
	i2a_full(i,16,16,&p);
	
	for(k=0; k<20&&Buffer[k]!=0; k++)
		WriteConsole(&SysCon,Buffer[k]);
	WriteConsole(&SysCon,'\n');
}

void print_int(long long i)
{
	char Buffer[20]={0};
	char *p =Buffer;
	int delay;
	int k;
	i2a(i,10,&p);
	
	for(k=0; k<20&&Buffer[k]!=0; k++)
		WriteConsole(&SysCon,Buffer[k]);
}

void print_str(char * str)
{
	int i;
	
	for(i=0; str[i]!=0; i++)
		WriteConsole(&SysCon,str[i]);
}

void print_int_at(long long i, UINT32 X, UINT32 Y, UINT32 Color)
{
	char Buffer[20]={0};
	char *p =Buffer;
	int delay;
	int k;
	i2a(i,16,&p);
	
	for(k=0; k<20&&Buffer[k]!=0; k++)
		PutChar(&SysCon,Buffer[k],(X+k)*8,Y*16,Color,SysCon.BackColor);
}
void print_str_at(char * str, UINT32 X, UINT32 Y, UINT32 Color)
{
	int i;
	
	for(i=0; str[i]!=0; i++)
		PutChar(&SysCon,str[i],(X+i)*8,Y*16,Color,SysCon.BackColor);
}
