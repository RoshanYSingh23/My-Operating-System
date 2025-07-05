#ifndef __INTERRUPTS_H
#define __INTERRUPTS_H
#include "types.h"
#include "port.h"
#include "gdt.h"

class InterruptManager;

class InterruptHandler {
    protected:
        uint8_t interruptNumber; // The interrupt number this handler is responsible for
        InterruptManager* interruptManager; // Pointer to the interrupt manager that this handler belongs to
        InterruptHandler(uint8_t interruptNumber, InterruptManager* interruptManager);
        ~InterruptHandler(); // Destructor, currently does nothing

    public:
        virtual uint32_t HandleInterrupt(uint32_t esp); // Handle the interrupt, esp is the stack pointer            
};

class InterruptManager {
    friend class InterruptHandler; // Allow InterruptHandler to access private members of InterruptManager

    protected:
        static InterruptManager* ActiveInterruptManager; // Pointer to the currently active interrupt manager, used for handling interrupts

        InterruptHandler* handlers[256]; // Array of pointers to interrupt handlers for each interrupt number

        struct GateDescriptor {
            uint16_t handlerAddressLowBits;   // Lower 16 bits of the address to jump to when this interrupt fires
            uint16_t gdt_codeSegmentsSelector;     // Kernel segment selector
            uint8_t  reserved;     // Reserved, must be zero
            uint8_t  access;    // Type and attributes (present, DPL, etc.), access rights
            uint16_t handlerAddressHighBits;  // Upper 16 bits of the address to jump to
        } __attribute__((packed)); // Ensure no padding is added by the compiler

        static GateDescriptor interruptDescriptorTable[256]; // Array of 256 Gate Descriptors for each interrupt

        static void SetInterruptDescriptorTableEntry(    // Set an entry in the interrupt descriptor table
            uint8_t interruptNumber,
            uint16_t codeSegmentSelectorOffset,
            void (*handler)(),
            uint8_t DecriptorPrivilegeLevel,
            uint8_t DescriptorType  // 0xE for interrupt gate, 0xF for trap gate
        );

        struct InterruptDescriptorTablePointer {
            uint16_t size; // Size of the IDT in bytes
            uint32_t base; // Address of the first entry in the IDT
        } __attribute__((packed)); // Ensure no padding is added by the compiler

        Port8BitSlow picMasterCommand; // Port for the master PIC command register, PIC stands for Programmable Interrupt Controller
        Port8BitSlow picMasterData;    // Port for the master PIC data register
        Port8BitSlow picSlaveCommand;  // Port for the slave PIC command register
        Port8BitSlow picSlaveData;     // Port for the slave PIC data register
        // We need master and slave PICs because we have more than 8 interrupts, and the master PIC can only handle 8 interrupts (0x00 to 0x07)

    public:
        InterruptManager(GlobalDescriptorTable* gdt); // Constructor to initialize the interrupt manager
        ~InterruptManager(); // Destructor to clean up the interrupt manager
        
        void Activate(); // Activate the interrupt manager, i.e., enable interrupts
        // This is needed as a separate function, as we first instantiate the IDT, then, we activate the hardware, and then start the interrupts
        // When we start the hardware, we already need the IDT set up

        void Deactivate(); // Deactivate the interrupt manager, i.e., disable interrupts
        // This is needed to disable interrupts when we switch to a different interrupt manager, or when

        static uint32_t handleInterrupt(uint8_t interruptNumber, uint32_t esp); // esp is the stack pointer, which will be given by interruptstubs.s
        uint32_t DoHandleInterrupt(uint8_t interruptNumber, uint32_t esp); // This function will handle the interrupt, i.e., call the appropriate handler
        // This function is called by handleInterrupt, and it will call the appropriate handler for the interrupt number
        
        static void IgnoreInterruptRequest(); // This function will ignore the interrupt request, i.e., do nothing

        static void HandleInterruptRequest0x00(); // This is the handler for the first interrupt, which is usually the timer interrupt
        static void HandleInterruptRequest0x01(); // This is the handler for the second interrupt, which is usually the keyboard interrupt
};

#endif