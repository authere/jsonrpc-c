PROJECT_NAME	= server
STATIC_LIB_NAME	= ../src/jsonrpc-c.a
INCLUDE_JSONRPC	= ../include/

all: $(PROJECT_NAME)

LDFLAGS		= -ldl $(STATIC_LIB_NAME) -lm
CFLAGS		= -I$(INCLUDE_JSONRPC)

$(PROJECT_NAME): server.o
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	-rm $(PROJECT_NAME) *.o
