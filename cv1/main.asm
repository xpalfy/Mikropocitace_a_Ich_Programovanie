;-------------------------------------------------------------------------------
; MSP430 Assembler Code Template for use with TI Code Composer Studio
;
;
;-------------------------------------------------------------------------------
            .cdecls C,LIST,"msp430.h"       ; Include device header file
            
;-------------------------------------------------------------------------------
            .def    RESET                   ; Export program entry-point to
                                            ; make it known to linker.
;-------------------------------------------------------------------------------
            .text                           ; Assemble into program memory.
            .retain                         ; Override ELF conditional linking
                                            ; and retain current section.
            .retainrefs                     ; And retain any sections that have
                                            ; references to current section.

;-------------------------------------------------------------------------------
RESET       mov.w   #__STACK_END,SP         ; Initialize stackpointer
StopWDT     mov.w   #WDTPW|WDTHOLD,&WDTCTL  ; Stop watchdog timer


;-------------------------------------------------------------------------------
; Main loop here
;-------------------------------------------------------------------------------

SetupP1		bis.b   #041h,&P1DIR		; P1.0, P1.6 nastaveny ako vystup, 0100 0001
			mov.b   #40h,&P1OUT			; P1.6 nastavime na logicku 1, a P1.0 nastavime na logicku 0, 0100 0000
Mainloop	xor.b   #041h,&P1OUT		; pomocou xor zmenime logicku hodnotu na port P1.0 a P1.6,

Wait		mov.w   #65000,R15			; register R15 - pocitadlo

L1			dec.w   R15			; dekrementacia R15
			jnz     L1			; test: R15 = 0?
			mov.w   #65000,R15
L2			dec.w   R15			; dekrementacia R15
			jnz     L2			; test: R15 = 0?
			mov.w   #65000,R15
			jmp     Mainloop		; ak R15 > 0, opakujeme cyklus
                                            

;-------------------------------------------------------------------------------
; Stack Pointer definition
;-------------------------------------------------------------------------------
            .global __STACK_END
            .sect   .stack
            
;-------------------------------------------------------------------------------
; Interrupt Vectors
;-------------------------------------------------------------------------------
            .sect   ".reset"                ; MSP430 RESET Vector
            .short  RESET
            
