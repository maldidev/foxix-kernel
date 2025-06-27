#bootloader
echo -e "\033[34m==> Compiling bootloader"
nasm -f elf32 boot.asm -o boot.o

#kernel
echo -e "\033[34m==> Compiling C (kernel)"
gcc -m32 -ffreestanding -nostdlib -Wall -Wextra -c foxix.c -o foxix.o

#linker-script
echo -e "\033[34m==> Compiling with linker (ld)"
ld -m elf_i386 -T linker.ld -nostdlib -n -o foxix.bin boot.o foxix.o

#running
echo -e "\033[34m==> Running"
chmod 775 ./run.sh
./run.sh
