/* 2nd Lab Exercise
2.3 (c)
*/
#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

volatile uint16_t led_on = 0;
volatile uint16_t reset_timer = 0;
volatile uint16_t flag = 0;

ISR(INT1_vect) { /* Interrupt Service Routine (INT1) */
    if (!led_on) {
        PORTB = 0x08; // PB3 on
        led_on = 1;
    } else {
        PORTB = 0x3f; // PB5-PB0 on
        reset_timer = 1;
        flag = 1;
    }
}

EIFR = (1 << INTF1);

int main(void) {
    /* INT1 - Rising Edge */
    EICRA = (1 << ISC11) | (1 << ISC10);
    EIMSK = (1 << INT1); // Enable INT1
    sei(); // Enable all interrupts
    
    DDRB = 0xff; // PORTB output
    PORTB = 0x00;
    
    int timer = 0;
    
    while (1) {
        if (led_on) {
            if (reset_timer) {
                if (flag) {
                    _delay_ms(1000);
                    PORTB = 0x08;
                }
                reset_timer = 0;
                timer = 0;
            }
            
            _delay_ms(1);
            timer++;
            
            if ((!flag) && (timer >= 4000)) {
                PORTB = 0x00;
                led_on = 0;
                timer = 0;
            }
            
            if (flag && (timer >= 3000)) {
                PORTB = 0x00;
                led_on = 0;
                timer = 0;
                flag = 0;
            }
        }
    } 
}