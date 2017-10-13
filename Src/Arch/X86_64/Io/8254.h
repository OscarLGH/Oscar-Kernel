#include "Type.h"
#include "Timer.h"
#include "IoPorts.h"
#include "InitCall.h"
#include "Irq.h"


#define I8254_COUNTER0_PORT 0x40
#define I8254_COUNTER1_PORT 0x41
#define I8254_COUNTER2_PORT 0x42
#define I8254_MODE_CONTROL_PORT 0x43

#define I8254_CW_SEL_COUNTER(x) (((x) & 0x3) << 6)
#define I8254_CW_LATCH 0
#define I8254_Cw_RMSB (1 << 4)
#define I8254_CW_RLSB (2 << 4)
#define I8254_CW_RLM (3 << 4)
#define I8254_CW_MODE0 (0 << 1)
#define I8254_CW_MODE1 (1 << 1)
#define I8254_CW_MODE2 (2 << 1)
#define I8254_CW_MODE3 (3 << 1)
#define I8254_CW_MODE4 (4 << 1)
#define I8254_CW_MODE5 (5 << 1)

#define I8254_CW_BIN 0
#define I8254_CW_BCD 1







