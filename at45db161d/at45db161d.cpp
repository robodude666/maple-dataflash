#include "at45db161d.h"

 
/* De-assert CS */
#define DF_CS_inactive digitalWrite(m_chipSelectPin,HIGH)
/* Assert CS */
#define DF_CS_active digitalWrite(m_chipSelectPin,LOW)

/** CTOR **/
ATD45DB161D::ATD45DB161D() :
	m_chipSelectPin   (SLAVESELECT),
	m_resetPin        (RESET),
	m_writeProtectPin (WP),
	m_controlRegister (0)
{}
/** DTOR **/
ATD45DB161D::~ATD45DB161D()
{}
	
/** 
 * Setup pinout and set SPI configuration
 * @param csPin Chip select (Slave select) pin (CS)
 * @param resetPin Reset pin (RESET)
 * @param wpPin Write protect pin (WP)
 * **/
void ATD45DB161D::begin(uint8_t csPin, uint8_t resetPin, uint8_t wpPin)
{
	uint8_t clr;
	
	m_chipSelectPin   = csPin;
	m_resetPin        = resetPin;
	m_writeProtectPin = wpPin;
	
	pinMode(m_chipSelectPin,   OUTPUT);
	pinMode(m_resetPin,        OUTPUT);
	pinMode(m_writeProtectPin, OUTPUT);
	
	digitalWrite(m_resetPin,        HIGH);
	digitalWrite(m_writeProtectPin, LOW);

	/* Backup registers */
	m_SPCR = SPCR;
	
	/* Clear pending SPI interrupts */
	clr = SPSR;
	clr = SPDR;
	
	/* Keep on backuping */
	m_SPSR = (SPSR & SPI_2XCLOCK_MASK);
	
	/* Setup SPI */
	SPI.setDataMode(SPI_MODE3);
	SPI.setBitOrder(MSBFIRST);
	SPI.setClockDivider(SPI_CLOCK_DIV2);
	
	/* Enable device */
  	DF_CS_active;
}

/**
 * Disable device and restore SPI configuration
 **/
void ATD45DB161D::end()
{
	/* Disable device */
  	DF_CS_inactive;

	/* Restore SPI registers */
	SPCR = m_SPCR;
	SPSR = m_SPSR;
}

/** 
 * Read status register
 * @return The content of the status register
 **/
uint8_t ATD45DB161D::ReadStatusRegister()
{
	uint8_t status;

	DF_CS_inactive;    /* Make sure to toggle CS signal in order */
	DF_CS_active;      /* to reset Dataflash command decoder     */
  
    /* Send status read command */
	SPI.transfer(AT45DB161D_STATUS_REGISTER_READ);
	/* Get result with a dummy write */
	status = SPI.transfer(0x00);

	return status;
}

/** 
 * Read Manufacturer and Device ID 
 * @note if id.extendedInfoLength is not equal to zero,
 *       successive calls to SPI.transfer(0xff) will return
 *       the extended device information string bytes.
 * @param id Pointer to the ID structure to initialize
 **/
void ATD45DB161D::ReadManufacturerAndDeviceID(struct ATD45DB161D::ID *id)
{
	
	DF_CS_inactive;    /* Make sure to toggle CS signal in order */
	DF_CS_active;      /* to reset Dataflash command decoder     */
  
    /* Send status read command */
	SPI.transfer(AT45DB161D_READ_MANUFACTURER_AND_DEVICE_ID);

	/* Manufacturer ID */
	id->manufacturer = SPI.transfer(0xff);
	/* Device ID (part 1) */
	id->device[0] = SPI.transfer(0xff);
	/* Device ID (part 2) */
	id->device[1] = SPI.transfer(0xff);
	/* Extended Device Information String Length */
	id->extendedInfoLength = SPI.transfer(0xff);
	
}

/** 
 * Main Memory Page Read. 
 * A main memory page read allows the user to read data directly from
 * any one of the 4096 pages in the main memory, bypassing both of the
 * data buffers and leaving the contents of the buffers unchanged.
 *
 * @param page Page of the main memory to read
 * @param offset Starting byte address within the page
 **/
