#include "interrupts.h"

void printf(char* str);

InterruptManager::GateDescriptor InterruptManager::interruptDescriptorTable[256]; // Initialize the interrupt descriptor table


void InterruptManager::SetInterruptDescriptorTableEntry(    // Set an entry in the interrupt descriptor table
    uint8_t interruptNumber,
    uint16_t codeSegmentSelectorOffset,
    void (*handler)(),
    uint8_t DecriptorPrivilegeLevel,
    uint8_t DescriptorType  // 0xE for interrupt gate, 0xF for trap gate
) 
{
    const uint8_t IDT_DESC_PRESENT = 0x80; // Present byte, must be set in access
    interruptDescriptorTable[interruptNumber].handlerAddressLowBits = ((uint32_t)handler & 0xFFFF); // Lower 16 bits of the handler address
    interruptDescriptorTable[interruptNumber].gdt_codeSegmentsSelector = codeSegmentSelectorOffset; 
    interruptDescriptorTable[interruptNumber].reserved = 0; // Reserved, must be zero
    interruptDescriptorTable[interruptNumber].access = IDT_DESC_PRESENT | ((DecriptorPrivilegeLevel&3) << 5) | DescriptorType; // Set the access byte, privilege has values 0-3, so we need only the last 2 bits of DescriptorPrivilegeLevel
    interruptDescriptorTable[interruptNumber].handlerAddressHighBits = (((uint32_t)handler >> 16) & 0xFFFF); // Upper 16 bits of the handler address
}


InterruptManager::InterruptManager(GlobalDescriptorTable* gdt) :
    picMasterCommand(0x20), // 0x20 is the command port for the master PIC
    picMasterData(0x21),    // 0x21 is the data port for the master PIC
    picSlaveCommand(0xA0),  // 0xA0 is the command port for the slave PIC
    picSlaveData(0xA1)      // 0xA1 is the data port for the slave PIC
{
    // Initialize the interrupt manager, set up the interrupt descriptor table
    uint16_t codeSegment = gdt->CodeSegmentSelector();
    const uint16_t IDT_INTERRUPT_GATE = 0xE; // Interrupt gate type
    const uint16_t IDT_TRAP_GATE = 0xF; // Trap gate type

    for(uint16_t i=0; i < 256; i++) {
        SetInterruptDescriptorTableEntry(
            i, 
            codeSegment, 
            &IgnoreInterruptRequest, // Default handler does nothing
            0, // DescriptorPrivilegeLevel 0 for kernel
            IDT_INTERRUPT_GATE // Use interrupt gate type
        );
    }

    SetInterruptDescriptorTableEntry(
        0x20,       // 0x20 because we added 0x20 to the 0x00 in interruptstubs.o, so this is actually timer interrupt
        codeSegment, 
        &HandleInterruptRequest0x00, // Timer interrupt handler
        0, // DescriptorPrivilegeLevel 0 for kernel
        IDT_INTERRUPT_GATE // Use interrupt gate type
    );

    SetInterruptDescriptorTableEntry(
        0x21,       // 0x21 because we added 0x20 to the 0x01 in interruptstubs.o, so this is actually keyboard interrupt
        codeSegment, 
        &HandleInterruptRequest0x01, // Keyboard interrupt handler
        0, // DescriptorPrivilegeLevel 0 for kernel
        IDT_INTERRUPT_GATE // Use interrupt gate type
    );

    picMasterCommand.Write(0x11);  // 0x11 is the initialization command for the master PIC 
    picSlaveCommand.Write(0x11);   // 0x11 is the initialization command for the slave PIC

    picMasterData.Write(0x20);     // 0x20 is the offset for the master PIC, this is the first interrupt
    picSlaveData.Write(0x28);      // 0x28 is the offset for the slave PIC, this is the second interrupt

    // Note, that both the master and slave PICs have 8 interrupts each
    // Master PIC from 0x20 to 0x27, and slave PIC from 0x28 to 0x2F

    picMasterData.Write(0x04);     // 0x04 tells the master PIC that there is a slave PIC at IRQ2 (that it is the master)
    picSlaveData.Write(0x02);      // 0x02 tells the slave PIC that it is connected to the master PIC at IRQ2 (that it is the slave)

    picMasterData.Write(0x01);     // 0x01 tells the master PIC to start in 8086 mode, the default mode for the PIC
    picSlaveData.Write(0x01);      // 0x01 tells the slave PIC to start in 8086 mode

    //picMasterData.Write(0x00);     // Clear the master PIC data register
    //picSlaveData.Write(0x00);      // Clear the slave PIC data register
    // Now, we have set up the PICs, and we can load the IDT

    picMasterData.Write(0x0);     // Mask IRQ0 (timer), leave IRQ1 (keyboard) unmasked
    picSlaveData.Write(0x0);      // Mask all slave IRQs (we’re only using keyboard on the master)

    // Now only your keyboard handler will ever fire,
    // so you’ll get a stable “Hello, World!” and then a single
    // “ INTERRUPT” only when you actually press a key.


    InterruptDescriptorTablePointer idt;
    idt.size = 256 * sizeof(GateDescriptor) - 1; // Size of the IDT in bytes
    idt.base = (uint32_t)interruptDescriptorTable; // Address of the first entry in the IDT
    asm volatile("lidt %0" : : "m"(idt)); // Load the IDT using the lidt instruction
}

void InterruptManager::Activate() {
    // This function will activate the interrupt manager, i.e., enable interrupts
    asm ("sti"); // Set the interrupt flag to enable interrupts, start interrupts
}

uint32_t InterruptManager::handleInterrupt(uint8_t interruptNumber, uint32_t esp) {
    // This function will handle the interrupt and return the new stack pointer
    // For now, we just return the same stack pointer
    //printf(" INTERRUPT");
    return esp;
}