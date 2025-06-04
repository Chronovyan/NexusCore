CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -I./include
LDFLAGS = 

SRC_DIR = src
INCLUDE_DIR = include
EXAMPLES_DIR = examples
BUILD_DIR = build
CONFIG_DIR = config

# Source files
SOURCES = $(SRC_DIR)/resource_optimizer.cpp \
          $(SRC_DIR)/resource_config.cpp

# Example executables
EXAMPLES = $(BUILD_DIR)/resource_config_demo

# Make sure the build directory exists
$(shell mkdir -p $(BUILD_DIR))

all: $(EXAMPLES)

# Compile the resource config demo
$(BUILD_DIR)/resource_config_demo: $(EXAMPLES_DIR)/resource_config_demo.cpp $(SOURCES) $(wildcard $(INCLUDE_DIR)/*.h)
	$(CXX) $(CXXFLAGS) -o $@ $< $(SOURCES) $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

run_demo: $(BUILD_DIR)/resource_config_demo
	$(BUILD_DIR)/resource_config_demo $(CONFIG_DIR)/resource_optimization.conf

.PHONY: all clean run_demo 