CC = x86_64-w64-mingw32-gcc

# Flags para o codigo do projeto
CFLAGS = -Wall -Wextra -std=c11 -I./include -I./src

# Flags extras de link: static runtime MinGW + GUI Win32 (sem console)
LDFLAGS = -mwindows -static-libgcc -lcomctl32 -lgdi32 -lcomdlg32

TARGET = TrocaOleo.exe

# Fontes do projeto (sem sqlite3)
APP_SRCS = src/main.c src/database.c src/gui.c src/validation.c src/config.c
APP_OBJS = $(APP_SRCS:.c=.o)

# Amalgamation SQLite compilado separadamente (sem warnings pedantes)
SQLITE_SRC  = src/sqlite3_amalgamation.c
SQLITE_OBJ  = src/sqlite3_amalgamation.o

all: $(TARGET)

$(TARGET): $(APP_OBJS) $(SQLITE_OBJ)
	$(CC) $(APP_OBJS) $(SQLITE_OBJ) -o $@ $(LDFLAGS)
	@echo Compilacao concluida: $(TARGET)

# Compilar amalgamation SQLite sem -Wall/-Wextra (muitos avisos internos da lib)
$(SQLITE_OBJ): $(SQLITE_SRC)
	$(CC) -O2 -std=c11 -I./include -DSQLITE_THREADSAFE=0 -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm -f src/*.o $(TARGET)

run: $(TARGET)
	wine $(TARGET) 2>/dev/null || echo "Instale o wine para testar no Linux. No Windows, execute diretamente."

.PHONY: all clean run
