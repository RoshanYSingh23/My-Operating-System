#ifndef __KEYBOARD_H
#define __KEYBOARD_H

#include "types.h"
#include "interrupts.h"
#include "port.h"

class KeyboardDriver : public InterruptHandler {
    Port8Bit dataPort; // Port for the keyboard data register
    Port8Bit commandPort; // Port for the keyboard command register

    public:
        KeyboardDriver(InterruptManager* manager);
        ~KeyboardDriver();
        
        virtual uint32_t HandleInterrupt(uint32_t esp); // Handle the keyboard interrupt, esp is the stack pointer
        
};

#endif