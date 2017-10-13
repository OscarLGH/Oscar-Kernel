#include "Lib.h"

void BeepOn(UINT16 Frequency)
{
	UINT8 BeepGate = In8(0x61) | 0x03;
	UINT16 BeepFrequency = 0x123280/Frequency;
	Out8(0x43, 0xB6);
	Out8(0x42,BeepFrequency);
	Out8(0x42,BeepFrequency>>8);
	Out8(0x61,BeepGate);
}

void BeepOff()
{
	UINT8 BeepGate = In8(0x61) & 0xFC;	 
	Out8(0x61,BeepGate);
}