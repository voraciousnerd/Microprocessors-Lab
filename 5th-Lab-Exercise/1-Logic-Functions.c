/*
* 5th Lab Exercise
* 
* 5.1
* 
*/ 
#define F_CPU 16000000UL  // 16 MHz

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdio.h>

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
#define TW_STATUS_MASK 0b11111000
#define TW_STATUS (TWSR0 & TW_STATUS_MASK)

void twi_init(void); 
unsigned char twi_readAck(void); 
unsigned char twi_readNak(void); 
unsigned char twi_start(unsigned char address); 
void twi_start_wait(unsigned char address);
unsigned char twi_write( unsigned char data);
unsigned char twi_rep_start(unsigned char address);
void twi_stop(void); 
void PCA9555_0_write(PCA9555_REGISTERS reg, uint8_t value); 
uint8_t PCA9555_0_read(PCA9555_REGISTERS reg); 

//initialize TWI clock
void twi_init(void)
{
    TWSR0 = 0; // PRESCALER_VALUE=1
    TWBR0 = TWBR0_VALUE; // SCL_CLOCK 100KHz
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

int main(void) {
    twi_init();
    PCA9555_0_write(REG_CONFIGURATION_0, 0x00); // input
    PCA9555_0_write(REG_CONFIGURATION_1, 0xFF); // output
    
    uint8_t input = 0; 
    uint8_t output = 0; 
    uint8_t F0, F1; 
    uint8_t term1, term2, term3; 
    
    while (1) {
        input = PCA9555_0_read(REG_INPUT_1); 
        input = (~input & 0x0F); // Keep 4 LSBs and invert
        
        /* Calculate terms */
        term1 = (input & 0b00000111); // _CBA
        term2 = (term1 | ((input & 0b00001000) >> 1)); // _DCB
        term3 = (term1 | ((input & 0b00001000) >> 1)); // _DBA
        
        /* Calculate F0 */
        if (term1 == 0x01 || term2 == 0x7) 
            F0 = 0x00; 
        else 
            F0 = 0x01;
            
        /* Calculate F1 */
        if (term2 == 0x7 || term3 == 0x07) 
            F1 = 0x02; 
        else 
            F1 = 0x00; 
            
        output = (F0 | F1); 
        PCA9555_0_write(REG_OUTPUT_0, output); 
    }
    return 0; 
}