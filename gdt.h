/* This header file is for the global descriptor table, and contains the size of memory segment, the pointers, and access flags/bits*/
#include "types.h"
#ifndef __GDT_H
#define __GDT_H
class GlobalDescriptorTable {
    public:
        class SegmentDescriptor {   // Total of 64b = 8B
            private:
                uint16_t limit_lo;       // Lower 16 bits of the segment limit
                uint16_t base_lo;        // Lower 16 bits of the base address or pointer
                uint8_t base_hi;         // Next 8 bits of the base address, 1 byte extension for pointer
                uint8_t type;            // Access flags (present, ring level, etc.)
                uint8_t flags_limit_hi;  // Granularity and upper bits of the segment limit
                uint8_t base_vhi;        // Upper 8 bits of the base address
            
            public:
                SegmentDescriptor(uint32_t base, uint32_t limit, uint8_t type) ;
                uint32_t Base();
                uint32_t Limit();
        
        } __attribute__((packed)); // Ensure no padding is added by the compiler

        SegmentDescriptor nullSegmentSelector;
        SegmentDescriptor codeSegmentSelector;
        SegmentDescriptor dataSegmentSelector;
        SegmentDescriptor unusedSegmentSelector;

        GlobalDescriptorTable();
        ~GlobalDescriptorTable();

        uint16_t CodeSegmentSelector();
        uint16_t DataSegmentSelector();
};
#endif
