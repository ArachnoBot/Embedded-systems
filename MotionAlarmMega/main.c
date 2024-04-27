/*
 * MotionAlarmMega.c
 *
 * Created: 25.4.2024 18.51.16
 * Author : Leevi
 */ 

#define F_CPU 16000000UL
#define FOSC 16000000UL
#define BAUD 115200
#define MYUBRR (FOSC/16/BAUD-1)

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "keypad/keypad.h"

#define BUZZER_PIN PE3
#define TRIGGER_PIN PE4
#define ECHO_PIN PE5
#define TRIGGER_DIST 30	// Sensor trigger distance in cm
#define ALARM_DELAY 10	// Time between motion detected and buzzer on in seconds
#define INPUTDELAY 400	// Minimum time between keypad inputs in ms
#define EEPROM_ADDRESS 0	// Address in EEPROM where the password string starts

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

volatile uint8_t state = 0;
volatile uint8_t secondsElapsed = 0;

void
savePassword(char password[]) {
	// Loop four times to save each character of the password
	for (uint8_t i = 0; i < 4; i++) {
		while (EECR & (1 << EEPE));
		EEAR = EEPROM_ADDRESS + i;
		EEDR = password[i];
		EECR |= (1 << EEMPE);
		EECR |= (1 << EEPE);
	}
	return;
}

void
loadPassword(char password[4]) {
	// Loop four times to get each character of the password
	for (uint8_t i = 0; i < 4; i++) {
		while (EECR & (1 << EEPE));
		EEAR = EEPROM_ADDRESS + i;
		EECR |= (1 << EERE);
		password[i] = EEDR;
	}
	return;
}

void 
initSerial()
{
	// Set baud rate in the USART Baud Rate Registers
	UBRR1H = (uint8_t) (MYUBRR >> 8);
	UBRR1L = (uint8_t) MYUBRR;
	
	// Enable transmitter and receiver
	UCSR1B = (1 << TXEN1) | (1 << RXEN1);
	
	// Set frame format: 8 data bits, 1 stop bit, no parity
    UCSR1C = (1 << UCSZ11) | (1 << UCSZ10);
	return;
}

void 
initTimers() 
{		
	// Set timers 4 and 5 to normal mode
	TCCR4A = 0;
	TCCR4B = 0;
	TCCR5A = 0;
	TCCR5B = 0;
	
	// Set timer prescalers to 256
	TCCR4B |= (1 << CS42);
	TCCR5B |= (1 << CS52);
	
	// Set timer 5 compare interrupt to trigger exactly every 1 second
	OCR5A = 62500;
	TIMSK5 |= (1 << OCIE5A);
	return;
}

ISR(TIMER5_COMPA_vect) {
	secondsElapsed++;
}

void
enableBuzzer() {
	// Set timer 3 to fast PWM with ICR as TOP and a prescaler of 256
	TCCR3A |= (1 << COM3A1) | (1 << WGM31);
	TCCR3B |= (1 << WGM32) | (1 << WGM33) | (1 << CS32);
		
	// Set ICR3 based on desired frequency
	// ICR3 should be (16000000/256)/frequency
	ICR3 = 125;
	return;
}

void
disableBuzzer() {
	// Reset all timer 3 registers to stop the buzzer
	TCCR3A = 0;
	TCCR3B = 0;
	ICR3 = 0;
	return;
}

// Receive a byte from the atmega358p controlling the LCD, waiting for the
// message as many milliseconds as the parameter "timeout" determines
unsigned char
receiveData(uint16_t timeout) {
	uint16_t timeElapsed = 0;
	while (!(UCSR1A & (1 << RXC1)))
	{
		timeElapsed += 1;
		_delay_ms(1);
		if (timeElapsed > timeout)
		{
			return 255;
		}
	}
	return UDR1;
}

// Send a byte to the atmega358p controlling the LCD
void 
sendData(uint8_t data)
{
	// Wait for empty transmit buffer
	while (!(UCSR1A & (1 << UDRE1))) {}
	// Send the data
	UDR1 = data;
	return;
}

// Get the motion sensor's measured distance in centimeters
uint8_t
getDistance()
{
	uint16_t tempDistance = 0;
	// Get the average of 5 readings to make them more reliable
	for (uint8_t i = 0; i < 5; i++) {
		// Give a 15 microsecond pulse to trigger pin
		PORTE &= ~(1 << TRIGGER_PIN);
		_delay_us(2);
		PORTE |= (1 << TRIGGER_PIN);
		_delay_us(15);
		PORTE &= ~(1 << TRIGGER_PIN);
		
		// Wait until the echo pin goes high
		while (!(PINE & (1 << ECHO_PIN))) {}
		// Start measuring time
		TCNT4 = 0;
		// Wait until output pulse goes low
		while ((PINE & (1 << ECHO_PIN))) {}
			
		// Calculate the distance, the multiplier 0.2755392 is 0.016 (ms per
		// timer tick) * 17.2212 (how many cm speed travels in a ms)
		tempDistance += TCNT4*0.2755392;
	}
	tempDistance /= 5;
	// Make sure the 16 bit integer doesnt overflow the 8 bit one
	if (tempDistance > 255)
	{
		tempDistance = 255;
	}
	uint8_t finalDistance = tempDistance;
	return finalDistance;
}

uint8_t 
attemptConnection() {
	uint8_t attempts = 0;
	// Attempt to receive connection message 111 until attempt count runs out
	while (attempts < 50)
	{
		uint8_t message = receiveData(200);
		if (message == 111)
		{
			// Send a message back to confirm the connection
			sendData(111);
			return 1;
		}
		attempts += 1;
	}
	return 0;
}

