/*
 * Exercise-Work.c
 *
 * Created: 8 Apr 2024 18.59.20
 * Author : Ville
 */ 

#include <stdlib.h>
#include <stdbool.h>
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include "lcd.h" // lcd header file made by Peter Fleury

#define F_CPU 16000000UL
#define COLUMN_gm (PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm)

volatile bool activated = false;
volatile bool code_correct = false;
volatile uint8_t key_pressed;
char             input_pass[20];
uint8_t          cursor       = 0;
char             passcode[20] = "123ABC";
/* Macro for buttons in the grid */
char btn_value[16] = "123A456B789C*0#D";

/* Function prototypes */
void row_output_column_input(void);
void row_input_column_output(void);
void scan_keys(void);
void check_passcode(void);

int main(void){
	/* Summerin ulostulo asetus */
	DDRB |=  (1 << PB0); // 0b00000110  ARDUINO PORTTI 8
	/* Näytön alustus */
	lcd_init(LCD_DISP_ON);
	int16_t time_left = 10;
	char time_left_array[16]; // 16-bit taulukko

	/* PB2 and PB3 used as status LEDs, initialize to low */
	PORTB.DIRSET = PIN2_bm | PIN3_bm;
	PORTB.OUTSET = PIN2_bm | PIN3_bm;

	/* initialize pins connected to the rows and columns of keypad */
	PORTA.OUTCLR = PIN1_bm | PIN2_bm;
	PORTB.OUTCLR = PIN0_bm | PIN1_bm;
	PORTC.OUTCLR = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm;

	/* Enable pullup for columns */
	PORTC.PIN0CTRL |= PORT_PULLUPEN_bm;
	PORTC.PIN1CTRL |= PORT_PULLUPEN_bm;
	PORTC.PIN2CTRL |= PORT_PULLUPEN_bm;
	PORTC.PIN3CTRL |= PORT_PULLUPEN_bm;

	/* Enable pullup for rows */
	PORTA.PIN1CTRL |= PORT_PULLUPEN_bm;
	PORTA.PIN2CTRL |= PORT_PULLUPEN_bm;
	PORTB.PIN0CTRL |= PORT_PULLUPEN_bm;
	PORTB.PIN1CTRL |= PORT_PULLUPEN_bm;

	row_output_column_input();

	while(1){
	if(activated == true){
	/* Poll for low input values for the columns */
	if ((PORTC.IN & COLUMN_gm) != COLUMN_gm) {
		/* Debounce */
		_delay_ms(10);
		/* If the button is still pressed after 10 ms, the press is registered as valid */
		if ((PORTC.IN & COLUMN_gm) != COLUMN_gm) {
			scan_keys();
			check_passcode();
			/* Wait for all buttons to be released */
			while ((PORTC.IN & COLUMN_gm) != COLUMN_gm);
		}
	}
		// Koodin tarkistus -> code_correct = true;
		// Hälyytyksen pysäytys
		if (code_entered == correct_code){
			lcd_puts("Code Correct!");
			_delay_ms(2000);
			lcd_clrscr();
			lcd_puts("Alarm");
			lcd_gotoxy(0,1);
			lcd_puts("Deactivated!");
			_delay_ms(2000);
			activated = false;
			time_left = 10;
			}
		itoa(time_left, time_left_array, 10);
		lcd_clrscr();
		lcd_puts("Enter code!");
		lcd_gotoxy(0,1);
		lcd_puts("Time left: ");
		lcd_puts(time_left_array);
		
		if(time_left == 0){
			lcd_clrscr();
			lcd_puts("Alarm activated!");
			/* Summeri päälle */
			// (Välkkyvä led)
			for(int i=0; i<10;i++){
				_delay_ms(500);
				PORTB |=  (1 << PB0);
				_delay_ms(500);
				PORTB &= ~(1 << PB0);
			}
			activated = false;
			time_left = 10;
			PORTB &= ~(1 << PB0);
			}
		else{
			time_left = time_left-1;
			}
		}
	else{
		lcd_clrscr();
		lcd_puts("Security system");
		lcd_gotoxy(0,1);
		lcd_puts("Active!");
		_delay_ms(5000);
		// Odota että sensori aktivoituu -> aktivoitu = true;
		activated = true;
		}

	_delay_ms(1000);
	}
	return 0;
}

