all: user calculator
	tar -c -f A3_Ismail_Zakaria_101143497.zip *.c *.h Makefile *.txt *.pdf

user: user.o common.o common.h
	gcc -g -Wall -o user user.o common.o

user.o: user.c common.h
	gcc -c -g -Wall -o user.o user.c

calculator: calculator.o common.o common.h
	gcc -g -Wall -o calculator calculator.o common.o

calculator.o: calculator.c common.h
	gcc -c -g -Wall -o calculator.o calculator.c

common.o: common.c common.h
	gcc -c -g -Wall -o common.o common.c

clean:
	rm *.o user calculator







