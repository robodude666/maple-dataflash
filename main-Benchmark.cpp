#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "wirish.h"
#include "dma.h"

#include "at45db161d/at45db161d.h"

// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated objects that need libmaple may fail.
__attribute__((constructor)) void premain()
{
	init();
}

volatile bool spi_rx_dma_done;
volatile uint16_t pages_to_write;

void spi_rx_dma_irq(void)
{
	static uint8_t pages_done = 0;
	pages_done++;
	
	/*
	 * Realistically, DMA would not be set for continious reads
	 * and we'd only read one page at a time then process those 528 bytes.
	 * We'd then manually enable DMA for another 528 bytes and repeat.
	 * Once we have enough bytes, we'd disable DMA and trigger our "flag."
	 * But this is just a simple test to see maximum possible read speeds.
	 *
	 */
	
	if(pages_done == pages_to_write)
		spi_rx_dma_done = true;	
}

int main()
{
	HardwareSPI SPI(1);
	AT45DB161D dataflash(&SPI, 5, 6, 7); // SPI, CS, RST, WP

	uint8_t status;
	AT45DB161D::ID id;

	/* Initialize SPI */
	SPI.begin(SPI_18MHZ, MSBFIRST, 0);

	/* Let's wait 1 second, allowing use to press the serial monitor button :p */
	delay(1000);

	/* Read status register */
	status = dataflash.ReadStatusRegister();

	/* Read manufacturer and device ID */
	dataflash.ReadManufacturerAndDeviceID(&id);

    /*
     * Using Serial2 so we don't have to worry about delaying for SerialUSB to be connected to.
     */
    Serial2.begin(9600);

	Serial2.println("General Information:");

	/* Display status register */
	Serial2.print("    Status register: 0b");
	Serial2.print(status, BIN);
	Serial2.println();

	/* Display manufacturer and device ID */
	Serial2.print("    Manufacturer ID: 0x");  // Should be 0x1F
	Serial2.print(id.manufacturer, HEX);
	Serial2.println();

	Serial2.print("    Device ID (part 1): 0x"); // Should be 0x26
	Serial2.print(id.device[0], HEX);
	Serial2.println();

	Serial2.print("    Device ID (part 2): 0x"); // Should be 0x00
	Serial2.print(id.device[1], HEX);
	Serial2.println();

	Serial2.print("    Extended Device Information String Length: 0x"); // 0x00
	Serial2.print(id.extendedInfoLength, HEX);
	Serial2.println();
	Serial2.println();

	// Benchmark stuff

	Serial2.println("Benchmark Running:");

	#define PAGES_TO_TEST 16
	#define START_PAGE 0
	#define BYTES_PER_PAGE 528
	#define BUFFER_TO_USE DATAFLASH_BUFFER1

	uint32_t write_time, write_start, write_end;
	uint32_t read_buffer_time, read_buffer_start, read_buffer_end, read_buffer_errors;
	uint32_t read_page_time, read_page_start, read_page_end, read_page_errors;
	uint32_t read_array_time, read_array_start, read_array_end, read_array_errors;
	uint32_t read_array_dma_time, read_array_dma_start, read_array_dma_end;
	uint32_t bytes_transfered;
	uint8_t data, k;

	bytes_transfered = 0;
	read_buffer_errors = 0;
	read_page_errors = 0;
	read_array_errors = 0;

	/*
	 * Write via Buffer
	 */
	
	Serial2.println("    Performing Write via Buffer Test.");
	k = 0;
	write_start = micros();
	for(uint16_t page = START_PAGE; page < (PAGES_TO_TEST + START_PAGE); page++)
	{
		dataflash.BufferWrite(BUFFER_TO_USE, 0);
		for(uint16_t i = 0; i < BYTES_PER_PAGE; i++)
		{
			bytes_transfered++;
			SPI.transfer(k);
			k++;
		}
		dataflash.BufferToPage(BUFFER_TO_USE, page, true);
	}
	write_end = micros();
	dataflash.Disable();

	/*
	 * Read via Buffer
	 */

	Serial2.println("    Performing Read via Buffer Test.");
	k = 0;
	read_buffer_start = micros();
	for(uint16_t page = START_PAGE; page < (PAGES_TO_TEST + START_PAGE); page++)
	{
		dataflash.PageToBuffer(page, BUFFER_TO_USE);
		dataflash.BufferRead(BUFFER_TO_USE, 0);
		for(uint16_t i = 0; i < BYTES_PER_PAGE; i++)
		{
			data = SPI.transfer(0xFF);
			if(data != k) read_buffer_errors++;
			k++;
		}
	}
	read_buffer_end = micros();
	dataflash.Disable();
	
	/*
	 * Read via Main Page
	 */

	Serial2.println("    Performing Read via Main Page Test.");
	k = 0;
	read_page_start = micros();
	for(uint16_t page = START_PAGE; page < (PAGES_TO_TEST + START_PAGE); page++)
	{
		dataflash.ReadMainMemoryPage(page, 0);
		for(uint16_t i = 0; i < BYTES_PER_PAGE; i++)
		{
			data = SPI.transfer(0xFF);
			if(data != k) read_page_errors++;
			k++;
		}
	}
	read_page_end = micros();
	dataflash.Disable();
	
	/*
	 * Read via Continuous Array
	 */

	Serial2.println("    Performing Read via Continuous Array Test.");
	k = 0;
	read_array_start = micros();
	dataflash.ContinuousArrayRead(START_PAGE, 0);
	for(uint16_t page = START_PAGE; page < (PAGES_TO_TEST + START_PAGE); page++)
	{
		for(uint16_t i = 0; i < BYTES_PER_PAGE; i++)
		{
			data = SPI.transfer(0xFF);
			if(data != k) read_array_errors++;
			k++;
		}
	}
	read_array_end = micros();
	dataflash.Disable();
		
	/*
	 * Read via Continuous Array & DMA
	 */

	Serial2.println("    Performing Read via Continuous Array with DMA Test.");

	// Configuring DMA
	
	#define SPI_DMA_DEV DMA1
	#define SPI_RX_DMA_CHANNEL DMA_CH2
	#define SPI_TX_DMA_CHANNEL DMA_CH3
	#define SPI_BUFF_SIZE BYTES_PER_PAGE
	
	uint8_t dma_rx_spi_buffer[SPI_BUFF_SIZE];
	uint8_t dma_tx_spi_buffer[SPI_BUFF_SIZE];
	
	memset(dma_rx_spi_buffer, 0x00, SPI_BUFF_SIZE);
	memset(dma_tx_spi_buffer, 0xFF, SPI_BUFF_SIZE);
	
	dma_init(SPI_DMA_DEV);
	spi_rx_dma_enable(SPI1);
	spi_tx_dma_enable(SPI1);
	
	dma_setup_transfer(SPI_DMA_DEV, SPI_RX_DMA_CHANNEL,
						&SPI1->regs->DR, DMA_SIZE_8BITS,
						dma_rx_spi_buffer,  DMA_SIZE_8BITS,
						(DMA_MINC_MODE | DMA_CIRC_MODE | DMA_TRNS_CMPLT)
						);
	dma_attach_interrupt(SPI_DMA_DEV, SPI_RX_DMA_CHANNEL, spi_rx_dma_irq);
	
	dma_setup_transfer(SPI_DMA_DEV, SPI_TX_DMA_CHANNEL,
						&SPI1->regs->DR, DMA_SIZE_8BITS,
						dma_tx_spi_buffer,  DMA_SIZE_8BITS,
						(DMA_MINC_MODE | DMA_CIRC_MODE| DMA_FROM_MEM)
						);

	read_array_dma_start = micros();

	// Sending "I want to read" command manually
	dataflash.ContinuousArrayRead(START_PAGE, 0);
	
	// Letting DMA rip!
	dma_set_num_transfers(SPI_DMA_DEV, SPI_RX_DMA_CHANNEL, SPI_BUFF_SIZE);
	dma_set_num_transfers(SPI_DMA_DEV, SPI_TX_DMA_CHANNEL, SPI_BUFF_SIZE);
	pages_to_write = PAGES_TO_TEST;
	
	spi_rx_dma_done = false;
	dma_enable(SPI_DMA_DEV, SPI_TX_DMA_CHANNEL);
	dma_enable(SPI_DMA_DEV, SPI_RX_DMA_CHANNEL);
	
	while(spi_rx_dma_done == false);
	dma_disable(SPI_DMA_DEV, SPI_TX_DMA_CHANNEL);
	dma_disable(SPI_DMA_DEV, SPI_RX_DMA_CHANNEL);
	
	read_array_dma_end = micros();
	
	Serial2.println("    Done.\n");
	
	write_time = write_end - write_start;
	read_buffer_time = read_buffer_end - read_buffer_start;
	read_page_time = read_page_end - read_page_start;
	read_array_time = read_array_end - read_array_start;
	read_array_dma_time = read_array_dma_end - read_array_dma_start;

	#define calculateDataRate(bytes, time_us) ((float)bytes * (1000000.0 / (float)(time_us)))

	Serial2.println("Benchmark 1 - Write via Buffer:");
	Serial2.print("    Time: "); Serial2.print(write_time); Serial2.println(" uS.");
	Serial2.print("    Wrote: "); Serial2.print(bytes_transfered); Serial2.println(" bytes.");
	Serial2.print("    Write Speed: "); Serial2.print(calculateDataRate(bytes_transfered, write_time)); Serial2.println(" Bps.");
	Serial2.println();

	Serial2.println("Benchmark 2 - Read via Buffer:");
	Serial2.print("    Time: "); Serial2.print(read_buffer_time); Serial2.println(" uS.");
	Serial2.print("    Read: "); Serial2.print(bytes_transfered); Serial2.println(" bytes.");
	Serial2.print("    Errors: "); Serial2.print(read_buffer_errors); Serial2.println(" errors.");
	Serial2.print("    Read Speed: "); Serial2.print(calculateDataRate(bytes_transfered, read_buffer_time)); Serial2.println(" Bps.");
	Serial2.println();

	Serial2.println("Benchmark 3 - Read via Memory Page:");
	Serial2.print("    Time: "); Serial2.print(read_page_time); Serial2.println(" uS.");
	Serial2.print("    Read: "); Serial2.print(bytes_transfered); Serial2.println(" bytes.");
	Serial2.print("    Errors: "); Serial2.print(read_page_errors); Serial2.println(" errors.");
	Serial2.print("    Read Speed: "); Serial2.print(calculateDataRate(bytes_transfered, read_page_time)); Serial2.println(" Bps.");
	Serial2.println();

	Serial2.println("Benchmark 4 - Read via Continuous Array:");
	Serial2.print("    Time: "); Serial2.print(read_array_time); Serial2.println(" uS.");
	Serial2.print("    Read: "); Serial2.print(bytes_transfered); Serial2.println(" bytes.");
	Serial2.print("    Errors: "); Serial2.print(read_array_errors); Serial2.println(" errors.");
	Serial2.print("    Read Speed: "); Serial2.print(calculateDataRate(bytes_transfered, read_array_time)); Serial2.println(" Bps.");
	Serial2.println();
	
	Serial2.println("Benchmark 5 - Read via Continuous Array with DMA:");
	Serial2.print("    Time: "); Serial2.print(read_array_dma_time); Serial2.println(" uS.");
	Serial2.print("    Read: "); Serial2.print(bytes_transfered); Serial2.println(" bytes.");
	//Serial2.print("    Errors: "); Serial2.print(read_array_errors); Serial2.println(" errors.");
	Serial2.print("    Read Speed: "); Serial2.print(calculateDataRate(bytes_transfered, read_array_dma_time)); Serial2.println(" Bps.");
	Serial2.println();

	// Just relax
	while(1);

    return 0;
}
