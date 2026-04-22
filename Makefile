# Compilers
CXX = g++
CC = gcc

SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include

CXXFLAGS = -std=c++17 -Wall -Iinclude
CFLAGS = -Wall -O2 -Iinclude

# -lglfw links GLFW, -lGL links OpenGL, -ldl is required by GLAD to load pointers, -lm is for math
LDFLAGS = -lglfw -lGL -ldl -lm

CPP_SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
C_SOURCES = $(wildcard $(SRC_DIR)/*.c)

# Generate corresponding .o object file names in the obj directory
CPP_OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(CPP_SOURCES))
C_OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(C_SOURCES))
OBJECTS = $(CPP_OBJECTS) $(C_OBJECTS)

TARGET = main

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean run