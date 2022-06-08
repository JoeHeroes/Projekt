OBJ1 = Semafor.o
OBJ2 = Zmienne.o 
all: Semafor Zmienne
Semafor: $(OBJ1)
	gcc $(OBJ1) -pthread -o Semafor
Zmienne: $(OBJ2)
	gcc $(OBJ2) -pthread -o Zmienne
.PHONY: clean
clean:
	rm -f *.o XX
Semafor.o: Semafor.c 
	gcc -c -pthread Semafor.c
Zmienne.o: Zmienne.c 
	gcc -c -pthread Zmienne.c