void ATD45DB161D::ReadMainMemoryPage(uint16_t page, uint16_t offset)
{
	DF_CS_inactive;    /* Make sure to toggle CS signal in order */
	DF_CS_active;      /* to reset Dataflash command decoder     */

	/* Send opcode */
	SPI.transfer(AT45DB161D_PAGE_READ);
	
	/* Address (page | offset)  */
	SPI.transfer((uint8_t)(page >> 6));
	SPI.transfer((uint8_t)((page << 2) | (offset >> 8)));
	SPI.transfer((uint8_t)(offset & 0xff));
	
	/* 4 "don't care" bytes */
	SPI.transfer(0x00);
	SPI.transfer(0x00);
	SPI.transfer(0x00);
	SPI.transfer(0x00);
}

/** 
 * Continuous Array Read.
 * Sequentially read a continuous stream of data.
 * @param page Page of the main memory where the sequential read will start
 * @param offset Starting byte address within the page
 * @param low If true the read operation will be performed in low speed mode (and in high speed mode if it's false).
 * @note The legacy mode is not currently supported
 **/
void ATD45DB161D::ContinuousArrayRead(uint16_t page, uint16_t offset, uint8_t low)
{
	DF_CS_inactive;    /* Make sure to toggle CS signal in order */
	DF_CS_active;      /* to reset Dataflash command decoder     */

	/* Send opcode */
	SPI.transfer( low ? AT45DB161D_CONTINUOUS_READ_LOW_FREQ :
	                    AT45DB161D_CONTINUOUS_READ_HIGH_FREQ );

	/* Address (page | offset)  */
	SPI.transfer((uint8_t)(page >> 6));
	SPI.transfer((uint8_t)((page << 2) | (offset >> 8)));
	SPI.transfer((uint8_t)(offset & 0xff));

	/* High frequency continuous read has a additional don't care byte */
	if(!low)
	{
		SPI.transfer(0x00);
	}
}


/** 
 * Read the content of one of the SRAM data buffers (in low or high speed mode).
 * @param bufferNum Buffer to read (1 or 2)
 * @param offset Starting byte within the buffer
 * @param low If true the read operation will be performed in low speed mode (and in high speed mode if it's false).
 **/
void ATD45DB161D::BufferRead(uint8_t bufferNum, uint16_t offset, uint8_t low)
{
	DF_CS_inactive;    /* Make sure to toggle CS signal in order */
	DF_CS_active;      /* to reset Dataflash command decoder     */

	/* Send opcode */
	if(bufferNum == 1)
	{
		SPI.transfer(low ? AT45DB161D_BUFFER_1_READ_LOW_FREQ :
		                   AT45DB161D_BUFFER_1_READ);
	}
	else
	{
		spi_transfer(low ? AT45DB161D_BUFFER_2_READ_LOW_FREQ :
		                   AT45DB161D_BUFFER_2_READ);

	}
	
	/* 14 "Don't care" bits */
	SPI.transfer(0x00);
	/* Rest of the "don't care" bits + bits 8,9 of the offset */
	SPI.transfer((uint8_t)(offset >> 8));
	/* bits 7-0 of the offset */
	SPI.transfer((uint8_t)(offset & 0xff));
}

/** 
 * Write data to one of the SRAM data buffers. Any further call to
 * spi_tranfer will return bytes contained in the data buffer until
 * a low-to-high transition is detected on the CS pin. If the end of
 * the data buffer is reached, the device will wrap around back to the
 * beginning of the buffer. 
 * @param bufferNum Buffer to read (1 or 2)
 * @param offset Starting byte within the buffer
 **/
void ATD45DB161D::BufferWrite(uint8_t bufferNum, uint16_t offset)
{
	DF_CS_inactive;    /* Make sure to toggle CS signal in order */
	DF_CS_active;      /* to reset Dataflash command decoder     */

	SPI.transfer( (bufferNum == 1) ? AT45DB161D_BUFFER_1_WRITE :
	                                 AT45DB161D_BUFFER_2_WRITE);
	
	/* 14 "Don't care" bits */
	SPI.transfer(0x00);
	/* Rest of the "don't care" bits + bits 8,9 of the offset */
	SPI.transfer((uint8_t)(offset >> 8));
	/* bits 7-0 of the offset */
	SPI.transfer((uint8_t)(offset & 0xff));
}

