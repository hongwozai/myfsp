TARGET = log.o
SOURCE = mylog.c
OBJECT = $(SOURCE:.c=.o)
CC     = gcc
CFLAGS = -std=gnu89 -O2 -UNDEBUG -c -fPIC

.PHONY:all clean test
all: $(TARGET)
clean:
	rm -f *.ini G* $(TARGET) *.o main *.log

$(TARGET): $(OBJECT)
	ld -r -o $@ $^
test: main.o $(TARGET)
	$(CC) -o main main.o $(TARGET)

%.o:%.c
	$(CC) $(CFLAGS) -o $@ $^
