CC = gcc
FLAGS = -Wall -Wextra -std=c23 -g
INCLUDES = 
LDFLAGS = 

TARGET = cpector

BUILD_DIR = build
SOURCE_DIR = src

SRCS = $(wildcard $(SOURCE_DIR)/*.c)
OBJS = $(patsubst $(SOURCE_DIR)/*.c, $(BUILD_DIR)/%.o, $(SRCS))

# Create default target
all: $(BUILD_DIR) $(TARGET)

# Clean target
clean:
	@rm -rf $(BUILD_DIR) $(TARGET)

# Create the build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compile obj files into an executable with same name as $TARGET
$(TARGET): $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $@ $(LDFLAGS) $(INCLUDES)

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(FLAGS) -c $< -o $@

# phony target provides generic executable targets
.PHONY: all clean
