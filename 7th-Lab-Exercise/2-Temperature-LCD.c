/*
* 7th Lab Exercise
* 
* 7.2
* 
*/ 
#define F_CPU 16000000UL
#define PD4 4

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define PCA9555_0_ADDRESS 0x40
#define TWI_READ    1
#define TWI_WRITE   0
#define SCL_CLOCK  100000L
//A0=A1=A2=0 by hardware
// reading from twi device
// writing to twi device
// twi clock in Hz
//Fscl=Fcpu/(16+2*TWBR0_VALUE*PRESCALER_VALUE)
#define TWBR0_VALUE ((F_CPU/SCL_CLOCK)-16)/2

// PCA9555 REGISTERS
typedef enum {
    REG_INPUT_0 = 0,
    REG_INPUT_1 = 1,
    REG_OUTPUT_0 = 2,
    REG_OUTPUT_1 = 3,
    REG_POLARITY_INV_0 = 4,
    REG_POLARITY_INV_1 = 5,
    REG_CONFIGURATION_0 = 6,
    REG_CONFIGURATION_1 = 7
} PCA9555_REGISTERS;

//----------- Master Transmitter/Receiver ------------------
#define TW_START 0x08
#define TW_REP_START 0x10
//---------------- Master Transmitter ----------------------
#define TW_MT_SLA_ACK 0x18
#define TW_MT_SLA_NACK 0x20
#define TW_MT_DATA_ACK 0x28
//---------------- Master Receiver ----------------
#define TW_MR_SLA_ACK 0x40
#define TW_MR_SLA_NACK 0x48
#define TW_MR_DATA_NACK 0x58
#define TW_STATUS_MASK  0b11111000
#define TW_STATUS (TWSR0 & TW_STATUS_MASK)

//initialize TWI clock
void twi_init(void)
{
    TWSR0 = 0;
    // PRESCALER_VALUE=1
    TWBR0 = TWBR0_VALUE; // SCL_CLOCK  100KHz
}

// Read one byte from the twi device (request more data from device)
unsigned char twi_readAck(void)
{
    TWCR0 = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
    while(!(TWCR0 & (1<<TWINT)));
    return TWDR0;
}

// Read one byte from the twi device (last byte read)
unsigned char twi_readNak(void)
{
    TWCR0 = (1<<TWINT) | (1<<TWEN);
    while(!(TWCR0 & (1<<TWINT)));
    return TWDR0;
}

// Send start condition, call it with address and read/write mode
unsigned char twi_start(unsigned char address)
{
    uint8_t twi_status;
    // send START condition
    TWCR0 = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
    // wait until transmission completed
    while(!(TWCR0 & (1<<TWINT)));
    // check value of TWI Status Register
    twi_status = TW_STATUS;
    if ( (twi_status != TW_START) && (twi_status != TW_REP_START)) return 1;
    // send device address
    TWDR0 = address;
    TWCR0 = (1<<TWINT) | (1<<TWEN);
    // wail until transmission completed and ACK/NACK has been received
    while(!(TWCR0 & (1<<TWINT)));
    // check value of TWI Status Register
    twi_status = TW_STATUS;
    if ( (twi_status != TW_MT_SLA_ACK) && (twi_status != TW_MR_SLA_ACK) ) return 1;
    return 0;
}

// Send start condition, wait until device accessible
void twi_start_wait(unsigned char address)
{
    uint8_t twi_status;
    while ( 1 )
    {
        // send START condition
        TWCR0 = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
        // wait until transmission completed
        while(!(TWCR0 & (1<<TWINT)));
        // check value of TWI Status Register
        twi_status = TW_STATUS;
        if ( (twi_status != TW_START) && (twi_status != TW_REP_START)) continue;
        // send device address
        TWDR0 = address;
        TWCR0 = (1<<TWINT) | (1<<TWEN);
        // wail until transmission completed
        while(!(TWCR0 & (1<<TWINT)));
        // check value of TWI Status Register
        twi_status = TW_STATUS;
        if ( (twi_status == TW_MT_SLA_NACK ) || (twi_status == TW_MR_SLA_NACK) )
        {
            TWCR0 = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN);
            while(TWCR0 & (1<<TWSTO));
            continue;
        }
        break;
    }
}

// Send repeated start condition, call it with address and read/write mode
unsigned char twi_rep_start(unsigned char address)
{
    return twi_start( address );
}

// Send stop condition
void twi_stop(void)
{
    // send stop condition
    TWCR0 = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN);
    // wait until stop condition is executed and bus becomes free
    while(TWCR0 & (1<<TWSTO));
}

