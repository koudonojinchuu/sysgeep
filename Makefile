CC=gcc -std=gnu11 -Wall
LDFLAGS=-lgit2
CFLAGS=-g

sysgeep: $(addprefix src/,sysgeep.o actions.o utils.o sorted_lines.o)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f src/*.o sysgeep
