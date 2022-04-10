CC=g++ -march=native -O2
CFLAGS=-c -I. -std=c++0x -Wfatal-errors -g

all: incavt  


incavt: main.o gadget.o heap.o treap.o incavt.o
	$(CC) main.o gadget.o heap.o treap.o incavt.o -o incavt
	rm *.o


main.o: mainavt.cpp
	$(CC) $(CFLAGS) mainavt.cpp -o main.o


gadget.o: gadget/gadget.cpp
	$(CC) $(CFLAGS) gadget/gadget.cpp -o gadget.o

heap.o: gadget/heap.cpp
	$(CC) $(CFLAGS) gadget/heap.cpp -o heap.o

treap.o: gadget/treap.cpp
	$(CC) $(CFLAGS) gadget/treap.cpp -o treap.o

traversal.o: traversal/traversal.cpp
	$(CC) $(CFLAGS) traversal/traversal.cpp -o traversal.o

incavt.o: glist/incavt.cpp 
	$(CC) $(CFLAGS) glist/incavt.cpp -o incavt.o

