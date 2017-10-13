/** @file
  Header file for ATA/ATAPI PASS THRU driver.

  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef __ATA_ATAPI_PASS_THRU_H__
#define __ATA_ATAPI_PASS_THRU_H__


#include "IDE.h"
#include "AHCI.h"

typedef struct _ATA_NONBLOCK_TASK ATA_NONBLOCK_TASK;

typedef enum {
  AtaIdeMode,
  AtaAhciMode,
  AtaRaidMode,
  AtaUnknownMode
} ATA_HC_WORK_MODE;

typedef enum {
  IdeCdrom,                  /* ATAPI CDROM */
  IdeHarddisk,               /* Hard Disk */
  PortMultiplier,            /* Port Multiplier */
  IdeUnknown
} ATA_DEVICE_TYPE;



//
// Timeout value which uses 100ns as a unit.
// It means 3 second span.
//
//#define ATA_ATAPI_TIMEOUT           TIMER_PERIOD_SECONDS(3)

#define IS_ALIGNED(addr, size)      (((UINT64) (addr) & (size - 1)) == 0)


#endif
