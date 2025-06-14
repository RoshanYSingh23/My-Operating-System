.set MAGIC, 0x1BADB002          # Magic number so the boot loader recognizes that this is a kernel
.set FLAGS, (1<<0 | 1<<1)       # Tells multiboot loader which features/flags are requested
.set CHECKSUM, -(MAGIC + FLAGS) # Verify the header's integrity, so that MAGIC + FLAGS + CHECKSUM = 0 (MOD 2<<32)

.section .multiboot
    .long MAGIC
    .long FLAGS
    .long CHECKSUM

.section .text
.extern roshMain
.extern callConstructors
.global loader

loader:
    mov $rosh_stack, %esp   # Move the stack pointer
    call callConstructors
    push %eax               # Bootloader provides pointer to multiboot structure in ax register
    push %ebx               # It also gives the magic number in bx register
    call roshMain

_loop:
    cli
    hlt
    jmp _loop

.section .bss
.space 2*1024*1024;      # 2 MB for the stack to grow downwards

rosh_stack:

