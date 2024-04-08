/*
 * mega_SPI_master_basics.c
 *
 * Created: 8.4.2024 18:00
 * Author : Glutex
 * Source : http://homepage.hispeed.ch/peterfleury/doxygen/avr-gcc-libraries/group__pfleury__lcd.html 
 *          http://homepage.hispeed.ch/peterfleury/avr-software.html
 */ 


#define F_CPU 16000000UL

#include <stdbool.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "lcd.h" // lcd header file made by Peter Fleury

volatile uint16_t g_adc_voltage = 0;
volatile bool g_b_is_adc_done = false;


/* When the timer/counter matches, the ADC starts */
ISR (ADC_vect)
{
    g_adc_voltage = ADC;
    g_b_is_adc_done = true;
}

/* timer/counter0 compare match A interrupt vector */
ISR (TIMER0_COMPA_vect)
{

}


int main(void)
{
    
    /* initialize display, cursor off */
    lcd_init(LCD_DISP_ON);
        
    /* clear display */
    lcd_clrscr();
    /* print string */
    lcd_puts("The ADC result:");
    lcd_gotoxy(0,1);
    
    /* set up the ports and pins */
    DDRK &= ~(1 << PK2); //A10 as an input
    
    // Enable interrupts 
    sei();

    /* set up the 8-bit timer/counter0 */
    TCNT0 = 0; // reset timer/counter0 register 
    TCCR0A = 0x02; // CTC, top is OCR0A
    TCCR0B = 0b00000101; // 1024 prescaling
    // TCCR0B |= 0x06;
    OCR0A = 0xFF; // set value to which to count /compare 
    // OCR0A = 0b11111111;
    // OCR0A = 255;
    TIMSK0 |= (1 << 1); // enable compare match A interrupt
    // TIMSK0 |= 2;
    // TIMSK0 |= 0x02;
    // TIMSK0 |= 0b00000010;
    
    /* Set up the ADC conversion */
    ADMUX |= 0b01000000; // AV_cc as voltage reference
    // ADMUX |= (3 << 6); 
    // ADMUX |= C0;
    ADMUX |= 0b00000010; // A10 input channel, set MUX1 
    ADCSRA |= (1 << 7); // turn ADC ON 
    // ADCSRA |= 0b10000000;
    ADCSRA |= (1 << 5); // enable ADC auto trigger
    // ADCSRA |= 0b00100000;
    ADCSRA |= (1 << 3); // enable ADC interrupt
    // ADCSRA |= 0b00001000;
    ADCSRA |= 5; // prescaler for the ADC, now 32
    // ADCSRA |= 0b00000101;
    ADCSRB |= 3; // timer/counter0 compare match A triggers ADC 
    // ADCSRB |= 0b00000011;
    ADCSRB |= (1 << 3); // MUX 5 for ADC10 
    // ADCSRB |= 0b00001000;
    
    /* The final register values for ADC
    ADMUX  = 0b01000010;
    ADCSRA = 0b10101101; // ADIE, ADEN, ADATE prescaler 32
    ADCSRB = 0b00001011; // timer/counter0 compare match A 
    */  
    
    while (1) 
    {
        /* When the ADC is complete, read the data and print it to LCD */
        if (true == g_b_is_adc_done)
        {
            g_b_is_adc_done = false;

            char adc_voltage_string[16]; // 16-bit array, assumes that the int given is 16-bits
            itoa(g_adc_voltage, adc_voltage_string, 10);
            /* clear display */
            lcd_clrscr();
            /* print ADC result string */
            lcd_puts("The ADC result:");
            lcd_gotoxy(0,1);
            lcd_puts(adc_voltage_string);

        }
        _delay_ms(250);
        
    }
    
    return 0;
}