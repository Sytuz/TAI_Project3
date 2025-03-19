# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2

# Target executable
TARGET = MetaClass

# Source files
SOURCES = MetaClass.cpp main.cpp
HEADERS = MetaClass.h

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Link rule
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lm

# Compilation rule
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJECTS) $(TARGET)

# Phony targets
.PHONY: all clean
