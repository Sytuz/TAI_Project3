# Define the compiler and flags
CXX      = g++
CXXFLAGS = -Wall -std=c++17

# List source files and automatically generate object file names
SOURCES  = main.cpp FCMModel.cpp
OBJECTS  = $(SOURCES:.cpp=.o)
TARGET   = testprogram.out

# Default target to build the executable
all: clean $(TARGET)

# Link object files into the final executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

# Rule for compiling .cpp files into .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(TARGET) $(OBJECTS)
