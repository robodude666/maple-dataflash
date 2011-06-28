#include <stdio.h>
#include <stdint.h>

#include "wirish.h"
#include "at45db161d/at45db161d.h"

#define NUM_PAGES 8

// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated objects that need libmaple may fail.
__attribute__((constructor)) void premain()
{
	init();
}

int main()
{
	HardwareSPI SPI(1);
	ATD45DB161D dataflash(&SPI); // A reference to our HardwareSPI interface is required.
	
	uint8_t loop_cnt;
	uint16_t page;
	
	uint8_t status;
	ATD45DB161D::ID id;
		
	uint16_t i, j;
	char message[] = "@ write test ";
	char overflow[] = "\nOVERFLOW!\n";
	uint8_t data;

	/* Initialize SPI */
	SPI.begin();

	/* Let's wait 1 second, allowing use to press the serial monitor button :p */
	delay(1000);

	/* Initialize dataflash */
	dataflash.begin(5, 6, 7); // CS, Reset, WP

	delay(10);

	/* Read status register */
	status = dataflash.ReadStatusRegister();

	/* Read manufacturer and device ID */
	dataflash.ReadManufacturerAndDeviceID(&id);

	/* Small delay to make sure we don't miss any action
	 * while connecting to the SerialUSB port
	 */
	delay(10000);

	/* Display status register */
	SerialUSB.print("Status register: 0b");
	SerialUSB.print(status, BIN);
	SerialUSB.print('\n');

	/* Display manufacturer and device ID */
	SerialUSB.print("Manufacturer ID: 0x");  // Should be 0x1F
	SerialUSB.print(id.manufacturer, HEX);
	SerialUSB.print('\n');

	SerialUSB.print("Device ID (part 1): 0x"); // Should be 0x26
	SerialUSB.print(id.device[0], HEX);
	SerialUSB.print('\n');

	SerialUSB.print("Device ID (part 2): 0x"); // Should be 0x00
	SerialUSB.print(id.device[1], HEX);
	SerialUSB.print('\n');

	SerialUSB.print("Extended Device Information String Length: 0x"); // 0x00
	SerialUSB.print(id.extendedInfoLength, HEX);
	SerialUSB.print('\n');

	loop_cnt = 0;
	page = 0;

	while(1)
	{

		/* Set dataflash so that any call to spi_tranfer will write the byte
		* given as argument to the Buffer 1 */
		dataflash.BufferWrite(1, 0);

		/* Transfer the message */
		for(i = 0; message[i] != '\0'; ++i)
			SPI.transfer(message[i]);

		/* Transfer variable length data */
		for(i = 0; i < page+1; i++)
		{
			for(j = 0; j < 10; j++)
				SPI.transfer('0' + j);
			
			SPI.transfer(' ');
			
		}

		SPI.transfer('\n');

		++loop_cnt;

		if(loop_cnt == 0)
		{
			/* loop_cnt overflow */
			/* To celebrate this we write the string "\nOVERFLOW!\n" to Buffer 1 */
			for( i =0; overflow[i] != '\0'; ++i)
				SPI.transfer(overflow[i]);
		}

		/* Write '\0' to buffer 1. This will help us know that we must stop reading from it. */
		SPI.transfer('\0');

		/* Transfer buffer 1 to 'page' page (with builtin erase) */
		dataflash.BufferToPage(1, page, 1);	

		++page;
		/* When we wrote the number of pages we wanted (NUM_PAGES), we display their contents by
		* using 2 methods alternatively ;
		*     - PageToBuffer + BufferRead
		*     - PageRead
		*/
		if(page >= NUM_PAGES)
		{
			for(i = 0; i < NUM_PAGES; ++i)
			{
				if(i & 1)
				{
					SerialUSB.println("Page to buffer");
					dataflash.PageToBuffer(i, 1);    
					dataflash.BufferRead(1, 0, true);
				}
				else
				{   
					SerialUSB.println("Page read");
					dataflash.ReadMainMemoryPage(i, 0); 
				}

				do
				{
					data = SPI.transfer(0xff);
					if(data != '\0')
						SerialUSB.print((char)data);
				}while(data != '\0');

				/* Add a little delay otherwise the display will be too fast */   
				delay(100); 
			}

			page = 0;
		}
	}	
    
    return 0;
}
