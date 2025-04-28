build:
# Each separate line is a file with a main function that can run separately 
# utilities is coupled with process generator and scheduler as they need it
	gcc process_generator.c PGS_MsgQ_Utilities.c -o process_generator.out
	gcc clk.c -o clk.out
	gcc scheduler.c PGS_MsgQ_Utilities.c -o scheduler.out
	gcc process.c -o process.out
	gcc test_generator.c -o test_generator.out

clean:
	rm -f *.out processes.txt

all: clean build

run:
	./process_generator.out
