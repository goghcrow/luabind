DEBUG := -fsanitize=address -fno-omit-frame-pointer

LLIB := -Wl,-rpath,./lib -L./lib

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LLIB += -llua_linux
endif
ifeq ($(UNAME_S),Darwin)
    LLIB += -llua_mac
endif
	
tcpsniff: sniff.c tcpsniff.c luabind.c 
	gcc -g -Wall -o $@ $^ -I./include -L./lib -lpcap $(LLIB)

tcpsniff-debug: sniff.c tcpsniff.c luabind.c 
	gcc $(DEBUG) -g -Wall -o $@ $^ -I./include -L./lib -llua -lpcap $(LLIB)

test: test.c luabind.c
	gcc -g -Wall -o $@ $^ -I./include -L./lib -llua

clean:
	rm -f test
	rm -f tcpsniff
	rm -f tcpsniff-debug
	rm -rf *.dSYM