// Send one byte to twi device
unsigned char twi_write(unsigned char data)
{
    uint8_t twi_status;
    // send data to the previously addressed device
    TWDR0 = data;
    TWCR0 = (1<<TWINT) | (1<<TWEN);
    // wait until transmission completed
    while(!(TWCR0 & (1<<TWINT)));
    // check value of TWI Status Register
    twi_status = TW_STATUS;
    if( twi_status != TW_MT_DATA_ACK ) return 1;
    return 0;
}

void PCA9555_0_write(PCA9555_REGISTERS reg, uint8_t value) {
    twi_start_wait(PCA9555_0_ADDRESS + TWI_WRITE);
    twi_write(reg);
    twi_write(value);
    twi_stop();
}

uint8_t PCA9555_0_read(PCA9555_REGISTERS reg) {
    uint8_t ret;
    twi_start_wait(PCA9555_0_ADDRESS + TWI_WRITE);
    twi_write(reg);
    twi_rep_start(PCA9555_0_ADDRESS + TWI_READ);
    ret = twi_readNak();
    twi_stop();
    return ret;
}

#define PD2 2
#define PD3 3

void write_2_nibbles(uint8_t x) {
    uint8_t temp;
    /* LCD data HIGH byte */
    temp = (PCA9555_0_read(REG_INPUT_0) & 0x0F) | (x & 0xF0); 
    PCA9555_0_write(REG_OUTPUT_0, temp);
    PCA9555_0_write(REG_OUTPUT_0, PCA9555_0_read(REG_OUTPUT_0) | (1 << PD3)); // Enable pulse high
    _delay_us(1);
    PCA9555_0_write(REG_OUTPUT_0, PCA9555_0_read(REG_OUTPUT_0) & ~(1 << PD3)); // Enable pulse low
    /* LCD data LOW byte */
    // swap nibbles
    temp = (PCA9555_0_read(REG_INPUT_0) & 0x0F) | ((x << 4) & 0xF0);
    PCA9555_0_write(REG_OUTPUT_0, temp);
    PCA9555_0_write(REG_OUTPUT_0, PCA9555_0_read(REG_OUTPUT_0) | (1 << PD3)); // Enable pulse high
    _delay_us(1);
    PCA9555_0_write(REG_OUTPUT_0, PCA9555_0_read(REG_OUTPUT_0) & ~(1 << PD3)); // Enable pulse low
}

void lcd_data(uint8_t data)
{
    PCA9555_0_write(REG_OUTPUT_0, PCA9555_0_read(REG_OUTPUT_0) | (1 << PD2));
    // LCD_RS = 1, (PD2 = 1) -> For Data
    write_2_nibbles(data);      
    // Send data
    _delay_ms(5);
    return;
}

void lcd_command(uint8_t data)
{
    PCA9555_0_write(REG_OUTPUT_0, PCA9555_0_read(REG_OUTPUT_0) & 0xFB);
    // LCD_RS = 0, (PD2 = 0) -> For Instruction
    write_2_nibbles(data);
    _delay_ms(5);
    return;
}

void lcd_clear_display()
{
    lcd_command(0x01);
    _delay_ms(5);
    return;
}

void lcd_init() {
    _delay_ms(200);
    // 0x30 for 8-bit mode (x3)
    PCA9555_0_write(REG_OUTPUT_0, 0x30);  // #1
    PCA9555_0_write(REG_OUTPUT_0, PCA9555_0_read(REG_OUTPUT_0) | (1 << PD3));
    // Enable pulse
    _delay_us(1);
    PCA9555_0_write(REG_OUTPUT_0, PCA9555_0_read(REG_OUTPUT_0) & ~(1 << PD3));   
    // Clear enable
    _delay_us(250);
    PCA9555_0_write(REG_OUTPUT_0, 0x30);  // #2
    PCA9555_0_write(REG_OUTPUT_0, PCA9555_0_read(REG_OUTPUT_0) | (1 << PD3));
    _delay_us(1);
    PCA9555_0_write(REG_OUTPUT_0, PCA9555_0_read(REG_OUTPUT_0) & ~(1 << PD3));
    _delay_us(250);
    PCA9555_0_write(REG_OUTPUT_0, 0x30);  // #3
    PCA9555_0_write(REG_OUTPUT_0, PCA9555_0_read(REG_OUTPUT_0) | (1 << PD3));
    _delay_us(1);
    PCA9555_0_write(REG_OUTPUT_0, PCA9555_0_read(REG_OUTPUT_0) & ~(1 << PD3));
    _delay_us(250);
    // Send 0x20 command to switch to 4-bit mode
    PCA9555_0_write(REG_OUTPUT_0, 0x20);
    PCA9555_0_write(REG_OUTPUT_0, PCA9555_0_read(REG_OUTPUT_0) | (1 << PD3));
    _delay_us(1);
    PCA9555_0_write(REG_OUTPUT_0, PCA9555_0_read(REG_OUTPUT_0) & ~(1 << PD3));
    _delay_us(250);
    // Set 4-bit mode, 2 lines, 5x8 dots
    lcd_command(0x28);
    // Display, Cursor = ON, OFF
    lcd_command(0x0C);
    // Clear display
    lcd_clear_display();
    // Increase address, no display shift
    lcd_command(0x06);
}

