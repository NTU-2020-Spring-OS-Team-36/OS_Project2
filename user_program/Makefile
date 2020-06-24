all:
	gcc -O3 master.c -o master
	gcc -O3 slave.c -o slave
debug:
	gcc -fsanitize=undefined,address -Wall -Wextra -Wshadow -Wconversion -Og -ggdb3 master.c -o master
	gcc -fsanitize=undefined,address -Wall -Wextra -Wshadow -Wconversion -Og -ggdb3 slave.c -o slave
clean:
	rm slave master
