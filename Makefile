all: kernel.iso

kernel.bin: boot.o kernel.o
	ld -m elf_x86_64 -T scripts/linker.ld -o kernel.bin boot.o kernel.o --no-warn-rwx-segments

boot.o: src/boot/boot.asm
	nasm -f elf64 src/boot/boot.asm -o boot.o

kernel.o: src/kernel/kernel.c
	gcc -m64 -c src/kernel/kernel.c -o kernel.o -ffreestanding -O2 -Wall -Wextra -fno-stack-protector -fno-pic

kernel.iso: kernel.bin
	mkdir -p isodir/boot/grub
	cp kernel.bin isodir/boot/kernel.bin
	echo 'menuentry "Codename: MRC // v0.0.1 Beta (no available use for end user distribution)" { multiboot2 /boot/kernel.bin }' > isodir/boot/grub/grub.cfg
	grub-mkrescue -o kernel.iso isodir

clean:
	rm -rf *.o kernel.bin kernel.iso isodir