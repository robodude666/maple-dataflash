/**
 * @file at45db161d.h
 * @brief AT45DB161D module
 **/
#ifndef _AT45DB161D_H_
#define _AT45DB161D_H_

#include <inttypes.h>
#include "wirish.h"

#include "gpio.h"

#include "at45db161d_commands.h"

/**
 * @defgroup AT45DB161D AT45DB161D module
 * @{
 **/

/**
 * @defgroup Chip erase command prevention
 * @note Will be removed once chip erase is re-implemented
 * @{
 **/
#ifdef CHIP_ERASE_ENABLED
#undef CHIP_ERASE_ENABLED
#endif
/**
 * @} 
 **/

/**
 * @defgroup PINOUT Default pinout
 * @{
 **/
/** Chip select (CS) **/
#define DATAFLASH_DEFAULT_CS	5
/** Reset (Reset) **/
#define DATAFLASH_DEFAULT_RESET	6
/** Write protect (WP) **/
#define DATAFLASH_DEFAULT_WP	7
/**
 * @} 
 **/

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
#define DATAFLASH_STATUS_READY_BUSY 0x80
/**
 * Result of the most recent Memory Page to Buffer Compare operation.
 * If this bit is equal to 0, then the data in the main memory page
 * matches the data in the buffer. If it's 1 then at least 1 byte in 
 * the main memory page does not match the data in the buffer.
 **/
#define DATAFLASH_STATUS_COMPARE 0x40
/**
 * Bit 1 in the Status Register is used to provide information to the
 * user whether or not the sector protection has been enabled or
 * disabled, either by software-controlled method or 
 * hardware-controlled method. 1 means that the sector protection has
 * been enabled and 0 that it has been disabled.
 **/
#define DATAFLASH_STATUS_PROTECT 0x02
/**
 * Bit 0 indicates whether the page size of the main memory array is
 * configured for "power of 2" binary page size (512 bytes) (bit=1) or 
 * standard DataFlash page size (528 bytes) (bit=0).
 **/
#define DATAFLASH_STATUS_PAGE_SIZE 0x01
/**
 * Bits 5, 4, 3 and 2 indicates the device density. The decimal value
 * of these four binary bits does not equate to the device density; the
 * four bits represent a combinational code relating to differing
 * densities of DataFlash devices. The device density is not the same
 * as the density code indicated in the JEDEC device ID information.
 * The device density is provided only for backward compatibility.
 **/
#define DATAFLASH_STATUS_DEVICE_DENSITY 0x2C 
/**
 * @}
 **/

/**
 * Enum used to identify buffer 1 and buffer 2 in method parameters.
 **/
typedef enum dataflash_buffer
{
	DATAFLASH_BUFFER1 = 1,
	DATAFLASH_BUFFER2 = 2
} dataflash_buffer;

/**
 * @brief at45db161d module
 **/
class AT45DB161D
{
	public:
		/** 
		 * @brief ID structure 
		 * This structure contains various informations about the
		 * dataflash chip being used.
		 **/
		struct ID
		{
			uint8_t manufacturer;       /**< Manufacturer id                           **/
			uint8_t device[2];          /**< Device id                                 **/
			uint8_t extendedInfoLength; /**< Extended device information string length **/
		};

	public:
		/**
		 * Constructor. Calls the corresponding begin function with pin definitions.
		 * @param spi Reference to HardwareSPI that the Dataflash module is connected to
		 * @note Calling begin manually is required with the use of this constructor.
		 **/
		AT45DB161D(HardwareSPI *spi);
		
		/**
		 * Constructor. Calls the corresponding begin function with pin definitions.
		 * @param spi Reference to HardwareSPI that the Dataflash module is connected to
		 * @param csPin Chip select (Slave select) pin (CS)
		 * @param resetPin Reset pin (RESET)
		 * @param wpPin Write protect pin (WP)
		 * @note Calling begin manually is not required with the use of this constructor.
		 **/
		AT45DB161D(HardwareSPI *spi, uint8_t csPin, uint8_t resetPin, uint8_t wpPin);

