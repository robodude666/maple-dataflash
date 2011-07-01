/**
 * @file at45db161d.h
 * @brief AT45DB161D module
 **/
#ifndef AT45DB161D_H
#define AT45DB161D_H

#include <inttypes.h>
#include "wirish.h"

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
#define SLAVESELECT 10
/** Reset (Reset) **/
#define RESET        8
/** Write protect (WP) **/
#define WP           7
/**
 * @} 
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
		/** CTOR **/
		AT45DB161D(HardwareSPI *spi);
		
		/** DTOR **/
		~AT45DB161D();

		/** 
 		 * Setup pinout and set SPI configuration
 		 * @param csPin Chip select (Slave select) pin (CS)
 		 * @param resetPin Reset pin (RESET)
 		 * @param wpPin Write protect pin (WP)
 		 * **/
		void begin(uint8_t csPin = SLAVESELECT, uint8_t resetPin = RESET, uint8_t wpPin = WP);
		
		/**
		 * Disable device and restore SPI configuration
		 **/
		void end();
		
		/**
		 * Activate device.
		 **/
		inline void Enable()
		{
			digitalWrite(m_chipSelectPin, LOW);
		}
	
		/**
		 * Deactivate device.
		 **/
		inline void Disable()
		{
			digitalWrite(m_chipSelectPin, HIGH);
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
			digitalWrite(m_writeProtectPin, LOW);
		}

		/**
		 * Disable write protection.
		 **/
		inline void DisableWriteProtection()
		{
			digitalWrite(m_writeProtectPin, HIGH);
		}
		
		/** Get chip Select (CS) pin **/
		inline uint8_t ChipSelectPin  () const { return m_chipSelectPin;   }
		/** Get reset (RESET) pin **/
		inline uint8_t ResetPin       () const { return m_resetPin;        }
		/** Get write protect (WP) pin **/
		inline uint8_t WriteProtectPin() const { return m_writeProtectPin; }
			
	private:
		HardwareSPI *m_SPI;
		uint8_t m_chipSelectPin;   /**< Chip select pin (CS)   **/
		uint8_t m_resetPin;        /**< Reset pin (RESET)      **/
		uint8_t m_writeProtectPin; /**< Write protect pin (WP) **/		
};

/**
 * @}
 **/

#endif /* AT45DB161D_H */
