/** @file

  The XHCI register operation routines.

Copyright (c) 2011 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Xhci.h"

/**
  Read 1-byte width XHCI capability register.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the 1-byte width capability register.

  @return The register content read.
  @retval If err, return 0xFF.

**/
UINT8
XhcReadCapReg8 (
  IN  USB_XHCI_INSTANCE   *Xhc,
  IN  UINT32              Offset
  )
{
  UINT8                   Data;
  UINT8                   *BarBase;

  BarBase = (UINT8 *)PHYS2VIRT(PciGetBarBase(Xhc->PciDev, XHC_BAR_INDEX));
  Data = BarBase[Offset];

  return Data;
}

/**
  Read 4-bytes width XHCI capability register.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the 4-bytes width capability register.

  @return The register content read.
  @retval If err, return 0xFFFFFFFF.

**/
UINT32
XhcReadCapReg (
  IN  USB_XHCI_INSTANCE   *Xhc,
  IN  UINT32              Offset
  )
{
	UINT32                   Data;
	UINT32                   *BarBase;

	BarBase = (UINT32 *)PHYS2VIRT(PciGetBarBase(Xhc->PciDev, XHC_BAR_INDEX) + Offset);
	Data = * BarBase;

	return Data;
}

/**
  Read 4-bytes width XHCI Operational register.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the 4-bytes width operational register.

  @return The register content read.
  @retval If err, return 0xFFFFFFFF.

**/
UINT32
XhcReadOpReg (
  IN  USB_XHCI_INSTANCE   *Xhc,
  IN  UINT32              Offset
  )
{
	UINT32                   Data;
	UINT32                   *BarBase;

	BarBase = (UINT32 *)PHYS2VIRT(PciGetBarBase(Xhc->PciDev, XHC_BAR_INDEX) + Xhc->CapLength + Offset);
	Data = * BarBase;
	
	return Data;
}

/**
  Write the data to the 4-bytes width XHCI operational register.

  @param  Xhc      The XHCI Instance.
  @param  Offset   The offset of the 4-bytes width operational register.
  @param  Data     The data to write.

**/
VOID
XhcWriteOpReg (
  IN USB_XHCI_INSTANCE    *Xhc,
  IN UINT32               Offset,
  IN UINT32               Data
  )
{
	UINT32                   *BarBase;

	BarBase = (UINT32 *)PHYS2VIRT(PciGetBarBase(Xhc->PciDev, XHC_BAR_INDEX) + Xhc->CapLength + Offset);
	* BarBase = Data;
}

/**
  Write the data to the 2-bytes width XHCI operational register.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the 2-bytes width operational register.
  @param  Data         The data to write.

**/
VOID
XhcWriteOpReg16 (
  IN USB_XHCI_INSTANCE    *Xhc,
  IN UINT32               Offset,
  IN UINT16               Data
  )
{
	UINT16                   *BarBase;

	BarBase = (UINT16 *)PHYS2VIRT(PciGetBarBase(Xhc->PciDev, XHC_BAR_INDEX) + Xhc->CapLength + Offset);
	* BarBase = Data;

}

/**
  Read XHCI door bell register.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the door bell register.

  @return The register content read

**/
UINT32
XhcReadDoorBellReg (
  IN  USB_XHCI_INSTANCE   *Xhc,
  IN  UINT32              Offset
  )
{
	UINT32                   Data;
	UINT32                   * BarBase;

	BarBase = (UINT32 *)PHYS2VIRT(PciGetBarBase(Xhc->PciDev, XHC_BAR_INDEX) + Xhc->DBOff + Offset);
	Data = * BarBase;

	return Data;
}

/**
  Write the data to the XHCI door bell register.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the door bell register.
  @param  Data         The data to write.

**/
VOID
XhcWriteDoorBellReg (
  IN USB_XHCI_INSTANCE    *Xhc,
  IN UINT32               Offset,
  IN UINT32               Data
  )
{
	UINT32                   *BarBase;

	BarBase = (UINT32 *)PHYS2VIRT(PciGetBarBase(Xhc->PciDev, XHC_BAR_INDEX) + Xhc->DBOff + Offset);
	* BarBase = Data;
}

