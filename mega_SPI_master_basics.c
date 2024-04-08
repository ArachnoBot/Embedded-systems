/*
 * Exercise-Work.c
 *
 * Created: 8 Apr 2024 18.59.20
 * Author : Ville
 */ 

#define F_CPU 16000000UL

#include <stdbool.h>
#include <avr/io.h>
#include <util/delay.h>
#include "lcd.h" // lcd header file made by Peter Fleury

volatile bool aktivoitu = false;
volatile bool code_correct = false;

int main(void){
	/* initialize display, cursor off */
	lcd_init(LCD_DISP_ON);
	
	
	int16_t time_left = 10;
	char time_left_array[16]; // 16-bit array, assumes that the int given is 16-bits
	lcd_clrscr();
	lcd_puts("Security system");
	lcd_gotoxy(0,1);
	lcd_puts("Active!");
	_delay_ms(5000);
	/* clear display */
	lcd_clrscr();
	// Odota että sensori aktivoituu -> aktivoitu = true;

	if (aktivoitu == true){
	
	while(1){
	// Koodin tarkistus -> code_correct = true;
	// Hälyytyksen pysäytys

	if (code_correct == true){
	lcd_clrscr();
	lcd_puts("Alarm");
	lcd_gotoxy(0,1);
	lcd_puts("Deactivated!");
	break;
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
		//Summeri päälle
	}
	else{
	time_left = time_left-1;
	}
	}
	_delay_ms(1000);
	}
}
	
	return 0;
}