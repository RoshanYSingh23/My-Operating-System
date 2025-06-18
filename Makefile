CMP := g++
ASM := as
OBJ := loader.o gdt.o port.o interrupts.o interruptstubs.o kernel.o

# 32 bit compilation, so -m32 and --32
# The -fno-use-cxa-atexit flag is used to disable the use of the C++ atexit() function
# The -nostdlib flag is used to prevent the use of the standard C++ library
# The -fno-builtin flag is used to disable the use of built-in functions
# The -fno-exceptions flag is used to disable C++ exceptions (that are handled by C++ or linux kernel)
# The -fno-rtti flag is used to disable C++ run-time type information (RTTI)
# The -fno-leading-underscore flag is used to prevent the compiler from adding a leading underscore 
# to the names of global symbols. Thus, in the loader, I can use roshMain, not _roshMain
CFLAGS := -m32 -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore
AFLAGS := --32
LFLAGS := -melf_i386

%.o: %.cpp
	$(CMP) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(ASM) $(AFLAGS) $< -o $@

roshos.bin: linker.ld $(OBJ)
	ld $(LFLAGS) -T $< -o $@ $(OBJ)

install: roshos.bin
	sudo cp $< /boot/roshos.bin

roshos.iso: roshos.bin
	mkdir iso
	mkdir iso/boot
	mkdir iso/boot/grub
	cp $< iso/boot/
	echo 'set timeout=0' >> iso/boot/grub/grub.cfg
	echo 'set default=0' >> iso/boot/grub/grub.cfg
	echo '' >> iso/boot/grub/grub.cfg
	echo 'menuentry "Roshan OS" {' >> iso/boot/grub/grub.cfg
	echo '	multiboot /boot/roshos.bin' >> iso/boot/grub/grub.cfg
	echo '	boot' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	grub-mkrescue --output=$@ iso
	sudo rm -rf iso

run: roshos.iso
	(killall qemu-system-i386 && sleep 1) || true
	qemu-system-i386 -enable-kvm -cpu host -m 128M -boot d -cdrom roshos.iso -serial stdio

.PHONY: clean

clean:
	sudo rm -f $(OBJ) roshos.bin roshos.iso