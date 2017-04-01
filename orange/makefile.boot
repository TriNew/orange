#make会将第一个出现的目标作为默认目标，就是只执行make不加目标名的时候，第一个目标名通常是all。 
#Makefile for boot 

#Programs, flags, etc 
ASM 		= nasm 
ASMFLAGS 	= -I boot/include/

#this Program 
ORANGESBOOT = boot/boot.bin boot/loader.bin 

#all Phony Targets 假的，欺骗的
#.PHONY后面跟的不是文件，而仅仅是一种行为的标号
#phony的作用是防止与文件夹中的同名文件冲突和改善性能
.PHONY : everything  clean  all 

#Default starting position 
#makefile默认从第一个标号开始执行，且只执行着一个标号中的内容
#若改为everything :boot.bin ,执行make时只编译生成boot.bin
#当执行make all时 执行clean和everything，everything又执行boot.bin和loader.bin
everything : $(ORANGESBOOT) 

all : clean everything

clean : 
	rm -f $(ORANGESBOOT)

boot/boot.bin : boot/boot.asm boot/include/load.inc boot/include/fat12hdr.inc
		$(ASM) $(ASMFLAGS) -o $@ $<
boot/loader.bin : boot/loader.asm boot/include/load.inc \
				boot/include/fat12hdr.inc boot/include/pm.inc  boot/include/lib.inc 
		$(ASM) $(ASMFLAGS) -o $@ $<
