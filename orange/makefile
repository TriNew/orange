#make会将第一个出现的目标作为默认目标，就是只执行make不加目标名的时候，第一个目标名通常是all。 
#Makefile for boot 
ENTRYPOINT 	= 0x30400 
ENTRYOFFSET = 0x400

#Programs, flags, etc 
ASM 		= nasm 
ASMFLAGS 	= -I boot/include/
DASM 		= ndisasm 
CC 			= gcc 
LD 			= ld
ASMKFLAGS 	= -I include/ -f elf 
CFLAGS 		= -I include/ -c -fno-builtin -fno-stack-protector -std=c99 
LDFLAGS 	= -S -Ttext $(ENTRYPOINT)
DASMFLAGS 	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)


#this Program 
ORANGESBOOT = boot/boot.bin boot/loader.bin 
ORANGESKERNEL= kernel.bin 
OBJS 		= kernel/kernel.o kernel/start.o kernel/i8259.o lib/kliba.o \
			lib/string.o kernel/global.o kernel/protect.o   lib/klib.o \
			kernel/main.o kernel/clock.o kernel/syscall.o kernel/proc.o \
			kernel/keyboard.o kernel/tty.o kernel/console.o kernel/printf.o \
			kernel/misc.o kernel/systask.o  kernel/hd.o #fs/main.o
DASMOUTPUT  = kernel.bin.asm 


#all Phony Targets 假的，欺骗的
#.PHONY后面跟的不是文件，而仅仅是一种行为的标号
#phony的作用是防止与文件夹中的同名文件冲突和改善性能
.PHONY : everything final image clean realclean disasm  all buildimg  

#Default starting position makefile默认从第一个标号开始执行，且只执行
#着一个标号中的内容若改为everything :boot.bin ,执行make时只编译生成
#boot.bin当执行make all时 执行clean和everything，everything又执行
#boot.bin和loader.bin
everything: $(ORANGESBOOT) $(ORANGESKERNEL)

all: realclean everything

final: all clean 
image: final buildimg

clean: 
	rm -f $(OBJS)
realclean: 
	rm -f $(OBJS) $(ORANGESBOOT) $(ORANGSEKERNEL)
disasm:
	$(DASM) $(DASMFLAGS) $(ORANGESKERNEL) > $(DASMOUTPUT)
buildimg: 
	dd if=boot/boot.bin of=a.img bs=512 count=1 conv=notrunc
	sudo mount -o loop a.img /media/g
	sudo cp -fv boot/loader.bin /media/g
	sudo cp -fv kernel.bin /media/g
	sudo umount /media/g



boot/boot.bin: boot/boot.asm boot/include/load.inc boot/include/fat12hdr.inc
		$(ASM) $(ASMFLAGS) -o $@ $<
boot/loader.bin: boot/loader.asm boot/include/load.inc \
				boot/include/fat12hdr.inc boot/include/pm.inc  boot/include/lib.inc 
		$(ASM) $(ASMFLAGS) -o $@ $<
$(ORANGESKERNEL):$(OBJS)
		$(LD) $(LDFLAGS) -o $(ORANGESKERNEL) $(OBJS)
kernel/kernel.o: kernel/kernel.asm 
		$(ASM) $(ASMKFLAGS) -o $@ $<
kernel/start.o: kernel/start.c include/type.h include/const.h include/protect.h \
				include/proto.h include/string.h 
		$(CC) $(CFLAGS) -o $@ $<
lib/kliba.o:lib/kliba.asm 
		$(ASM) $(ASMKFLAGS) -o $@ $<
lib/string.o:lib/string.asm 
		$(ASM) $(ASMKFLAGS) -o $@ $<
lib/klib.o: lib/klib.c 
		$(CC) $(CFLAGS) -o $@ $<
kernel/i8259.o:kernel/i8259.c 
		$(CC) $(CFLAGS) -o $@ $<
kernel/protect.o:kernel/protect.c 	
		$(CC) $(CFLAGS) -o $@ $<	
kernel/global.o:kernel/global.c
		$(CC) $(CFLAGS) -o $@ $<
kernel/main.o:kernel/main.c 
		$(CC) $(CFLAGS) -o $@ $<
kernel/clock.o:kernel/clock.c 
		$(CC) $(CFLAGS) -o $@ $<
kernel/syscall.o:kernel/syscall.asm 
		$(ASM) $(ASMKFLAGS) -o $@ $<
kernel/proc.o:kernel/proc.c 
		$(CC) $(CFLAGS) -o $@ $<
kernel/keyboard.o:kernel/keyboard.c 
		$(CC) $(CFLAGS) -o $@ $<
kernel/tty.o:kernel/tty.c
		$(CC) $(CFLAGS) -o $@ $<
kernel/console.o:kernel/console.c
		$(CC) $(CFLAGS) -o $@ $<
kernel/printf.o:kernel/printf.c
		$(CC) $(CFLAGS) -o $@ $<
kernel/misc.o:lib/misc.c
		$(CC) $(CFLAGS) -o $@ $<
kernel/systask.o:kernel/systask.c
		$(CC) $(CFLAGS) -o $@ $<
fs/main.o:fs/main.c
		$(CC) $(CFLAGS) -o $@ $<
kernel/hd.o: kernel/hd.c 
		$(CC) $(CFLAGS) -o $@ $<
 
