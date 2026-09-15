CXX ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Iinclude

# The SDL window backend is auto-detected via sdl2-config. SDL=0 builds a
# terminal-only binary that needs no SDL2 at all (FreeBSD base systems,
# minimal containers); SDL=1 forces the window backend with plain -lSDL2
# for toolchains that ship SDL2 without the helper script.
SDL ?= auto
SDL_CFLAGS :=
SDL_LIBS :=
HAVE_SDL :=
ifneq ($(SDL),0)
    ifeq ($(SDL),1)
        HAVE_SDL := 1
        SDL_CFLAGS := -I/usr/include/SDL2
        SDL_LIBS := -lSDL2
    else
        HAVE_SDL := $(shell sdl2-config --version 2>/dev/null)
        ifneq ($(HAVE_SDL),)
            SDL_CFLAGS := $(shell sdl2-config --cflags)
            SDL_LIBS := $(shell sdl2-config --libs)
        endif
    endif
endif

SRC := $(wildcard src/*.cpp src/*/*.cpp)
ifneq ($(HAVE_SDL),)
    CXXFLAGS += -DASCII3D_SDL=1
else
    SRC := $(filter-out src/platform/SdlDisplay.cpp,$(SRC))
endif

# The client runs the game; the server relays sessions and needs neither
# rendering nor input, so it links only the net module.
CLIENT_SRC := $(filter-out src/server/main.cpp,$(SRC))
SERVER_SRC := src/server/main.cpp src/net/Protocol.cpp src/net/Socket.cpp
CLIENT_OBJ := $(CLIENT_SRC:src/%.cpp=build/%.o)
SERVER_OBJ := $(SERVER_SRC:src/%.cpp=build/%.o)
DEP := $(CLIENT_OBJ:.o=.d) $(SERVER_OBJ:.o=.d)
BIN := build/ascii3d
SERVER_BIN := build/ascii3d-server

all: $(BIN) $(SERVER_BIN)

$(BIN): $(CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(SDL_LIBS)

$(SERVER_BIN): $(SERVER_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

build/%.o: src/%.cpp | build
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) -MMD -MP -c -o $@ $<

build:
	mkdir -p build

run: $(BIN)
	./$(BIN)

server: $(SERVER_BIN)
	./$(SERVER_BIN)

clean:
	rm -rf build

-include $(DEP)

.PHONY: all run server clean
