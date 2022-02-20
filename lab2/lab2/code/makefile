output: main.cpp my_print.o
	g++ -std=c++11 main.cpp my_print.o -o output
	
my_print.o: my_print.asm
	nasm -f elf my_print.asm
