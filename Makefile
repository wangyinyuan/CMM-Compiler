CC = gcc
CFLAGS = 

SRC_DIR = src
OBJ_DIR = build
INC_DIR = -I./
HELPER_DIR = helpers

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
HDRS = $(wildcard $(INC_DIR)/*.h)
HELPER_SRCS = $(wildcard $(HELPER_DIR)/*.c)
HELPER_OBJS = $(patsubst $(HELPER_DIR)/%.c, $(OBJ_DIR)/$(HELPER_DIR)/%.o, $(HELPER_SRCS))

TARGET = main

$(TARGET): $(OBJS) $(HELPER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/$(HELPER_DIR)/%.o: $(HELPER_DIR)/%.c $(HDRS)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJ_DIR)/*.o $(TARGET) $(OBJ_DIR)/$(HELPER_DIR)/*.o
	
