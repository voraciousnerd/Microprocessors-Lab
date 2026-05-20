/*
* 3rd Lab Exercise
* 
* 3.2
* 
*/
#define F_CPU 16000000UL // 16 MHz operating frequency
#define PB1 1
#define PB3 3
#define PB4 4
#define PB5 3 // 3 or 5 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

uint16_t average;
uint8_t input;
int index = 8; // initially 50% DC

const uint8_t OCR_TABLE[17] = {
    5, 20, 36, 51, 66, 82, 97, 112, 128,
    143, 158, 173, 189, 204, 220, 235, 250
};

int main(void) {
    /* Initializing timer as in 3.1 */
    TCCR1A = (1 << COM1A1) | (1 << WGM10);
    TCCR1B = (1 << WGM12) | (1 << CS12);
    
    /* PB1 as output,
    * PB4-PB5 as input,
    * PORTD as output
    */
    DDRB = (1 << PB1);
    DDRD |= 0b00011111;
    PORTB |= (1 << PB4) | (1 << PB5); // pull-ups
    
    /* Initializing ADC
    * Channel (ADC1) to read from PB1_PWM -> MUX0=1
    * VREF -> REFS0=1
    * Right adjustment -> ADLAR=0
    */
    ADMUX = (1 << REFS0) | (1 << MUX0);
    
    /* Enable ADC -> ADEN=1
    * No conversion -> ADCS=0
    * Disable ADC interrupt -> ADIE=0
    * Prescaler at 128 (125 kHz) -> ADPS[2:0]=111
    */
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    
    /* Init DC at 50% */
    OCR1A = OCR_TABLE[index];
    
    while(1) {
        average = 0;
        
        for (int i = 0; i < 16; i++) {
            /* IF PB4 pressed, increase DC */
            if (!(PINB & (1 << PB4)) && (index < 16)) {
                _delay_ms(50);
                index++;
                OCR1A = OCR_TABLE[index];
                while (!(PINB & (1 << PB4)));
            }
            /* IF PB5 pressed, decrease DC */
            else if (!(PINB & (1 << PB5)) && index > 0) {
                _delay_ms(50);
                index--;
                OCR1A = OCR_TABLE[index];
                while (!(PINB & (1 << PB5)));
            }
            else {
                _delay_ms(50);
            }
            
            _delay_ms(50);
            ADCSRA |= (1 << ADSC);
            while (ADCSRA & (1 << ADSC));
            average += ADC;
        }
        
        average = (average >> 4);
        
        if ((average >= 0) && (average <= 200)) {
            PORTD = 0x01;
        }
        else if (average <= 400) {
            PORTD = 0x02;
        }
        else if (average <= 600) {
            PORTD = 0x04;
        }
        else if (average <= 800) {
            PORTD = 0x08;
        }
        else {
            PORTD = 0x10;
        }
    }
    
    return 0;
}