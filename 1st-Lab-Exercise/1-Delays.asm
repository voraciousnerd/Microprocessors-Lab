; 1st Lab Exercise
;
; 1.1
.include "m328PBdef.inc"

main: 
    ldi r24, low(5000)  ; load x-msec to count 
    ldi r25, high(5000)
    ser r16
    out DDRB, r16

loop: 
    sbi PORTB, 0
    ; to check on board (led on)
    rcall wait_x_msec
    cbi PORTB, 0
    ; led off
    rcall wait_x_msec
    nop ; used for debugging 
    rjmp loop

; waiting routine (f = 16 MHz)
wait_x_msec:
    push r24
    push r25
    outer_loop:
    ldi r26, low(3999) ; loads 0xA0
    ldi r27, high(3999) ; loads 0x0F
    inner_loop: 
    sbiw r26, 1
    brne inner_loop
    sbiw r24, 1
    brne outer_loop
    pop r25
    pop r24
    ret