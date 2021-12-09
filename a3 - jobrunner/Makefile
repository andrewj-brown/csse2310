CC = gcc
CFLAGS = -g -Wall -pedantic -std=gnu99
CLIB = -I/local/courses/csse2310/include -L/local/courses/csse2310/lib -lcsse2310a3
OFILES = run.o finalise.o setup.o

.DEFAULT_GOAL := all
.PHONY: all success fail clean

all: jobrunner success

jobrunner: $(OFILES)
	@echo "Compiling main..."
	$(CC) $(CFLAGS) $(CLIB) main.c $(OFILES) -o jobrunner || make fail

run.o:
	@echo "Compiling run..."
	$(CC) $(CFLAGS) $(CLIB) -c run.c || make fail

finalise.o:
	@echo "Compiling finalise..."
	$(CC) $(CFLAGS) $(CLIB) -c finalise.c || make fail

setup.o:
	@echo "Compiling setup..."
	$(CC) $(CFLAGS) $(CLIB) -c setup.c || make fail

# cleans up after successes
success:
	@make --no-print-directory clean
	@echo "Compilation successful!"

# cleans up after failures
fail:
	@make --no-print-directory clean
	@$(error Compilation failed!)

# I've had troubles where make sees files as up-to-date despite them having
# been changed, especially where a problem in one file makes another file
# break. So I decided to just delete all object files all the time.
# `.DELETE_ON_ERROR:` also does this but recursion is cooler... B-)
clean:
	@echo "Cleaning up."
	@rm *.o || echo "Already cleaned, it seems"

