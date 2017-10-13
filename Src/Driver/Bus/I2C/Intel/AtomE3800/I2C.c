#include "I2CRegs.h"


RETURN_STATUS AtomE3800I2CSupported(DEVICE_NODE * Dev)
{	
	PCI_DEVICE * PciDev = Dev->Device;
	if(Dev->BusType != BUS_TYPE_PCI)
		return RETURN_UNSUPPORTED;
	
	//if(Dev->DeviceType != 0)
	//	return RETURN_UNSUPPORTED;

	if(INTEL_VENDOR_ID != PciCfgRead16(PciDev, PCI_CONF_VENDORID_OFF_16))
		return RETURN_UNSUPPORTED;
	
	if(0x0f46 != PciCfgRead16(PciDev, PCI_CONF_DEVICEID_OFF_16))
		return RETURN_UNSUPPORTED;
	
	KDEBUG("Intel I2C driver supported on device:%02x:%02x.%02x\n", 
		PCI_GET_BUS(Dev->Position),
		PCI_GET_DEVICE(Dev->Position),
		PCI_GET_FUNCTION(Dev->Position));
	
	I2CRegister(Dev);
	
	return RETURN_SUCCESS;
}


RETURN_STATUS AtomE3800I2CStart(DEVICE_NODE * Dev)
{
	PCI_DEVICE * PciDev = Dev->Device;
	KDEBUG("Intel I2C driver starting...\n"); 

	UINT16 PCICommand = PciCfgRead16(PciDev, PCI_CONF_COMMAND_OFF_16);
	PCICommand |= 0x2;
	PciCfgWrite16(PciDev, PCI_CONF_COMMAND_OFF_16, PCICommand);

	return RETURN_SUCCESS;
}

UINT32 AtomE3800I2CRegRead(I2CNODE * Dev, UINT32 Offset)
{
	PCI_DEVICE * PciDev = Dev->Device->Device;
	UINT64 ABAR = PHYS2VIRT(PciCfgRead32(PciDev, PCI_CONF_BAR0_OFF_32) & (~0xF));
	//KDEBUG("ABAR = %016x\n",ABAR);
	volatile UINT32 * Reg = (UINT32 *)(ABAR + Offset);
	//KDEBUG("I2C Reg:Offset %08x,Value:%08x\n",Offset, *Reg);
	return *Reg;
}

void AtomE3800I2CRegWrite(I2CNODE * Dev, UINT32 Offset, UINT32 Value)
{
	PCI_DEVICE * PciDev = Dev->Device->Device;
	UINT64 ABAR = PHYS2VIRT(PciCfgRead32(PciDev, PCI_CONF_BAR0_OFF_32) & (~0xF));
	//KDEBUG("ABAR = %016x\n",ABAR);
	volatile UINT32 * Reg = (UINT32 *)(ABAR + Offset);
	*Reg = Value;
	//KDEBUG("I2C Reg:Offset %08x,Value:%08x\n",Offset, *Reg);
}


RETURN_STATUS AtomE3800I2CEnable(IN I2CNODE * Dev)
{
  //
  // 0.1 seconds
  //
  UINT32 NumTries = 10000;
  
  AtomE3800I2CRegWrite(Dev, R_IC_ENABLE, 1);
  while ( 0 == ( AtomE3800I2CRegRead(Dev, R_IC_ENABLE_STATUS) & 1)) {
    MilliSecondDelay (10);
    NumTries --;
    if(0 == NumTries) {
      return RETURN_NOT_READY;
    }
  }
  KDEBUG("E3800 I2C Enabled.\n");
  return RETURN_SUCCESS;
}

RETURN_STATUS AtomE3800I2CDisable(IN I2CNODE * Dev)
{
  //
  // 0.1 seconds
  //
  UINT32 NumTries = 10000;
  
  AtomE3800I2CRegWrite(Dev, R_IC_ENABLE, 0);
  
  while (0 != (AtomE3800I2CRegRead(Dev, R_IC_ENABLE_STATUS) & 1)) {
    MilliSecondDelay (10);
    NumTries --;
    if(0 == NumTries){
       return RETURN_NOT_READY;
    }
  }
  KDEBUG("E3800 I2C Disabled.\n");
  return RETURN_SUCCESS;
}


