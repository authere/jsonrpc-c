PROJECT_NAME	= jsonrpc-c
INCLUDE		= include/
SRC_DIR		= src/
EXEMPLE_DIR	= example
STATIC_LIB_NAME	= $(SRC_DIR)jsonrpc-c.a

CFLAGS 		= -I$(INCLUDE) -Wall -Werror
ARCHIVER	= ar rvs

SRC_FILES = $(wildcard $(SRC_DIR)*.c)
OBJS = $(SRC_FILES:.c=.o)

all: $(PROJECT_NAME)
	$(MAKE) -C $(EXEMPLE_DIR)

$(PROJECT_NAME): $(OBJS)
	$(ARCHIVER) $(STATIC_LIB_NAME) $(OBJS)

.PHONY: clean
clean:
	-rm $(SRC_DIR)*.o $(SRC_DIR)$(PROJECT_NAME).a
	$(MAKE) -C $(EXEMPLE_DIR) clean