void
setPassword(char password[4])
{
	// Signal beginning of password
	sendData(INPUT);
	uint8_t inputsGiven = 0;
	
	// Get four inputs, adding them to the password string as well as sending
	// them to the LCD
	while (1)
	{
		char input = KEYPAD_GetKey();
		// Check if input is a character between 0-9
		if (input > 47 && input < 58 && inputsGiven < 4)
		{
			password[inputsGiven] = input;
			inputsGiven += 1;
			sendData(input);
			_delay_ms(INPUTDELAY);
		}
		else if (input == '#' && inputsGiven == 4)
		{
			sendData('#');
			break;
		}
		else if (input == '*' && inputsGiven > 0)
		{
			inputsGiven -= 1;
			sendData('*');
			_delay_ms(INPUTDELAY);
		}
		else
		{
			// Ignore other inputs
		}
	}
	
	// Save the password and inform the LCD we just set it
	savePassword(password);
	sendData(SETPASSWORD);
	_delay_ms(1000);
	return;
}

uint8_t 
checkPassword(char password[4], uint8_t timeoutEnabled)
{
	// Signal start of writing password
	sendData(INPUT);
	char inputPassword[4];
	uint8_t inputsGiven = 0;
	uint8_t passwordIsCorrect = 1;
	
	// Loop until user presses # after giving 4 inputs
	while (1)
	{
		char input = KEYPAD_GetKey();
		// If timeout is enabled and time goes over 10 seconds, inform the LCD
		if (timeoutEnabled && secondsElapsed > ALARM_DELAY) 
		{
			sendData('#');
			sendData(ALARMTIMEOUT);
			_delay_ms(1000);
			return 0;
		}
		// Check if input is a character between 0-9, inform the LCD and add it to
		// to the password string
		else if (input > 47 && input < 58 && inputsGiven < 4)
		{
			inputPassword[inputsGiven] = input;
			inputsGiven += 1;
			sendData(input);
			_delay_ms(INPUTDELAY);
		}
		// If # is pressed, inform the LCD and break the input loop
		else if (input == '#' && inputsGiven == 4)
		{
			sendData('#');
			break;
		}
		// If * is pressed, inform the LCD and go back one index
		else if (input == '*' && inputsGiven > 0)
		{
			inputsGiven -= 1;
			sendData('*');
			_delay_ms(INPUTDELAY);
		}
		else
		{
			// Ignore other inputs
		}
	}
	
	// Check if the given password matches the real one
	for (uint8_t i = 0; i < 4; i++)
	{
		if (inputPassword[i] != password[i])
		{
			passwordIsCorrect = 0;
			break;
		}
	}
	
	// Inform the LCD whether the password was correct or not
	if (passwordIsCorrect) {
		sendData(CORRECTPASS);
	}
	else
	{
		sendData(WRONGPASS);	
	}
	
	_delay_ms(1000); // Delay so the message isnt immediately overwritten
	return passwordIsCorrect;	
}

int
main(void)
{
	sei();
	_delay_ms(2000); // REMEMBER TO REMOVE
	
	// Set used pins as inputs/outputs
	DDRE |= (1 << TRIGGER_PIN);
	DDRE &= ~(1 << ECHO_PIN);
	DDRE |= (1 << BUZZER_PIN);
	
	// Create and load password from EEPROM
	char password[4];
	loadPassword(password);
	
	// Initialize everything, connect to the LCD and set state as disarmed
	initSerial();
	initTimers();
	KEYPAD_Init();
	attemptConnection();
	_delay_ms(500);
	state = DISARMED;
	
	while (1) 
	{
		switch (state)
		{
			case ARMED:
				sendData(ARMED);
				_delay_ms(INPUTDELAY);
				while (1)
				{
					char key = KEYPAD_GetKey();
					if (key == '#') {
						if(checkPassword(password, 0))
						{
							state = DISARMED;
							break;
						}
						else
						{
							state = TRIGGERED;
							break;
						}
					}
					else if (getDistance() < TRIGGER_DIST) {
						state = MOVEMENT;
						break;
					}
				}
				break;
			
			case MOVEMENT:
				sendData(MOVEMENT);
				TCNT5 = 0;
				secondsElapsed = 0;
				_delay_ms(INPUTDELAY);
				while (1)
				{
					char key = KEYPAD_GetKey();
					// Wait until # to start inputting password
					if (key == '#') {
						if(checkPassword(password, 1))
						{
							state = DISARMED;
							break;
						}
						else
						{
							state = TRIGGERED;
							break;
						}
					}
					// If no input is given, trigger the alarm
					else if (secondsElapsed > ALARM_DELAY) {
						sendData(ALARMTIMEOUT);
						_delay_ms(1000);
						state = TRIGGERED;
						break;
					}
				}
				break;
			
			case DISARMED:
				sendData(DISARMED);
				_delay_ms(INPUTDELAY);
				while (1) 
				{
					char key = KEYPAD_GetKey();
					if (key == '*')
					{
						setPassword(password);
						break;
					}
					else if (key == '#')
					{
						state = ARMED;
						break;
					}
					else
					{
						// Ignore other inputs
					}
				}
				break;
			
			case TRIGGERED:
				enableBuzzer();
				// Wait for the LCD to display the reason for the alarm
				_delay_ms(1000);
				
				// Loop until correct password is given, then disarm system
				while (checkPassword(password, 0) == 0);
				disableBuzzer();
				state = DISARMED;
				break;
		}
	}
	return 0;
}
