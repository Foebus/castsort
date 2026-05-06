castsort: main.c
	gcc -o castsort main.c -lm -lgsl

clean:
	rm castsort
