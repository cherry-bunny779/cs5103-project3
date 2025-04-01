CC = g++
CC_FLAGS = -Wall -g -c

virtmem: main.o page_table.o disk.o program.o
	$(CC) main.o page_table.o disk.o program.o -o virtmem

main.o: main.cpp
	$(CC) $(CC_FLAGS) main.cpp -o main.o

page_table.o: page_table.cpp
	$(CC) $(CC_FLAGS) page_table.cpp -o page_table.o

disk.o: disk.cpp
	$(CC) $(CC_FLAGS) disk.cpp -o disk.o

program.o: program.cpp
	$(CC) $(CC_FLAGS) program.cpp -o program.o


clean:
	rm -f *.o virtmem myvirtualdisk
