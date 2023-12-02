all:
	g++ fuzz.cpp -o fuzz -lrt
	
clean:
	g++ cleanup.cpp -o clean -lrt
	./clean
	rm clean
	rm cases/*
