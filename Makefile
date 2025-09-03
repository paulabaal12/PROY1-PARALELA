# Makefile — compila versiones secuencial y paralela
APP_SEQ := screensaver_seq
APP_PAR := screensaver_par
SRC_SEQ := src/main_seq.c src/entities_seq.c
SRC_PAR := src/main_par.c src/entities_par.c
INCLUDES := -Iinclude
CFLAGS := -O2 -Wall -Wextra -std=c11 -fopenmp $(INCLUDES) `sdl2-config --cflags`
LDFLAGS := -fopenmp `sdl2-config --libs` -lSDL2_ttf -lm

all: $(APP_SEQ) $(APP_PAR)

$(APP_SEQ): $(SRC_SEQ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(APP_PAR): $(SRC_PAR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

run_seq: $(APP_SEQ)
	./$(APP_SEQ) 200

run_par: $(APP_PAR)
	./$(APP_PAR) 200

clean:
	rm -f $(APP_SEQ) $(APP_PAR)
