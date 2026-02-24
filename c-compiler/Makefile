# QScript C prototype compiler (qlangc)
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra
TARGET = qlangc

SRCS = main.c lexer.c ast.c parser.c typecheck.c ir.c backend.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
