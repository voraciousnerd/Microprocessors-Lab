/*
* 7th Lab Exercise
* 
* 7.1
* 
*/ 
#define F_CPU 16000000UL
#define PD4 4

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

/* Detects if there is a connected device */ 
int one_wire_reset () {
    DDRD |= (1 << PD4);            // set PD4 as output
    PORTD &= ~(1 << PD4);          // set 0 
    _delay_us(480);                // wait 480 us
    DDRD &= ~(1 << PD4);           // set PD4 as input
    PORTD &= ~(1 << PD4);          // disable pull-up
    _delay_us(100);                // wait 100us for connected devices to transmit the presence pulse
    
    uint8_t x = PIND & (1 << PD4); // read PORTD
    _delay_us(380);                // wait for 380us
    
    if (x == 0x10)
        return 0;                  // if there is NO device (PD4 == 1)
    else 
        return 1;                  // if there is a connected device (PD4 == 0)
}

/* Reads one BIT from the sensor via PD4 */ 
uint8_t one_wire_receive_bit () {
    DDRD |= (1 << PD4);            // set PD4 as output
    PORTD &= ~(1 << PD4);
    _delay_us(2); 
    DDRD &= ~(1 << PD4);           // set PD4 as input
    PORTD &= ~(1 << PD4);          // disable pull-up
    _delay_us(10);                 // wait for 10us
    
    /* if PD4 set, return 1, else 0 */
    uint8_t bit = (PIND & (1 << PD4)) ? 1 : 0; 
    _delay_us(49);                 // delay 49us to meet the standards 
    return bit; 
}

/* Reads one BYTE from the sensor */ 
uint8_t one_wire_receive_byte () {
    uint8_t byte = 0x00;
    uint8_t bit = 0x00; 
    
    /* Received LSB to MSB */ 
    for (int i = 0; i < 8; i++) {
        bit = one_wire_receive_bit();
        byte |= (bit << i);        // 'or' to insert bit into byte 
    }
    return byte; 
}

/* Transmits one BIT to the sensor via PD4 */
void one_wire_transmit_bit (uint8_t bit) {
    DDRD |= (1 << PD4);            // set PD4 as output
    PORTD &= ~(1 << PD4);          // set PD4 to 0
    _delay_us(2);
    
    /* Transmit 1 or 0 based on 'bit' */ 
    if (bit & 0x01)
        PORTD |= (1 << PD4); 
    else
        PORTD &= ~(1 << PD4); 
        
    _delay_us(58);                 // wait 58us for device to sample the line
    DDRD &= ~(1 << PD4);           // set PD4 as input
    PORTD &= ~(1 << PD4);          // disable pull-up
    _delay_us(1);                  // recovery time 1us
}

/* Transmits one BYTE to the sensor */
void one_wire_transmit_byte (uint8_t byte) {
    uint8_t bit; 
    /* Transmit one bit at a time, using masking */
    for (int i = 0; i < 8; i++) {
        bit = (0x01 & (byte >> i));  
        one_wire_transmit_bit(bit); 
    }
}

uint16_t receive_temperature () {
    if (!(one_wire_reset()))
        return 0x800;              // error: no connected device
        
    one_wire_transmit_byte(0xCC);  // bypass device selection
    one_wire_transmit_byte(0x44);  // start temperature sensing 
    
    /* Block until temperature is ready (1) */ 
    while (!(one_wire_receive_bit()));
    
    if (!(one_wire_reset()))
        return 0x800;              // re-initialize just for safety 
        
    one_wire_transmit_byte(0xCC);
    one_wire_transmit_byte(0xBE);  // 16-bit temp reading 
    
    uint16_t temperature = 0x00;
    temperature |= one_wire_receive_byte(); // read the LSByte (0x0X)
    temperature |= ((uint16_t)one_wire_receive_byte() << 8); // the MSByte by shifting left 8 times (0xYX)
    
    return temperature;
}

int main () {
    // nothing happens yet
    return 0; 
}