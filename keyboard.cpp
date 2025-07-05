#include "keyboard.h"

void printf(char* str); // Forward declaration of printf function to print messages

KeyboardDriver::KeyboardDriver(InterruptManager* manager) 
: InterruptHandler(0x21,manager), dataPort(0x60), commandPort(0x64)
{
    while(commandPort.Read() & 0x1) {
        dataPort.Read(); // Clear the keyboard buffer by reading from the data port until it is empty
    }
    commandPort.Write(0xAE); // Enable the keyboard to send interrupts
    commandPort.Write(0x20); // Get the current state of the keyboard controller
    uint8_t status = (dataPort.Read() | 1) & ~0x10; // Read the state
    commandPort.Write(0x60); // Set the command port to change the current state and write the new state
    dataPort.Write(status); // Write the new state to the data port
    dataPort.Write(0xF4); // Send the command to enable the keyboard
}

KeyboardDriver::~KeyboardDriver() {
    // Destructor, currently does nothing
}

uint32_t KeyboardDriver::HandleInterrupt(uint32_t esp) {
    uint8_t key = dataPort.Read(); // Read the key from the keyboard data port
    if(key < 0x80) {
        switch(key) {
            case 0xFA:
                break;
            case 0x1E:
                printf("a"); // Print 'A' when the key with code 0x1E is pressed
                break;
            case 0x45:
                break;
            case 0xc5:
                break;
            default:
                char* msg = "KEYBOARD 0x00 ";
                char* hex = "0123456789ABCDEF";
                msg[11] = hex[(key >> 4) & 0x0F];
                msg[12] = hex[key & 0x0F];
                printf(msg); // Print the unhandled interrupt message
        }
    }
    
    return esp; // Handle the keyboard interrupt, currently does nothing
}