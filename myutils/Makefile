TARGET = libmyutils.so
OBJECT = mylog/log.o mycfg/cfg.o mysock/sock.o
CC     = gcc
CFLAGS = -std=gnu89 -UNDEBUG -O2 -c -fPIC

.PHONY:all clean test
all: $(TARGET)
clean:
	@cd mylog && make clean;
	@cd mycfg && make clean;
	@cd mysock&& make clean;
	rm -f $(TARGET) main *.o
test:$(TARGET) main.o
	$(CC) -o main main.o -lmyutils -L.

$(TARGET):
	@cd mylog && make;
	@cd mycfg && make;
	@cd mysock&& make;
	$(CC) -shared  -o $@ $(OBJECT)
%.o:%.c
	$(CC) $(CFLAGS) -o $@ $^
