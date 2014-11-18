CC = g++ -Wall -pedantic -O2 -std=c++11

all: os3

os3: os3.o
	$(CC) -o os3 os3.o

os3.o: os3.cpp
	$(CC) -c os3.cpp

valgrind:
	valgrind --leak-check=full --track-origins=yes os3

clean:
	rm -rf *.o *~ os3
