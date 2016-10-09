        PROCESSOR       16F84
        RADIX           DEC
        INCLUDE         "p16f84.inc"

        __CONFIG        _CP_OFF & _PWRTE_OFF & _WDT_OFF & _XT_OSC

LED     EQU     0

        ORG     0CH

Count   RES     2

        ; Reset vector
        ; Program starting point at the CPU reset

        ORG     00H

        bsf     STATUS,RP0

        movlw   00011111B
        movwf   TRISA

        movlw   11111110B
        movwf   TRISB

        bcf     STATUS,RP0

        bsf     PORTB,LED

MainLoop
        call    Delay

        btfsc   PORTB,LED
        goto    SetToZero

        bsf     PORTB,LED
        goto    MainLoop

SetToZero
        bcf     PORTB,LED
        goto    MainLoop

        ; Subroutines

Delay
        clrf    Count
        clrf    Count+1

DelayLoop
        decfsz  Count,1
        goto    DelayLoop

        decfsz  Count+1,1
        goto    DelayLoop

        return

        END
