/*
* 6th Lab Exercise
* 
* 6.2
* 
*/ 
#define F_CPU 16000000UL
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
    _delay_ms(5);
    return;
}

// Send data
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

/* USEFUL STRUCTURES FOR FUNCTIONALITY 
* 
* ROW_MASKS     ->  mask for each individual row
* pressed_keys  ->  current keyboard status
* KEY_ASCII     ->  bit-to-ASCII lookup-table 
*
*/ 
const uint8_t ROW_MASKS[4] = {0xFE, 0xFD, 0xFB, 0xF7};
uint16_t pressed_keys = 0;        
const char KEY_ASCII[16] = {
    '*', '0', '#', 'D', // Row 1, bits 0–3
    '7', '8', '9', 'C', // Row 2, etc.
    '4', '5', '6', 'B',
    '1', '2', '3', 'A'
};

/* 
* This struct is designed to store both the full keypad state and the newly pressed keys.
* - "pressed_keys" keeps track of all keys currently pressed.
* - "new_keys" tracks only the keys that were newly pressed since the last scan.
* This allows us to handle both continuous actions (holding a key) 
* and one-time events (new presses), which is useful for this and future exercises.
*/
typedef struct {
    uint16_t all_pressed; 
    uint16_t new_pressed;
} KeyStatus;

KeyStatus k = {0, 0}; 

/* --- SCAN ROW --- */ 
uint8_t scan_row(uint8_t row_num) {
    if (row_num < 1 || row_num > 4) return 0;  // invalid row
    uint8_t row_mask = ROW_MASKS[row_num - 1];
    PCA9555_0_write(REG_OUTPUT_1, row_mask);
    uint8_t input = PCA9555_0_read(REG_INPUT_1);
    // return just column information (0 -> pressed key) (LOW)
    return (input & 0xF0) >> 4;
}

/* --- SCAN KEYPAD --- 
*
* Row 4: [Bit 12] [Bit 13] [Bit 14] [Bit 15]  -> Col1 Col2 Col3 Col4
* Row 3: [Bit 8 ] [Bit 9 ] [Bit 10] [Bit 11]  -> Col1 Col2 Col3 Col4
* Row 2: [Bit 4 ] [Bit 5 ] [Bit 6 ] [Bit 7 ]  -> Col1 Col2 Col3 Col4
* Row 1: [Bit 0 ] [Bit 1 ] [Bit 2 ] [Bit 3 ]  -> Col1 Col2 Col3 Col4 (LSB)
*
*/
uint16_t scan_keypad () {
    uint16_t state = 0;
    for (uint8_t row = 1; row <= 4; row++) {
        state |= ((uint16_t)scan_row(row)) << ((row - 1) * 4);
    }
    return ~state; // Inverse logic for easier manipulation (PRESSED -> HIGH)
}

/* --- SCAN KEYPAD RISING EDGE --- */ 
KeyStatus scan_keypad_rising_edge () {
    uint16_t pressed_keys_tempo = 0;  
    k.new_pressed = 0; 
    
    pressed_keys_tempo = scan_keypad(); // Read current state
    _delay_ms(15);                      // Debouncing delay
    pressed_keys_tempo &= scan_keypad(); // Keep only stable pressed keys
    
    k.new_pressed = pressed_keys_tempo & (~k.all_pressed); // Find newly pressed keys
    k.all_pressed = pressed_keys_tempo; // Update pressed keys
    return k; // (PRESSED -> HIGH) logic 
}

/* --- KEYPAD TO ASCII --- 
*
* !!! ONLY ONE KEY PRESSED EACH TIME !!!
*
*/
KeyStatus keypad_to_ascii () {
    KeyStatus kk = scan_keypad_rising_edge();
    int flag[2] = {0}; 
    uint16_t new_keys = kk.new_pressed;
    uint16_t all_keys = kk.all_pressed;
    
    for (uint8_t bit = 0; bit < 16; bit++) {
        if (new_keys & (1 << bit)) { // if key is newly pressed
            kk.new_pressed = KEY_ASCII[bit]; // lookup ASCII + return
            flag[0] = 1; 
        }
        if (all_keys & (1 << bit)) { // if key is stably pressed
            kk.all_pressed = KEY_ASCII[bit]; // lookup ASCII + return
            flag[1] = 1; 
        }
    }
    if (flag[0] == 0) kk.new_pressed = '-'; 
    if (flag[1] == 0) kk.all_pressed = '-'; 
    return kk;  
}

int main (void) {
    twi_init();
    DDRB = 0xFF; // PORTB as output
    DDRC = 0xFF; // PORTC as output
    DDRD = 0xFF; // IO as output to LCD
    
    PCA9555_0_write(REG_CONFIGURATION_1, 0xF0); // IO1[0:3] as output, IO1[4:7] as input
    PCA9555_0_write(REG_CONFIGURATION_0, 0x00);
    lcd_init(); 
    
    char last_key = '-';
    char key = '-';  
    
    // initial display
    lcd_clear_display();
    lcd_command(0x80);
    lcd_string("Last Pressed: ");
    lcd_data(last_key);
    
    while(1) {
        key = keypad_to_ascii().new_pressed;

        if (key != '-' && key != last_key) {
            // Update LCD display
            lcd_clear_display();
            lcd_command(0x80);
            lcd_string("Last Pressed: ");
            lcd_data(last_key);

            last_key = key;
        }
    }
    
    return 0; 
}