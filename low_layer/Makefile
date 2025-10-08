CXX := g++
CXXFLAGS := -std=c++17 -Wall -Iinclude

SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin

TARGET := $(BIN_DIR)/pfsp_sdst

# Znajdź wszystkie pliki .cpp rekurencyjnie (w src/, src/core/, src/app/)
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp) \
             $(wildcard $(SRC_DIR)/core/*.cpp) \
             $(wildcard $(SRC_DIR)/app/*.cpp)

OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC_FILES))


all: build

build: $(TARGET)

$(TARGET): $(OBJ_FILES)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

run: build
	$(TARGET) ../data/10_2_2_2.txt test_algorithm

.PHONY: all build clean run