/**
  Read XHCI runtime register.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the runtime register.

  @return The register content read

**/
UINT32
XhcReadRuntimeReg (
  IN  USB_XHCI_INSTANCE   *Xhc,
  IN  UINT32              Offset
  )
{
	UINT32                   Data;
	UINT32                   *BarBase;

	BarBase = (UINT32 *)PHYS2VIRT(PciGetBarBase(Xhc->PciDev, XHC_BAR_INDEX) + Xhc->RTSOff + Offset);
	Data = * BarBase;

	return Data;
}

/**
  Write the data to the XHCI runtime register.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the runtime register.
  @param  Data         The data to write.

**/
VOID
XhcWriteRuntimeReg (
  IN USB_XHCI_INSTANCE    *Xhc,
  IN UINT32               Offset,
  IN UINT32               Data
  )
{
	UINT32                   *BarBase;

	BarBase = (UINT32 *)PHYS2VIRT(PciGetBarBase(Xhc->PciDev, XHC_BAR_INDEX) + Xhc->RTSOff + Offset);
	* BarBase = Data;
}

/**
  Read XHCI extended capability register.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the extended capability register.

  @return The register content read

**/
UINT32
XhcReadExtCapReg (
  IN  USB_XHCI_INSTANCE   *Xhc,
  IN  UINT32              Offset
  )
{
	UINT32                   Data;
	UINT32                   *BarBase;

	if (Xhc->ExtCapRegBase == 0)
	{
		KDEBUG("ERROR XHCI...\n");
		return 0xFFFFFFFF;
	}

	BarBase = (UINT32 *)PHYS2VIRT(PciGetBarBase(Xhc->PciDev, XHC_BAR_INDEX) + Xhc->ExtCapRegBase + Offset);
	Data = *BarBase;

	return Data;
}

/**
  Write the data to the XHCI extended capability register.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the extended capability register.
  @param  Data         The data to write.

**/
VOID
XhcWriteExtCapReg (
  IN USB_XHCI_INSTANCE    *Xhc,
  IN UINT32               Offset,
  IN UINT32               Data
  )
{
	UINT32                   *BarBase;

	if (Xhc->ExtCapRegBase == 0)
	{
		KDEBUG("ERROR XHCI...\n");
	}

	BarBase = (UINT32 *)PHYS2VIRT(PciGetBarBase(Xhc->PciDev, XHC_BAR_INDEX) + Xhc->ExtCapRegBase + Offset);
	* BarBase = Data;
}


/**
  Set one bit of the runtime register while keeping other bits.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the runtime register.
  @param  Bit          The bit mask of the register to set.

**/
VOID
XhcSetRuntimeRegBit (
  IN USB_XHCI_INSTANCE    *Xhc,
  IN UINT32               Offset,
  IN UINT32               Bit
  )
{
  UINT32                  Data;

  Data  = XhcReadRuntimeReg (Xhc, Offset);
  Data |= Bit;
  XhcWriteRuntimeReg (Xhc, Offset, Data);
}

/**
  Clear one bit of the runtime register while keeping other bits.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the runtime register.
  @param  Bit          The bit mask of the register to set.

**/
VOID
XhcClearRuntimeRegBit (
  IN USB_XHCI_INSTANCE    *Xhc,
  IN UINT32               Offset,
  IN UINT32               Bit
  )
{
  UINT32                  Data;

  Data  = XhcReadRuntimeReg (Xhc, Offset);
  Data &= ~Bit;
  XhcWriteRuntimeReg (Xhc, Offset, Data);
}

/**
  Set one bit of the operational register while keeping other bits.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the operational register.
  @param  Bit          The bit mask of the register to set.

**/
VOID
XhcSetOpRegBit (
  IN USB_XHCI_INSTANCE    *Xhc,
  IN UINT32               Offset,
  IN UINT32               Bit
  )
{
  UINT32                  Data;

  Data  = XhcReadOpReg (Xhc, Offset);
  Data |= Bit;
  XhcWriteOpReg (Xhc, Offset, Data);
}


