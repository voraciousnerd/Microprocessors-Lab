; 2nd Lab Exercise
;
; 2.3 (AVR)
.include "m328PBdef.inc"
.equ FOSC_MHZ = 16 ; microcontroller operating frequency
.equ DEL_1s = FOSC_MHZ * 1000 ; for 1 sec delay
.def temp = r16
.def light_flag = r17 ; flag set when light is on 
.def refresh_flag = r18 ; flag set on interrupt
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
    ; init PORTS
    ser temp
    out DDRB, temp ; output
    clr temp
    out DDRD, temp ; input 
    out PORTB, temp ; light off
    ; init INT1 
    ldi temp, (1<<ISC11) | (1<<ISC10)
    sts EICRA, temp
    ldi temp, (1<<INT1)
    out EIMSK, temp
    sei 
    clr refresh_flag
    clr light_flag
main: 
    sei
    sbrs refresh_flag, 0
    rjmp main
light_on: 
    cpi light_flag, 0x0
    brne refresh
    ; 
    ; loop (wait for interrupt)
    ; light is already on
    ; turn on light
    sbi PORTB, 3
    ldi light_flag, 1 ; set flag light is on 
    clr refresh_flag
    rjmp timer_loop
    ; get ready for new interrupt
refresh: 
    ldi temp, 0x3f 
    out PORTB, temp
    clr refresh_flag
timer_loop: 
    ldi r24, low(DEL_1s)
    ldi r25, high(DEL_1s)
    rjmp delay_mS1
return_from_1delay:
    ldi temp, 0x8
    out PORTB, temp
    ldi r24, low(3*DEL_1s)
    ldi r25, high(3*DEL_1s)
    rjmp delay_mS3
return_from_3delay:
turn_off: 
    cbi PORTB, 3
    clr refresh_flag
    clr light_flag
    rjmp main 
; === ISR for INT1 interrupt ===
isr1: 
    sei 
    in temp, SREG
    push temp
    ldi refresh_flag, 0x1 ; set flag
    ldi temp, (1<<INTF1)
    out EIFR, temp
    pop temp
    out SREG, temp
    rjmp main
; === delay code blocks ===
delay_mS1:
    ldi r23, 249        
loop_inn1:
    dec r23            
    nop                
    brne loop_inn1     
    sbiw r24, 1        
    brne delay_mS1      
    rjmp return_from_1delay        
delay_mS3:
    ldi r23, 249        
loop_inn3:
    dec r23             
    nop                 
    brne loop_inn3      
    sbiw r24, 1         
    brne delay_mS3      
    rjmp return_from_3delay