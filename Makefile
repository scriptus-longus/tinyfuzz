SOURCE_FILE = target/simple.c
OUTPUT_PROG_NAME = target/simple_instr

all:
	g++ fuzz.cpp -o fuzz
	g++ fuzz-gcc.cpp -o fuzz-gcc

fuzz:
	g++ fuzz.cpp -o fuzz

target: fuzz-gcc
	gcc -S $(SOURCE_FILE) -o $(OUTPUT_PROG_NAME).s
	./fuzz-gcc $(OUTPUT_PROG_NAME).s $(OUTPUT_PROG_NAME)_instr.s
	gcc  $(OUTPUT_PROG_NAME)_instr.s -o $(OUTPUT_PROG_NAME)

fuzz-gcc:
	g++ fuzz-gcc.cpp -o fuzz-gcc
	
clean:
	g++ cleanup.cpp -o clean -lrt
	./clean
	rm clean
	rm $(OUPUT_PROG_NAME)*.s
	rm cases/*
