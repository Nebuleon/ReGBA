# Top-level

all: main

clean: main_clean

main:
#	./counter.pl
	make -C ./obj/

main_clean:
	make -C ./obj/ clean

