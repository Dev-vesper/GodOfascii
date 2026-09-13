CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Isrc
# Fall back to plain -lSDL2 when sdl2-config is unavailable (some MinGW or
# vcpkg toolchains ship SDL2 without the helper script).
SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null || echo -I/usr/include/SDL2)
SDL_LIBS := $(shell sdl2-config --libs 2>/dev/null || echo -lSDL2)

SRC := $(wildcard src/*.cpp src/*/*.cpp)
OBJ := $(SRC:src/%.cpp=build/%.o)
DEP := $(OBJ:.o=.d)
BIN := build/ascii3d

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(SDL_LIBS)

build/%.o: src/%.cpp | build
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) -MMD -MP -c -o $@ $<

build:
	mkdir -p build

run: $(BIN)
	./$(BIN)

clean:
	rm -rf build

-include $(DEP)

.PHONY: all run clean
