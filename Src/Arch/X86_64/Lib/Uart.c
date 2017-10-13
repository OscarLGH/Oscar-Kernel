#include "Lib.h"

//---------------------------------------------
// UART Register Offsets
//---------------------------------------------
#define BAUD_LOW_OFFSET         0x00
#define BAUD_HIGH_OFFSET        0x01
#define IER_OFFSET              0x01
#define LCR_SHADOW_OFFSET       0x01
#define FCR_SHADOW_OFFSET       0x02
#define IR_CONTROL_OFFSET       0x02
#define FCR_OFFSET              0x02
#define EIR_OFFSET              0x02
#define BSR_OFFSET              0x03
#define LCR_OFFSET              0x03
#define MCR_OFFSET              0x04
#define LSR_OFFSET              0x05
#define MSR_OFFSET              0x06

//---------------------------------------------
// UART Register Bit Defines
//---------------------------------------------
#define LSR_TXRDY               0x20
#define LSR_RXDA                0x01
#define DLAB                    0x01

//---------------------------------------------
// UART Settings
//---------------------------------------------
UINT16  gUartBase = 0x3F8;
UINT32  gBps      = 115200;
UINT8   gData     = 8;
UINT8   gStop     = 1;
UINT8   gParity   = 0;
UINT8   gBreakSet = 0;


void SerialPortInit()
{
	UINT32 Divisor;
  	UINT8  OutputData;
  	UINT8  Data;

  	//
  	// Map 5..8 to 0..3
  	//
  	Data = (UINT8) (gData - (UINT8) 5);

  	//
  	// Calculate divisor for baud generator
  	//
  	Divisor = 115200 / gBps;

  	//
  	// Set communications format
  	//
  	OutputData = (UINT8) ((DLAB << 7) | ((gBreakSet << 6) | ((gParity << 3) | ((gStop << 2) | Data))));
  	Out8 (gUartBase + LCR_OFFSET, OutputData);

  	//
  	// Configure baud rate
  	//
  	Out8 (gUartBase + BAUD_HIGH_OFFSET, (UINT8) (Divisor >> 8));
  	Out8 (gUartBase + BAUD_LOW_OFFSET, (UINT8) (Divisor & 0xff));

  	//
  	// Switch back to bank 0
  	//
  	OutputData = (UINT8) ((~DLAB << 7) | ((gBreakSet << 6) | ((gParity << 3) | ((gStop << 2) | Data))));
  	Out8 (gUartBase + LCR_OFFSET, OutputData);
}

void SerialPortWrite(UINT8 Value)
{
	UINT8 Data;
	//do {
      Data = In8 ((UINT16) gUartBase + LSR_OFFSET);
    //} while ((Data & LSR_TXRDY) == 0);
	
    Out8 ((UINT16) gUartBase, Value);
}

UINT8 SerialPortRead()
{
	UINT8 Data;
	do {
      Data = In8 ((UINT16) gUartBase + LSR_OFFSET);
    } while ((Data & LSR_RXDA) == 0);

    return In8 ((UINT16) gUartBase);
}
