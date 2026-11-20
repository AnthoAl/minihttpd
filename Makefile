COMP    	:= gcc
CFLAGS  	:= -Wall -Wextra -D_XOPEN_SOURCE=700 -Iinclude
EJECUTABLE  := minihttpd
SRVOBJS    	:= src/main.c src/server.c src/http.c src/files.c src/mime.c

all: $(EJECUTABLE)

$(EJECUTABLE): $(SRVOBJS)
	$(COMP) $(CFLAGS) -o $@ $^

clean:
	rm -f $(EJECUTABLE)