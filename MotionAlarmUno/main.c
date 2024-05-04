/*
 * MotionAlarmUno.c
 *
 * Created: 25.4.2024 18.50.47
 */ 

#define F_CPU 16000000UL
#define FOSC 16000000UL
#define BAUD 115200
#define MYUBRR (FOSC/16/BAUD-1)

#include <avr/io.h>
#include <util/delay.h>
#include "lcd/lcd.h" // lcd header file made by Peter Fleury

// System states and communication constants
#define ARMED 246
#define MOVEMENT 247
#define DISARMED 248
#define TRIGGERED 249
#define INPUT 250
#define SETPASSWORD 251
#define CORRECTPASS 252
#define ALARMTIMEOUT 253
#define WRONGPASS 254
#define TIMEOUT 255

void 
initSerial() 
{
	// Set baud rate
	UBRR0H = (uint8_t) (MYUBRR >> 8);
	UBRR0L = (uint8_t) MYUBRR;

	// Enable transmitter and receiver
	UCSR0B = (1 << TXEN0) | (1 << RXEN0);

	// Set frame format: 8 data bits, 1 stop bit, no parity
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
	return;
}

// Send a byte to the atmega2560
void 
sendData(uint8_t data)
{
	// Wait for empty transmit buffer
	while (!(UCSR0A & (1 << UDRE0))) {}

	// Send the data
	UDR0 = data;
	return;
}

// Receive a byte from the atmega2560, waiting for the message as many
// milliseconds as the parameter "timeout" determines
unsigned char 
receiveData(uint16_t timeout) 
{
	uint16_t timeElapsed = 0;
	while (!(UCSR0A & (1 << RXC0)))
	{
		timeElapsed += 1;
		_delay_ms(1);
		if (timeElapsed > timeout)
		{
			return TIMEOUT;
		}
	}
	return UDR0;
}

// Try to connect to the atmega2560
uint8_t 
attemptConnection()
{
	uint8_t attempts = 0;
	lcd_puts("Connecting...");
	// Send value 111 up to 50 times, while listening for echo each time
	while (attempts < 50)
	{
		sendData(111);
		uint8_t response = receiveData(200);
		if (response == 111)
		{
			// If we get 111 in response, the connection is established
			return 1;
		}
		attempts += 1;
	}
	return 0;
}

// Update the LCD based on the inputs the user gives
void
handleKeypadInput()
{
	lcd_clrscr();
	lcd_puts("Input password:");
	lcd_gotoxy(0,1);
	
	uint8_t inputsGiven = 0;
	char input = 0;
	
	while (input != '#')
	{
		input = receiveData(1000);
		// Check if input is a character between 0-9
		if (input > 47 && input < 58)
		{
			lcd_putc(input);
			inputsGiven += 1;
		}
		// If # is pressed, erase character
		else if (input == '*')
		{
			inputsGiven -= 1;
			lcd_gotoxy(inputsGiven, 1);
			lcd_putc(' ');
			lcd_gotoxy(inputsGiven, 1);
		}
		else
		{
			// Ignore other inputs
		}
	}
	
	// Receive result of password input and display it
	input = receiveData(5000);
	lcd_clrscr();
	switch (input) 
	{
		case SETPASSWORD:
			lcd_puts("Password set");
			break;
			
		case CORRECTPASS:
			lcd_puts("Correct password");
			break;
			
		case DISARMED:
			lcd_puts("Alarm disarmed");
			break;
			
		case WRONGPASS:
			lcd_puts("Wrong password");
			break;
			
		case ALARMTIMEOUT:
			lcd_puts("Alarm timeout");
			break;
			
		default:
			lcd_puts("input error");
			lcd_putc(input);
			break;
	}
	return;
}

int
main(void)
{
	// initialize everything and connect to the atmega2560
	lcd_init(LCD_DISP_ON);
	initSerial();
	if (attemptConnection())
	{
		lcd_clrscr();
		lcd_puts("Connected");
	} 
	else
	{
		lcd_clrscr();
		lcd_puts("Not connected");
		return 0;
	}
	
	while (1) {
		uint8_t newState = receiveData(1000);
		
		switch (newState) 
		{
			case ARMED:
				lcd_clrscr();
				lcd_puts("Alarm armed");
				break;
			
			case MOVEMENT:
				lcd_clrscr();
				lcd_puts("Motion detected");
				break;
			
			case DISARMED:
				lcd_clrscr();
				lcd_puts("Alarm disarmed");
				break;
				
			case ALARMTIMEOUT:
				lcd_clrscr();
				lcd_puts("Alarm timeout");
				break;
				
			case INPUT:
				handleKeypadInput();
				break;
	
			case TIMEOUT:
				// Ignore timeout signal and go back to listening
				break;
	
			default:
				lcd_clrscr();
				lcd_puts("Unknown data: ");
				lcd_putc(newState);
				break;
		}
	}
	return 0;
}