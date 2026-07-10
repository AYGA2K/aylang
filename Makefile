.PHONY: all build run test clean rebuild help

BUILD_DIR := build
TARGET    := aylang
CMAKE_FLAGS ?= -DCMAKE_BUILD_TYPE=Debug
EXAMPLE   := example.aylang

all: build

$(BUILD_DIR)/CMakeCache.txt:
	mkdir -p $(BUILD_DIR)
	cmake -B $(BUILD_DIR) -S . $(CMAKE_FLAGS)

build: $(BUILD_DIR)/CMakeCache.txt
	cmake --build $(BUILD_DIR)

run: build
	$(BUILD_DIR)/$(TARGET)

test: build
	$(BUILD_DIR)/$(TARGET) $(EXAMPLE)

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean build

help:
	@echo "Targets:"
	@echo "  make / build  - configure (if needed) and compile"
	@echo "  make run      - build and run $(TARGET)"
	@echo "  make test     - build and run $(TARGET) on $(EXAMPLE)"
	@echo "  make clean    - remove $(BUILD_DIR)/"
	@echo "  make rebuild  - clean then build"
	@echo "Override CMAKE_FLAGS, e.g.: make CMAKE_FLAGS=\"-DCMAKE_BUILD_TYPE=Release\""
