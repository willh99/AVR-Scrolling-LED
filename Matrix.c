#include "matrix.h"

//Macros for scrolling speed and number of LED matrix
#define NUM_DEVICES     1		// Number of cascaded max7219's, or just 1

unsigned int DEL = 100;		// Delay for scrolling speed in microseconds


// Message to be displayed, stored in flash 
const char message[3][15] PROGMEM = {"Embedded System", "Do I get an A? ", "AVR ucontroller"};

//Global variables used in main and ISR
volatile unsigned char time = 0, x, x_old = 0xFE;
volatile char mode = -1;

// Buffer array of bytes to store current display 
// data for each column in each cascaded device
uint8_t buffer [NUM_DEVICES*8];

// Send a byte through SPI
void writeByte(uint8_t byte) {
	SPDR = byte;                      // SPI starts sending immediately  
	while(!(SPSR & (1 << SPIF)));     // Loop until complete bit set
}


// Send a word through SPI
void writeWord(uint8_t address, uint8_t data) {
	writeByte(address);			// Write first byte
	writeByte(data);      		// Write Second byte
}


// Initializes all cascaded devices
void Matrix_init() 
{
	uint8_t i;

	// Set display brighness
	SLAVE_SELECT;
	for(i = 0; i < NUM_DEVICES; i++)   // Loop through number of cascaded devices
	{
		writeByte(0x0A); // Select Intensity register
		writeByte(0x07); // Set brightness
	}
	SLAVE_DESELECT;

	
	// Set display refresh
	SLAVE_SELECT;
	for(i = 0; i < NUM_DEVICES; i++)
	{
		writeByte(0x0B); // Select Scan-Limit register
		writeByte(0x07); // Select columns 0-7
	}
	SLAVE_DESELECT;

	 
	// Turn on the display
	SLAVE_SELECT;
	for(i = 0; i < NUM_DEVICES; i++)
	{
		writeByte(0x0C); // Select Shutdown register
		writeByte(0x01); // Select Normal Operation mode
	}
	SLAVE_DESELECT;

	 
	// Disable Display-Test
	SLAVE_SELECT;
	for(i = 0; i < NUM_DEVICES; i++)
	{
		writeByte(0x0F); // Select Display-Test register
		writeByte(0x00); // Disable Display-Test
	}
	SLAVE_DESELECT;
}

// Clears all columns on all devices
void clearMatrix(void)
{
	for(uint8_t x = 1; x < 9; x++) // for all columns
	{   
        SLAVE_SELECT;
        for(uint8_t i = 0; i < NUM_DEVICES; i++)
		{
			writeByte(x);    // Select column x
			writeByte(0x00); // Set column to 0
		}
		SLAVE_DESELECT;
	}
}

// Initializes buffer empty
void Buffer_init(void)
{   
	for(uint8_t i = 0; i < NUM_DEVICES*8; i++)
		buffer[i] = 0x00;
}

// Moves each byte forward in the buffer and adds next byte in at the end
void pushBuffer(uint8_t x)
{
	for(uint8_t i = 0; i < NUM_DEVICES*8 - 1; i++)
		buffer[i] = buffer[i+1];
	
	buffer[NUM_DEVICES*8 - 1] = x;
}

// Pushes in 5 characters columns into the buffer.
void pushCharacter(uint8_t c)
{	
		for(uint8_t j = 0; j < 5; j++){
			// Loop for 5 bytes representing each character
			pushBuffer(pgm_read_byte(&ascii[c][j]));   // Push the byte from the characters array to the display buffer
			displayBuffer();					// Display the current buffer on the devices
			_delay_ms(DEL);
		}
}

// Takes a pointer to the beginning of a char array holding message, and array size, scrolls message.
void displayMessage(const char *arrayPointer, uint16_t arraySize)
{
	for(uint16_t i = 0; i < arraySize; i++)
	{
		// Send converted ASCII value of character in message to index in characters array 
		// (-32 sends corrent index to characters array)
		pushCharacter(pgm_read_byte_near(arrayPointer + i) - 32);
		pushBuffer(0x00);						// Add empty column after character for letter spacing
		displayBuffer();						// Display &
		_delay_ms(DEL); 						// Delay
	}
	
}


// Displays current buffer on the cascaded devices
void displayBuffer()
{   
   for(uint8_t i = 0; i < NUM_DEVICES; i++) // For each cascaded device
   {
	   for(uint8_t j = 1; j < 9; j++) // For each column
	   {
		   SLAVE_SELECT;
		   
		   for(uint8_t k = 0; k < i; k++) // Write Pre No-Op code
			   writeWord(0x00, 0x00);
		   
		   writeWord(j, buffer[j + i*8 - 1]); // Write column data from buffer
		   
		   for(uint8_t k = NUM_DEVICES-1; k > i; k--) // Write Post No-Op code
			   writeWord(0x00, 0x00);
		   
		   SLAVE_DESELECT;
	   }
   }
}


// Main Loop
int main(void) 
{
	unsigned char i=0;

	// Inits
	SPI_init();
	Matrix_init();
	clearMatrix();
	Buffer_init();
	port_init();
	timer_init();
  
	// Event loop
	while (1) {
		// Pointer to beginning of message
		const char *messagePointer = &message[i][0];
  
		// Size of message matrix
		uint16_t messageSize = sizeof(message[i]);
		displayMessage(messagePointer, messageSize);	// Display the message

		if(mode == 0 && time == 0){
			if(i==2)
				i=0;
			else
				i++;
			mode = -1;
		}
		else if(time == 1){
			if(DEL == 200)
				DEL = 50;
			else
				DEL = DEL*2;
			time=0;
			mode=-1;
		}
		
		else if(time == 2){
			if(DEL == 50)
				DEL = 200;
			else
				DEL = DEL/2;
			time=0;
			mode=-1;
		}
			
		}
	return (0);                          
}


ISR(TIMER0_OVF_vect){

	//Timer sensitive button press event. Timer1 OCF is
	//polled in order to increment the mode while
	//a button is held down.
	x=PINA;

	if(x!=x_old && x!=0xFE){
	
		_delay_ms(10);
		x=PINA;
		if(x!=0xFE && x!=x_old){

			TCNT1 = 0;			//Reset Timer/Counter value
			TIFR |= (1 << OCF1A);		//Clear timer interrupt flag
			mode = 0;			//Reset mode of operation

			while(x != 0xFE){
	
				x = PINA;
				if(TIFR & (1 << OCF1B)  && (mode<2)){
					mode++;
					time++;
					TIFR |= (1 << OCF1B);
				}
				else if(mode == 2) break;
			}
		}
	}
}