RETURN_STATUS AtomE3800I2CBusFrequencySet (
  IN I2CNODE * Dev,
  IN UINT64 BusClockHertz
  )
{
  KDEBUG("InputFreq BusClockHertz: %d\n", BusClockHertz);
  
  //
  //  Set the 100 KHz clock divider according to SV result and I2C spec
  //
  AtomE3800I2CRegWrite(Dev, R_IC_SS_SCL_HCNT, (UINT16)0x214 );
  AtomE3800I2CRegWrite(Dev, R_IC_SS_SCL_LCNT, (UINT16)0x272 );
  
  //
  //  Set the 400 KHz clock divider according to SV result and I2C spec
  //
  AtomE3800I2CRegWrite(Dev, R_IC_FS_SCL_HCNT, (UINT16)0x50 );
  AtomE3800I2CRegWrite(Dev, R_IC_FS_SCL_LCNT, (UINT16)0xAD );

  switch ( BusClockHertz ) {
    case 100 * 1000:
      AtomE3800I2CRegWrite(Dev, R_IC_SDA_HOLD, (UINT16)0x40);//100K
      //mI2cMode = V_SPEED_STANDARD;
      break;
    case 400 * 1000:
      AtomE3800I2CRegWrite(Dev, R_IC_SDA_HOLD, (UINT16)0x32);//400K
      //mI2cMode = V_SPEED_FAST;
      break;
    default:
      AtomE3800I2CRegWrite(Dev, R_IC_SDA_HOLD, (UINT16)0x09);//3.4M
      //mI2cMode = V_SPEED_HIGH;
  }
  
  return RETURN_SUCCESS;
}

RETURN_STATUS AtomE3800I2CInit(IN I2CNODE * Dev, IN UINT32 SlaveAddress)
{
	RETURN_STATUS Status = RETURN_SUCCESS;
	
	if (SlaveAddress > 1023) 
	{
    	Status =  RETURN_INVALID_PARAMETER;
	
    	KDEBUG("I2CInit Exit with RETURN_INVALID_PARAMETER\n");
    	return Status;
  	}
	
	AtomE3800I2CRegWrite(Dev, R_IC_TAR, (UINT16)SlaveAddress);

	if(Dev->InitStatus == 1)
	{
		return RETURN_SUCCESS;
	}
	else
	{
		Dev->InitStatus = 1;
	}
	
	UINT64 NumTries = 100; 
    while ((1 == (AtomE3800I2CRegRead(Dev, R_IC_STATUS) & STAT_MST_ACTIVITY))) {
    	MilliSecondDelay(10);
    NumTries --;
    if(0 == NumTries) {
      KDEBUG("Try timeout\n");
      return RETURN_DEVICE_ERROR;
    }
  }

  Status = AtomE3800I2CDisable(Dev);
  KDEBUG("I2cDisable Status = %x\n", Status);
  
  AtomE3800I2CRegWrite(Dev, R_IC_INTR_MASK, 0x0);
  
  if (0x7f < SlaveAddress )
    SlaveAddress = ( SlaveAddress & 0x3ff ) | IC_TAR_10BITADDR_MASTER;
  
  AtomE3800I2CRegWrite(Dev, R_IC_TAR, (UINT16)SlaveAddress );

  AtomE3800I2CBusFrequencySet(Dev, 400 * 1000);
  AtomE3800I2CRegWrite(Dev, R_IC_RX_TL, 0);
  AtomE3800I2CRegWrite(Dev, R_IC_TX_TL, 0 );
  AtomE3800I2CRegWrite(Dev, R_IC_CON, V_SPEED_FAST| B_IC_RESTART_EN | B_IC_SLAVE_DISABLE | B_MASTER_MODE);
  
  Status = AtomE3800I2CEnable(Dev);

  KDEBUG("I2cEnable Status = %x\n", Status);
  AtomE3800I2CRegRead(Dev, R_IC_CLR_TX_ABRT );
  
  return RETURN_SUCCESS;
}


