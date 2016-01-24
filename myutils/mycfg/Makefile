TARGET = cfg.o
SOURCE = mycfg.c mycfg_parse.c
OBJECT = $(SOURCE:.c=.o)
CC     = gcc
CFLAGS = -std=gnu89 -O2 -UNDEBUG -c -fPIC

.PHONY:all clean test
all: $(TARGET)
clean:
	rm -f G* $(TARGET) *.o main *.log

test: main.o $(TARGET) ../mylog/log.o
	$(CC) -o main $^

$(TARGET):$(OBJECT)
	ld -r -o $@ $^
%.o:%.c
	$(CC) $(CFLAGS) -o $@ $^