/**
 * Transfer data from buffer 1 or 2 to main memory page.
 * @param bufferNum Buffer to use (1 or 2)
 * @param page Page where the content of the buffer will transfered
 * @param erase If set the page will be first erased before the buffer transfer.
 * @note If erase is equal to zero, the page must have been previously erased using one of the erase command (Page or Block Erase).
 **/
void ATD45DB161D::BufferToPage(uint8_t bufferNum, uint16_t page, uint8_t erase)
{
	DF_CS_inactive;    /* Make sure to toggle CS signal in order */
	DF_CS_active;      /* to reset Dataflash command decoder     */

	/* Opcode */
	if(erase)
	{
		SPI.transfer( (bufferNum == 1) ? AT45DB161D_BUFFER_1_TO_PAGE_WITH_ERASE :
	                                     AT45DB161D_BUFFER_2_TO_PAGE_WITH_ERASE);
	}
	else
	{
		SPI.transfer( (bufferNum == 1) ? AT45DB161D_BUFFER_1_TO_PAGE_WITHOUT_ERASE :
	                                     AT45DB161D_BUFFER_2_TO_PAGE_WITHOUT_ERASE);
	}
	
	/*
	 * 3 address bytes consist of :
	 *     - 2 don’t care bits
	 *     - 12 page address bits (PA11 - PA0) that specify the page in 
	 *       the main memory to be written
	 *     - 10 don’t care bits
	 */
	SPI.transfer((uint8_t)(page >> 6));
	SPI.transfer((uint8_t)(page << 2));
	SPI.transfer(0x00);
	
	DF_CS_inactive;  /* Start transfer */
	DF_CS_active;    /* If erase was set, the page will first be erased */

	/* Wait for the end of the transfer */
  	while(!(ReadStatusRegister() & READY_BUSY))
  	{}
}

/**
 * Transfer a page of data from main memory to buffer 1 or 2.
 * @param page Main memory page to transfer
 * @param buffer Buffer (1 or 2) where the data will be written
 **/
void ATD45DB161D::PageToBuffer(uint16_t page, uint8_t bufferNum)
{
	DF_CS_inactive;    /* Make sure to toggle CS signal in order */
	DF_CS_active;      /* to reset Dataflash command decoder     */
 
	/* Send opcode */
	SPI.transfer((bufferNum == 1) ? AT45DB161D_TRANSFER_PAGE_TO_BUFFER_1 :
	                                AT45DB161D_TRANSFER_PAGE_TO_BUFFER_2);

	/*
	 * 3 address bytes consist of :
	 *     - 2 don’t care bits
	 *     - 12 page address bits (PA11 - PA0) that specify the page in 
	 *       the main memory to be written
	 *     - 10 don’t care bits
	 */
	SPI.transfer((uint8_t)(page >> 6));
	SPI.transfer((uint8_t)(page << 2));
	SPI.transfer(0x00);
		
	DF_CS_inactive;  /* Start page transfer */
	DF_CS_active;

	/* Wait for the end of the transfer */
  	while(!(ReadStatusRegister() & READY_BUSY))
  	{}
}

/** 
 * Erase a page in the main memory array.
 * @param page Page to erase
 **/
void ATD45DB161D::PageErase(uint16_t page)
{
	DF_CS_inactive;    /* Make sure to toggle CS signal in order */
	DF_CS_active;      /* to reset Dataflash command decoder     */

	/* Send opcode */
	SPI.transfer(AT45DB161D_PAGE_ERASE);
	
	/*
	 * 3 address bytes consist of :
	 *     - 2 don’t care bits
	 *     - 12 page address bits (PA11 - PA0) that specify the page in 
	 *       the main memory to be written
	 *     - 10 don’t care bits
	 */
	SPI.transfer((uint8_t)(page >> 6));
	SPI.transfer((uint8_t)(page << 2));
	SPI.transfer(0x00);
		
	DF_CS_inactive;  /* Start block erase */
	DF_CS_active;

	/* Wait for the end of the block erase operation */
  	while(!(ReadStatusRegister() & READY_BUSY))
  	{}
}