RETURN_STATUS AtomE3800I2CReadByte(IN I2CNODE * Dev, UINT32 SlaveAddress, UINT8* Byte, UINT8 Start, UINT8 End)
{
	RETURN_STATUS Status;
  	UINT32 I2cStatus;
  	UINT16 RawIntrStat;

  	Status = RETURN_SUCCESS;

	KDEBUG("Reading I2C:\n");

	if(End && Start) {
	    AtomE3800I2CRegWrite(Dev, R_IC_DATA_CMD, B_READ_CMD|B_CMD_RESTART|B_CMD_STOP);
	} else if (!End && Start) {
	    AtomE3800I2CRegWrite(Dev, R_IC_DATA_CMD, B_READ_CMD|B_CMD_RESTART);
	} else if (End && !Start) {
	    AtomE3800I2CRegWrite(Dev, R_IC_DATA_CMD, B_READ_CMD|B_CMD_STOP);
	} else if (!End && !Start) {
	    AtomE3800I2CRegWrite(Dev, R_IC_DATA_CMD, B_READ_CMD);
	}
	MilliSecondDelay (FIFO_WRITE_DELAY);

	//
	//  Check for NACK
	//
	RawIntrStat = (UINT16)AtomE3800I2CRegRead(Dev, R_IC_RawIntrStat);
	if ( 0 != ( RawIntrStat & I2C_INTR_TX_ABRT )) {
	    AtomE3800I2CRegRead(Dev, R_IC_CLR_TX_ABRT );
	    Status = RETURN_DEVICE_ERROR;
		KDEBUG("TX ABRT,byte hasn't been transferred\n");
	    return Status;
	}

	I2cStatus = (UINT16)AtomE3800I2CRegRead(Dev, R_IC_STATUS);
	if(0 != ( I2cStatus & STAT_RFNE )) {
		*Byte = (UINT16)AtomE3800I2CRegRead(Dev, R_IC_DATA_CMD );
		KDEBUG("1 byte 0x%02x is received\n",*Byte);
	}

 	return Status;
}

RETURN_STATUS AtomE3800I2CWriteByte(IN I2CNODE * Dev, UINT32 SlaveAddress, UINT8 Byte, UINT8 Start, UINT8 End)
{
	RETURN_STATUS Status;

	UINT32 I2cStatus;
    UINT16 RawIntrStat;

	KDEBUG("Writing I2C...\n");
	
	Status=AtomE3800I2CInit(Dev, SlaveAddress);
	
    if(Status != RETURN_SUCCESS)
    	return Status;
		
    UINT32 Count=0;

    I2cStatus = AtomE3800I2CRegRead(Dev, R_IC_STATUS);
    RawIntrStat = (UINT16)AtomE3800I2CRegRead(Dev, R_IC_RawIntrStat);
	
    if (0 != ( RawIntrStat & I2C_INTR_TX_ABRT)) {
        AtomE3800I2CRegRead(Dev, R_IC_CLR_TX_ABRT);
        Status = RETURN_DEVICE_ERROR;
		KDEBUG("TX ABRT\n");
        return Status;
    }
	
    while(0 == (I2cStatus & STAT_TFNF)) 
	{
        //
        // If TX not full , will  send cmd  or continue to wait
        //
        MilliSecondDelay (FIFO_WRITE_DELAY);
    }

	KDEBUG("Sending byte:%02x\n", Byte);

	
	//MilliSecondDelay (FIFO_WRITE_DELAY);

    if(End && Start) {
        AtomE3800I2CRegWrite(Dev, R_IC_DATA_CMD, Byte|B_CMD_RESTART|B_CMD_STOP);
    } else if (!End && Start) {
        AtomE3800I2CRegWrite(Dev, R_IC_DATA_CMD, Byte|B_CMD_RESTART);
    } else if (End && !Start) {
        AtomE3800I2CRegWrite(Dev, R_IC_DATA_CMD, Byte|B_CMD_STOP);
    } else if (!End && !Start ) {
        AtomE3800I2CRegWrite(Dev, R_IC_DATA_CMD, Byte);
    }
      
    //
    // Add a small delay to work around some odd behavior being seen.  Without this delay bytes get dropped.
    //
    MilliSecondDelay ( FIFO_WRITE_DELAY );//wait after send cmd

    //
    // Time out
    //
    while(1) {
        RawIntrStat = (UINT16)AtomE3800I2CRegRead(Dev, R_IC_RawIntrStat );
        if (0 != ( RawIntrStat & I2C_INTR_TX_ABRT)) {
            (UINT16)AtomE3800I2CRegRead(Dev, R_IC_CLR_TX_ABRT);
            Status = RETURN_DEVICE_ERROR;
		    KDEBUG("TX ABRT\n");
        }
        if(0 == (UINT16)AtomE3800I2CRegRead(Dev, R_IC_TXFLR)) 
			break;

        MilliSecondDelay (FIFO_WRITE_DELAY);
        Count++;
        if(Count<1024) {
          //
          // to avoid sys hung without ul-pmc device on RVP.
          // Waiting the last request to get data and make (ReceiveDataEnd > ReadBuffer) =TRUE.
          //
          continue;
        } else {
        	Status = RETURN_TIMEOUT;
         	break;
        }
    }//while( 1 )
      
    return Status;
}

