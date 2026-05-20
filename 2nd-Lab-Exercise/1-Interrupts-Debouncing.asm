; 2nd Lab Exercise
;
; 2.1
.include "m328PBdef.inc"

.equ FOSC_MHZ = 16              ; microcontroller operating frequency
.equ DEL_mS = 500               ; delay in ms
.equ DEL_NU = FOSC_MHZ * DEL_mS ; used for delay subroutine
.equ DEL_INT1 = FOSC_MHZ * 50   ; for 50 ms delay 

.def temp = r16                 ; just a helping register
.def counter = r17              ; used to count interrupts 

.org 0x0
    rjmp reset

.org 0x4
    rjmp isr1

reset: 
    ; init stack pointer
    ldi temp, low(RAMEND)
    out SPL, temp
    ldi temp, high(RAMEND)
    out SPH, temp

    ; init PORTB & PORTC as output
    ser temp
    out DDRB, temp
    out DDRC, temp

    ; init PORTD as input
    clr temp
    out DDRD, temp 

    ; interrupt on rising edge of INT1 pin
    ldi temp, (1<<ISC11|1<<ISC10)
    sts EICRA, temp

    ; enable the INT1 interrupt (PD3)
    ldi temp, (1<<INT1)
    out EIMSK, temp

    sei                         ; sets global interrupt flag
    clr counter                 ; init counter for interrupts

loop1: 
    clr r26

loop2: 
    cpi counter, 0x20           ; check if counter is 32 (0-31 interrupts)
    brne cont
    clr counter                 ; if it is, clear

cont: 
    lsl counter                 ; shift left to output LSB in PC1 
    out PORTC, counter
    lsr counter                 ; undo left shift
    
    out PORTB, r26
    ldi r24, low(DEL_NU)
    ldi r25, high(DEL_NU)       ; set delay (num of cycles)
    rcall delay_mS
    
    inc r26
    cpi r26, 16                 ; compare r26 with 16
    breq loop1
    rjmp loop2

; === ISR for INT1 interrupt ===
isr1: 
    push r23
    push r24
    push r25
    push r26
    in temp, SREG
    push temp

; --- debouncing --- 
debounce: 
    ldi temp, (1<<INTF1)
    out EIFR, temp              ; clear flag (EIFR.1 == 1)
    ldi r24, low(DEL_INT1) 
    ldi r25, high(DEL_INT1)     ; delay 50 ms
    rcall delay_mS
    
    sbis EIFR, 1                ; if flag clear (1) skip the next instruction
    rjmp debounce               ; else debounce loop
; ------------------

    sbic PIND, 1                ; if PD1 pressed, freeze counter (skip inc)
    inc counter 
    
    ldi temp, (1<<INTF1)
    out EIFR, temp              ; clear flag (EIFR.1 == 1)
    
    pop temp
    out SREG, temp
    pop r26
    pop r25
    pop r24
    pop r23
    reti                        ; return from interrupt

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