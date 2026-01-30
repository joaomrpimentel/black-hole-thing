# Defaults
BUILD_DIR = build
EXECUTABLE = $(BUILD_DIR)/BlackHoleThing
CMAKE_FLAGS = -DCMAKE_BUILD_TYPE=Release

# Detect number of processors for parallel build
NPROCS = $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)

.PHONY: all configure build run clean

all: build

# Generate build system using CMake
configure:
	cmake -B $(BUILD_DIR) -S . $(CMAKE_FLAGS) -DCMAKE_POLICY_VERSION_MINIMUM=3.5

# Compile the project
build:
	@if [ ! -d "$(BUILD_DIR)" ]; then $(MAKE) configure; fi
	cmake --build $(BUILD_DIR) -j$(NPROCS)

# Run the application
run: build
	./$(EXECUTABLE)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
