all: buffer.o main.o
	g++ -std=c++0x -g -Wall -o main buffer.o main.o

main.o: main.cpp
	g++ -std=c++0x -g -Wall -c main.cpp

buffer.o: buffer.cpp buffer.h
	g++ -std=c++0x -g -Wall -c buffer.cpp buffer.h

clean:
	rm main *.o *.gch
