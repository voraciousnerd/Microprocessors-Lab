; 1st Lab Exercise
;
; 1.3
.include "m328PBdef.inc"

.def output = r16
.def counter = r17

main: 
    ldi output, 0xFF
    out DDRD, output        ; PORTD output
    
    ldi output, 0x80
    out PORTD, output       ; initialize LSB on
    
    ldi counter, 0x08       ; counter of bits
    ldi r24, low(1000)      ; setup for 1-sec delay
    ldi r25, high(1000)
    
    ; T(0,1) = (<-- , -->) 
    clt                     ; start by (<--) direction

wagon_loop: 
move_R_to_L: 
    dec counter
    breq halt_edge1
    rcall wait_x_msec
    rcall wait_x_msec
    lsr output
    out PORTD, output
    nop
    rjmp move_R_to_L

halt_edge1:
    rcall change_dir

move_L_to_R:
    dec counter
    breq halt_edge2
    rcall wait_x_msec
    rcall wait_x_msec
    lsl output
    out PORTD, output
    rjmp move_L_to_R

halt_edge2: 
    rcall change_dir
    rjmp wagon_loop

; routine to change direction 
change_dir:
    brtc set_T
    clt
    rjmp halt

set_T:
    set

halt:
    rcall wait_x_msec
    ldi counter, 0x08
    ret

; (1.1) waiting routine
wait_x_msec:
    push r24
    push r25

outer_loop:
    ldi r26, low(3999)      ; loads 0xA0
    ldi r27, high(3999)     ; loads 0x0F

inner_loop: 
    sbiw r26, 1
    brne inner_loop
    
    sbiw r24, 1
    brne outer_loop
    
    pop r25
    pop r24
    ret