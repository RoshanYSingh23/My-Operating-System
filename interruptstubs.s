# Taken from Tyndur project, low-level.eu

.set IRQ_BASE, 0x20

.section .text

.extern _ZN16InterruptManager15handleInterruptEhj # This is the name of the handleInterrupts method in interrupts.o (using nm interrupts.o)
.global _ZN16InterruptManager22IgnoreInterruptRequestEv

.macro HandleException num
.global _ZN16InterruptManager16HandleException\num\()Ev   # Similar name, 26 instead of 15 by cpp convention 
_ZN16InterruptManager16HandleException\num\()Ev:
    movb $\num, (interruptnumber)
    jmp int_bottom
.endm

.macro HandleInterruptRequest num
.global _ZN16InterruptManager26HandleInterruptRequest\num\()Ev   # Similar name, 26 instead of 15 by cpp convention 
_ZN16InterruptManager26HandleInterruptRequest\num\()Ev:
    movb $\num + IRQ_BASE, (interruptnumber)
    jmp int_bottom
.endm

HandleInterruptRequest 0x00
HandleInterruptRequest 0x01

int_bottom:

    pusha           # Push all the registers, for recovering the context before interrupt
    pushl %ds       # Push data segment registers, to restore after handling the interrupt
    pushl %es       # Push the extra segment registers (extra for computation)
    pushl %fs       # Push the next extra registers, fs and gs segments
    pushl %gs       # gs is for per-cpu data in kernel mode, fs for thread local storage in user mode

    pushl %esp
    push (interruptnumber)
    call _ZN16InterruptManager15handleInterruptEhj
    #addl $8, %esp # to remove the previous stuff, but we overwrite it, so not needed
    movl %eax, %esp  # movb, to move a byte, movl for long, movw for a word (16 bits)

    popl %gs       # Restore the registers onto the previous stack, by popping from where we pushed them
    popl %fs
    popl %es
    popl %ds
    popa

    #movb $0x20, %al
    #outb %al, $0x20


_ZN16InterruptManager22IgnoreInterruptRequestEv:

    iret           # Tell the processor, we have handled the interrupt, return to what you were doing before

.data
    interruptnumber: .byte 0 # interruptNumberis a byte, initialized to 0