/**
  Clear one bit of the operational register while keeping other bits.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the operational register.
  @param  Bit          The bit mask of the register to clear.

**/
VOID
XhcClearOpRegBit (
  IN USB_XHCI_INSTANCE    *Xhc,
  IN UINT32               Offset,
  IN UINT32               Bit
  )
{
  UINT32                  Data;

  Data  = XhcReadOpReg (Xhc, Offset);
  Data &= ~Bit;
  XhcWriteOpReg (Xhc, Offset, Data);
}

/**
  Wait the operation register's bit as specified by Bit
  to become set (or clear).

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the operation register.
  @param  Bit          The bit of the register to wait for.
  @param  WaitToSet    Wait the bit to set or clear.
  @param  Timeout      The time to wait before abort (in millisecond, ms).

  @retval EFI_SUCCESS  The bit successfully changed by host controller.
  @retval EFI_TIMEOUT  The time out occurred.

**/
RETURN_STATUS
XhcWaitOpRegBit (
  IN USB_XHCI_INSTANCE    *Xhc,
  IN UINT32               Offset,
  IN UINT32               Bit,
  IN BOOLEAN              WaitToSet,
  IN UINT32               Timeout
  )
{
  UINT32                  Index;
  UINT64                  Loop;

  Loop   = Timeout * XHC_1_MILLISECOND;

  for (Index = 0; Index < Loop; Index++) {
    if (XHC_REG_BIT_IS_SET (Xhc, Offset, Bit) == WaitToSet) {
      return RETURN_SUCCESS;
    }

    MicroSecondDelay (XHC_1_MICROSECOND);
  }

  return RETURN_TIMEOUT;
}

/**
  Set Bios Ownership

  @param  Xhc          The XHCI Instance.

**/
VOID
XhcSetBiosOwnership (
  IN USB_XHCI_INSTANCE    *Xhc
  )
{
  UINT32                    Buffer;

  if (Xhc->UsbLegSupOffset == 0xFFFFFFFF) {
    return;
  }

  KDEBUG ("XhcSetBiosOwnership: called to set BIOS ownership\n");

  Buffer = XhcReadExtCapReg (Xhc, Xhc->UsbLegSupOffset);
  Buffer = ((Buffer & (~USBLEGSP_OS_SEMAPHORE)) | USBLEGSP_BIOS_SEMAPHORE);
  XhcWriteExtCapReg (Xhc, Xhc->UsbLegSupOffset, Buffer);
}

/**
  Clear Bios Ownership

  @param  Xhc       The XHCI Instance.

**/
VOID
XhcClearBiosOwnership (
  IN USB_XHCI_INSTANCE    *Xhc
  )
{
  UINT32                    Buffer;

  if (Xhc->UsbLegSupOffset == 0xFFFFFFFF) {
    return;
  }

  KDEBUG ("XhcClearBiosOwnership: called to clear BIOS ownership\n");

  Buffer = XhcReadExtCapReg (Xhc, Xhc->UsbLegSupOffset);
  Buffer = ((Buffer & (~USBLEGSP_BIOS_SEMAPHORE)) | USBLEGSP_OS_SEMAPHORE);
  XhcWriteExtCapReg (Xhc, Xhc->UsbLegSupOffset, Buffer);
}

/**
  Calculate the offset of the XHCI capability.

  @param  Xhc     The XHCI Instance.
  @param  CapId   The XHCI Capability ID.

  @return The offset of XHCI legacy support capability register.

**/
UINT32
XhcGetCapabilityAddr (
  IN USB_XHCI_INSTANCE    *Xhc,
  IN UINT8                CapId
  )
{
  UINT32 ExtCapOffset;
  UINT8  NextExtCapReg;
  UINT32 Data;

  ExtCapOffset = 0;

  do {
    //
    // Check if the extended capability register's capability id is USB Legacy Support.
    //
    Data = XhcReadExtCapReg (Xhc, ExtCapOffset);
    if ((Data & 0xFF) == CapId) {
      return ExtCapOffset;
    }
    //
    // If not, then traverse all of the ext capability registers till finding out it.
    //
    NextExtCapReg = (UINT8)((Data >> 8) & 0xFF);
    ExtCapOffset += (NextExtCapReg << 2);
  } while (NextExtCapReg != 0);

  return 0xFFFFFFFF;
}

