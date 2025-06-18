#include "gdt.h"

GlobalDescriptorTable::GlobalDescriptorTable() :
    nullSegmentSelector(0, 0, 0),
    codeSegmentSelector(0, 64*1024*1024, 0x9A), // 0x9A : Present, executable, read/write, accessed flags, with 64MB limit
    dataSegmentSelector(0, 64*1024*1024, 0x92),   // 0x92 : Present, read/write, accessed flags
    unusedSegmentSelector(0, 0, 0) 
{
    uint32_t i[2];
    i[0] = (uint32_t) this;
    i[1] = sizeof(GlobalDescriptorTable) << 16; // High 16 bits are the size of the GDT   

    asm volatile("lgdt (%0)" : : "p" (((uint16_t *) i) + 2)); // asm/assembly instruction lgdt to load the GDT
}

GlobalDescriptorTable::~GlobalDescriptorTable() {
    // Destructor does not need to do anything for GDT
}

uint16_t GlobalDescriptorTable::DataSegmentSelector() {
    return (uint16_t *) (&dataSegmentSelector) - (uint16_t *) (this);
}

uint16_t GlobalDescriptorTable::CodeSegmentSelector() {
    return (uint16_t *) (&codeSegmentSelector) - (uint16_t *) (this);
}

/*
* Out of the 4 bytes as a uint32_t, we get to use only 
* the upper 20 bits (2.5 bytes) for the base address.
* But this is only legal if the other 12 bits are all 1s.
* If they are not, and we replace with all 1s, we may
* overlap with the next segment, which is not what we want.
* So, to align it, we remove a 1 from the first 2.5 bytes,
* and then set the lower 12 bits to all 1s. This will waste
* some space, but will ensure there us no overlap.
*/
/* Byte 0,1 : limit_lo, 2,3 : base_lo, 4 : base_hi, 5 : type, 
6 : limit_hi(lower 4) and flags, 7 : base_vhi */
GlobalDescriptorTable::SegmentDescriptor::SegmentDescriptor(uint32_t base, uint32_t limit, uint8_t type) {  

    uint16_t* target = (uint16_t*) this;
    
    if(limit <= 65536) {
        target[6] = 0x40;  // Set the 6th byte to 0x40 to tell the processor that its a 16 bit entry
    }

    else {
        if(limit & 0xFFF != 0xFFF) { // If last 12 bits are not all 1s
            limit = (limit >> 12) - 1; // Remove the lower last bit from MSB
        }

        else {
            limit >>= 12;
        }

        target[6] = 0xC0;  // Set the 6th byte to 0xC0 to tell the processor that its a 32 bit entry
    }

    target[0] = limit & 0xFF;        // Lower 8 bits of the segment limit
    target[1] = (limit >> 8) & 0xFF; // Next 8 bits of the segment limit
    // We have now filled the last 2 bytes of the address, now, we need to fill the remaining high half byte

    target[6] |= (limit >> 16) & 0xF; // Upper 4 bits of the segment limit, and or because we set some part before

    // Now, we start by encoding the pointer, which is 4 bytes
    target[2] = base & 0xFF;         // Lower 8 bits of the base address
    target[3] = (base >> 8) & 0xFF;  // Next 8 bits of the base address
    target[4] = (base >> 16) & 0xFF; // Next 8 bits of the base address
    target[7] = (base >> 24) & 0xFF; // Upper 8 bits of the base address, 7 because this part was added later, so its the last part of 64B

    // Finally, we set the type and access flags, which is 1 byte
    target[5] = type;
}

uint32_t GlobalDescriptorTable::SegmentDescriptor::Base() {
    uint16_t* target = (uint16_t*) this;
    return (target[2] | (target[3] << 8) | (target[4] << 16) | (target[7] << 24));
}

uint32_t GlobalDescriptorTable::SegmentDescriptor::Limit() {
    uint16_t* target = (uint16_t*) this;
    uint32_t result = (target[0] | (target[1] << 8) | ((target[6] & 0xF) << 16));
    if((target[6] & 0xC0) == 0xC0)
        result = (result << 12) | 0xFFF; // If its a 32 bit entry, we need to shift it back to the original limit (last 12 bits)
    return result;
}

/**
 * 0	bits 0–7	Limit[7:0]	Lower 8 bits of the segment limit.
 * 1	bits 8–15	Limit[15:8]	Next‑higher 8 bits of the segment limit.
 * 2	bits 0–7	Base[7:0]	Base address bits 0–7.
 * 3	bits 8–15	Base[15:8]	Base address bits 8–15.
 * 4	bits 16–23	Base[23:16]	Base address bits 16–23.
 * 5	bits 0–3	Type	“Type” nibble (read/write, executable, system vs data, etc.) bits 0–3 of the Access Byte.
 *      bits 4	S (Descriptor Type)	0 = system descriptor, 1 = code/data descriptor
 *      bits 5–6	DPL (Privilege)	Descriptor Privilege Level (0–3)
 *      bit 7	P (Present)	1 = segment is present in memory
 * 6	bits 0–3	Limit[19:16]	Upper 4 bits of the segment limit.
 *      bit 4	AVL	Available for system software (unused by CPU)
 *      bit 5	L (64‑bit)	1 = 64‑bit code segment (IA‑32e mode only)
 *      bit 6	D/B (Default/Big)	0 = 16‑bit segment, 1 = 32‑bit segment
 *      bit 7	G (Granularity)	0 = limit in bytes, 1 = limit in 4 KiB pages
 * 7	bits 24–31	Base[31:24]	Base address bits 24–31.
*/