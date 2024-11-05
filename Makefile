include ./Makefile.inc

SERVER_SOURCES=$(wildcard src/server/*.c)
CLIENT_SOURCES=$(wildcard src/client/*.c)

OUTPUT_FOLDER=./bin
OBJECTS_FOLDER=./obj

SERVER_OBJECTS=$(SERVER_SOURCES:src/server/%.c=$(OBJECTS_FOLDER)/server/%.o)
CLIENT_OBJECTS=$(CLIENT_SOURCES:src/client/%.c=$(OBJECTS_FOLDER)/client/%.o)

SERVER_OUTPUT_FILE=$(OUTPUT_FOLDER)/server
CLIENT_OUTPUT_FILE=$(OUTPUT_FOLDER)/client

all: server client

server: $(SERVER_OUTPUT_FILE)
client: $(CLIENT_OUTPUT_FILE)

$(SERVER_OUTPUT_FILE): $(SERVER_OBJECTS)
	mkdir -p $(OUTPUT_FOLDER)
	$(COMPILER) $(CFLAGS) $(SERVER_OBJECTS) -o $(SERVER_OUTPUT_FILE)

$(CLIENT_OUTPUT_FILE): $(CLIENT_OBJECTS)
	mkdir -p $(OUTPUT_FOLDER)
	$(COMPILER) $(CFLAGS) $(CLIENT_OBJECTS) -o $(CLIENT_OUTPUT_FILE)

# Pattern rule to create object files
$(OBJECTS_FOLDER)/server/%.o: src/server/%.c
	mkdir -p $(OBJECTS_FOLDER)/server
	$(COMPILER) $(CFLAGS) -c $< -o $@

$(OBJECTS_FOLDER)/client/%.o: src/client/%.c
	mkdir -p $(OBJECTS_FOLDER)/client
	$(COMPILER) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OUTPUT_FOLDER)
	rm -rf $(OBJECTS_FOLDER)

.PHONY: all server client clean