RETURN_STATUS AtomE3800I2CReadBuffer(IN I2CNODE * Dev, UINT32 SlaveAddress, UINT8* Buffer, UINT32 Length)
{
	RETURN_STATUS Status;
	UINT64 Index = 0;

	if(Length == 0 || Buffer == NULL || Dev == NULL)
		return RETURN_INVALID_PARAMETER;

	Status = AtomE3800I2CReadByte(Dev, SlaveAddress, Buffer++, 1, 0);
	
	if(Status)
		return Status;
	
	Index++;
	
	while(Index < Length - 1)
	{
		Status = AtomE3800I2CReadByte(Dev, SlaveAddress, Buffer++, 1, 0);
		
		if(Status)
			return Status;
		
		Index++;
		MilliSecondDelay (FIFO_WRITE_DELAY);
	}

	Status = AtomE3800I2CReadByte(Dev, SlaveAddress, Buffer, 1, 1);
	
	return Status;
}

RETURN_STATUS AtomE3800I2CWriteBuffer(IN I2CNODE * Dev, UINT32 SlaveAddress, UINT8* Buffer, UINT32 Length)
{
	RETURN_STATUS Status;
	UINT64 Index = 0;

	if(Length == 0 || Buffer == NULL || Dev == NULL)
		return RETURN_INVALID_PARAMETER;

	Status = AtomE3800I2CWriteByte(Dev, SlaveAddress, *Buffer++, 1, 0);
	
	if(Status)
		return Status;
	
	Index++;
	
	while(Index < Length - 1)
	{
		Status = AtomE3800I2CWriteByte(Dev, SlaveAddress, *Buffer++, 1, 0);
		
		if(Status)
			return Status;
		
		Index++;
		MilliSecondDelay (FIFO_WRITE_DELAY);
	}

	Status = AtomE3800I2CWriteByte(Dev, SlaveAddress, *Buffer++, 1, 1);

	return Status;
}

I2CDRIVER AtomE3800I2COperations = 
{
	.I2COpen = 0,
	//.I2CRead = AtomE3800I2CRead,
	.I2CRead = AtomE3800I2CReadBuffer,
	.I2CWrite = AtomE3800I2CWriteBuffer,
	//.I2CWrite = AtomE3800I2CWrite,
	.I2CClose = 0
};

DRIVER_NODE AtomE3800I2CDriver = 
{
	.BusType = BUS_TYPE_PCI,
	.DriverSupported = AtomE3800I2CSupported,
	.DriverStart = AtomE3800I2CStart,
	.DriverStop = NULL,
	.Operations = &AtomE3800I2COperations,
	.PrevDriver = 0,
	.NextDriver = 0
};

static void AtomE3800Init(void)
{
	DriverRegister(&AtomE3800I2CDriver, &DriverList[DEVICE_TYPE_SERIAL_BUS]);
}

MODULE_INIT(AtomE3800Init);



