# Compiler and flags
CC = gcc
CFLAGS = -Iinclude

# Folders
SRC = src
BIN = bin

build:
	$(CC) $(CFLAGS) $(SRC)/PGS_MsgQ_Utilities.c $(SRC)/queue.c $(SRC)/loadFile.c $(SRC)/process_generator.c -o $(BIN)/process_generator.out
	$(CC) $(CFLAGS) $(SRC)/clk.c -o $(BIN)/clk.out
	$(CC) $(CFLAGS) $(SRC)/PGS_MsgQ_Utilities.c $(SRC)/queue.c $(SRC)/Scheduler_log.c $(SRC)/scheduler.c -o $(BIN)/scheduler.out
	$(CC) $(CFLAGS) $(SRC)/process.c -o $(BIN)/process.out
	$(CC) $(CFLAGS) $(SRC)/test_generator.c -o $(BIN)/test_generator.out

clean:
	rm -f $(BIN)/*.out processes.txt scheduler.log

all: clean build

run:
run:
	$(BIN)/test_generator.out
	$(BIN)/process_generator.out

