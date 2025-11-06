CC = gcc
CFLAGS = -Wall -Wextra -fsanitize=address
LEX = flex
YACC = bison --yacc
YFLAGS = -d -Wno-yacc

# Structure
SRC_DIR = src
BIN_DIR = bin
TARGET = msh

# Find all source files
LEX_SRC = $(wildcard $(SRC_DIR)/*.l)
YACC_SRC = $(wildcard $(SRC_DIR)/*.y)
C_SRCS = $(wildcard $(SRC_DIR)/*.c)

# Generated files from flex/bison
LEX_C = $(LEX_SRC:.l=.c)
YACC_C = $(SRC_DIR)/y.tab.c
# YACC_C = $(YACC_SRC:.y=.c)
YACC_H = $(SRC_DIR)/y.tab.h
# YACC_H = $(YACC_SRC:.y=.h)

# Object files
LEX_OBJ = $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(LEX_C))
# YACC_OBJ = $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(YACC_C))
YACC_OBJ = $(BIN_DIR)/y.tab.o
C_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(C_SRCS))

# All object files
OBJS = $(C_OBJS) $(LEX_OBJ) $(YACC_OBJ)

# Default target
all: $(BIN_DIR) $(TARGET)

# Create bin directory if it doesn't exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Link
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	rm $(SRC_DIR)/y.tab.*

debug: CFLAGS+=-g
debug: all

# min: $(OBJS)
# 	$(CC) $(CFLAGS) -o $@ $^

# Compile C files normally
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Flex rule: generate .c from .l
$(SRC_DIR)/%.c: $(SRC_DIR)/%.l
	$(LEX) -o $@ $<

# Bison rule: generate .c and .h from .y
$(YACC_C) $(YACC_H): $(YACC_SRC)
	cd $(SRC_DIR) && $(YACC) $(YFLAGS) $(notdir $<)

# Clean all*
clean:
	rm -rf $(BIN_DIR) $(TARGET) $(LEX_C) $(YACC_C) $(YACC_H)

# Clean everything + flex/bison files
# distclean: clean
# 	rm -f $(LEX_C) $(YACC_C) $(YACC_H)

debugVars:
	@echo "LEX_SRC: $(LEX_SRC)"
	@echo "YACC_SRC: $(YACC_SRC)"
	@echo "C_SRCS: $(C_SRCS)"
	@echo "LEX_C: $(LEX_C)"
	@echo "YACC_C: $(YACC_C)"
	@echo "YACC_H: $(YACC_H)"
	@echo "OBJS: $(OBJS)"

.PHONY: all clean debugVars

# Dependencies
# Bison needs to be available b4 flex and c
$(LEX_OBJ) $(C_OBJS): $(YACC_H)
