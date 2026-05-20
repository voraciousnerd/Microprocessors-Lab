; 3rd Lab Exercise
; 
; 3.1
.include "m328PBdef.inc"
.equ FOSC_MHZ = 16              ; microcontroller operating frequency
.equ DEL_mS = 50                ; delay in ms
.equ DEL_NU = FOSC_MHZ * DEL_mS ; used for delay subroutine

.def temp = r16
.def DC_VALUE = r17 
.def zero = r18

.org 0x00
    rjmp reset
    nop

; OCR1A precomputed table for 2%–98% DC (step 6%)
; 17 elements: 2%, 8%, 14%, ..., 98%
; values scaled to 8-bit PWM (0–255)
; DC_scaled = (DC*256)/100
OCR_TABLE:
.db 5, 20, 36, 51, 66, 82, 97, 112, 128, 143, 158, 173, 189, 204, 220, 235, 250

reset:
    ; initialize stack pointer
    ldi temp, high(RAMEND)
    out SPH, temp
    ldi temp, low(RAMEND)
    out SPL, temp 
    clr zero

    ; initialize timer: 
    ; COM1A1 = 1  (non inverting PWM)
    ; WGM10 = 1 (Fast PWM, 8-bit)
    ldi temp, 0b10000001 
    sts TCCR1A, temp 
    
    ; WGM12 = 1 (Fast PWM, 8-bit)
    ; CS1n[2:0] = 100 (prescale to 256)
    ldi temp, 0b00001100 
    sts TCCR1B, temp

    ; set PB1 as output &
    ; PB4-PB5 as input
    ldi temp, 0b00000010
    out DDRB, temp
    ldi temp, 0b00110000
    out PORTB, temp             ; pull-ups

    ; for debugging (PORTD output)
    ser temp
    out DDRD, temp

    ; initialize DC to 50 %
    ldi DC_VALUE, 8
    rcall update_PWM

main:
    in temp, PINB
    
    ; jump if PB4 pressed
    sbrs temp, 4
    rjmp pressed_PB4_inc
    
    ; jump if PB5(3) pressed
    sbrs temp, 3
    rjmp pressed_PB5_dec
    rjmp main 

pressed_PB4_inc:
    rcall delay 
    cpi DC_VALUE, 16
    breq upper_lim
    inc DC_VALUE

upper_lim:
    rcall update_PWM

debounce4: 
    rcall delay
    sbis PINB, 4
    rjmp debounce4
    rjmp main

pressed_PB5_dec: 
    rcall delay
    cpi DC_VALUE, 0
    breq lower_lim
    dec DC_VALUE

lower_lim:
    rcall update_PWM 

debounce5: 
    rcall delay
    sbis PINB, 3
    rjmp debounce5
    rjmp main 

; === Routine to update DC according to DC_VALUE ===
update_PWM: 
    ldi zl, low(OCR_TABLE) << 1
    ldi zh, high(OCR_TABLE) << 1    ; byte addressing
    add zl, DC_VALUE
    clr r1
    adc zh, r1                      ; adjust ZH with carry
    lpm r19, z
    sts OCR1AL, r19
    out PORTD, DC_VALUE
    ret

; === Delay routine ===
delay:
    ldi r24, low(DEL_NU)
    ldi r25, high(DEL_NU)

delay_in: 
    ldi r23, 249

loop_inn: 
    dec r23
    nop
    brne loop_inn
    sbiw r24, 1
    brne delay_in
    ret