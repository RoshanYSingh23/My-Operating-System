/*
* This is the header file to handle the read and write ports to enable hardware communication
* Here, the convention is weird, but is done to ensure backward-compatibility with older legacy systems.
* In older systems, 1 word = 16 bits, so outb is an assembly instruction that writes a byte to a port,
* But, outw will write a word (2 bytes) to a port. Although in new systems, 1 word is 32 or 64 bits.
* Similarly, a double word is 32 bits, but, we use the instruction outl (l for long)
* Finally, quad-words exist in 64 bit systems, but we do not use them here.
* The assembly instruction has format: instruction_name value, port. But, we need to know its bandwidth before-hand
* Instead, we do it in an Object-Oriented way using class methods read and write.
* Similarly, we have inb, inw, inl for reading from ports, with format instruction_name result taken / data , port
*/

#ifndef __PORT_H__
#define __PORT_H__
#include "types.h"

    class Port {
        protected:
            uint16_t port_number; // The port number to read/write from/to
            Port(uint16_t port_number);
            ~Port();
    };

    class Port8Bit : public Port { // Class to handle 8-bit ports
        public:
            Port8Bit(uint16_t port_number);
            ~Port8Bit();
            virtual void Write(uint8_t data); // Write a byte to the port
            virtual uint8_t Read(); // Read a byte from the port
    };

    class Port8BitSlow : public Port8Bit { // Class to handle 8-bit ports with a delay
        public:
            Port8BitSlow(uint16_t port_number);
            ~Port8BitSlow();
            virtual void Write(uint8_t data); // Write a byte to the port
    };

    class Port16Bit : public Port { // Class to handle 8-bit ports
        public:
            Port16Bit(uint16_t port_number);
            ~Port16Bit();
            virtual void Write(uint16_t data); // Write a byte to the port
            virtual uint16_t Read(); // Read a byte from the port
    };
    
    class Port32Bit : public Port { // Class to handle 8-bit ports
        public:
            Port32Bit(uint16_t port_number);
            ~Port32Bit();
            virtual void Write(uint32_t data); // Write a byte to the port
            virtual uint32_t Read(); // Read a byte from the port
    };

#endif