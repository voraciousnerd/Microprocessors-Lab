; 4th Lab Exercise
; 
; 4.1
.include "m328PBdef.inc"
.equ FOSC_MHZ = 16              ; microcontroller operating frequency
.equ DEL_mS = 50                ; delay in ms
.equ DEL_NU = FOSC_MHZ * DEL_mS ; used for delay subroutine
.equ PD0 = 0 
.equ PD1 = 1
.equ PD2 = 2
.equ PD3 = 3
.equ PD4 = 4
.equ PD5 = 5
.equ PD6 = 6
.equ PD7 = 7
.equ ASCII_INIT = 0x30

.def temp = r16
.def val = r19
.def ADC_L = r21
.def ADC_H = r22

; 0x00 - reset
; 0x1A - TIMER1 interrupt
; 0x2A - ADC interrupt
.org 0x00
    rjmp reset
.org 0x1A
    rjmp TIMER_ISR
.org 0x2A 
    rjmp ADC_ISR

reset: 
    ldi temp, high(RAMEND)
    out SPH, temp
    ldi temp, low(RAMEND)
    out SPL, temp 
    
    ser temp
    out DDRD, temp
    clr temp 
    out DDRC, temp
    out PORTD, temp             ; set PORTD as output, set PORTC as input, default 0 
    
    rcall lcd_init
    rcall lcd_clear_display
    sei                         ; enable interrupts
    
    ; --- setting up ADC --
    ; REFSn[1:0] = 01   (Vref = 5V)
    ; ADLAR = 0         (right adjusted)
    ; MUXn[3:0]  = 0011 (ADC3 input)
    ldi temp, 0b01000011
    sts ADMUX, temp
    
    ; ADEN  = 1         (ADC enable)
    ; ADSC  = 0         (don't start conversion)
    ; ADIE  = 1         (enable ADC interrupt)
    ; ADPS[2:0] = 111   (prescaler to 128 -> 125kHz)
    ldi temp, 0b10001111
    sts ADCSRA, temp
    
    ; --- setting up timer --
    clr temp 
    sts TCCR1B, temp            ; timer stopped / normal func
    sts TCCR1A, temp
    ldi temp, 0x01
    sts TIMSK1, temp            ; normal function, allow overflow interrupts
    
    ; timer shall overflow every 1 second
    ; 1*15625 cc => 65535-15625 = [49910] initial value
    ldi temp, high(49910)
    sts TCNT1H, temp
    ldi temp, low(49910)
    sts TCNT1L, temp
    
    ; start timer
    ; CS1n[2:0] = 101 (CLK/1024)
    ldi temp, 0b00000101
    sts TCCR1B, temp

main:
    rjmp main                   ; loop 

; Interrupt Service Routines
TIMER_ISR: 
    clr temp
    sts TCCR1B, temp
    ldi temp, high(49910)
    sts TCNT1H, temp
    ldi temp, low(49910)
    sts TCNT1L, temp
    ldi temp, 0b00000101
    sts TCCR1B, temp            ; restart timer
    
    lds temp, ADCSRA
    ori temp, (1 << ADSC)
    sts ADCSRA, temp            ; start ADC conversion 
    reti

ADC_ISR: 
    rcall lcd_clear_display
    lds ADC_L, ADCL
    lds ADC_H, ADCH
    mov r17, ADC_L
    mov r18, ADC_H
    
    ; 5*r <=> (r << 2) + r
    lsl ADC_L
    rol ADC_H
    lsl ADC_L
    rol ADC_H
    add ADC_L, r17
    adc ADC_H, r18              ; we got 5 * ADC
    
    ldi r24, 'V'
    rcall lcd_data
    ldi r24, '='
    rcall lcd_data
    
    ; find integer part (5*ADC/1024)
    clr val
    rcall divide 
    mov r24, val
    ldi temp, ASCII_INIT
    add r24, temp
    rcall lcd_data
    
    ; seperate x.yz
    ldi r24, '.'
    rcall lcd_data
    
    ; find 1st decimal (remainder*10/1024)
    ; r*10 <=> (r << 3) + r + r
    mov r17, ADC_L
    mov r18, ADC_H
    lsl ADC_L
    rol ADC_H
    lsl ADC_L
    rol ADC_H
    lsl ADC_L
    rol ADC_H
    add ADC_L, r17
    adc ADC_H, r18
    add ADC_L, r17
    adc ADC_H, r18
    clr val
    rcall divide 
    mov r24, val
    ldi temp, ASCII_INIT
    add r24, temp
    rcall lcd_data
    
    ; find 2nd decimal (remainder*10/1024)
    ; r*10 <=> (r << 3) + r + r
    mov r17, ADC_L
    mov r18, ADC_H
    lsl ADC_L
    rol ADC_H
    lsl ADC_L
    rol ADC_H
    lsl ADC_L
    rol ADC_H
    add ADC_L, r17
    adc ADC_H, r18
    add ADC_L, r17
    adc ADC_H, r18
    clr val
    rcall divide 
    mov r24, val
    ldi temp, ASCII_INIT
    add r24, temp
    rcall lcd_data
    
    ldi r24, 'V'
    rcall lcd_data
    reti

; Division subroutine (compare HByte with 4 -> 4 * 256 = 1024)
divide:
    cpi ADC_H, 0x04              ; compare with 4
    brlo end                    ; if < 4 branch to end
    subi ADC_H, 0x04             ; else subtract
    inc val                     ; and increment quotient
    rjmp divide                 ; repeat
end: 
    ret

; Basic LCD + Delay subroutines 
write_2_nibbles: 
    push r24
    in r25, PIND
    andi r25, 0x0f
    andi r24, 0xf0
    add r24, r25
    out PORTD, r24
    sbi PORTD, PD3
    nop 
    nop
    cbi PORTD, PD3
    pop r24
    swap r24
    andi r24, 0xf0
    add r24, r25
    out PORTD, r24
    sbi PORTD, PD3
    nop
    nop
    cbi PORTD, PD3
    ret

lcd_data: 
    sbi PORTD, PD2
    rcall write_2_nibbles
    ldi r24, 250
    ldi r25, 0
    rcall wait_usec
    ret

lcd_command: 
    cbi PORTD, PD2
    rcall write_2_nibbles
    ldi r24, 250
    ldi r25, 0
    rcall wait_usec
    ret

lcd_clear_display:
    ldi r24, 0x01
    rcall lcd_command
    ldi r24, low(5)
    ldi r25, high(5)
    rcall wait_msec
    ret

lcd_init: 
    ldi r24, low(200)
    ldi r25, high(200)
    rcall wait_msec
    ldi r24, 0x30
    out PORTD, r24
    sbi PORTD, PD3
    nop
    nop
    cbi PORTD, PD3
    ldi r24, 250
    ldi r25, 0
    rcall wait_usec
    ldi r24, 0x30
    out PORTD, r24
    sbi PORTD, PD3
    nop
    nop
    cbi PORTD, PD3
    ldi r24, 250
    ldi r25, 0
    rcall wait_usec
    ldi r24, 0x30
    out PORTD, r24
    sbi PORTD, PD3
    nop
    nop
    cbi PORTD, PD3
    ldi r24, 250
    ldi r25, 0
    rcall wait_usec
    ldi r24, 0x20
    out PORTD, r24
    sbi PORTD, PD3
    nop
    nop
    cbi PORTD, PD3
    ldi r24, 250
    ldi r25, 0
    rcall wait_usec
    ldi r24, 0x28
    rcall lcd_command
    ldi r24, 0x0c
    rcall lcd_command
    rcall lcd_clear_display
    ldi r24, 0x06
    rcall lcd_command
    ret

wait_msec: 
    push r24
    push r25
    ldi r24, low(999)
    ldi r25, high(999)
    rcall wait_usec
    pop r25
    pop r24
    nop
    nop
    sbiw r24, 1
    brne wait_msec
    ret

wait_usec: 
    sbiw r24, 1
    call delay_8cycles
    brne wait_usec
    ret

delay_8cycles: 
    nop
    nop
    nop
    nop
    ret