/**
  Whether the XHCI host controller is halted.

  @param  Xhc     The XHCI Instance.

  @retval TRUE    The controller is halted.
  @retval FALSE   It isn't halted.

**/
BOOLEAN
XhcIsHalt (
  IN USB_XHCI_INSTANCE    *Xhc
  )
{
  return XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HALT);
}


/**
  Whether system error occurred.

  @param  Xhc      The XHCI Instance.

  @retval TRUE     System error happened.
  @retval FALSE    No system error.

**/
BOOLEAN
XhcIsSysError (
  IN USB_XHCI_INSTANCE    *Xhc
  )
{
  return XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HSE);
}

/**
  Reset the XHCI host controller.

  @param  Xhc          The XHCI Instance.
  @param  Timeout      Time to wait before abort (in millisecond, ms).

  @retval EFI_SUCCESS  The XHCI host controller is reset.
  @return Others       Failed to reset the XHCI before Timeout.

**/
RETURN_STATUS
XhcResetHC (
  IN USB_XHCI_INSTANCE    *Xhc,
  IN UINT32               Timeout
  )
{
  RETURN_STATUS              Status;

  Status = RETURN_SUCCESS;

  //KDEBUG ("XhcResetHC!\n");
  //
  // Host can only be reset when it is halt. If not so, halt it
  //
  if (!XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HALT)) {
    Status = XhcHaltHC (Xhc, Timeout);

    if (RETURN_ERROR (Status)) {
      return Status;
    }
  }

  if ((Xhc->DebugCapSupOffset == 0xFFFFFFFF) || ((XhcReadExtCapReg (Xhc, Xhc->DebugCapSupOffset) & 0xFF) != XHC_CAP_USB_DEBUG) ||
      ((XhcReadExtCapReg (Xhc, Xhc->DebugCapSupOffset + XHC_DC_DCCTRL) & BIT0) == 0)) {
    XhcSetOpRegBit (Xhc, XHC_USBCMD_OFFSET, XHC_USBCMD_RESET);
    Status = XhcWaitOpRegBit (Xhc, XHC_USBCMD_OFFSET, XHC_USBCMD_RESET, FALSE, Timeout);
  }

  return Status;
}


/**
  Halt the XHCI host controller.

  @param  Xhc          The XHCI Instance.
  @param  Timeout      Time to wait before abort (in millisecond, ms).

  @return EFI_SUCCESS  The XHCI host controller is halt.
  @return EFI_TIMEOUT  Failed to halt the XHCI before Timeout.

**/
RETURN_STATUS
XhcHaltHC (
  IN USB_XHCI_INSTANCE   *Xhc,
  IN UINT32              Timeout
  )
{
  RETURN_STATUS              Status;

  XhcClearOpRegBit (Xhc, XHC_USBCMD_OFFSET, XHC_USBCMD_RUN);
  Status = XhcWaitOpRegBit (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HALT, TRUE, Timeout);
  return Status;
}


/**
  Set the XHCI host controller to run.

  @param  Xhc          The XHCI Instance.
  @param  Timeout      Time to wait before abort (in millisecond, ms).

  @return EFI_SUCCESS  The XHCI host controller is running.
  @return EFI_TIMEOUT  Failed to set the XHCI to run before Timeout.

**/
RETURN_STATUS
XhcRunHC (
  IN USB_XHCI_INSTANCE    *Xhc,
  IN UINT32               Timeout
  )
{
  RETURN_STATUS              Status;

  XhcSetOpRegBit (Xhc, XHC_USBCMD_OFFSET, XHC_USBCMD_RUN);
  Status = XhcWaitOpRegBit (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HALT, FALSE, Timeout);
  return Status;
}