		/**
		 * Constructor. Calls the corresponding begin function with pin definitions.
		 * @param cs_dev GPIO the Chip/Slave Select pin is located on.
		 * @param cs_pin Bit within the cs_dev GPIO the Chip/Slave Select pin is located on.
		 * @param reset_dev GPIO the reset pin is located on.
		 * @param reset_pin Bit within the reset_dev GPIO the reset pin is located on.
		 * @param wp_dev GPIO the Write Protect pin is located on.
		 * @param wp_pin Bit within the wp_dev GPIO the Write Protect pin is located on.
		 * @note Calling begin manually is not required with the use of this constructor.
		 **/
		AT45DB161D(HardwareSPI *spi, gpio_dev *cs_dev, uint8_t cs_pin, gpio_dev *reset_dev, uint8_t reset_pin, gpio_dev *wp_dev, uint8_t wp_pin);
		
		/**
		 * Deconstructor
		 **/
		~AT45DB161D();

		/** 
 		 * Setup pinout for DataFlash using wirish pin numbers.
 		 * @param csPin Chip select (Slave select) pin (CS)
 		 * @param resetPin Reset pin (RESET)
 		 * @param wpPin Write protect pin (WP)
 		 * **/
		void begin(uint8_t csPin = DATAFLASH_DEFAULT_CS, uint8_t resetPin = DATAFLASH_DEFAULT_RESET, uint8_t wpPin = DATAFLASH_DEFAULT_WP);
		
		/**
		 * Setup pinout for DataFlash using libmaple GPIO & pin numbers.
		 * @param cs_dev GPIO the Chip/Slave Select pin is located on.
		 * @param cs_pin Bit within the cs_dev GPIO the Chip/Slave Select pin is located on.
		 * @param reset_dev GPIO the reset pin is located on.
		 * @param reset_pin Bit within the reset_dev GPIO the reset pin is located on.
		 * @param wp_dev GPIO the Write Protect pin is located on.
		 * @param wp_pin Bit within the wp_dev GPIO the Write Protect pin is located on.
		 **/
		void begin(gpio_dev *cs_dev, uint8_t cs_pin, gpio_dev *reset_dev, uint8_t reset_pin, gpio_dev *wp_dev, uint8_t wp_pin);
								
		/**
		 * Disable device and restore SPI configuration
		 **/
		void end();
		
		/**
		 * Activate device.
		 **/
		inline void Enable()
		{
			gpio_write_bit(m_chipSelectGPIO, m_chipSelectPin, 0);
		}
	
		/**
		 * Deactivate device.
		 **/
		inline void Disable()
		{
			gpio_write_bit(m_chipSelectGPIO, m_chipSelectPin, 1);
		}
		
		/** 
		 * Read status register 
		 * @return The content of the status register
		 * **/
		uint8_t ReadStatusRegister();
		
		/** 
		 * Read Manufacturer and Device ID 
		 * @note if id.extendedInfoLength is not equal to zero,
		 *       successive calls to SPI.transfer(0xff) will return
		 *       the extended device information string bytes.
		 * @param id Pointer to the ID structure to initialize
		 **/
		void ReadManufacturerAndDeviceID(struct AT45DB161D::ID *id);
		
		/** 
		 * A main memory page read allows the user to read data directly from
		 * any one of the 4096 pages in the main memory, bypassing both of the
		 * data buffers and leaving the contents of the buffers unchanged.
		 * @param page Page of the main memory to read
		 * @param offset Starting byte address within the page
		 **/
		void ReadMainMemoryPage(uint16_t page, uint16_t offset);

		/** 
		 * Sequentially read a continuous stream of data.
		 * @param page Page of the main memory where the sequential read will start
		 * @param offset Starting byte address within the page
		 * @note The legacy mode is not currently supported
		 * @warning UNTESTED
		 **/
		void ContinuousArrayRead(uint16_t page, uint16_t offset);

		/** 
		 * Read the content of one of the SRAM data buffers (in low or high speed mode).
		 * @param bufferNum Buffer to read (1 or 2)
		 * @param offset Starting byte within the buffer
		 **/
		void BufferRead(dataflash_buffer bufferNum, uint16_t offset);

		/** 
		 * Write data to one of the SRAM data buffers. Any further call to
		 * spi_tranfer will return bytes contained in the data buffer until
		 * a low-to-high transition is detected on the CS pin. If the end of
		 * the data buffer is reached, the device will wrap around back to the
		 * beginning of the buffer. 
		 * @param bufferNum Buffer to read (1 or 2)
		 * @param offset Starting byte within the buffer
		 **/
		void BufferWrite(dataflash_buffer bufferNum, uint16_t offset);
		
