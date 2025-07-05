#include "interrupts.h"

void printf(char* str);
inline void clear_screen();

InterruptHandler::InterruptHandler(uint8_t interruptNumber, InterruptManager* interruptManager) {
    this->interruptNumber = interruptNumber; // Set the interrupt number this handler is responsible for
    this->interruptManager = interruptManager; // Set the interrupt manager that this handler belongs to
    interruptManager->handlers[interruptNumber] = this; // Register this handler in the interrupt manager's handlers array
}

InterruptHandler::~InterruptHandler() {
    if(interruptManager->handlers[interruptNumber] == this) {
        interruptManager->handlers[interruptNumber] = 0; // Unregister this handler from the interrupt manager's handlers array
    }
}

uint32_t InterruptHandler::HandleInterrupt(uint32_t esp) {
    // This function will handle the interrupt, i.e., call the appropriate handler
    return esp; // Return the stack pointer, currently does nothing
}

InterruptManager::GateDescriptor InterruptManager::interruptDescriptorTable[256]; // Initialize the interrupt descriptor table

InterruptManager* InterruptManager::ActiveInterruptManager = 0; // Initialize the active interrupt manager to null

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
    interruptDescriptorTable[interruptNumber].access = IDT_DESC_PRESENT | DescriptorType | ((DecriptorPrivilegeLevel&3) << 5); // Set the access byte, privilege has values 0-3, so we need only the last 2 bits of DescriptorPrivilegeLevel
    interruptDescriptorTable[interruptNumber].handlerAddressHighBits = (((uint32_t)handler >> 16) & 0xFFFF); // Upper 16 bits of the handler address
}


InterruptManager::InterruptManager(GlobalDescriptorTable* gdt) :
    picMasterCommand(0x20), // 0x20 is the command port for the master PIC
    picMasterData(0x21),    // 0x21 is the data port for the master PIC
    picSlaveCommand(0xA0),  // 0xA0 is the command port for the slave PIC
    picSlaveData(0xA1)      // 0xA1 is the data port for the slave PIC
{
    
    uint16_t CodeSegment = gdt->CodeSegmentSelector();

    const uint16_t IDT_INTERRUPT_GATE = 0xE; // Interrupt gate type
    const uint16_t IDT_TRAP_GATE = 0xF; // Trap gate type

    for(uint16_t i=0; i < 256; i++) {
        handlers[i] = 0; // Initialize all handlers to null
        SetInterruptDescriptorTableEntry(
            i, 
            CodeSegment, 
            &IgnoreInterruptRequest, // Default handler does nothing
            0, // DescriptorPrivilegeLevel 0 for kernel
            IDT_INTERRUPT_GATE // Use interrupt gate type
        );
    }

    SetInterruptDescriptorTableEntry(
        0x20,       // 0x20 because we added 0x20 to the 0x00 in interruptstubs.o, so this is actually timer interrupt
        CodeSegment, 
        &HandleInterruptRequest0x00, // Timer interrupt handler
        0, // DescriptorPrivilegeLevel 0 for kernel
        IDT_INTERRUPT_GATE // Use interrupt gate type
    );

    SetInterruptDescriptorTableEntry(
        0x21,       // 0x21 because we added 0x20 to the 0x01 in interruptstubs.o, so this is actually keyboard interrupt
        CodeSegment, 
        &HandleInterruptRequest0x01,//(void(*)()) &keyboard_handler, // Keyboard interrupt handler
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

    picMasterData.Write(0x01);     // Clear the master PIC data register, make it ready for keyboard interrupt
    picSlaveData.Write(0x00);      // Clear the slave PIC data register
    // Now, we have set up the PICs, and we can load the IDT

    InterruptDescriptorTablePointer idt;
    idt.size = 256 * sizeof(GateDescriptor) - 1; // Size of the IDT in bytes
    idt.base = (uint32_t)interruptDescriptorTable; // Address of the first entry in the IDT
    char debug[32];

    asm volatile("lidt %0" : : "m"(idt)); // Load the IDT using the lidt instruction
}

InterruptManager::~InterruptManager() {
    // Destructor to clean up the interrupt manager, currently does nothing
    // We could add code to disable interrupts or clean up resources if needed
}

void InterruptManager::Activate() {
    // This function will activate the interrupt manager, i.e., enable interrupts
    if(ActiveInterruptManager != 0) {
        ActiveInterruptManager->Deactivate(); // If there is already an active interrupt manager, deactivate it
    }
    ActiveInterruptManager = this; // Set the active interrupt manager to this instance
    asm("sti"); // Set the interrupt flag to enable interrupts, start interrupts
}

void InterruptManager::Deactivate() {
    // This function will deactivate the interrupt manager, i.e., disable interrupts
    if(ActiveInterruptManager == this) {
        ActiveInterruptManager = 0; // Set the active interrupt manager to null
        asm("cli"); // Clear the interrupt flag to disable interrupts
    }
}

uint32_t InterruptManager::handleInterrupt(uint8_t interruptNumber, uint32_t esp) {
    // This function will handle the interrupt and return the new stack pointer
    if(ActiveInterruptManager != 0) {
        return ActiveInterruptManager->DoHandleInterrupt(interruptNumber, esp); // Call the DoHandleInterrupt function of the active interrupt manager
    }
    return esp;
}

uint32_t InterruptManager::DoHandleInterrupt(uint8_t interruptNumber, uint32_t esp) {
    // This function will handle the interrupt, i.e., call the appropriate handler

    if(handlers[interruptNumber] != 0) {
        esp = handlers[interruptNumber]->HandleInterrupt(esp);
    }

    else if (interruptNumber != 0x20) {
        char* msg = "UNHANDLED INTERRUPT 0x00";
        char* hex = "0123456789ABCDEF";
        msg[22] = hex[(interruptNumber >> 4) & 0x0F];
        msg[23] = hex[interruptNumber & 0x0F];
        printf(msg); // Print the unhandled interrupt message
    }
    
    if(0x20 <= interruptNumber && interruptNumber < 0x30) {
        // If the interrupt number is in the range of the master PIC (0x20 to 0x27)
        picMasterCommand.Write(0x20); // Send an end-of-interrupt command to the master PIC
        if(0x28 <= interruptNumber) {
        // If the interrupt number is in the range of the slave PIC (0x28 to 0x2F)
            picSlaveCommand.Write(0x20); // Send an end-of-interrupt command to the slave PIC
        }
    }
    return esp; // If no active interrupt manager, just return the stack pointer
}