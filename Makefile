# LAKY'S OPENGL BUILDER MAKEFILE v1.0.0
# -------------------------------------

# Define variables
CXX = g++
OPENSSL_DIR = "D:/msys64/mingw64"
INCLUDE = -Iinclude -Llib -Iinclude/freetype  -Iinclude/imgui
SRC = $(wildcard src/*.cpp src/glad.c src/*.h src/*.hpp src/imgui/*.cpp src/imgui/*.h) 

INCLUDE += -I$(OPENSSL_DIR)/include
LIBPATH += -L$(OPENSSL_DIR)/lib

LIBS = -lglfw3dll -lopengl32 -lfreetype -lssl -lcrypto -lcrypt32 -lws2_32
DEBUG_FLAGS = -g -std=c++17  -static-libgcc -static-libstdc++ -static # These will be used for the debug build (-g for debugging, -std=c++17 for C++17 standard)
RELEASE_FLAGS = -O2 -std=c++17 -static-libgcc -static-libstdc++ -static # These will be used for the release build (-O2 for optimization, -std=c++17 for C++17 standard)
DEBUG_OUT = main_debug.exe
RELEASE_OUT = main_release.exe

# Debug target
debug: CXXFLAGS = $(DEBUG_FLAGS)
debug: LDFLAGS = 
debug: $(DEBUG_OUT)

$(DEBUG_OUT): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(SRC) $(LIBPATH) $(LIBS) $(LDFLAGS) -o $(DEBUG_OUT)

# Release target
release: CXXFLAGS = $(RELEASE_FLAGS)
release: LDFLAGS = -mwindows
release: $(RELEASE_OUT)

$(RELEASE_OUT): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(SRC) $(LIBPATH) $(LIBS) $(LDFLAGS) -o $(RELEASE_OUT)

# Clean target
clean:
	rm -f $(DEBUG_OUT) $(RELEASE_OUT)
