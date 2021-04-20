# Makefile for JamesM's kernel tutorials.
# The C and C++ rules are already setup by default.
# The only one that needs changing is the assembler 
# rule, as we use nasm instead of GNU as.

SOURCES=boot.o main.o monitor.o common.o descriptor_tables.o isr.o interrupt.o gdt.o timer.o \
		kheap.o paging.o heapindex.o heap.o task.o algorithm.o kernel_ken.o process.o syscall.o\
		print.o  ulib.o app.o linked_list.o binaryheap.o queue.o klib.o autotest.o pipe.o semaphore.o

CFLAGS=-m32 -std=gnu99 -ffreestanding -Wno-main -O0 -DNON_PORTABLE_COLOURS
#-pedantic-errors
LDFLAGS=-melf_i386 -Tlink.ld
ASFLAGS=-felf

all: $(SOURCES) link

clean:
	-rm *.o kernel

link:
	ld $(LDFLAGS) -o kernel $(SOURCES)

.s.o:
	nasm $(ASFLAGS) $<



