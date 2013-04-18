PROJECT_NAME	= server
STATIC_LIB_NAME	= ../src/jsonrpc-c.a
INCLUDE_JSONRPC	= ../include/
COMPILER	= $(CROSS_COMPILE)$(CC)

all: $(PROJECT_NAME)

LDFLAGS		= -ldl $(STATIC_LIB_NAME) -lm
CFLAGS		= -I$(INCLUDE_JSONRPC) -Wall -Werror -O2

$(PROJECT_NAME): server.o
	$(COMPILER) -o $@ $^ $(LDFLAGS)

server.o: server.c
	$(COMPILER) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	-rm $(PROJECT_NAME) *.o
