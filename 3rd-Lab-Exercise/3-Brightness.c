/*
* 3rd Lab Exercise
* 
* 3.3
* 
*/
#define F_CPU 16000000UL // 16 MHz operating frequency
#define PD0 0
#define PD1 1
#define PB1 1
#define PB4 4
#define PB5 3 // or 5 whatever works best, :)

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

int Mode = 0;
uint16_t DC_VALUE;
int index = 8; // initially 50% DC

const uint8_t OCR_TABLE[17] = {
    5, 20, 36, 51, 66, 82, 97, 112,
    128, 143, 158, 173, 189, 204, 220, 235, 250
};

void SelectMode() {
    if (!(PIND & (1 << PD0))) {
        _delay_ms(100);
        while (!(PIND & (1 << PD0))); // Debounce
        Mode = 1;
    }
    if (!(PIND & (1 << PD1))) {
    }
    _delay_ms(100);
    while (!(PIND & (1 << PD1))); // Debounce
    Mode = 2;
}

uint16_t ConvADC() {
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

void Mode1() {
    /* IF PB4 pressed, increase DC */
    if (!(PINB & (1 << PB4)) && (index < 16)) {
        _delay_ms(200);
        index++;
        OCR1A = OCR_TABLE[index];
        while (!(PINB & (1 << PB4)));
    }
    /* IF PB5 pressed, decrease DC */
    else if (!(PINB & (1 << PB5)) && index > 0) {
        _delay_ms(200);
        index--;
        OCR1A = OCR_TABLE[index];
        while (!(PINB & (1 << PB5)));
    }
}

void Mode2() {
    DC_VALUE = ConvADC(); // Read POT1
    OCR1A = DC_VALUE >> 2; // Scale 10-bit ADC to 8-bit PWM
}

int main(void) {
    /* Initializing timer as in 3.1 */
    TCCR1A = (1 << COM1A1) | (1 << WGM10);
    TCCR1B = (1 << WGM12) | (1 << CS12);
    
    /* PB1 as output,
    * PB4-PB5 as input,
    * PD0-PD1 as input
    */
    DDRB = (1 << PB1);
    DDRD &= ~((1 << PD0) | (1 << PD1));
    
    /* pull-ups */
    PORTB |= (1 << PB4) | (1 << PB5);
    PORTD |= (1 << PD0) | (1 << PD1);
    
    /* Initializing ADC
    * Channel (ADC0) to read from POT1 -> MUX all nulls
    * VREF -> REFS0=1
    * Right adjustment -> ADLAR=0
    */
    ADMUX = (1 << REFS0);
    
    /* Enable ADC -> ADEN=1
    * No conversion -> ADCS=0
    * Disable ADC interrupt -> ADIE=0
    * Prescaler at 128 (125 kHz) -> ADPS[2:0]=111
    */
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    
    /* Init Duty Cycle at 50% */
    DC_VALUE = OCR_TABLE[index];
    OCR1A = DC_VALUE;
    
    while(1) {
        SelectMode();
        
        if (Mode == 1) {
            while (Mode == 1) {
                Mode1();
                SelectMode();
            }
        }
        if (Mode == 2) {
            while (Mode == 2) {
                Mode2();
                SelectMode();
            }
        }
    }
    
    return 0; 
}