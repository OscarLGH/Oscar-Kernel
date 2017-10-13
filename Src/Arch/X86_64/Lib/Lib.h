#ifndef _LIB_H
#define _LIB_H

#include "../Task.h"
#include "Type.h"
#include "../Io/IoPorts.h"

void BeepOn(UINT16 Frequency);
void BeepOff();

void SerialPortInit();
void SerialPortWrite(UINT8 Value);
UINT8 SerialPortRead();

void MicroSecondDelay(UINT64 counter);
void MilliSecondDelay(UINT64 counter);



#endif
