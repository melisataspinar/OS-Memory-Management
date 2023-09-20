all: libsbmemlib.a  app create_memory_sb destroy_memory_sb

libsbmemlib.a:  sbmemlib.c
	gcc -Wall -c sbmemlib.c -lpthread -lm
	ar -cvq libsbmemlib.a sbmemlib.o
	ranlib libsbmemlib.a

app: app.c
	gcc -Wall -o app app.c -L. -lsbmemlib  -lrt -lpthread -lm

create_memory_sb: create_memory_sb.c
	gcc -Wall -o create_memory_sb create_memory_sb.c -L. -lsbmemlib -lrt -lpthread -lm

destroy_memory_sb: destroy_memory_sb.c
	gcc -Wall -o destroy_memory_sb destroy_memory_sb.c -L. -lsbmemlib -lrt -lpthread -lm

clean: 
	rm -fr *.o *.a *~ a.out  app sbmemlib.o sbmemlib.a libsbmemlib.a  create_memory_sb destroy_memory_sb

experiment: experiment.c
	gcc -Wall -o experiment experiment.c -L. -lm -lsbmemlib -lrt -lpthread 

doexperiment:
	rm -fr *.o *.a *~ a.out experiment sbmemlib.o sbmemlib.a libsbmemlib.a  create_memory_sb destroy_memory_sb
	gcc -Wall -c sbmemlib.c -lpthread -lm
	ar -cvq libsbmemlib.a sbmemlib.o
	ranlib libsbmemlib.a
	gcc -Wall -o experiment experiment.c -L. -lm -lsbmemlib -lrt -lpthread 
	./experiment


tests: sbmemlib.c test.c create_memory_sb.c destroy_memory_sb.c
	rm -fr *.o *.a *~ a.out test sbmemlib.o sbmemlib.a libsbmemlib.a  create_memory_sb destroy_memory_sb
	gcc -Wall -c sbmemlib.c -lpthread -lm
	ar -cvq libsbmemlib.a sbmemlib.o
	ranlib libsbmemlib.a
	gcc -Wall -o test test.c -L. -lsbmemlib  -lrt -lpthread -lm
	gcc -Wall -o create_memory_sb create_memory_sb.c -L. -lsbmemlib -lrt -lpthread -lm
	gcc -Wall -o destroy_memory_sb destroy_memory_sb.c -L. -lsbmemlib -lrt -lpthread -lm
	./test
