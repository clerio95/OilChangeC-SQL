CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I./include -I./src
LDFLAGS = -L./lib -lsqlite3 -lcomctl32 -lgdi32 -lcomdlg32
TARGET = TrocaOleo.exe

SRCS = src/main.c src/database.c src/gui.c src/validation.c src/config.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo Compilacao concluida: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm -f src/*.o $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
