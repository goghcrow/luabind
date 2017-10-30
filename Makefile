DEBUG := -fsanitize=address -fno-omit-frame-pointer

LLIB := -I./include/lua5.3 -L./lib/lua5.3 -Wl,-rpath,./lib/lua5.3 -lm -ldl

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LLIB += -llua_linux
endif
ifeq ($(UNAME_S),Darwin)
    LLIB += -llua_mac
endif
	
tcpsniff: sniff.c tcpsniff.c luabind.c 
	gcc -g -Wall -o $@ $^ $(LLIB) -lpcap

tcpsniff-debug: sniff.c tcpsniff.c luabind.c 
	gcc $(DEBUG) -g -Wall -o $@ $^  $(LLIB) -lpcap

test: test.c luabind.c
	gcc -g -Wall -o $@ $^ $(LLIB)

clean:
	rm -f test
	rm -f tcpsniff
	rm -f tcpsniff-debug
	rm -rf *.dSYM