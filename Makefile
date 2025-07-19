CFLAGS = -g -O0 -std=c99 -Wall -Wextra

SRC = main.c		\
      context_amd64.c	\
      stack.c		\
      task.c

OBJ = ${SRC:.c=.o}

all: kernel

run: kernel
	./kernel
	
clean:
	rm -f kernel *.o

kernel: ${OBJ}
	${CC} -o $@ ${OBJ} ${CFLAGS} ${LDFLAGS}

.c.o:
	${CC} -c -o $@ $< ${CFLAGS}
