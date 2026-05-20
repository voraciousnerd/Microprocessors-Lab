/*
* 4th Lab Exercise
* 
* 4.3
* 
*/ 
#define  F_CPU 16000000UL  // 16 MHz
#define  PD2 2
#define PD3 3

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdio.h>

volatile uint32_t ms_counter = 0; 
volatile uint32_t last_led_toggle = 0;
volatile bool led_state = false;
volatile uint16_t adc_result = 0; 
volatile uint8_t adc_flag = 0; 
volatile uint8_t fresh_adc_data = 0; 

void timer1_init () {
    TCCR1A = 0; 
    TCCR1B = (1 << CS11) | (1 << CS10); // prescaler 64
    TCNT1 = 65286;
    TIMSK1 = (1 << TOIE1);
}
// preload for 1ms overflow
// enable overflow interrupt

void adc_init () {
    ADMUX = 0b01000011;
    ADCSRA = 0b10001111; // enable interrupt
}

double calc_PPM () {
    double voltage = 5.0 * adc_result / 1024.0; // used given formula 
    double PPM_value = 10000 * (voltage - 0.1) / 129.0; // another formula
    return PPM_value;
}

void blink (uint8_t out, uint8_t blink_interval) {
    uint32_t current_ms;
    cli(); // Disable interrupts for atomic reading
    current_ms = ms_counter;
    sei(); // Re-enable interrupts
    
    if (current_ms - last_led_toggle >= blink_interval) {
        led_state = !led_state;  // toggle LED state
        if (led_state) PORTB = out;   // turn LED ON
        else   
            PORTB = 0x00;  // turn LED OFF
        last_led_toggle = current_ms;   // reset timer reference
    }
}

/* Basic LCD routines */
void write_2_nibbles(uint8_t x) {
    uint8_t temp;
    /* LCD data HIGH byte */
    temp = (PIND & 0x0F) | (x & 0xF0);
    PORTD = temp;
    PORTD |= (1 << PD3); // Enable pulse high
    _delay_us(5);
    PORTD &= ~(1 << PD3); // Enable pulse low
    /* LCD data LOW byte */
    // swap nibbles
    temp = (PIND & 0x0F) | ((x << 4) & 0xF0);
    PORTD = temp;
    PORTD |= (1 << PD3); // Enable pulse high
    _delay_us(5);
    PORTD &= ~(1 << PD3); // Enable pulse low
}

void lcd_data(uint8_t data) {
    PORTD |= (1 << PD2);
    // data
    write_2_nibbles(data); // Send data
    _delay_us(250);
    // Wait 250us
}

void lcd_command(uint8_t command){
    PORTD &= ~(1 << PD2); // instruction
    write_2_nibbles(command);
    _delay_us(250);
    // Wait 250us
}

void lcd_clear_display() {
    lcd_command(0x01);
    _delay_ms(5);
}

void lcd_init() {
    _delay_ms(200);
    /* Command to switch to 8 bit mode (3 times) */
    for (int i = 0; i < 3; i++) {
        PORTD = 0x30;
        PORTD |= (1 << PD3);
        _delay_us(5);
        PORTD &= ~(1 << PD3);
        _delay_us(250);
    }
    /* Command to switch to 4 bit mode */
    PORTD = 0x20;
    PORTD |= (1 << PD3);
    _delay_us(5);
    PORTD &= ~(1 << PD3);
    _delay_us(250);
    /* 2 lines, 5x8 dots */
    lcd_command(0x28);
    /* Display, Cursor = ON, OFF */
    lcd_command(0x0C);
    /* Clear display */
    lcd_clear_display();
    /* Increase address, no display shift */
    lcd_command(0x06);
}

void lcd_string(const char *str) {
    lcd_clear_display();
    // lcd_command(0x80); // set the cursor line 1 
    while (*str) lcd_data(*str++);
}

/* TIMER_ISR */
ISR(TIMER1_OVF_vect) {
    ms_counter++;
    /* Every 100ms -> ADC sampling */
    if (ms_counter % 100 == 0) {
        adc_flag = 1; 
    }
    TCNT1 = 65286; // Reset timer
}

/* ADC_ISR */
ISR(ADC_vect) {
    fresh_adc_data = 1; // new data
    adc_result = ADC;  
}

int main () {
    /* IO setup */ 
    DDRC = 0x00;    
    DDRD = 0xFF;    
    DDRB = 0xFF;    
    // ADC input
    // PORTD output
    // LEDs
    
    /* Timer + ADC + LCD + Interrupts setup */
    timer1_init(); 
    adc_init(); 
    lcd_init();
    lcd_clear_display();
    sei(); 
    
    int gas = 0;  
    double ppm = 0; 
    
    while (1) {
        if (adc_flag) {
            adc_flag = 0;           
            // clear the flag
            ADCSRA |= (1 << ADSC);  // start ADC conversion
        }
        
        if (fresh_adc_data) {
            fresh_adc_data = 0;
            ppm = calc_PPM();
            if (ppm < 76) {
                if (ppm < 63) PORTB = 0x01; 
                else  
                    PORTB = 0x03;
                if (gas) {  // Transition from gas -> clear
                    lcd_string("CLEAR");
                    gas = 0;
                    led_state = false;
                }
            } else {  // ppm >= 76
                if (!gas) {  // Transition from clear ? gas
                    lcd_string("GAS DETECTED");
                    gas = 1;
                }
            }
        }
        
        if (ppm >= 76) {
            if (ppm < 126)       
                blink(0x03, 250);
            else if (ppm < 188)  blink(0x07, 250);
            else if (ppm < 251)  blink(0x0F, 200);
            else if (ppm < 313)  blink(0x1F, 200);
            else if (ppm < 376)  blink(0x3F, 100);
            else 
                PORTB = 0x3F;
        }
    }
    return 0; 
}