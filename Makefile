# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -Wno-error
LEX = flex
YACC = bison
YFLAGS = -d

# Project structure
SRC_DIR = src
BIN_DIR = bin
TARGET = myprogram

# Find all source files
LEX_SRC = $(wildcard $(SRC_DIR)/*.l)
YACC_SRC = $(wildcard $(SRC_DIR)/*.y)
C_SRCS = $(wildcard $(SRC_DIR)/*.c)

# Generated files from flex/bison
LEX_C = $(LEX_SRC:.l=.c)
YACC_C = $(YACC_SRC:.y=.c)
YACC_H = $(YACC_SRC:.y=.h)

# Object files (will be in bin directory)
LEX_OBJ = $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(LEX_C))
YACC_OBJ = $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(YACC_C))
C_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(C_SRCS))

# All object files
OBJS = $(C_OBJS) $(LEX_OBJ) $(YACC_OBJ)

# Default target
all: $(BIN_DIR) $(TARGET)

# Create bin directory if it doesn't exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Link all objects into executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

	# Compile main.c with no warnings at all
$(BIN_DIR)/main.o: $(SRC_DIR)/main.c | $(BIN_DIR)
	$(CC) -g -c -o $@ $<

# Compile other C files normally
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Flex rule: generate .c from .l
$(SRC_DIR)/%.c: $(SRC_DIR)/%.l
	$(LEX) -o $@ $<

# Bison rule: generate .c and .h from .y
$(SRC_DIR)/%.c $(SRC_DIR)/%.h: $(SRC_DIR)/%.y
	$(YACC) $(YFLAGS) -o $(SRC_DIR)/$*.c $<

# Clean build artifacts
clean:
	rm -rf $(BIN_DIR) $(TARGET) $(LEX_C) $(YACC_C) $(YACC_H)

# Clean everything including generated flex/bison files
distclean: clean
	rm -f $(LEX_C) $(YACC_C) $(YACC_H)

# Show variables for debugging
debug:
	@echo "LEX_SRC: $(LEX_SRC)"
	@echo "YACC_SRC: $(YACC_SRC)"
	@echo "C_SRCS: $(C_SRCS)"
	@echo "LEX_C: $(LEX_C)"
	@echo "YACC_C: $(YACC_C)"
	@echo "YACC_H: $(YACC_H)"
	@echo "OBJS: $(OBJS)"

.PHONY: all clean distclean debug

# Dependencies
# Make sure bison-generated header is available before compiling flex and C files
$(LEX_OBJ) $(C_OBJS): $(YACC_H)
