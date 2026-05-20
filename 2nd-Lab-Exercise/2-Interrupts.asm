; 2nd Lab Exercise
;
; 2.2
.include "m328PBdef.inc"
.equ FOSC_MHZ = 16 ; microcontroller operating frequency
.equ DEL_mS = 1000
; delay in ms
.equ DEL_NU = FOSC_MHZ * DEL_mS ; used for delay subroutine
.equ DEL_INT0 = FOSC_MHZ * 5
; for 5 ms delay 
.def temp = r16 ; just a helping register
.def counter = r17 ; used to count 0-31 
.def countb = r18 ; used to count pressed buttons of PortB
.org 0x0
    rjmp reset
.org 0x2
    rjmp isr0
reset: 
    ; init stack pointer
    ldi temp, low(RAMEND)
    out SPL, temp
    ldi temp, high(RAMEND)
    out SPH, temp
    ; init PORTC as output
    ser temp
    out DDRC, temp
    ; init PORTB & PORTD as input
    clr temp
    out DDRB, temp
    out DDRD, temp
    ; interrupt on rising edge of INT0 pin
    ldi temp, (1<<ISC01 | 1<<ISC00)
    sts EICRA, temp
    ; enable the INT0 interrupt (PD2)
    ldi temp, (1<<INT0)
    out EIMSK, temp
    sei ; sets global interrupt flag
loop1: 
    clr counter
loop2: 
    out PORTC, counter
    ldi r24, low(DEL_NU)
    ; 
    ldi r25, high(DEL_NU) ; set delay (num of cycles)
    rcall delay_mS
    ; 
    inc counter
    cpi counter, 32 ; compare with 32
    breq loop1
    rjmp loop2
; === ISR for INT0 interrupt ===
isr0: 
    push r23
    ;
    push r24
    push r25
    ; used in delay subroutine
    ; 
    in temp, SREG
    push temp
; --- debouncing --
debounce: 
    ldi temp, (1<<INTF0)
    out EIFR, temp ; clear flag (EIFR.0 == 1)
    ldi r24, low(DEL_INT0) ;
    ldi r25, high(DEL_INT0) ; delay 5 ms
    rcall delay_mS
    sbis EIFR, 0
    ;
    ; if flag clear (1) skip the next instruction
    rjmp debounce ; and continue with interrupt routine, 
    ; else debounce loop
;------------------
    rcall countPB ; routine to count pressed buttons
    rcall outputPORTC ; routine to output accordingly
    ldi temp, (1<<INTF0)
    out EIFR, temp ; clear flag (EIFR.0 == 1)
    pop temp
    out SREG, temp
    pop r25
    pop r24
    pop r23
    reti
    ; return from interrupt
; === delay subroutine ===
delay_mS: 
    ldi r23, 249
loop_inn: 
    dec r23
    nop
    brne loop_inn
    sbiw r24, 1
    brne delay_mS
    ret
; === PB(1-4) button counter ===
countPB: 
    in temp, PINB
    com temp
    andi temp, 0b00011110      
    ldi  r19, 5               
    clr  countb
count_loop:
    lsr temp                  
    brcc skip_inc             
    inc countb
skip_inc:
    dec r19
    brne count_loop
    ret
    ; keep bits 1–4 only
    ; loop 4 times
    ; shift next bit into carry
    ; if carry = 0, skip inc
; === make output for PORTC ===
outputPORTC: 
    ldi temp, 0x1  
    clr r19
    cpi countb, 0
    breq leds_done
leds_on:
    or r19, temp
    lsl temp
    dec countb
    brne leds_on
leds_done: 
    out PORTC, r19
    ret