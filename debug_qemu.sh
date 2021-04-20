
#qemu-system-i386 -fda floppy.img -m 16M

gdb -q -iex 'set architecture i386' -iex 'file kernel_ken/src/kernel' -iex 'symbol-file kernel_ken/src/kernel'  -iex 'target remote |
qemu-system-i386 -m 16M -gdb stdio -S -kernel kernel_ken/src/kernel 2> /dev/null'

