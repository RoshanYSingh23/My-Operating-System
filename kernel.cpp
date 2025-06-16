#include "types.h"
#include "gdt.h"
typedef void (*constructor)();

/** 
 * Pointer to the start of video memory
 * The video memory starts at 0xb8000, and is 80x25 characters in size
 * Each character is represented by 2 bytes: the first byte is the character, and the second byte is the colour code
 * The colour code is a combination of the foreground and background colours
 * The foreground colour is the lower 4 bits, and the background colour is the upper 4 bits
 */

static uint16_t* screen_ptr = (uint16_t*)0xb8000;
static uint16_t row = 0, col = 0;

inline void clear_cell(uint16_t r, uint16_t c) {    // Clear a single cell on the screen by writing a space character with the default colour
    screen_ptr[80*r + c] = (0x07 << 8) | ' ';
}

inline void clear_line(uint16_t row, uint16_t start_col) { // Clear a line from the start column to the end of the line
    for (uint16_t c = start_col; c < 80; ++c) {
        clear_cell(row, c);
    }
}

inline void clear_screen() {
    for(uint16_t r = 0; r < 25; ++r) { // Clear the entire screen
        clear_line(r, 0);
    }
    row = 0; // Reset the row and column counters
    col = 0;
}

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
    for (int i = 0; str[i]; ++i) {
        char ch = str[i];
        switch(str[i]) {

            case '\n':
                clear_line(row, col);  // Before advancing, clear the rest of this line
                col++;
                row = 0;
                break;
            
            default:
                screen_ptr[80*col + row] = (0x07 << 8) | uint8_t(ch);
                row++;

        }
        if(row >= 80) { // If we reach the end of the line, move to the next line
            clear_line(col, row);  // clear any leftover on this wrapped line
            row = 0;
            col++;
        }

        if (col >= 25) {    // If we reach the end of the screen, clear the screen and go to the start
            clear_screen();
        }

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
 * Since this printf writes from the start of video memory, it will overwrite the first 2 bytes
 * So, if I call printf again with some other text, the older statement will be overwritten
 * So, only the string from the last printf will be visible, thus, we added a change to printf next
 */
extern "C" void roshMain(void* multiboot_structure, uint32_t magic) {
    // Start by clearing the entire screen, so that we can print our own text
    clear_screen();
    printf("Hello, World!\n");      // Without headers, printf is not recognized, so we buiild our own
    printf("Hello, World!");
    printf("Hi");
    GlobalDescriptorTable gdt;    // Create a GDT object, which will initialize the GDT
    while(1);                     // Loop at the end
}