/**
 * Erase a block of eight pages at one time.
 * @param block Index of the block to erase
 **/
void ATD45DB161D::BlockErase(uint16_t block)
{
	DF_CS_inactive;    /* Make sure to toggle CS signal in order */
	DF_CS_active;      /* to reset Dataflash command decoder     */

	/* Send opcode */
	SPI.transfer(AT45DB161D_BLOCK_ERASE);
	
	/*
	 * 3 address bytes consist of :
	 *     - 2 don’t care bits
	 *     - 9 block address bits (PA11 - PA3)
	 *     - 13 don’t care bits
	 */
	SPI.transfer((uint8_t)(block >> 3));
	SPI.transfer((uint8_t)(block << 5));
	SPI.transfer(0x00);
		
	DF_CS_inactive;  /* Start block erase */
	DF_CS_active;

	/* Wait for the end of the block erase operation */
  	while(!(ReadStatusRegister() & READY_BUSY))
  	{}
}

/** 
 * Erase a sector in main memory. There are 16 sector on the
 * at45db161d and only one can be erased at one time.
 * @param sector Sector to erase (1-15)
 **/
void ATD45DB161D::SectoreErase(uint8_t sector)
{
	DF_CS_inactive;    /* Make sure to toggle CS signal in order */
	DF_CS_active;      /* to reset Dataflash command decoder     */

	/* Send opcode */
	SPI.transfer(AT45DB161D_SECTOR_ERASE);
	
	/*
	 * 3 address bytes consist of :
	 */
	if((sector == 0x0a) || (sector == 0x0b))
	{
		/*
		 *  - 11 don’t care bits
		 *  - 
		 *  - 12 don’t care bits
		 */
		SPI.transfer(0x00);
		SPI.transfer(((sector & 0x01) << 4));
		SPI.transfer(0x00);
	}
	else
	{
		/*
		 *  - 2 don't care bits 
		 *  - 4 sector number bits
		 *  - 18 don't care bits 
		 */
		SPI.transfer(sector << 1);
		SPI.transfer(0x00);
		SPI.transfer(0x00);
	}
				
	DF_CS_inactive;  /* Start block erase */
	DF_CS_active;

	/* Wait for the end of the block erase operation */
  	while(!(ReadStatusRegister() & READY_BUSY))
  	{}
}

#ifdef CHIP_ERASE_ENABLED
/** 
 * Erase the entire chip memory. Sectors proteced or locked down will
 * not be erased.
 **/
void ATD45DB161D::ChipErase()
{
	DF_CS_inactive;    /* Make sure to toggle CS signal in order */
	DF_CS_active;      /* to reset Dataflash command decoder     */

	/* Send chip erase sequence */
	SPI.transfer(AT45DB161D_CHIP_ERASE_0);
	SPI.transfer(AT45DB161D_CHIP_ERASE_1);
	SPI.transfer(AT45DB161D_CHIP_ERASE_2);
	SPI.transfer(AT45DB161D_CHIP_ERASE_3);
				
	DF_CS_inactive;  /* Start chip erase */
	DF_CS_active;

	/* Wait for the end of the chip erase operation */
  	while(!(ReadStatusRegister() & READY_BUSY))
  	{}
}
#endif

/**
 * This a combination of Buffer Write and Buffer to Page with
 * Built-in Erase.
 * @note You must call EndAndWait in order to start transfering data from buffer to page
 * @param page Page where the content of the buffer will transfered
 * @param offset Starting byte address within the buffer
 * @param bufferNum Buffer to use (1 or 2)
 * @warning UNTESTED
 **/
void ATD45DB161D::BeginPageWriteThroughBuffer(uint16_t page, uint16_t offset, uint8_t bufferNum)
{
	DF_CS_inactive;    /* Make sure to toggle CS signal in order */
	DF_CS_active;      /* to reset Dataflash command decoder     */

	/* Send opcode */
	SPI.transfer((bufferNum == 1) ? AT45DB161D_PAGE_THROUGH_BUFFER_1 :
	                                AT45DB161D_PAGE_THROUGH_BUFFER_2);

	/* Address */
	SPI.transfer((uint8_t)(page >> 6));
	SPI.transfer((uint8_t)((page << 2) | (offset >> 8)));
	SPI.transfer((uint8_t)offset);
}

