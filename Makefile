CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
OBJS = grafo.o tipos.o pokemon.o treinador.o batalha.o rocket.o config.o item.o main.o
TARGET = pokemon_liga

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all run clean
