# Makefile — compila el proyecto secuencial
APP := screensaver
SRC := src/main.c src/entities.c
INCLUDES := -Iinclude
CFLAGS := -O2 -Wall -Wextra -std=c11 $(INCLUDES) `sdl2-config --cflags`
LDFLAGS := `sdl2-config --libs` -lSDL2_ttf -lm

$(APP): $(SRC)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

run: $(APP)
	./$(APP) 200

clean:
	rm -f $(APP)