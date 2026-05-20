/*
* 5th Lab Exercise
* 
* 5.3
* 
*/ 
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define PCA9555_0_ADDRESS 0x40      
#define TWI_READ 1                  
#define TWI_WRITE 0                 
#define SCL_CLOCK 100000L           
//A0=A1=A2=0 by hardware
// reading from twi device
// writing to twi device
// twi clock in Hz

#define TWBR0_VALUE ((F_CPU/SCL_CLOCK)-16)/2
//Fscl=Fcpu/(16+2*TWBR0_VALUE*PRESCALER_VALUE)

// PCA9555 REGISTERS
typedef enum {
    REG_INPUT_0 = 0,
    REG_INPUT_1 = 1,
    REG_OUTPUT_0 = 2,
    REG_OUTPUT_1 = 3,
    REG_POLARITY_INV_0 = 4,
    REG_POLARITY_INV_1 = 5,
    REG_CONFIGURATION_0 = 6,
    REG_CONFIGURATION_1 = 7,
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
#define TW_STATUS_MASK 0b11111000
#define TW_STATUS (TWSR0 & TW_STATUS_MASK)

//initialize TWI clock
void twi_init(void)
{
    TWSR0 = 0;              
    TWBR0 = TWBR0_VALUE;    
}
// PRESCALER_VALUE=1
// SCL_CLOCK 100KHz

// Read one byte from the twi device ( request more data from device)
unsigned char twi_readAck(void)
{
    TWCR0 = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
    while(!(TWCR0 & (1<<TWINT))); 
    return TWDR0;
}

// Issues a start condition and sends address and transfer direction.
// return 0 = device accessible, 1= failed to access device
unsigned char twi_start(unsigned char address)
{
    uint8_t twi_status;
    // send START condition
    TWCR0 = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
    // wait until transmission completed
    while(!(TWCR0 & (1<<TWINT)));
    // check value of TWI Status Register.
    twi_status = TW_STATUS & 0xF8;
    if ( (twi_status != TW_START) && (twi_status != TW_REP_START)) return 1;
    // send device address
    TWDR0 = address;
    TWCR0 = (1<<TWINT) | (1<<TWEN);
    // wail until transmission completed and ACK/NACK has been received
    while(!(TWCR0 & (1<<TWINT)));
    // check value of TWI Status Register.
    twi_status = TW_STATUS & 0xF8;
    if ( (twi_status != TW_MT_SLA_ACK) && (twi_status != TW_MR_SLA_ACK) )
    {
        return 1; // failed to access device
    }
    return 0;
}

// Send start condition, address, transfer direction.
// Use ACK polling to wait until device is ready
void twi_start_wait(unsigned char address)
{
    uint8_t twi_status;
    while ( 1 )
    {
        // send START condition
        TWCR0 = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
        // wait until transmission completed
        while(!(TWCR0 & (1<<TWINT)));
        // check value of TWI Status Register.
        twi_status = TW_STATUS & 0xF8;
        if ( (twi_status != TW_START) && (twi_status != TW_REP_START))
            continue;
        // send device address
        TWDR0 = address;
        TWCR0 = (1<<TWINT) | (1<<TWEN);
        // wail until transmission completed
        while(!(TWCR0 & (1<<TWINT)));
        // check value of TWI Status Register.
        twi_status = TW_STATUS & 0xF8;
        if ( (twi_status == TW_MT_SLA_NACK )||(twi_status == TW_MR_DATA_NACK) )
        {
            /* device busy, send stop condition to terminate write operation */
            TWCR0 = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
            // wait until stop condition is executed and bus released
            while(TWCR0 & (1<<TWSTO));
            continue;
        }
        break;
    }
}

// Send one byte to twi device, Return 0 if write successful or 1 if write failed
unsigned char twi_write(unsigned char data)
{
    // send data to the previously addressed device
    TWDR0 = data;
    TWCR0 = (1<<TWINT) | (1<<TWEN);
    // wait until transmission completed
    while(!(TWCR0 & (1<<TWINT)));
    if((TW_STATUS & 0xF8) != TW_MT_DATA_ACK) return 1; // write failed
    return 0;
}

// Send repeated start condition, address, transfer direction
//Return: 0 device accessible
// 1 failed to access device
unsigned char twi_rep_start(unsigned char address)
{
    return twi_start(address);
}

// Terminates the data transfer and releases the twi bus
void twi_stop(void)
{
    // send stop condition
    TWCR0 = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
    // wait until stop condition is executed and bus released
    while(TWCR0 & (1<<TWSTO));
}

unsigned char twi_readNak(void)
{
    TWCR0 = (1<<TWINT) | (1<<TWEN);
    while(!(TWCR0 & (1<<TWINT)));
    return TWDR0;
}

void PCA9555_0_write(PCA9555_REGISTERS reg, uint8_t value)
{
    twi_start_wait(PCA9555_0_ADDRESS + TWI_WRITE);
    twi_write(reg);
    twi_write(value);
    twi_stop();
}

uint8_t PCA9555_0_read(PCA9555_REGISTERS reg)
{
    uint8_t ret_val;
    twi_start_wait(PCA9555_0_ADDRESS + TWI_WRITE);
    twi_write(reg);
    twi_rep_start(PCA9555_0_ADDRESS + TWI_READ);
    ret_val = twi_readNak();
    twi_stop();
    return ret_val;
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

int main(){
    DDRB = 0xff;
    DDRC = 0xff;
    DDRD = 0xff;
    twi_init();
    PCA9555_0_write(REG_CONFIGURATION_0, 0x00);
    lcd_init();
    
    while (1){
        lcd_clear_display();
        lcd_command(0x80);
        lcd_string("Dionysis");
        lcd_command(0xC0); 
        lcd_string("Mourelatos");
        _delay_ms(2000);
        lcd_clear_display();
        lcd_command(0x80);
        lcd_string("Luke");
        lcd_command(0xC0);
        lcd_string("Skywalker");
        _delay_ms(2000);
        lcd_clear_display();
    } 
}