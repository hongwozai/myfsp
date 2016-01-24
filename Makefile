SERVER = bin/myfsp
CLIENT = bin/client
WINCLI = bin/wincli
CLI-SOURCE = client.c myfsp_fileutils.c myfsp_protocol.c
CLI-OBJECT = $(CLI-SOURCE:.c=.o)
WINCLI-SOURCE = client-win.c myfsp_fileutils.c myfsp_protocol.c
WINCLI-OBJECT = $(WINCLI-SOURCE:.c=.o)
SER-SOURCE = main.c myfsp_db.c myfsp_fileutils.c myfsp_protocol.c myfsp.c
SER-OBJECT = $(SER-SOURCE:.c=.o)


CC     = gcc
CFLAGS = `mysql_config --cflags` -O2 -UNDEBUG
LIBS   = `mysql_config --libs` -lmyutils -Lmyutils
GTK    = `pkg-config --cflags --libs gtk+-3.0`

.PHONY:all clean test so
all:$(SERVER) $(CLIENT) $(WINCLI)
clean:
	rm -f *.o $(SERVER) $(CLIENT)
so:
	cd myutils && make -B && cp libmyutils.so ../bin/;

$(CLIENT):$(CLI-OBJECT)
	$(CC) $(CFLAGS) $(LIBS) -o $@ $^
$(WINCLI):$(WINCLI-OBJECT)
	$(CC) $(CFLAGS) $(GTK) $(LIBS) -o $@ $^
$(SERVER): $(SER-OBJECT)
	$(CC) $(CFLAGS) $(LIBS) -o $@ $^
%.o:%.c
	$(CC) $(GTK) $(CFLAGS) $(LIBS) -c -o $@ $^
