LDFLAGS=-lgit2
CFLAGS=-g

sysgeep: utils.o sysgeep.o actions.o sorted_lines.o

clean:
	rm -f *.o sysgeep
