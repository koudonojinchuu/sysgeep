CC=gcc -std=gnu11
LDFLAGS=-lgit2
CFLAGS=-g

sysgeep: $(addprefix src/,utils.o sysgeep.o actions.o sorted_lines.o)
	$(CC) -o $% $(LDFLAGS) $^

clean:
	rm -f src/*.o sysgeep
