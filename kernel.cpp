#include "types.h"
#include "gdt.h"
typedef void (*constructor)();

/**
 * @param Take the string to be printed as an input
 * Here, we use the standard video memory address 0xb8000
 * Whatever is at this address, the graphics card puts onto the screen
 * In between the characters, we can put a colour code
 * These 8 bits (1 byte) of colour code, are 4 upper and 4 lower bits
 * The upper 4 bits are the background colour, and the lower 4 bits are the foreground colour
 * Currently, we leave them at their default value, so as to print in black and white
 */
void printf(const char* str) {
    static uint16_t* screen_ptr = (uint16_t*) 0xb8000; // Video memory address
    for(int i=0; str[i]!='\0'; i++) {
        // Since unsigned short is 2 bytes, we copy the colour byte, and combine with character byte
        //screen_ptr[i] = (screen_ptr[i] & 0xFF00) | str[i];
        screen_ptr[i] = (uint16_t(0x07) << 8) | uint16_t(str[i]); 
    }
}

extern "C" constructor start_ctors;
extern "C" constructor end_ctors;

extern "C" void callConstructors() {
    for(constructor* i = &start_ctors; i != &end_ctors; i++) {
        (*i)();
    }
}

/**
 * extern "C" forces the compiler to keep the name of the function as it is in the .o file too
 */
extern "C" void roshMain(void* multiboot_structure, uint32_t magic) {
    printf("Hello, World!");      // Without headers, printf is not recognized, so we buiild our own
    GlobalDescriptorTable gdt;    // Create a GDT object, which will initialize the GDT
    while(1);                     // Loop at the end
}