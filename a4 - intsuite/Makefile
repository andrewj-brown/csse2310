CC = gcc
CFLAGS = -g -Wall -pedantic --std=gnu99
CLIB = -I/local/courses/csse2310/include -L/local/courses/csse2310/lib -lcsse2310a4 -lcsse2310a3 -ltinyexpr -lm -lpthread
S_OFILES = serverComp.o serverHup.o

.DEFAULT_GOAL := all
.PHONY := all clean

all: server client clean

server: s_comp s_signal 
	$(CC) $(CFLAGS) $(CLIB) serverMain.c $(S_OFILES) -o intserver || make clean
	@echo -e "Successfully compiled intserver\n"

s_signal:
	$(CC) $(CFLAGS) $(CLIB) serverHup.c -c || make clean

s_comp:
	$(CC) $(CFLAGS) $(CLIB) serverComp.c -c || make clean

client:
	$(CC) $(CFLAGS) $(CLIB) clientMain.c -o intclient || make clean
	@echo -e "Successfully compiled intclient\n"

clean:
	@rm *.o || echo "No ofiles to remove!"

testclean:
	@rm *.out || echo "No \"out\" files"
	@rm .intserver.stdout || echo ""
	@rm .intserver.stderr || echo ""
	@rm .intserver.stderr-minus-portnum || echo ""
	@rm *.stdout || echo "No \"stdout\" files"
	@rm *.stderr || echo "No \"stderr\" files"
