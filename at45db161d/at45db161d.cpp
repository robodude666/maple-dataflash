#include "at45db161d.h"
#include "wirish.h"

/**
 * @defgroup STATUS_REGISTER_FORMAT Status register format
 * @{
 **/
/**
 * Ready/busy status is indicated using bit 7 of the status register.
 * If bit 7 is a 1, then the device is not busy and is ready to accept
 * the next command. If bit 7 is a 0, then the device is in a busy 
 * state.
 **/
#define READY_BUSY 0x80
/**
 * Result of the most recent Memory Page to Buffer Compare operation.
 * If this bit is equal to 0, then the data in the main memory page
 * matches the data in the buffer. If it's 1 then at least 1 byte in 
 * the main memory page does not match the data in the buffer.
 **/
#define COMPARE 0x40
/**
 * Bit 1 in the Status Register is used to provide information to the
 * user whether or not the sector protection has been enabled or
 * disabled, either by software-controlled method or 
 * hardware-controlled method. 1 means that the sector protection has
 * been enabled and 0 that it has been disabled.
 **/
#define PROTECT 0x02
/**
 * Bit 0 indicates whether the page size of the main memory array is
 * configured for "power of 2" binary page size (512 bytes) (bit=1) or 
 * standard DataFlash page size (528 bytes) (bit=0).
 **/
#define PAGE_SIZE 0x01
/**
 * Bits 5, 4, 3 and 2 indicates the device density. The decimal value
 * of these four binary bits does not equate to the device density; the
 * four bits represent a combinational code relating to differing
 * densities of DataFlash devices. The device density is not the same
 * as the density code indicated in the JEDEC device ID information.
 * The device density is provided only for backward compatibility.
 **/
#define DEVICE_DENSITY 0x2C 
/**
 * @}
 **/

/* De-assert CS */
#define DF_CS_deselect() digitalWrite(m_chipSelectPin, HIGH)

/* Assert CS */
#define DF_CS_select() digitalWrite(m_chipSelectPin, LOW)

/** CTOR **/
AT45DB161D::AT45DB161D(HardwareSPI *spi) :
	m_chipSelectPin   (SLAVESELECT),
	m_resetPin        (RESET),
	m_writeProtectPin (WP)
{
	m_SPI = spi;
}

/** DTOR **/
AT45DB161D::~AT45DB161D()
{
	m_SPI = NULL;	
}
	
/** 
 * Setup pinout and set SPI configuration
 * @param csPin Chip select (Slave select) pin (CS)
 * @param resetPin Reset pin (RESET)
 * @param wpPin Write protect pin (WP)
 * **/
void AT45DB161D::begin(uint8_t csPin, uint8_t resetPin, uint8_t wpPin)
{

	m_chipSelectPin   = csPin;
	m_resetPin        = resetPin;
	m_writeProtectPin = wpPin;
	
	pinMode(m_chipSelectPin,   OUTPUT);
	pinMode(m_resetPin,        OUTPUT);
	pinMode(m_writeProtectPin, OUTPUT);
	
	digitalWrite(m_resetPin,        HIGH);
	digitalWrite(m_writeProtectPin, LOW);
	
	/* Enable device */
  	DF_CS_select();
}

/**
 * Disable device and restore SPI configuration
 **/
void AT45DB161D::end()
{
	/* Disable device */
  	DF_CS_deselect();

}

/** 
 * Read status register
 * @return The content of the status register
 **/
uint8_t AT45DB161D::ReadStatusRegister()
{
	uint8_t status;

	DF_CS_deselect();    /* Make sure to toggle CS signal in order */
	DF_CS_select();      /* to reset Dataflash command decoder     */
  
    /* Send status read command */
	m_SPI->transfer(AT45DB161D_STATUS_REGISTER_READ);
	
	/* Get result with a dummy write */
	status = m_SPI->transfer(0x00);

	return status;
}

