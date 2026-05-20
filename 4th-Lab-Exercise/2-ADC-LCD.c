/*
* 4th Lab Exercise
* 
* 4.2
* 
*/ 
#define F_CPU 16000000UL // 16 MHz operating frequency
#define PD2 2
#define PD3 3

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>

void write_2_nibbles(uint8_t x) {
    uint8_t temp; 
    /* LCD data HIGH byte */ 
    temp = (PIND & 0x0F) | (x & 0xF0); 
    PORTD = temp; 
    PORTD |= (1 << PD3); // Enable pulse high
    _delay_ms(3);
    PORTD &= ~(1 << PD3); // Enable pulse low 
    /* LCD data LOW byte */
    // swap nibbles
    temp = (PIND & 0x0F) | ((x << 4) & 0xF0);
    PORTD = temp;
    PORTD |= (1 << PD3); // Enable pulse high
    _delay_ms(3);
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

/* TIMER_ISR */ 
ISR(TIMER1_OVF_vect) {
    TCCR1B = 0x00; 
    TCNT1H = (49910 >> 8);
    TCNT1L = 49910;
    TCCR1B = 0x05; 
    /* Start ADC Conversion */
    ADCSRA |= (1 << ADSC); 
    while (ADCSRA & (1 << ADSC)) {};
    lcd_clear_display();
    _delay_ms(5); 
    double voltage = (ADC * 5.0) / 1023.0; 
    uint8_t int_part = (uint8_t)voltage; 
    uint8_t frac_part = (uint8_t)((voltage - int_part) * 100); 
    lcd_data('V');
    lcd_data('=');
    lcd_data(int_part + '0'); 
    lcd_data('.');
    lcd_data((frac_part / 10) + '0');
    lcd_data((frac_part % 10) + '0');
    lcd_data('V');
}

int main() {
    DDRC = 0x00;
    DDRD = 0xFF;
    PORTD = 0x00;
    lcd_init(); 
    lcd_clear_display();
    sei(); 
    /* ADC setup */ 
    ADMUX = 0b01000011; 
    ADCSRA = 0b10000111; // ADIE = 0 
    /* Timer setup */ 
    TCCR1B = 0x00; 
    TIMSK1 = 0x01; 
    TCNT1H = (49910 >> 8); 
    TCNT1L = 49910; 
    TCCR1B = 0b00000101;
    while(1) {
        // loop
    }
    return 0; 
}