/**
 * Perform a low-to-high transition on the CS pin and then poll
 * the status register to check if the dataflash is busy.
 **/
void ATD45DB161D::EndAndWait()
{
	DF_CS_inactive;  /* End current operation */
	DF_CS_active;    /* Some internal operation may occur
	                  * (buffer to page transfer, page erase, etc... ) */

	/* Wait for the chip to be ready */
  	while(!(ReadStatusRegister() & READY_BUSY))
  	{}

	DF_CS_active;	/* Release SPI bus */
}

/**
 * Compare a page of data in main memory to the data in buffer 1 or 2.
 * @param page Page to test
 * @param bufferNum Buffer number
 * @return
 *		- 1 if the page and the buffer contains the same data
 * 		- 0 else
 * @warning UNTESTED
 **/
int8_t ATD45DB161D::ComparePageToBuffer(uint16_t page, uint8_t bufferNum)
{
	uint8_t status;
	
	DF_CS_inactive;    /* Make sure to toggle CS signal in order */
	DF_CS_active;      /* to reset Dataflash command decoder     */

	/* Send opcode */
	SPI.transfer((bufferNum == 1) ? AT45DB161D_COMPARE_PAGE_TO_BUFFER_1 :
	                                AT45DB161D_COMPARE_PAGE_TO_BUFFER_2);
	
	/* Page address */
	SPI.transfer((uint8_t)(page >> 6));
	SPI.transfer((uint8_t)(page << 2));
	SPI.transfer(0x00);
	
	DF_CS_inactive;  /* Start comparaison */
	DF_CS_active;

	/* Wait for the end of the comparaison and get the result */
  	while(!((status = ReadStatusRegister()) & READY_BUSY))
  	{}
  		
	/* If bit 6 of the status register is 0 then the data in the
  	 * main memory page matches the data in the buffer. 
 	 * If it's 1 then the data in the main memory page doesn't match.
 	 */
	 return ((status & COMPARE) ? 0 : 1);
}

/**
 * Put the device into the lowest power consumption mode.
 * Once the device has entered the Deep Power-down mode, all
 * instructions are ignored except the Resume from Deep
 * Power-down command.
 * @warning UNTESTED
 **/
void ATD45DB161D::DeepPowerDown()
{
	DF_CS_inactive;    /* Make sure to toggle CS signal in order */
	DF_CS_active;      /* to reset Dataflash command decoder     */
	
	/* Send opcode */
	SPI.transfer(AT45DB161D_DEEP_POWER_DOWN);
	
	/* Enter Deep Power-Down mode */
	DF_CS_inactive;
	
	/* Safety delay */
	delay(100);
}

/**
 * Takes the device out of Deep Power-down mode.
 **/
void ATD45DB161D::ResumeFromDeepPowerDown()
{
	DF_CS_inactive;    /* Make sure to toggle CS signal in order */
	DF_CS_active;      /* to reset Dataflash command decoder     */
	
	/* Send opcode */
	SPI.transfer(AT45DB161D_RESUME_FROM_DEEP_POWER_DOWN);
	
	/* Resume device */
	DF_CS_inactive;
	
	/* The CS pin must stay high during t_RDPD microseconds before the device
	 * can receive any commands.
	 * On the at45db161D t_RDPD = 35 microseconds. But we will wait 100
	 * (just to be sure). */
	delay(100);
}

void ATD45DB161D::HardReset()
{
	digitalWrite(m_resetPin, LOW);

	/* The reset pin should stay low for at least 10ms (table 18.4)*/
	delayMicroseconds(10);
	
	/* According to the Dataflash spec (21.6 Reset Timing),
	 * the CS pin should be in high state before RESET
	 * is deasserted (ie HIGH) */
	digitalWrite(m_chipSelectPin, HIGH);
	/* Just to be sure that the high state is reached */
	delayMicroseconds(1);
		
	digitalWrite(m_resetPin, HIGH);
	
	/* Reset recovery time = 1ms */
	delayMicroseconds(1);
	digitalWrite(m_chipSelectPin, LOW);
}

/** **/