/** 
 * Read Manufacturer and Device ID 
 * @note if id.extendedInfoLength is not equal to zero,
 *       successive calls to m_SPI->transfer(0xff) will return
 *       the extended device information string bytes.
 * @param id Pointer to the ID structure to initialize
 **/
void AT45DB161D::ReadManufacturerAndDeviceID(struct AT45DB161D::ID *id)
{
	
	DF_CS_deselect();    /* Make sure to toggle CS signal in order */
	DF_CS_select();      /* to reset Dataflash command decoder     */
  
    /* Send status read command */
	m_SPI->transfer(AT45DB161D_READ_MANUFACTURER_AND_DEVICE_ID);

	/* Manufacturer ID */
	id->manufacturer = m_SPI->transfer(0xff);
	/* Device ID (part 1) */
	id->device[0] = m_SPI->transfer(0xff);
	/* Device ID (part 2) */
	id->device[1] = m_SPI->transfer(0xff);
	/* Extended Device Information String Length */
	id->extendedInfoLength = m_SPI->transfer(0xff);
	
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
void AT45DB161D::ReadMainMemoryPage(uint16_t page, uint16_t offset)
{
	DF_CS_deselect();    /* Make sure to toggle CS signal in order */
	DF_CS_select();      /* to reset Dataflash command decoder     */

	/* Send opcode */
	m_SPI->transfer(AT45DB161D_PAGE_READ);
	
	/* Address (page | offset)  */
	m_SPI->transfer((uint8_t)(page >> 6));
	m_SPI->transfer((uint8_t)((page << 2) | (offset >> 8)));
	m_SPI->transfer((uint8_t)(offset & 0xff));
	
	/* 4 "don't care" bytes */
	m_SPI->transfer(0x00);
	m_SPI->transfer(0x00);
	m_SPI->transfer(0x00);
	m_SPI->transfer(0x00);
}

/** 
 * Continuous Array Read.
 * Sequentially read a continuous stream of data.
 * @param page Page of the main memory where the sequential read will start
 * @param offset Starting byte address within the page
 * @note The legacy mode is not currently supported
 **/
void AT45DB161D::ContinuousArrayRead(uint16_t page, uint16_t offset)
{
	DF_CS_deselect();    /* Make sure to toggle CS signal in order */
	DF_CS_select();      /* to reset Dataflash command decoder     */

	/* Send opcode */
	m_SPI->transfer(AT45DB161D_CONTINUOUS_READ_LOW_FREQ);

	/* Address (page | offset)  */
	m_SPI->transfer((uint8_t)(page >> 6));
	m_SPI->transfer((uint8_t)((page << 2) | (offset >> 8)));
	m_SPI->transfer((uint8_t)(offset & 0xff));
}


/** 
 * Read the content of one of the SRAM data buffers (in low or high speed mode).
 * @param bufferNum Buffer to read (1 or 2)
 * @param offset Starting byte within the buffer
 **/
void AT45DB161D::BufferRead(dataflash_buffer bufferNum, uint16_t offset)
{
	DF_CS_deselect();    /* Make sure to toggle CS signal in order */
	DF_CS_select();      /* to reset Dataflash command decoder     */

	/* Send opcode */
	if(bufferNum == 1)
	{
		m_SPI->transfer(AT45DB161D_BUFFER_1_READ_LOW_FREQ);
	}
	else
	{
		m_SPI->transfer(AT45DB161D_BUFFER_2_READ_LOW_FREQ);

	}
	
	/* 14 "Don't care" bits */
	m_SPI->transfer(0x00);
	/* Rest of the "don't care" bits + bits 8,9 of the offset */
	m_SPI->transfer((uint8_t)(offset >> 8));
	/* bits 7-0 of the offset */
	m_SPI->transfer((uint8_t)(offset & 0xff));
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
void AT45DB161D::BufferWrite(dataflash_buffer bufferNum, uint16_t offset)
{
	DF_CS_deselect();    /* Make sure to toggle CS signal in order */
	DF_CS_select();      /* to reset Dataflash command decoder     */

	m_SPI->transfer( (bufferNum == 1) ? AT45DB161D_BUFFER_1_WRITE :
	                                 AT45DB161D_BUFFER_2_WRITE);
	
	/* 14 "Don't care" bits */
	m_SPI->transfer(0x00);
	/* Rest of the "don't care" bits + bits 8,9 of the offset */
	m_SPI->transfer((uint8_t)(offset >> 8));
	/* bits 7-0 of the offset */
	m_SPI->transfer((uint8_t)(offset & 0xff));
}

/**
 * Transfer data from buffer 1 or 2 to main memory page.
 * @param bufferNum Buffer to use (1 or 2)
 * @param page Page where the content of the buffer will transfered
 * @param erase If set the page will be first erased before the buffer transfer.
 * @note If erase is equal to zero, the page must have been previously erased using one of the erase command (Page or Block Erase).
 **/
void AT45DB161D::BufferToPage(dataflash_buffer bufferNum, uint16_t page, uint8_t erase)
{
	DF_CS_deselect();    /* Make sure to toggle CS signal in order */
	DF_CS_select();      /* to reset Dataflash command decoder     */

	/* Opcode */
	if(erase)
	{
		m_SPI->transfer( (bufferNum == 1) ? AT45DB161D_BUFFER_1_TO_PAGE_WITH_ERASE :
	                                     AT45DB161D_BUFFER_2_TO_PAGE_WITH_ERASE);
	}
	else
	{
		m_SPI->transfer( (bufferNum == 1) ? AT45DB161D_BUFFER_1_TO_PAGE_WITHOUT_ERASE :
	                                     AT45DB161D_BUFFER_2_TO_PAGE_WITHOUT_ERASE);
	}
	
	/*
	 * 3 address bytes consist of :
	 *     - 2 don’t care bits
	 *     - 12 page address bits (PA11 - PA0) that specify the page in 
	 *       the main memory to be written
	 *     - 10 don’t care bits
	 */
	m_SPI->transfer((uint8_t)(page >> 6));
	m_SPI->transfer((uint8_t)(page << 2));
	m_SPI->transfer(0x00);
	
	DF_CS_deselect();  /* Start transfer */
	DF_CS_select();    /* If erase was set, the page will first be erased */

	/* Wait for the end of the transfer */
	while(!(ReadStatusRegister() & READY_BUSY));

}

/**
 * Transfer a page of data from main memory to buffer 1 or 2.
 * @param page Main memory page to transfer
 * @param bufferNum Buffer (1 or 2) where the data will be written
 **/
void AT45DB161D::PageToBuffer(uint16_t page, dataflash_buffer bufferNum)
{
	DF_CS_deselect();    /* Make sure to toggle CS signal in order */
	DF_CS_select();      /* to reset Dataflash command decoder     */
 
	/* Send opcode */
	m_SPI->transfer((bufferNum == 1) ? AT45DB161D_TRANSFER_PAGE_TO_BUFFER_1 :
	                                AT45DB161D_TRANSFER_PAGE_TO_BUFFER_2);

	/*
	 * 3 address bytes consist of :
	 *     - 2 don’t care bits
	 *     - 12 page address bits (PA11 - PA0) that specify the page in 
	 *       the main memory to be written
	 *     - 10 don’t care bits
	 */
	m_SPI->transfer((uint8_t)(page >> 6));
	m_SPI->transfer((uint8_t)(page << 2));
	m_SPI->transfer(0x00);
		
	DF_CS_deselect();  /* Start page transfer */
	DF_CS_select();

	/* Wait for the end of the transfer */
	while(!(ReadStatusRegister() & READY_BUSY));

}

/** 
 * Erase a page in the main memory array.
 * @param page Page to erase
 **/
void AT45DB161D::PageErase(uint16_t page)
{
	DF_CS_deselect();    /* Make sure to toggle CS signal in order */
	DF_CS_select();      /* to reset Dataflash command decoder     */

	/* Send opcode */
	m_SPI->transfer(AT45DB161D_PAGE_ERASE);
	
	/*
	 * 3 address bytes consist of :
	 *     - 2 don’t care bits
	 *     - 12 page address bits (PA11 - PA0) that specify the page in 
	 *       the main memory to be written
	 *     - 10 don’t care bits
	 */
	m_SPI->transfer((uint8_t)(page >> 6));
	m_SPI->transfer((uint8_t)(page << 2));
	m_SPI->transfer(0x00);
		
	DF_CS_deselect();  /* Start block erase */
	DF_CS_select();

	/* Wait for the end of the block erase operation */
	while(!(ReadStatusRegister() & READY_BUSY));

}

/**
 * Erase a block of eight pages at one time.
 * @param block Index of the block to erase
 **/
void AT45DB161D::BlockErase(uint16_t block)
{
	DF_CS_deselect();    /* Make sure to toggle CS signal in order */
	DF_CS_select();      /* to reset Dataflash command decoder     */

	/* Send opcode */
	m_SPI->transfer(AT45DB161D_BLOCK_ERASE);
	
	/*
	 * 3 address bytes consist of :
	 *     - 2 don’t care bits
	 *     - 9 block address bits (PA11 - PA3)
	 *     - 13 don’t care bits
	 */
	m_SPI->transfer((uint8_t)(block >> 3));
	m_SPI->transfer((uint8_t)(block << 5));
	m_SPI->transfer(0x00);
		
	DF_CS_deselect();  /* Start block erase */
	DF_CS_select();

	/* Wait for the end of the block erase operation */
	while(!(ReadStatusRegister() & READY_BUSY));

}

/** 
 * Erase a sector in main memory. There are 16 sector on the
 * at45db161d and only one can be erased at one time.
 * @param sector Sector to erase (1-15)
 **/
void AT45DB161D::SectorErase(uint8_t sector)
{
	DF_CS_deselect();    /* Make sure to toggle CS signal in order */
	DF_CS_select();      /* to reset Dataflash command decoder     */

	/* Send opcode */
	m_SPI->transfer(AT45DB161D_SECTOR_ERASE);
	
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
		m_SPI->transfer(0x00);
		m_SPI->transfer(((sector & 0x01) << 4));
		m_SPI->transfer(0x00);
	}
	else
	{
		/*
		 *  - 2 don't care bits 
		 *  - 4 sector number bits
		 *  - 18 don't care bits 
		 */
		m_SPI->transfer(sector << 1);
		m_SPI->transfer(0x00);
		m_SPI->transfer(0x00);
	}
				
	DF_CS_deselect();  /* Start block erase */
	DF_CS_select();

	/* Wait for the end of the block erase operation */
	while(!(ReadStatusRegister() & READY_BUSY));

}

#ifdef CHIP_ERASE_ENABLED
/** 
 * Erase the entire chip memory. Sectors proteced or locked down will
 * not be erased.
 **/
void AT45DB161D::ChipErase()
{
	DF_CS_deselect();    /* Make sure to toggle CS signal in order */
	DF_CS_select();      /* to reset Dataflash command decoder     */

	/* Send chip erase sequence */
	m_SPI->transfer(AT45DB161D_CHIP_ERASE_0);
	m_SPI->transfer(AT45DB161D_CHIP_ERASE_1);
	m_SPI->transfer(AT45DB161D_CHIP_ERASE_2);
	m_SPI->transfer(AT45DB161D_CHIP_ERASE_3);
				
	DF_CS_deselect();  /* Start chip erase */
	DF_CS_select();

	/* Wait for the end of the chip erase operation */
	while(!(ReadStatusRegister() & READY_BUSY));

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
void AT45DB161D::BeginPageWriteThroughBuffer(uint16_t page, uint16_t offset, dataflash_buffer bufferNum)
{
	DF_CS_deselect();    /* Make sure to toggle CS signal in order */
	DF_CS_select();      /* to reset Dataflash command decoder     */

	/* Send opcode */
	m_SPI->transfer((bufferNum == 1) ? AT45DB161D_PAGE_THROUGH_BUFFER_1 :
	                                AT45DB161D_PAGE_THROUGH_BUFFER_2);

	/* Address */
	m_SPI->transfer((uint8_t)(page >> 6));
	m_SPI->transfer((uint8_t)((page << 2) | (offset >> 8)));
	m_SPI->transfer((uint8_t)offset);
}

/**
 * Perform a low-to-high transition on the CS pin and then poll
 * the status register to check if the dataflash is busy.
 **/
void AT45DB161D::EndAndWait()
{
	DF_CS_deselect();  /* End current operation */
	DF_CS_select();    /* Some internal operation may occur
	                  * (buffer to page transfer, page erase, etc... ) */

	/* Wait for the chip to be ready */
	while(!(ReadStatusRegister() & READY_BUSY));


	DF_CS_select();	/* Release SPI bus */
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
int8_t AT45DB161D::ComparePageToBuffer(uint16_t page, dataflash_buffer bufferNum)
{
	uint8_t status;
	
	DF_CS_deselect();    /* Make sure to toggle CS signal in order */
	DF_CS_select();      /* to reset Dataflash command decoder     */

	/* Send opcode */
	m_SPI->transfer((bufferNum == 1) ? AT45DB161D_COMPARE_PAGE_TO_BUFFER_1 :
	                                AT45DB161D_COMPARE_PAGE_TO_BUFFER_2);
	
	/* Page address */
	m_SPI->transfer((uint8_t)(page >> 6));
	m_SPI->transfer((uint8_t)(page << 2));
	m_SPI->transfer(0x00);
	
	DF_CS_deselect();  /* Start comparaison */
	DF_CS_select();

	/* Wait for the end of the comparaison and get the result */
	while(!((status = ReadStatusRegister()) & READY_BUSY));

  		
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
void AT45DB161D::DeepPowerDown()
{
	DF_CS_deselect();    /* Make sure to toggle CS signal in order */
	DF_CS_select();      /* to reset Dataflash command decoder     */
	
	/* Send opcode */
	m_SPI->transfer(AT45DB161D_DEEP_POWER_DOWN);
	
	/* Enter Deep Power-Down mode */
	DF_CS_deselect();
	
	/* Safety delay */
	delay(100);
}

/**
 * Takes the device out of Deep Power-down mode.
 **/
void AT45DB161D::ResumeFromDeepPowerDown()
{
	DF_CS_deselect();    /* Make sure to toggle CS signal in order */
	DF_CS_select();      /* to reset Dataflash command decoder     */
	
	/* Send opcode */
	m_SPI->transfer(AT45DB161D_RESUME_FROM_DEEP_POWER_DOWN);
	
	/* Resume device */
	DF_CS_deselect();
	
	/* The CS pin must stay high during t_RDPD microseconds before the device
	 * can receive any commands.
	 * On the at45db161D t_RDPD = 35 microseconds. But we will wait 100
	 * (just to be sure). */
	delay(100);
}

void AT45DB161D::HardReset()
{
	digitalWrite(m_resetPin, LOW);

	/* The reset pin should stay low for at least 10ms (table 18.4)*/
	delayMicroseconds(10);
	
	/* According to the Dataflash spec (21.6 Reset Timing),
	 * the CS pin should be in high state before RESET
	 * is deasserted (ie HIGH) */
	DF_CS_deselect();
	/* Just to be sure that the high state is reached */
	delayMicroseconds(1);
		
	digitalWrite(m_resetPin, HIGH);
	
	/* Reset recovery time = 1ms */
	delayMicroseconds(1);
	DF_CS_select();
}

/** **/