		/**
		 * Transfer data from buffer 1 or 2 to main memory page.
		 * @param bufferNum Buffer to use (1 or 2)
		 * @param page Page where the content of the buffer will transfered
		 * @param erase If set the page will be first erased before the buffer transfer.
		 * @note If erase is equal to zero, the page must have been previously erased using one of the erase command (Page or Block Erase).
		 **/
		void BufferToPage(dataflash_buffer bufferNum, uint16_t page, uint8_t erase);		

		/**
		 * Transfer a page of data from main memory to buffer 1 or 2.
		 * @param page Main memory page to transfer
		 * @param bufferNum Buffer (1 or 2) where the data will be written
		 **/
		void PageToBuffer(uint16_t page, dataflash_buffer bufferNum);

		/** 
		 * Erase a page in the main memory array.
		 * @param page Page to erase
		 * @warning UNTESTED
		 **/
		void PageErase(uint16_t page);
		
		/**
		 * Erase a block of eight pages at one time.
		 * @param block Index of the block to erase
		 * @warning UNTESTED
		 **/
		void BlockErase(uint16_t block);

		/** 
		 * Erase a sector in main memory. There are 16 sector on the
		 * at45db161d and only one can be erased at one time.
		 * @param sector Sector to erase
		 * @warning UNTESTED
		 **/
		void SectorErase(uint8_t sector);

#ifdef CHIP_ERASE_ENABLED
		/** 
		 * Erase the entire chip memory. Sectors proteced or locked down will
		 * not be erased.
		 * @warning UNTESTED
		 * @warning MAY DAMAGE CHIP, READ DATASHEET FOR DETAILS
		 **/
		void ChipErase();
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
		void BeginPageWriteThroughBuffer(uint16_t page, uint16_t offset, dataflash_buffer bufferNum);
		
		/**
		 * Perform a low-to-high transition on the CS pin and then poll
		 * the status register to check if the dataflash is busy.
		 **/
		void EndAndWait();

		/**
		 * Compare a page of data in main memory to the data in buffer 1 or 2.
		 * @param page Page to test
		 * @param bufferNum Buffer number
		 * @return
		 *		- 1 if the page and the buffer contains the same data
		 * 		- 0 else
		 * @warning UNTESTED
		 **/
		int8_t ComparePageToBuffer(uint16_t page, dataflash_buffer bufferNum);

		/**
		 * Put the device into the lowest power consumption mode.
		 * Once the device has entered the Deep Power-down mode, all
		 * instructions are ignored except the Resume from Deep
		 * Power-down command.
		 * @warning UNTESTED
		 **/
		void DeepPowerDown();

		/**
		 * Takes the device out of Deep Power-down mode.
		 * @warning UNTESTED
		 **/
		void ResumeFromDeepPowerDown();

		/**
		 * Reset device via the reset pin.
		 **/
		void HardReset();
		
		/**
		 * Enable write protection.
		 **/
		inline void EnableWriteProtection()
		{
			gpio_write_bit(m_writeProtectGPIO, m_writeProtectPin, 0);
		}

		/**
		 * Disable write protection.
		 **/
		inline void DisableWriteProtection()
		{
			gpio_write_bit(m_writeProtectGPIO, m_writeProtectPin, 1);
		}
		
	private:
		/**
		 * Reads status register and waits for Dataflash to be ready.
		 **/
		void WaitForReady();
					
	private:
		HardwareSPI *m_SPI;
		
		gpio_dev *m_chipSelectGPIO;		/**< Chip select GPIO (CS)   **/
		uint8_t m_chipSelectPin;		/**< Chip select pin (CS)    **/

		gpio_dev *m_resetGPIO;			/**< Reset GPIO (RESET)      **/
		uint8_t m_resetPin;				/**< Reset pin (RESET)       **/

		gpio_dev *m_writeProtectGPIO;	/**< Write protect GPIO (WP) **/
		uint8_t m_writeProtectPin;		/**< Write protect pin (WP)  **/
};

/**
 * @}
 **/

#endif /* AT45DB161D_H */