void row_output_column_input()
{
	/* Set rows to output */
	PORTA.DIRSET = PIN1_bm | PIN2_bm;
	PORTB.DIRSET = PIN0_bm | PIN1_bm;

	/* Set columns to input */
	PORTC.DIRCLR = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm;
}

void row_input_column_output()
{
	/* Set rows to input */
	PORTA.DIRCLR = PIN1_bm | PIN2_bm;
	PORTB.DIRCLR = PIN0_bm | PIN1_bm;

	/* Set columns to output */
	PORTC.DIRSET = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm;
}

void scan_keys()
{
	key_pressed = 0;

	/* If PC3 (COLUMN 0) is pulled low */
	if (!(PORTC.IN & PIN3_bm)) {
		key_pressed = 0;
	}
	/* If PC0 (COLUMN 1) is pulled low */
	else if (!(PORTC.IN & PIN0_bm)) {
		key_pressed = 1;
	}
	/* If PC1 (COLUMN 2) is pulled low */
	else if (!(PORTC.IN & PIN1_bm)) {
		key_pressed = 2;
	}
	/* If PC2 (COLUMN 3) is pulled low */
	else if (!(PORTC.IN & PIN2_bm)) {
		key_pressed = 3;
	}

	row_input_column_output();

	/* If PB0 (ROW 0) is pulled low */
	if (!(PORTB.IN & PIN0_bm)) {
		key_pressed += 0;
	}
	/* If PB1 (ROW 1) is pulled low */
	else if (!(PORTB.IN & PIN1_bm)) {
		key_pressed += 4;
	}
	/* If PA2 (ROW 2) is pulled low */
	else if (!(PORTA.IN & PIN2_bm)) {
		key_pressed += 8;
	}
	/* If PA1 (ROW 3) is pulled low */
	else if (!(PORTA.IN & PIN1_bm)) {
		key_pressed += 12;
	}

	row_output_column_input();
}

void check_passcode()
{
	/* If star is pressed */
	if (btn_value[key_pressed] == '*') {
		/* Reset input_pass */
		memset(input_pass, 0, sizeof(input_pass));
		cursor = 0;
	}
	/* If pound is pressed */
	else if (btn_value[key_pressed] == '#') {
		/* If input passcode is the same as the set passcode */
		if (!(strcmp(passcode, input_pass))) {
			/* Reset input_pass */
			memset(input_pass, 0, sizeof(input_pass));
			/* Flash green LED */
			activated = false;
			PORTB.OUTCLR = PIN2_bm;
			_delay_ms(500);
			PORTB.OUTSET = PIN2_bm;
			} else {
			/* Reset input_pass */
			memset(input_pass, 0, sizeof(input_pass));
			/* Flash red LED */
			PORTB.OUTCLR = PIN3_bm;
			_delay_ms(500);
			PORTB.OUTSET = PIN3_bm;
		}
		cursor = 0;
	}
	/* If no special character (star or pound) is pressed */
	else {
		/* If length is not surpassed (20 characters) */
		if (cursor < 20) {
			/* Record press */
			input_pass[cursor] = btn_value[key_pressed];
			cursor++;
			} else {
			/* Flash red LED if limit is reached and reset input_pass */
			cursor       = 0;
			PORTB.OUTSET = PIN3_bm;
			_delay_ms(250);
			PORTB.OUTCLR = PIN3_bm;
			_delay_ms(250);
			PORTB.OUTSET = PIN3_bm;
			_delay_ms(250);
			PORTB.OUTCLR = PIN3_bm;
			memset(input_pass, 0, sizeof(input_pass));
		}
	}
}