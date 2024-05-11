# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -pedantic -std=c11 -I$(CJSON_DIR)

# Linker flags
LDFLAGS = -L. -Wl,-rpath=$(CJSON_DIR) -Wl,-rpath=$(CJSON_DIR)/$(CJSON_DIR) -lcjson -L$(CJSON_DIR) -lpthread -lrt

# Source files
SERVER_SRC = server.c
SOCKETS_SRC = sockets_functions.c 
MSG_JSON_SRC = mensaje_json.c


# Object files
SERVER_OBJ = $(SERVER_SRC:.c=.o)
SOCKETS_OBJ = $(SOCKETS_SRC:.c=.o) 
MSG_JSON_OBJ = $(MSG_JSON_SRC:.c=.o)

# Target executables and library
SERVER_BIN = server

# cJSON files
CJSON_DIR = cJSON
CJSON_LIB = $(CJSON_DIR)/libcjson.so
CJSON_URL = https://github.com/DaveGamble/cJSON.git

.PHONY: all clean cJSON

all: cJSON $(SERVER_BIN) clean

$(SERVER_BIN): $(SERVER_OBJ) $(SOCKETS_OBJ) $(MSG_JSON_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

cJSON: $(CJSON_LIB)

$(CJSON_LIB):
	git clone $(CJSON_URL) $(CJSON_DIR)
	cd $(CJSON_DIR) && make

run-server:
	./server -p 8080

run-client:
	python3 ./client.py -s localhost -p 8080

clean:
	rm -f *.o
