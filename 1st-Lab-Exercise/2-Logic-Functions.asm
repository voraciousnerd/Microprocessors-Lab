; 1st Lab Exercise
;
; 1.2 
.include "m328PBdef.inc"

.def A = r16
.def B = r17
.def C = r18
.def D = r19
.def F0 = r20
.def F1 = r21
.def count = r22

reset: 
    ldi r23, low(RAMEND)  ; initialize stack 
    out SPL, r23
    ldi r23, high(RAMEND)
    out SPH, r23
main: 
    ldi A, 0x52  ; initialize registers 
    ldi B, 0x42
    ldi C, 0x22
    ldi D, 0x02
    ldi count, 0x06
    loop:
    rcall calc_F0
    rcall calc_F1
    nop ; for debugging 
    subi A, -0x01
    subi B, -0x02
    subi C, -0x03
    subi D, -0x04
    nop ; for debugging 
    dec count
    breq exit
    rjmp loop 
exit:   
    nop 
    rjmp exit

; subroutine to calculate F0
calc_F0: 
    push A
    push B
    push D
    com A
    and A, B
    com B
    and B, D
    or A, B
    com A
    mov F0, A
    pop D
    pop B
    pop A
    ret

; subroutine to calculate F1
calc_F1: 
    push A
    push B
    push C
    push D
    or A, C
    or B, D
    and A, B
    mov F1, A
    pop D
    pop C
    pop B
    pop A
    ret