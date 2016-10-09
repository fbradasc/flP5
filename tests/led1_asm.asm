        PROCESSOR       16F84
        RADIX           DEC
        INCLUDE         "p16f84.inc"

        __CONFIG        _CP_OFF & _PWRTE_OFF & _WDT_OFF & _XT_OSC

LED     EQU     0

        ORG     0CH

Count   RES     2

SetBit  macro reg, bit
        bsf reg, bit
      endm

ClearBit macro reg, bit
        bcf reg, bit
      endm

ToggleBit macro reg, bit
        btfsc    reg, bit
        goto     ToggleBit_set
        ClearBit reg, bit
        goto     ToggleBit_end

ToggleBit_set
        SetBit   reg, bit
ToggleBit_end
      endm

SetBank macro bank
        if (bank == 0)
          SetBit   STATUS, RP0
        else
          ClearBit STATUS, RP0
        endif
      endm

SetPortDir macro port, dir
        movlw dir
        movwf port
      endm


        ; Reset vector
        ; Program starting point at the CPU reset

        ORG     00H

        SetBank 0
        SetPortDir TRISA, 00011111B
        SetPortDir TRISB, 11111110B
        SetBank 1
        SetBit PORTB, LED
Loop:
        goto Delay
        ToggleBit PORTB, LED
        goto Loop

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
