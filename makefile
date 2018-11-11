#compilsal -Wall -Wextra -Werror
#compilcov -fprogile-arcs -ftest-coverage
DEBUG = -g

slave_   = slave_add slave_div slave_mod slave_mul slave_sous
slave_.o = slave_add.o slave_div.o slave_mod.o slave_mul.o slave_sous.o

all : master $(slave_)

master : master.o common.o
				gcc $(DEBUG) master.o common.o -o master -l pthread

$(filter %,$(slave_)): %:%.o common.o
				gcc $(DEBUG) $< common.o -o $@ -l pthread

master.o : master.c
				gcc $(DEBUG) -c master.c -o master.o

$(filter %.o,$(slave_.o)): %.o:%.c
				gcc $(DEBUG) -c $< -o $@

common.o : common.c
				gcc $(DEBUG) -c common.c -o common.o

clean :
				rm *.o
				rm master
				rm slave_add
				rm slave_mod
				rm slave_mul
				rm slave_sous
				rm slave_div