void lcd_string(const char *str) {
    while (*str) lcd_data(*str++);
}

/* === ONE WIRE FUNCTIONS + TEMPERATURE === */ 
/* Detects if there is a connected device */ 
int one_wire_reset () {
    // set PD4 as output
    DDRD |= (1 << PD4);
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
    temperature |= ((uint16_t)one_wire_receive_byte() << 8); // insert the MSByte by shifting left 8 times (0xYX)
    return temperature;
}

typedef struct {
    uint16_t raw_temperature; 
    uint16_t last_temperature; 
    char sign; 
    uint8_t integer_part;
    uint16_t decimal_part; 
} temperature_t;

const uint8_t integer_lookup[7] = {1, 2, 4, 8, 16, 32, 64}; 
const uint16_t decimal_lookup[4] = {62, 125, 250, 500}; 

void format_temperature(temperature_t *t, uint16_t raw_temp) {
    /* find the sign, + or - */
    uint16_t masked_sign = raw_temp & 0xF800; // get bin sign part
    t->sign = (masked_sign == 0xF800) ? '-' : '+'; 
    
    /* use absolute value for lookup */
    int16_t abs_temp = raw_temp;
    if (t->sign == '-')
        abs_temp = (~raw_temp) + 1; // invert bits, then add 1 (2's complement)
        
    /* extract integer & decimal part */
    uint8_t masked_int  = (abs_temp >> 4) & 0x7F;  // get bin integer part 
    uint8_t masked_dec  = abs_temp & 0x0F;         // get bin decimal part
    
    /* convert integer part using lookup table */
    uint16_t local_sum = 0;
    for (int i = 0; i < 7; i++) {
        if (masked_int & 0x01) 
            local_sum += integer_lookup[i];
        masked_int >>= 1;
    }
    t->integer_part = local_sum;
    
    /* convert decimal part using lookup table */
    local_sum = 0;
    for (int i = 0; i < 4; i++) {
        if (masked_dec & 0x01) 
            local_sum += decimal_lookup[i];
        masked_dec >>= 1;
    }
    t->decimal_part = local_sum;
}

void lcd_print_uint(uint16_t n) {
    if (n >= 100) {
        lcd_data((n / 100) + '0');         // hundreds
        lcd_data(((n / 10) % 10) + '0');   // tens
        lcd_data((n % 10) + '0');          // ones
    }
    else if (n >= 10) {
        lcd_data((n / 10) + '0');          // tens
        lcd_data((n % 10) + '0');          // ones
    }
    else {
        lcd_data(n + '0');                 // ones
    }
}

void lcd_print_3decimals(uint16_t n) {
    lcd_data((n / 100) + '0');          // hundreds
    lcd_data(((n / 10) % 10) + '0');    // tens
    lcd_data((n % 10) + '0');           // ones
}

int main () {
    DDRB = 0xFF;
    DDRC = 0x00; 
    
    twi_init(); 
    PCA9555_0_write(REG_CONFIGURATION_0, 0x00);
    lcd_init(); 
    lcd_clear_display(); 
    
    temperature_t t;
    t.last_temperature = 0xFFFF; // an impossible value (ensure 1st loop executes)
    
    while (1) {
        // create (t)emperature struct
        t.raw_temperature = receive_temperature(); 
        if (t.raw_temperature == t.last_temperature)
            goto out; // update lcd iff temp has changed
            
        if (t.raw_temperature == 0x800) {
            lcd_clear_display(); 
            lcd_command(0x80); 
            lcd_string("No Device");
            goto out; 
        }
        
        format_temperature(&t, t.raw_temperature); // pass by reference
        /* testing for negative values (brrrrr *-*) */
        format_temperature(&t, 0xFFF8);
        t.last_temperature = t.raw_temperature;
        
        lcd_command(0x80); 
        lcd_string("Temperature:");
        lcd_command(0xC0); 
        lcd_data(t.sign); 
        lcd_print_uint(t.integer_part); 
        lcd_data('.'); 
        lcd_print_3decimals(t.decimal_part); 
        lcd_string(" "); 
        lcd_data((char)223); 
        lcd_data('C'); 
        
    out: 
        _delay_ms(750); 
        continue;
    }
    
    return 0;
}