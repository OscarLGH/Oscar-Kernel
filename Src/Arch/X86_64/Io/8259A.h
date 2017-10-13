#include "Type.h"
#include "IoPorts.h"
#include "Irq.h"
#include "InitCall.h"

#define I8259A_MASTER_ICW1_PORT 0x20
#define I8259A_MASTER_OCW2_PORT 0x20
#define I8259A_MASTER_OCW3_PORT 0x20
#define I8259A_MASTER_ICW2_PORT 0x21
#define I8259A_MASTER_ICW3_PORT 0x21
#define I8259A_MASTER_ICW4_PORT 0x21
#define I8259A_MASTER_OCW1_PORT 0x21

#define I8259A_SLAVE_ICW1_PORT 0xA0
#define I8259A_SLAVE_OCW2_PORT 0xA0
#define I8259A_SLAVE_OCW3_PORT 0xA0
#define I8259A_SLAVE_ICW2_PORT 0xA1
#define I8259A_SLAVE_ICW3_PORT 0xA1
#define I8259A_SLAVE_ICW4_PORT 0xA1
#define I8259A_SLAVE_OCW1_PORT 0xA1


#define MASTER_ELCR1_PORT 0x4d0
#define SLAVE_ELCR2_PORT 0x4d1

#define ICW1_FIXED 0x11
#define ICW2(Vector,Irq) (((Vector) & (0xf8)) | ((Irq) & 0x7))

#define ICW3_MASTER_FIXED 0x4
#define ICW3_SLAVE_FIXED  0x2

#define ICW4_AEOI 0x2
#define ICW4_MASTER_SFNM 0x10
#define ICW4_FIXED 0x1

#define OCW2_R 0x80
#define OCW2_SL 0x40
#define OCW2_EOI 0x20
#define OCw3_SMM 0x40
#define OCW3_ESMM 0x20
#define OCW3_POLL 0x4
#define OCW3_READ_IRR 0x2
#define OCW3_READ_ISR 0x3
#define OCW3_FIXED 0x8

RETURN_STATUS Init8259A();
RETURN_STATUS Enable8259Irq(UINT32 Irq);
RETURN_STATUS Disable8259Irq(UINT32 Irq);
RETURN_STATUS Remapping8259Irq(UINT64 Irq, UINT64 Vector, UINT64 Cpu);