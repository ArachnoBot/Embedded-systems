/*
 * Exercise-Work.c
 *
 * Created: 8 Apr 2024 18.59.20
 * Author : Ville
 */ 

#define F_CPU 16000000UL

#include <stdlib.h>
#include <stdbool.h>
#include <avr/io.h>
#include <util/delay.h>
#include "lcd.h" // lcd header file made by Peter Fleury

volatile bool activated = false;
volatile bool code_correct = false;

int main(void){
	/* Summerin ulostulo asetus */
	DDRB |=  (1 << PB0); // 0b00000110  ARDUINO PORTTI 8
	/* Näytön alustus */
	lcd_init(LCD_DISP_ON);
	int16_t time_left = 10;
	char time_left_array[16]; // 16-bit taulukko
	int16_t code_entered = 0;
	int16_t correct_code = 1234;

	while(1){
	if(activated == true){
		// Koodin tarkistus -> code_correct = true;
		// Hälyytyksen pysäytys
		if (code_entered == correct_code){
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